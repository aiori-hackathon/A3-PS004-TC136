# Test cases

Formal test cases derived **only** from the raw files in [`results/`](../results/). Each *Observed* line is quoted verbatim from the evidence file.

**Status:** PASS means the recorded output matches the expected column. It is the author's recorded run, not an independent re-run. The automated re-check for the gateway rows is [`tests/e2e.sh`](../tests/e2e.sh).

**Environments:** the runs used three set-ups, named in each case:

| Name | What |
|---|---|
| Podman lab (27 Oct) | mode 1 on the first Podman lab, `lab.test` signed by Knot 3.6.1 |
| Docker/Containerlab lab (1 Nov) | mode 1 on the Containerlab lab, Knot 3.6.2 |
| Mode 2 hardware (31 Oct, 1 Nov) | ESP32-S3, Raspberry Pi Zero 2 W gateway, Knot on the laptop |

## Summary

| ID | Test | Status | Evidence |
|---|---|---|---|
| TC-01 | Signed zone validates | PASS | [results/01](../results/01-zones-via-dot.txt) |
| TC-02 | Signed child zone validates through the chain | PASS | [results/01](../results/01-zones-via-dot.txt) |
| TC-03 | Unsigned zone is insecure, not bogus | PASS | [results/01](../results/01-zones-via-dot.txt) |
| TC-04 | Expired signatures are refused | PASS | [results/01](../results/01-zones-via-dot.txt) |
| TC-05 | Signatures not yet valid are refused | PASS | [results/01](../results/01-zones-via-dot.txt) |
| TC-06 | A corrupted signature is refused | PASS | [results/01](../results/01-zones-via-dot.txt) |
| TC-07 | Unknown DS algorithm: silent downgrade | PASS | [results/02](../results/02-unknownalg-downgrade.txt) |
| TC-08 | Missing signatures are refused | PASS | [results/03](../results/03-nosig-missing-rrsigs.txt) |
| TC-09 | A DS for a key that is not in the zone is refused | PASS (EDE 6, not the 9 the RFC suggests) | [results/04](../results/04-nokey-ds-without-key.txt) |
| TC-10 | A device with a certificate from our CA is served | PASS | [results/05](../results/05-dot-mtls-ok.txt) |
| TC-11 | No client certificate is refused before any DNS | PASS | [results/06](../results/06-dot-no-cert.txt) |
| TC-12 | A client certificate from a fake CA (same names) is refused | PASS | [results/08](../results/08-fake-ca-rejected.txt) |
| TC-13 | The gateway's own certificate cannot be used as a client certificate | PASS | [results/13](../results/13-gateway-cert-as-client.txt) |
| TC-14 | A client cannot switch validation off with the CD bit | PASS | [results/11](../results/11-cd-flag.txt) |
| TC-15 | Unbound cannot be reached except through stunnel | PASS | [results/10](../results/10-back-door.txt) |
| TC-16 | The reason for every failure is visible in Unbound's log | PASS | [results/12](../results/12-unbound-reasons.txt) |
| TC-17 | ESP32 end to end, first run (mode 1, early firmware) | Superseded by TC-18 and TC-19 (kept as history) | [results/07](../results/07-esp32-dot-verdicts.txt) |
| TC-18 | ESP32 end to end through the Raspberry Pi (mode 2, six names) | PASS | [results/14](../results/14-esp32-via-pi-verdicts.txt) |
| TC-19 | ESP32 on all nine names through the Pi (mode 2) | PASS | [results/16](../results/16-esp32-all-zones-via-pi.txt) |
| TC-20 | The ESP32 refuses a rogue gateway | PASS | [results/15](../results/15-rogue-gateway.txt) |
| TC-21 | Fail closed: the gateway is down | PASS | [results/09](../results/09-gateway-down-fail-closed.txt) |

## TC-01 · Signed zone validates

