#pragma once

// run the benchmark: only called in the bench envs (built with -D BENCH_ROUNDS=...)
void bench_run(const char *const *names, int n);

#ifdef PLAIN_DNS_PORT
// the same names as plain DNS over UDP: the baseline for what the secure channel costs
void bench_udp_run(const char *const *names, int n);
#endif
