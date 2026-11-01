// benchmark mode: ask every name for BENCH_ROUNDS rounds and print one CSV line per query,
// so a script can work out mean / median / p95 afterwards. Built only in the bench envs.
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "dot.h"
#include "bench.h"

#ifndef BENCH_ROUNDS
#define BENCH_ROUNDS 50
#endif
// pause between rounds: longer than the gateway's cache lifetime in a cold-cache run
#ifndef BENCH_GAP_MS
#define BENCH_GAP_MS 2000
#endif
// 0: one TLS connection for all rounds, 1: a new connection (new handshake) every round
#ifndef BENCH_NEW_CONN
#define BENCH_NEW_CONN 0
#endif

void bench_run(const char *const *names, int n)
{
    int errors = 0;
    bool connected = false;

    printf("BENCH_START,rounds=%d,names=%d,gap_ms=%d,new_conn=%d\n", BENCH_ROUNDS, n, BENCH_GAP_MS, BENCH_NEW_CONN);
    printf("# H = handshake: H,round,handshake_us,free_heap\n");
    printf("# Q = query:     Q,round,name,verdict,rcode,ad,latency_us,free_heap\n");

    for (int round = 1; round <= BENCH_ROUNDS; round++) {
        if (!connected) {
            int64_t t0 = esp_timer_get_time();
            if (dot_connect() != ESP_OK) {
                printf("E,%d,connect\n", round);
                errors++;
                vTaskDelay(pdMS_TO_TICKS(BENCH_GAP_MS));
                continue;
            }
            printf("H,%d,%lld,%lu\n", round, esp_timer_get_time() - t0, (unsigned long)esp_get_free_heap_size());
            connected = true;
        }

        for (int i = 0; i < n; i++) {
            dns_result_t r;
            int64_t t0 = esp_timer_get_time();
            if (dot_query(names[i], &r) != ESP_OK) {
                printf("E,%d,%s\n", round, names[i]);
                errors++;
                dot_close();
                connected = false;   // the next round connects again
                break;
            }
            int64_t us = esp_timer_get_time() - t0;
            printf("Q,%d,%s,%s,%d,%d,%lld,%lu\n", round, names[i], dns_verdict_str(dns_verdict(&r)),
                   r.rcode, r.ad, us, (unsigned long)esp_get_free_heap_size());
        }

        if (BENCH_NEW_CONN && connected) {
            dot_close();
            connected = false;
        }
        vTaskDelay(pdMS_TO_TICKS(BENCH_GAP_MS));
    }

    if (connected) {
        dot_close();
    }
    printf("BENCH_DONE,errors=%d\n", errors);
}