| | |
|---|---|
| Objective | The gateway validates a correctly signed zone and sets AD. |
| Where | mode 1, Podman lab, 27 Oct 2026 (Knot 3.6.1, Unbound 1.25.2, stunnel 5.76) |
| Procedure | `kdig @127.0.0.1 -p 8530 +tls +tls-ca=ca.crt +tls-hostname=gateway.lab.test +tls-certfile=esp32-1.crt +tls-keyfile=esp32-1.key A ota.lab.test` |
| Expected | NOERROR with the `ad` flag |
| Observed | `;; ->>HEADER<<- opcode: QUERY; status: NOERROR; id: 52395`<br>`;; Flags: qr rd ra ad; QUERY: 1; ANSWER: 1; AUTHORITY: 0; ADDITIONAL: 1` |
| Status | PASS |
| Evidence | [results/01](../results/01-zones-via-dot.txt) |

## TC-02 · Signed child zone validates through the chain

| | |
|---|---|
| Objective | A child zone with a DS in the parent validates (`ad`). |
| Where | mode 1, Podman lab, 27 Oct 2026 (Knot 3.6.1, Unbound 1.25.2, stunnel 5.76) |
| Procedure | `kdig @127.0.0.1 -p 8530 +tls +tls-ca=ca.crt +tls-hostname=gateway.lab.test +tls-certfile=esp32-1.crt +tls-keyfile=esp32-1.key A ota.signed.lab.test` |
| Expected | NOERROR with the `ad` flag |
| Observed | `;; ->>HEADER<<- opcode: QUERY; status: NOERROR; id: 25363`<br>`;; Flags: qr rd ra ad; QUERY: 1; ANSWER: 1; AUTHORITY: 0; ADDITIONAL: 1` |
| Status | PASS |
| Evidence | [results/01](../results/01-zones-via-dot.txt) |

## TC-03 · Unsigned zone is insecure, not bogus

| | |
|---|---|
| Objective | An unsigned zone answers, without `ad`. |
| Where | mode 1, Podman lab, 27 Oct 2026 (Knot 3.6.1, Unbound 1.25.2, stunnel 5.76) |
| Procedure | `kdig @127.0.0.1 -p 8530 +tls +tls-ca=ca.crt +tls-hostname=gateway.lab.test +tls-certfile=esp32-1.crt +tls-keyfile=esp32-1.key A ota.unsigned.lab.test` |
| Expected | NOERROR, no `ad` flag |
| Observed | `;; ->>HEADER<<- opcode: QUERY; status: NOERROR; id: 17700`<br>`;; Flags: qr rd ra; QUERY: 1; ANSWER: 1; AUTHORITY: 0; ADDITIONAL: 1` |
| Status | PASS |
| Evidence | [results/01](../results/01-zones-via-dot.txt) |

## TC-04 · Expired signatures are refused

| | |
|---|---|
| Objective | Expired RRSIGs make the answer bogus. |
| Where | mode 1, Podman lab, 27 Oct 2026 (Knot 3.6.1, Unbound 1.25.2, stunnel 5.76) |
| Procedure | `kdig @127.0.0.1 -p 8530 +tls +tls-ca=ca.crt +tls-hostname=gateway.lab.test +tls-certfile=esp32-1.crt +tls-keyfile=esp32-1.key A ota.expired.lab.test` |
| Expected | SERVFAIL, EDE 7 |
| Observed | `;; ->>HEADER<<- opcode: QUERY; status: SERVFAIL; id: 9549`<br>`;; EDE: 7 (Signature Expired): 'validation failure <ota.expired.lab.test. A IN>: signature expired from 10.53.0.2 for key expired.lab.test. while building chain of trust'` |
| Status | PASS |
| Evidence | [results/01](../results/01-zones-via-dot.txt) |

## TC-05 · Signatures not yet valid are refused

| | |
|---|---|
| Objective | RRSIGs with a future inception date make the answer bogus. |
| Where | mode 1, Podman lab, 27 Oct 2026 (Knot 3.6.1, Unbound 1.25.2, stunnel 5.76) |
| Procedure | `kdig @127.0.0.1 -p 8530 +tls +tls-ca=ca.crt +tls-hostname=gateway.lab.test +tls-certfile=esp32-1.crt +tls-keyfile=esp32-1.key A ota.future.lab.test` |
| Expected | SERVFAIL, EDE 8 |
| Observed | `;; ->>HEADER<<- opcode: QUERY; status: SERVFAIL; id: 29753`<br>`;; EDE: 8 (Signature Not Yet Valid): 'validation failure <ota.future.lab.test. A IN>: signature before inception date from 10.53.0.2 for key future.lab.test. while building chain of trust'` |
| Status | PASS |
| Evidence | [results/01](../results/01-zones-via-dot.txt) |

