#pragma once

// time SHA-256 and one ECDSA P-256 check on this chip, no network: only called in the crypto-bench env
void crypto_bench_run(void);
