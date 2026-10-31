#pragma once

// run the benchmark: only called in the bench envs (built with -D BENCH_ROUNDS=...)
void bench_run(const char *const *names, int n);