## TC-06 · A corrupted signature is refused

| | |
|---|---|
| Objective | One changed character in the A record's RRSIG makes the answer bogus. |
| Where | mode 1, Podman lab, 27 Oct 2026 (Knot 3.6.1, Unbound 1.25.2, stunnel 5.76) |
| Procedure | `kdig @127.0.0.1 -p 8530 +tls +tls-ca=ca.crt +tls-hostname=gateway.lab.test +tls-certfile=esp32-1.crt +tls-keyfile=esp32-1.key A ota.badsig.lab.test` |
| Expected | SERVFAIL, EDE 6 |
| Observed | `;; ->>HEADER<<- opcode: QUERY; status: SERVFAIL; id: 41033`<br>`;; EDE: 6 (DNSSEC Bogus): 'validation failure <ota.badsig.lab.test. A IN>: signature crypto failed from 10.53.0.2'` |
| Status | PASS |
| Evidence | [results/01](../results/01-zones-via-dot.txt) |

## TC-07 · Unknown DS algorithm: silent downgrade

| | |
|---|---|
| Objective | A signed zone whose parent publishes a DS with an unassigned algorithm (80) looks unsigned. Control: the same DS with algorithm 8 gives `ad`. |
| Where | mode 1, Docker/Containerlab lab, 1 Nov 2026 (Knot 3.6.2, Unbound 1.25.2) |
| Procedure | `kdig @127.0.0.1 -p 8530 +tls +tls-ca=ca.crt +tls-hostname=gateway.lab.test +tls-certfile=esp32-1.crt +tls-keyfile=esp32-1.key A ota.unknownalg.lab.test` (Unbound restarted first); then the same with the DS algorithm set to 8 |
| Expected | NOERROR without `ad` (algorithm 80); NOERROR with `ad` (control, algorithm 8) |
| Observed | `;; ->>HEADER<<- opcode: QUERY; status: NOERROR; id: 7120`<br>`# A/B result: DS algorithm 8 -> NOERROR + ad (SECURE). DS algorithm 80 -> NOERROR, no ad (INSECURE). Nothing else changed,` |
| Status | PASS |
| Evidence | [results/02](../results/02-unknownalg-downgrade.txt) |

## TC-08 · Missing signatures are refused

| | |
|---|---|
| Objective | The parent publishes a DS and the zone has a DNSKEY but no RRSIG at all. |
| Where | mode 1, Docker/Containerlab lab, 1 Nov 2026 (Knot 3.6.2, Unbound 1.25.2) |
| Procedure | `kdig @127.0.0.1 -p 8530 +tls +tls-ca=ca.crt +tls-hostname=gateway.lab.test +tls-certfile=esp32-1.crt +tls-keyfile=esp32-1.key A ota.nosig.lab.test` |
| Expected | SERVFAIL, EDE 10 |
| Observed | `;; EDE: 10 (RRSIGs Missing): 'validation failure <ota.nosig.lab.test. A IN>: no signatures from 10.54.0.2 for key nosig.lab.test. while building chain of trust'`<br>`zone file RRSIG records: 0` |
| Status | PASS |
| Evidence | [results/03](../results/03-nosig-missing-rrsigs.txt) |

## TC-09 · A DS for a key that is not in the zone is refused

| | |
|---|---|
| Objective | Valid signatures, but the parent's DS belongs to a different key. |
| Where | mode 1, Docker/Containerlab lab, 1 Nov 2026 (Knot 3.6.2, Unbound 1.25.2) |
| Procedure | `kdig @127.0.0.1 -p 8530 +tls +tls-ca=ca.crt +tls-hostname=gateway.lab.test +tls-certfile=esp32-1.crt +tls-keyfile=esp32-1.key A ota.nokey.lab.test` |
| Expected | SERVFAIL (the RFC suggests EDE 9) |
| Observed | `;; EDE: 6 (DNSSEC Bogus): 'validation failure <ota.nokey.lab.test. A IN>: no keys have a DS with algorithm ECDSAP256SHA256 from 10.54.0.2 for key nokey.lab.test. while building chain of trust'`<br>`# - the RFC suggests EDE 9 'DNSKEY Missing' for this case; Unbound 1.25.2 answers EDE 6 'DNSSEC Bogus' with 'no keys have a DS with algorithm ... for key nokey.lab.test.'` |
| Status | PASS (EDE 6, not the 9 the RFC suggests) |
| Evidence | [results/04](../results/04-nokey-ds-without-key.txt) |

