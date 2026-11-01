# Setup

You need your own PKI: the committed `.crt` files belong to the authors' CA and their keys are not published.

**Tools used:** Docker 29, Containerlab 0.82, Knot 3.6.2, Unbound 1.25.2 (Pi: 1.26.2), stunnel 5.76 (Pi: 5.74), OpenSSL 3.5, PlatformIO 6.1 with ESP-IDF (ESP32-S3).

If you just want it to happen, `tests/setup-lab.sh --dry-run` prints every step for mode 1 and `tests/setup-lab.sh` runs them, then runs the test suite. Its self-test and dry run pass, but it hasn't had a full run on a clean clone yet, so treat it like a keen intern and watch what it does.

## Mode 1: everything in containers

```bash
# 1. PKI (P-256)
cd gateway/pki
openssl genpkey -algorithm ec -pkeyopt ec_paramgen_curve:prime256v1 -out ca.key
openssl req -new -x509 -key ca.key -days 360 -subj "/CN=LAB CA" \
  -addext "basicConstraints=critical,CA:TRUE" -addext "keyUsage=critical,keyCertSign,cRLSign" -out ca.crt
for n in gateway esp32-1; do
  openssl genpkey -algorithm ec -pkeyopt ec_paramgen_curve:prime256v1 -out $n.key
  openssl req -new -key $n.key -subj "/CN=$n" -out $n.csr
  openssl x509 -req -in $n.csr -CA ca.crt -CAkey ca.key -days 360 -extfile $n.ext -out $n.crt && rm $n.csr
done
cd ../..

# 2. the two small images
docker build -t lab-unbound -f gateway/unbound/Containerfile gateway/unbound
docker build -t lab-stunnel -f gateway/stunnel/Containerfile gateway/stunnel

# 3. start the lab (Knot generates new DNSSEC keys on first start)
cd lab && containerlab deploy && cd ..
```

Knot's new keys mean two new DS values (the hand-signed zones ship pre-signed and need nothing):

```bash
docker exec clab-dnssec-knot keymgr lab.test. list           # note the KSK tag (flag 257)
docker exec clab-dnssec-knot keymgr lab.test. ds <tag>       # digest-2 line -> trust-anchor in gateway/unbound/unbound.conf
docker exec clab-dnssec-knot keymgr signed.lab.test. list
docker exec clab-dnssec-knot keymgr signed.lab.test. ds <tag>  # -> the signed.lab.test. DS line in gateway/knot/lab.test.zone
docker exec clab-dnssec-knot knotc zone-reload lab.test.
docker exec clab-dnssec-knot knotc zone-ksk-submitted lab.test.
docker exec clab-dnssec-knot knotc zone-ksk-submitted signed.lab.test.
docker restart clab-dnssec-unbound
```

Ask through the gateway as the device would (`ad` flag expected):

```bash
kdig @127.0.0.1 -p 8530 +tls +tls-ca=gateway/pki/ca.crt +tls-hostname=gateway.lab.test \
  +tls-certfile=gateway/pki/esp32-1.crt +tls-keyfile=gateway/pki/esp32-1.key A ota.lab.test
```

The nine names: `ota.lab.test`, `ota.signed`, `ota.unsigned`, `ota.expired`, `ota.future`, `ota.badsig`, `ota.unknownalg`, `ota.nosig`, `ota.nokey` (all under `.lab.test`).
Expected results are in `results/01` to `04`. Restart Unbound between tests: it caches.

Then run the suite:

```bash
tests/e2e.sh --mode lab
```


## Mode 2: Raspberry Pi gateway

```bash
cd lab && containerlab deploy --node-filter knot     # laptop: Knot only, published on :5300
```

On the Pi (Debian): `apt install unbound stunnel4 knot-dnsutils`; copy **only** `ca.crt`, `gateway.crt` and `gateway.key` to
`/etc/stunnel/` (owner root, key mode 600, **never `ca.key`**); use `pi/stunnel/dot.conf` and `pi/unbound/lab.conf`
(set `stub-addr` to `<laptop-ip>@5300` and the trust anchor); then `systemctl enable --now stunnel@dot`
(keep `stunnel4.service` disabled: both would claim port 853).


## Flashing the ESP32

```bash
cd client/esp32-1
cp src/secrets.example.h src/secrets.h                 # Wi-Fi name and password (git-ignored)
cp ../../gateway/pki/{ca.crt,esp32-1.crt,esp32-1.key} certs/
# set the gateway address of each env in platformio.ini
pio run -e pi  -t upload      # mode 2: the Pi on :853
pio run -e lab -t upload      # mode 1: the laptop's stunnel on :8530
pio device monitor
```


## Attacks you can repeat

| Test | How | Expected |
|---|---|---|
| no client certificate | the kdig command from mode 1 without `+tls-certfile/keyfile` | TLS alert, nothing reaches Unbound |
| fake CA | `tests/make-fakeca.sh ~/fakeca`, then kdig with `fake-esp32-1.*` | refused |
| rogue gateway | run `attacks/rogue-gateway/rogue-stunnel.conf` with the fake gateway cert on port 8531, then `pio run -e rogue -t upload` | `mbedtls -0x2700`, `no gateway, stopping` |
| CD bit | kdig with `+cd` on `ota.badsig.lab.test` | `SERVFAIL`, same as without it |
| gateway down | stop stunnel, reset the ESP32 | refuses, no fallback |


## Who holds which key

| Key | Held by | Never |
|---|---|---|
| `ca.key` | the laptop only | on the gateway or the ESP32 |
| `gateway.key` | the gateway | in git |
| `esp32-1.key` | the device (and its build) | in git |
| Knot's DNSSEC keys | a Docker volume on the laptop | in git (the DS values are public and are in git) |


## Best before October 2027

The test data has an expiry date, and I find this funnier than I should. All the certificates end on 17 October 2027. The hand-signed zones stop being valid on 20 October 2027. On 21 October 2027 `ota.future` stops being from the future. After that, make your own PKI and re-sign the zones, or the results will drift for reasons that have nothing to do with your code.

## What's where

| Path | What |
|---|---|
| `client/esp32-1/` | The ESP32 firmware (ESP-IDF via PlatformIO) |
| `gateway/` | Knot, Unbound and stunnel configs, the nine zones, and public certificates. Private keys are not in the repo |
| `pi/` | The configs that differ on the Pi |
| `lab/dnssec.clab.yml` | The Containerlab topology |
| `attacks/rogue-gateway/` | The fake gateway |
| `tests/` | `e2e.sh`, `setup-lab.sh`, `bench_stats.py`, `make-fakeca.sh` |
| `results/` | The evidence |
| `docs/` | Figures and the longer write-ups |
