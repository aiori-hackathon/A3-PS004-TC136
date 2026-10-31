#include <string.h>
#include "esp_log.h"
#include "esp_tls.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "dot.h"
#include "certs.h"
#include "config.h"

#ifdef GATEWAY_PIN_SHA256
#include <stdio.h>
#include <strings.h>
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/pk.h"
#include "psa/crypto.h"
#endif

static const char *TAG = "dot";
static esp_tls_t *s_tls = NULL;

#ifdef GATEWAY_PIN_SHA256
// pin env: besides the CA check, the gateway's PUBLIC KEY must hash to GATEWAY_PIN_SHA256, i.e. the
// SHA-256 of its SubjectPublicKeyInfo (openssl x509 -pubkey | openssl pkey -pubin -outform DER |
// openssl dgst -sha256). A cert signed by our CA for another key is refused
static bool gateway_pin_ok(void)
{
    mbedtls_ssl_context *ssl = esp_tls_get_ssl_context(s_tls);
    const mbedtls_x509_crt *crt = ssl ? mbedtls_ssl_get_peer_cert(ssl) : NULL;
    if (crt == NULL) {
        ESP_LOGE(TAG, "no peer certificate to check the pin against");
        return false;
    }

    unsigned char der[160];
    int len = mbedtls_pk_write_pubkey_der(&crt->pk, der, sizeof(der));   // written at the end of der
    unsigned char hash[32];
    size_t hlen = 0;
    if (len <= 0 || psa_hash_compute(PSA_ALG_SHA_256, der + sizeof(der) - len, len,
                                     hash, sizeof(hash), &hlen) != PSA_SUCCESS) {
        ESP_LOGE(TAG, "could not hash the gateway key");
        return false;
    }

    char got[65];
    for (int i = 0; i < 32; i++) {
        snprintf(got + 2 * i, 3, "%02x", hash[i]);
    }
    ESP_LOGI(TAG, "gateway key sha256 %s", got);
    return strcasecmp(got, GATEWAY_PIN_SHA256) == 0;
}
#endif

esp_err_t dot_connect(void)
{
    esp_tls_cfg_t cfg = {
        .cacert_buf       = (const unsigned char *)ca_crt_start,
        .cacert_bytes     = ca_crt_end - ca_crt_start,
        .clientcert_buf   = (const unsigned char *)client_crt_start,
        .clientcert_bytes = client_crt_end - client_crt_start,
        .clientkey_buf    = (const unsigned char *)client_key_start,
        .clientkey_bytes  = client_key_end - client_key_start,
        .common_name      = GATEWAY_NAME,
        .timeout_ms       = 5000,
    };

    s_tls = esp_tls_init();
    if (s_tls == NULL) {
        ESP_LOGE(TAG, "out of memory");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "connecting to %s:%d as %s", GATEWAY_IP, GATEWAY_PORT, GATEWAY_NAME);
    int64_t t0 = esp_timer_get_time();
    int ret = esp_tls_conn_new_sync(GATEWAY_IP, strlen(GATEWAY_IP), GATEWAY_PORT, &cfg, s_tls);
    int64_t ms = (esp_timer_get_time() - t0) / 1000;

    if (ret != 1) {
        ESP_LOGE(TAG, "mTLS handshake FAILED after %lld ms", ms);
        dot_close();
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "mTLS handshake OK in %lld ms", ms);
#ifdef GATEWAY_PIN_SHA256
    if (!gateway_pin_ok()) {
        ESP_LOGE(TAG, "gateway key does not match the pin: refusing");
        dot_close();
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "gateway key matches the pin");
#endif
    return ESP_OK;
}

static int write_all(const uint8_t *buf, int len)
{
    while (len > 0) {
        int n = esp_tls_conn_write(s_tls, buf, len);
        if (n <= 0) {
            return -1;
        }
        buf += n;
        len -= n;
    }
    return 0;
}

static int read_all(uint8_t *buf, int len)
{
    while (len > 0) {
        int n = esp_tls_conn_read(s_tls, buf, len);
        if (n <= 0) {
            return -1;   // 0 = gateway closed the connection
        }
        buf += n;
        len -= n;
    }
    return 0;
}

esp_err_t dot_query(const char *name, dns_result_t *out)
{
    if (s_tls == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t buf[512];
    uint16_t id = esp_random() & 0xFFFF;   // random id: a forged reply must guess it

    // DNS over TCP/TLS: every message starts with its length in 2 bytes
    int qlen = dns_build_query(id, name, buf + 2, sizeof(buf) - 2);
    if (qlen < 0) {
        return ESP_ERR_INVALID_ARG;
    }
    buf[0] = qlen >> 8;
    buf[1] = qlen & 0xFF;
    if (write_all(buf, qlen + 2) != 0) {
        ESP_LOGE(TAG, "write failed");
        return ESP_FAIL;
    }

    uint8_t lenbuf[2];
    if (read_all(lenbuf, 2) != 0) {
        ESP_LOGE(TAG, "no reply");
        return ESP_FAIL;
    }
    int rlen = (lenbuf[0] << 8) | lenbuf[1];
    if (rlen > (int)sizeof(buf) || read_all(buf, rlen) != 0) {
        ESP_LOGE(TAG, "bad reply length %d", rlen);
        return ESP_FAIL;
    }
    if (dns_parse_reply(buf, rlen, id, out) != 0) {
        ESP_LOGE(TAG, "could not parse reply");
        return ESP_FAIL;
    }
    return ESP_OK;
}

void dot_close(void)
{
    if (s_tls != NULL) {
        esp_tls_conn_destroy(s_tls);
        s_tls = NULL;
    }
}