## TC-10 · A device with a certificate from our CA is served

| | |
|---|---|
| Objective | Mutual TLS accepts the device certificate. |
| Where | mode 1, Podman lab, 27 Oct 2026 (Knot 3.6.1, Unbound 1.25.2, stunnel 5.76) |
| Procedure | `kdig @127.0.0.1 -p 8530 +tls +tls-ca=ca.crt +tls-hostname=gateway.lab.test +tls-certfile=esp32-1.crt +tls-keyfile=esp32-1.key A ota.lab.test` |
| Expected | TLS session, certificate accepted, DNS answer with `ad` |
| Observed | `;; TLS session (TLS1.3)-(ECDHE-X25519)-(ECDSA-SECP256R1-SHA256)-(AES-256-GCM)`<br>`2026.10.27 07:40:36 LOG5[16]: Certificate accepted at depth=0: CN=esp32-1` |
| Status | PASS |
| Evidence | [results/05](../results/05-dot-mtls-ok.txt) |

## TC-11 · No client certificate is refused before any DNS

| | |
|---|---|
| Objective | stunnel requires a client certificate. |
| Where | mode 1, Podman lab, 27 Oct 2026 (Knot 3.6.1, Unbound 1.25.2, stunnel 5.76) |
| Procedure | `kdig @127.0.0.1 -p 8530 +tls +tls-ca=ca.crt +tls-hostname=gateway.lab.test A ota.lab.test` (no client cert) |
| Expected | TLS alert, 0 bytes reach Unbound |
| Observed | `;; WARNING: TLS, failed to receive reply (A TLS fatal alert has been received.)`<br>`2026.10.27 07:40:38 LOG3[17]: SSL_accept: tls_process_client_certificate@ssl/statem/statem_srvr.c:3764: error:0A0000C7:SSL routines::peer did not return a certificate`<br>`2026.10.27 07:40:38 LOG5[17]: Connection reset/closed: 0 byte(s) sent to TLS, 0 byte(s) sent to socket` |
| Status | PASS |
| Evidence | [results/06](../results/06-dot-no-cert.txt) |

## TC-12 · A client certificate from a fake CA (same names) is refused

| | |
|---|---|
| Objective | An attacker's CA called "LAB CA", with the same CN and SAN, cannot get a device in. |
| Where | mode 1, Podman lab, 27 Oct 2026 (Knot 3.6.1, Unbound 1.25.2, stunnel 5.76) |
| Procedure | `tests/make-fakeca.sh <dir>`, then kdig with `fake-esp32-1.crt` and `fake-esp32-1.key` |
| Expected | Certificate rejected, nothing reaches Unbound |
| Observed | `2026.10.27 07:40:41 LOG4[18]: CERT: Pre-verification error: unable to get local issuer certificate`<br>`2026.10.27 07:40:41 LOG4[18]: Rejected by CERT at depth=0: CN=esp32-1` |
| Status | PASS |
| Evidence | [results/08](../results/08-fake-ca-rejected.txt) |

## TC-13 · The gateway's own certificate cannot be used as a client certificate

| | |
|---|---|
| Objective | `gateway.crt` is signed by the real CA but marked serverAuth only. |
| Where | mode 1, Docker/Containerlab lab, 1 Nov 2026 (Knot 3.6.2, Unbound 1.25.2) |
| Procedure | kdig with `gateway.crt` and `gateway.key` as the client certificate; `openssl verify -CAfile ca.crt -purpose sslclient gateway.crt` |
| Expected | Refused: wrong certificate purpose |
| Observed | `LOG4[41]: CERT: Pre-verification error: unsuitable certificate purpose`<br>`error 26 at 0 depth lookup: unsuitable certificate purpose` |
| Status | PASS |
| Evidence | [results/13](../results/13-gateway-cert-as-client.txt) |

