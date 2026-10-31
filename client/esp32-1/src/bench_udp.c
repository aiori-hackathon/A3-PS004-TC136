// baseline benchmark: the same nine names as bench.c, but as plain DNS over UDP (no TLS, no
// authentication). Comparing it with the DoT numbers shows what the secure channel costs.
// Only built into the bench-udp env (-D PLAIN_DNS_PORT=53). Lines start with U so a script can
// tell them from the Q / H lines of bench.c.
#ifdef PLAIN_DNS_PORT

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "lwip/sockets.h"
#include "dns.h"
#include "config.h"
#include "bench.h"

#ifndef BENCH_ROUNDS
#define BENCH_ROUNDS 50
#endif
#ifndef BENCH_GAP_MS
#define BENCH_GAP_MS 2000
#endif

// static: the main task's stack is only 3.5 KB
static uint8_t s_buf[1232];

void bench_udp_run(const char *const *names, int n)
{
    int errors = 0;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        printf("E,0,socket\n");
        return;
    }
    struct timeval tv = { .tv_sec = 2, .tv_usec = 0 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in gw = { .sin_family = AF_INET, .sin_port = htons(PLAIN_DNS_PORT) };
    inet_pton(AF_INET, GATEWAY_IP, &gw.sin_addr);

    printf("BENCH_START,mode=udp,target=%s:%d,rounds=%d,names=%d,gap_ms=%d\n",
           GATEWAY_IP, PLAIN_DNS_PORT, BENCH_ROUNDS, n, BENCH_GAP_MS);
    printf("# U = plain UDP query: U,round,name,verdict,rcode,ad,latency_us,free_heap\n");

    for (int round = 1; round <= BENCH_ROUNDS; round++) {
        for (int i = 0; i < n; i++) {
            uint16_t id = (uint16_t)esp_random();
            int qlen = dns_build_query(id, names[i], s_buf, sizeof(s_buf));

            int64_t t0 = esp_timer_get_time();
            if (qlen < 0 || sendto(sock, s_buf, qlen, 0, (struct sockaddr *)&gw, sizeof(gw)) < 0) {
                printf("E,%d,%s\n", round, names[i]);
                errors++;
                continue;
            }
            int len = recvfrom(sock, s_buf, sizeof(s_buf), 0, NULL, NULL);
            int64_t us = esp_timer_get_time() - t0;

            dns_result_t r;
            if (len <= 0 || dns_parse_reply(s_buf, len, id, &r) != 0) {
                printf("E,%d,%s\n", round, names[i]);   // timeout, wrong id or bad reply
                errors++;
                continue;
            }
            printf("U,%d,%s,%s,%d,%d,%lld,%lu\n", round, names[i], dns_verdict_str(dns_verdict(&r)),
                   r.rcode, r.ad, us, (unsigned long)esp_get_free_heap_size());
        }
        vTaskDelay(pdMS_TO_TICKS(BENCH_GAP_MS));
    }

    close(sock);
    printf("BENCH_DONE,errors=%d\n", errors);
}

#endif /* PLAIN_DNS_PORT */
