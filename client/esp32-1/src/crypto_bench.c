// crypto cost on the device: how long do SHA-256 and one ECDSA P-256 signature check take on this
// chip? A validating resolver does one such check per RRSIG, so this backs the answer to
// "why not validate DNSSEC on the ESP32?". No network needed. Only built into the crypto-bench env.
#ifdef CRYPTO_BENCH

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "sdkconfig.h"
#include "psa/crypto.h"
#include "crypto_bench.h"

#define MSG_LEN   200      // about the size of one canonical RRset
#define RUNS      200
// signatures a validator checks for ota.signed.lab.test: DNSKEY of lab.test, DS of signed.lab.test,
// DNSKEY of signed.lab.test, the A record. Counted with kdig +dnssec: one RRSIG per RRset
#define LAB_SIGS  4

#if defined(CONFIG_COMPILER_OPTIMIZATION_DEBUG)
#define APP_OPT "-Og"
#elif defined(CONFIG_COMPILER_OPTIMIZATION_SIZE)
#define APP_OPT "-Os"
#elif defined(CONFIG_COMPILER_OPTIMIZATION_PERF)
#define APP_OPT "-O2"
#else
#define APP_OPT "-O0"
#endif

// mbedTLS is compiled with its own optimisation setting, independent of the app's
#if defined(CONFIG_MBEDTLS_COMPILER_OPTIMIZATION_SIZE)
#define TLS_OPT "-Os"
#elif defined(CONFIG_MBEDTLS_COMPILER_OPTIMIZATION_PERF)
#define TLS_OPT "-O2"
#else
#define TLS_OPT "default"
#endif

#ifdef CONFIG_MBEDTLS_HARDWARE_SHA
#define HW_SHA "yes"
#else
#define HW_SHA "no"
#endif

// mean in microseconds; also prints min / max
static int64_t report(const char *what, const int64_t *us, int n)
{
    int64_t sum = 0, lo = us[0], hi = us[0];
    for (int i = 0; i < n; i++) {
        sum += us[i];
        if (us[i] < lo) lo = us[i];
        if (us[i] > hi) hi = us[i];
    }
    printf("R,%s,runs=%d,mean_us=%lld,min_us=%lld,max_us=%lld\n", what, n, sum / n, lo, hi);
    return sum / n;
}

static void crypto_task(void *arg)
{
    static int64_t us[RUNS];                  // static: keeps the task stack small
    uint8_t msg[MSG_LEN], hash[32], sig[64];  // P-256 signature = 64 bytes
    size_t hlen = 0, slen = 0;
    int bad = 0;

    for (int i = 0; i < MSG_LEN; i++) {
        msg[i] = (uint8_t)i;
    }

    printf("CRYPTO_BENCH_START,cpu_mhz=%d,app_opt=%s,mbedtls_opt=%s,hw_sha=%s,msg_bytes=%d\n",
           CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ, APP_OPT, TLS_OPT, HW_SHA, MSG_LEN);

    // one key pair and one signature, made once
    psa_algorithm_t alg = PSA_ALG_ECDSA(PSA_ALG_SHA_256);
    psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&attr, 256);
    psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH);
    psa_set_key_algorithm(&attr, alg);

    mbedtls_svc_key_id_t key;
    if (psa_generate_key(&attr, &key) != PSA_SUCCESS ||
        psa_hash_compute(PSA_ALG_SHA_256, msg, MSG_LEN, hash, sizeof(hash), &hlen) != PSA_SUCCESS ||
        psa_sign_hash(key, alg, hash, hlen, sig, sizeof(sig), &slen) != PSA_SUCCESS) {
        printf("E,setup\n");
        vTaskDelete(NULL);
        return;
    }

    for (int r = 0; r < RUNS; r++) {
        int64_t t0 = esp_timer_get_time();
        psa_hash_compute(PSA_ALG_SHA_256, msg, MSG_LEN, hash, sizeof(hash), &hlen);
        us[r] = esp_timer_get_time() - t0;
    }
    int64_t sha_us = report("sha256", us, RUNS);

    for (int r = 0; r < RUNS; r++) {
        int64_t t0 = esp_timer_get_time();
        if (psa_verify_hash(key, alg, hash, hlen, sig, slen) != PSA_SUCCESS) {
            bad++;
        }
        us[r] = esp_timer_get_time() - t0;
    }
    int64_t ver_us = report("ecdsa_p256_verify", us, RUNS);

    for (int r = 0; r < 20; r++) {            // signing, for reference (a validator never signs)
        int64_t t0 = esp_timer_get_time();
        psa_sign_hash(key, alg, hash, hlen, sig, sizeof(sig), &slen);
        us[r] = esp_timer_get_time() - t0;
    }
    report("ecdsa_p256_sign", us, 20);

    int64_t per_check = ver_us + sha_us;
    printf("ESTIMATE,signatures=%d,per_check_us=%lld,total_us=%lld,verify_failures=%d\n",
           LAB_SIGS, per_check, LAB_SIGS * per_check, bad);
    printf("# estimate = %d signatures x (one ECDSA verify + one SHA-256 of ~%d bytes) for ota.signed.lab.test\n",
           LAB_SIGS, MSG_LEN);
    printf("# assumption: one verify per RRSIG, one RRSIG per RRset (checked with kdig +dnssec on Knot).\n");
    printf("# It leaves out DNS parsing, putting records into canonical form, the 20+ queries and their\n");
    printf("# round trips over Wi-Fi, and the cache. A real-internet chain has more signatures than this lab.\n");

    psa_destroy_key(key);
    printf("CRYPTO_BENCH_DONE\n");
    vTaskDelete(NULL);
}

void crypto_bench_run(void)
{
    // its own task: the main task's stack (3.5 KB) is too small for the key generation
    xTaskCreate(crypto_task, "crypto", 8192, NULL, 5, NULL);
}

#endif /* CRYPTO_BENCH */