## TC-14 · A client cannot switch validation off with the CD bit

| | |
|---|---|
| Objective | With `ignore-cd-flag: no` a query with CD returns the bogus data; with `ignore-cd-flag: yes` it must not. |
| Where | mode 1, Podman lab, 27 Oct 2026 (Knot 3.6.1, Unbound 1.25.2, stunnel 5.76) |
| Procedure | `kdig ... +cd A ota.badsig.lab.test`, before and after the setting |
| Expected | before: NOERROR plus the bogus address; after: SERVFAIL |
| Observed | `;; ->>HEADER<<- opcode: QUERY; status: NOERROR; id: 23160`<br>`;; ->>HEADER<<- opcode: QUERY; status: SERVFAIL; id: 58035` |
| Status | PASS |
| Evidence | [results/11](../results/11-cd-flag.txt) |

## TC-15 · Unbound cannot be reached except through stunnel

| | |
|---|---|
| Objective | Two locks: `access-control` allows only stunnel, and no host port is published. |
| Where | mode 1, Podman lab, 27 Oct 2026 (Knot 3.6.1, Unbound 1.25.2, stunnel 5.76) |
| Procedure | `kdig @10.53.0.3 A ota.lab.test` from a throwaway container; `dig @127.0.0.1 -p 5301 ota.lab.test`; `podman port unbound` |
| Expected | REFUSED (EDE 18); connection refused; no published ports |
| Observed | `;; ->>HEADER<<- opcode: QUERY; status: REFUSED; id: 22601`<br>`;; EDE: 18 (Prohibited)`<br>`;; communications error to 127.0.0.1#5301: connection refused`<br>`(no published ports)` |
| Status | PASS |
| Evidence | [results/10](../results/10-back-door.txt) |

## TC-16 · The reason for every failure is visible in Unbound's log

| | |
|---|---|
| Objective | `val-log-level: 2` names the cause of each validation failure. |
| Where | mode 1, Podman lab, 27 Oct 2026 (Knot 3.6.1, Unbound 1.25.2, stunnel 5.76) |
| Procedure | `docker logs`/`podman logs unbound` for the run of TC-04 to TC-06 |
| Expected | one `validation failure` line per bogus name |
| Observed | `[1793086833] unbound[1:0] info: validation failure <ota.badsig.lab.test. A IN>: signature crypto failed from 10.53.0.2`<br>`[1793086833] unbound[1:0] info: validation failure <ota.expired.lab.test. A IN>: signature expired from 10.53.0.2 for key expired.lab.test. while building chain of trust` |
| Status | PASS |
| Evidence | [results/12](../results/12-unbound-reasons.txt) |

## TC-17 · ESP32 end to end, first run (mode 1, early firmware)

| | |
|---|---|
| Objective | First proof that the device talks mutual TLS and reads the verdicts. |
| Where | mode 1, 23 Oct 2026, early firmware (no must-be-secure list yet), phone hotspot |
| Procedure | `pio run -e lab`, serial monitor |
| Expected | TLS handshake, six verdicts |
| Observed | `I (10499) dot: mTLS handshake OK in 5908 ms`<br>`I (12109) client: ota.unsigned.lab.test    INSECURE  10.53.0.7  (rcode 0, ad 0, 171 ms)`<br>`W (14259) client: ota.badsig.lab.test      BOGUS     refused  (rcode 2, ede 6, 281 ms)` |
| Status | Superseded by TC-18 and TC-19 (kept as history) |
| Evidence | [results/07](../results/07-esp32-dot-verdicts.txt) |

## TC-18 · ESP32 end to end through the Raspberry Pi (mode 2, six names)

| | |
|---|---|
| Objective | The real gateway on the Pi, the device refusing what it must. |
| Where | mode 2, 31 Oct 2026, Pi Zero 2 W (Unbound 1.26.2, stunnel 5.74) |
| Procedure | `pio run -e pi -t upload`, serial monitor |
| Expected | mutual TLS OK; SECURE names used; unsigned must-be-secure name refused; bogus names refused |
| Observed | `I (5879) dot: mTLS handshake OK in 1684 ms`<br>`W (6649) client: ota.unsigned.lab.test    INSECURE  refused: must be secure  (rcode 0, ad 0, 493 ms)`<br>`2026-10-31T18:58:37: LOG5[3]: Certificate accepted at depth=0: CN=esp32-1` |
| Status | PASS |
| Evidence | [results/14](../results/14-esp32-via-pi-verdicts.txt) |

## TC-19 · ESP32 on all nine names through the Pi (mode 2)

| | |
|---|---|
| Objective | The silent downgrade and the two new broken zones are refused by the device. |
| Where | mode 2, 1 Nov 2026, Pi Zero 2 W; all nine answers came from the Pi's cache |
| Procedure | `pio run -e pi -t upload`, serial monitor |
| Expected | `ota.unknownalg` refused (must be secure); `ota.nosig` and `ota.nokey` refused |
| Observed | `W (6339) client: ota.unknownalg.lab.test  INSECURE  refused: must be secure  (rcode 0, ad 0, 10 ms)`<br>`W (6349) client: ota.nosig.lab.test       BOGUS     refused  (rcode 2, ede 10, 10 ms)`<br>`W (6369) client: ota.nokey.lab.test       BOGUS     refused  (rcode 2, ede 6, 20 ms)` |
| Status | PASS |
| Evidence | [results/16](../results/16-esp32-all-zones-via-pi.txt) |

## TC-20 · The ESP32 refuses a rogue gateway

| | |
|---|---|
| Objective | A gateway with a certificate from a fake CA (same CN, SAN and issuer name) must not be trusted. |
| Where | mode 1 laptop as the rogue, 31 Oct 2026 |
| Procedure | `tests/make-fakeca.sh`, rogue stunnel on port 8531, `pio run -e rogue -t upload`, serial monitor |
| Expected | certificate verification fails on the device, no DNS query sent |
| Observed | `E (5299) esp-tls-mbedtls: mbedtls_ssl_handshake returned -0x2700`<br>`E (5309) dot: mTLS handshake FAILED after 1139 ms`<br>`E (5309) client: no gateway, stopping` |
| Status | PASS |
| Evidence | [results/15](../results/15-rogue-gateway.txt) |

## TC-21 · Fail closed: the gateway is down

| | |
|---|---|
| Objective | With stunnel stopped on the Pi the ESP32 must refuse everything and never fall back to plain DNS. |
| Where | mode 2, 31 Oct 2026 |
| Procedure | `sudo systemctl stop stunnel@dot` on the Pi, reset the ESP32 |
| Expected | connection fails, device stops, no DNS query |
| Observed | `E (4229) esp-tls: [sock=54] delayed connect error: Connection reset by peer`<br>`E (4229) dot: mTLS handshake FAILED after 44 ms`<br>`E (4229) client: no gateway, stopping` |
| Status | PASS |
| Evidence | [results/09](../results/09-gateway-down-fail-closed.txt) |

## Automated re-check

`tests/e2e.sh` repeats the gateway checks and prints a PASS/FAIL table (`--mode lab`, or `--mode pi --pi user@host`). It judges verdicts only (status, `ad` flag, EDE), never timings.

| e2e row | Test case |
|---|---|
| V1 to V3 | TC-01 to TC-03 |
| V4 to V6 | TC-04 to TC-06 |
| V7 | TC-07 |
| V8 | TC-08 |
| V9 | TC-09 |
| A1 | TC-11 |
| A2 | TC-12 |
| A3 | TC-13 |
| C1 | TC-14 |
| B1, B2 | TC-15 |

## Not tested yet

No result file exists for any of these, so none of them is claimed anywhere above.

| Item | State |
|---|---|
| Benchmark | only the warm-cache, reused-connection run exists ([results/17](../results/17-benchmark-warm-reused.txt)); it is a measurement, not a test case. New-connection and cold-cache runs not done |
| Plain-UDP baseline and on-device crypto cost | not measured |
| SNTP, so the ESP32 checks certificate dates | not implemented (a limit in the README) |
| Pinning the gateway's key | not implemented |
| Certificate revocation | not implemented |
| Upstream tampering attack on the Pi to Knot hop | not run |
| ESP32 on all nine names in mode 1 with the current firmware | TC-17 is the early firmware; TC-19 is mode 2 |
| `tests/e2e.sh --mode pi` against the real Pi | the script exists; only `--mode lab` was run |
