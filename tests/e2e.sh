#!/usr/bin/env bash
# End-to-end checks of the gateway: prints a PASS/FAIL table, exits 1 if anything failed.
#
#   tests/e2e.sh                          mode 1: the containerlab lab on this laptop (stunnel on :8530)
#   tests/e2e.sh --mode pi --pi user@host mode 2: the Raspberry Pi gateway (stunnel on :853, back door checked over ssh)
#
# It judges verdicts only (status, AD flag, EDE code), never timings, so a warm or cold
# cache cannot change a result. It does not restart or change anything.
# "Refused" checks only count when a valid client was served first (positive control),
# so a dead gateway can never make an attack test pass.
# Unbound caches answers (300 s, bogus ones 60 s): right after you changed a zone or a key,
# a FAIL can be a stale cache. Wait, or restart Unbound by hand, then run it again.
#
# Needs: kdig, openssl, bash. Lab mode also needs docker. Keys are only passed to kdig by path.
set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PKI="$ROOT/gateway/pki"
MODE=lab
PI=""

usage() { sed -n '2,12p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; }
die() { echo "e2e: $*" >&2; exit 2; }

while [ $# -gt 0 ]; do
  case "$1" in
    --mode) MODE="${2:-}"; shift 2 ;;
    --pi)   PI="${2:-}"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) die "unknown argument: $1 (try --help)" ;;
  esac
done

case "$MODE" in
  lab) GW_HOST=127.0.0.1; GW_PORT=8530 ;;
  pi)  [ -n "$PI" ] || die "--mode pi needs --pi user@host"
       GW_HOST="${PI#*@}"; GW_PORT=853 ;;
  *)   die "--mode must be lab or pi" ;;
esac

command -v kdig >/dev/null    || die "kdig not found (package knot-utils / knot-dnsutils)"
command -v openssl >/dev/null || die "openssl not found"
[ "$MODE" = lab ] && { command -v docker >/dev/null || die "docker not found (needed for the lab back-door check)"; }
for f in ca.crt esp32-1.crt esp32-1.key gateway.crt gateway.key; do
  [ -f "$PKI/$f" ] || die "missing gateway/pki/$f (run tests/setup-lab.sh, or see the README)"
done

# one place that knows how to talk to the gateway; extra arguments go to kdig
kd() {
  kdig @"$GW_HOST" -p "$GW_PORT" +tls +tls-ca="$PKI/ca.crt" +tls-hostname=gateway.lab.test +timeout=8 "$@" 2>&1
}
OKCERT=(+tls-certfile="$PKI/esp32-1.crt" +tls-keyfile="$PKI/esp32-1.key")
GWCERT=(+tls-certfile="$PKI/gateway.crt" +tls-keyfile="$PKI/gateway.key")

FAKE=""
cleanup() { [ -n "$FAKE" ] && rm -rf "$FAKE"; }
trap cleanup EXIT

# parse kdig output into STATUS, AD (0/1), EDE (number or empty), TTL (of the A record, or empty)
parse() {
  local o="$1" flags
  STATUS=$(sed -n 's/.*status: \([A-Z]*\).*/\1/p' <<<"$o" | head -1)
  flags=$(sed -n 's/.*Flags: \([a-z ]*\);.*/\1/p' <<<"$o" | head -1)
  case " $flags " in *" ad "*) AD=1 ;; *) AD=0 ;; esac
  EDE=$(sed -n 's/.*EDE: \([0-9][0-9]*\).*/\1/p' <<<"$o" | head -1)
  TTL=$(awk '$3=="IN" && $4=="A" {print $2; exit}' <<<"$o")
}

# ask for a name; retry once if nothing came back (the first connection after a restart can time out)
ask() {
  OUT=$(kd "$@")
  parse "$OUT"
  if [ -z "$STATUS" ]; then sleep 2; OUT=$(kd "$@"); parse "$OUT"; fi
}

PASSN=0; FAILN=0
GW_UP=0   # set once a valid client got an answer: the positive control for every "refused" check
HEAD='%-6s %-46s %-20s %-36s %s\n'
printf "$HEAD" ID CHECK EXPECTED OBSERVED RESULT
printf "$HEAD" ------ ---------------------------------------------- -------------------- ------------------------------------ ------
row() { # id check expected observed ok(0/1) [note]
  local res=PASS
  if [ "$5" != 1 ]; then res=FAIL; FAILN=$((FAILN+1)); else PASSN=$((PASSN+1)); fi
  printf "$HEAD" "$1" "$2" "$3" "$4" "$res${6:+  ($6)}"
}

# 1. the nine names through the gateway, as the ESP32 would ask
NAMES=(
  "ota.lab.test|NOERROR ad"
  "ota.signed.lab.test|NOERROR ad"
  "ota.unsigned.lab.test|NOERROR no ad"
  "ota.expired.lab.test|SERVFAIL EDE 7"
  "ota.future.lab.test|SERVFAIL EDE 8"
  "ota.badsig.lab.test|SERVFAIL EDE 6"
  "ota.unknownalg.lab.test|NOERROR no ad"
  "ota.nosig.lab.test|SERVFAIL EDE 10"
  "ota.nokey.lab.test|SERVFAIL EDE 6"
)
n=0
for entry in "${NAMES[@]}"; do
  n=$((n+1)); name="${entry%%|*}"; want="${entry#*|}"
  ask "${OKCERT[@]}" A "$name"
  got="${STATUS:-no answer}"
  if [ "$STATUS" = NOERROR ]; then if [ "$AD" = 1 ]; then got="$got ad"; else got="$got no ad"; fi; fi
  [ -n "$EDE" ] && got="$got EDE $EDE"
  note=""; [ -n "$TTL" ] && [ "$TTL" -lt 300 ] 2>/dev/null && note="cached answer"
  [ -n "$STATUS" ] && GW_UP=1
  ok=0; [ "$got" = "$want" ] && ok=1
  row "V$n" "$name" "$want" "$got" "$ok" "$note"
done

# 2. attackers: no DNS answer must come back at all
refused() { # id check  [kdig args...]
  local id="$1" label="$2"; shift 2
  if [ "$GW_UP" != 1 ]; then row "$id" "$label" "refused" "gateway not answering: no positive control" 0; return; fi
  OUT=$(kd "$@")
  if grep -q 'status:' <<<"$OUT"; then
    parse "$OUT"; row "$id" "$label" "refused" "answered: $STATUS" 0
  else
    why=$(grep -oE 'TLS fatal alert|handshake failed|failed to receive reply|certificate' <<<"$OUT" | head -1)
    row "$id" "$label" "refused" "refused (${why:-no answer})" 1
  fi
}

refused A1 "no client certificate"                    A ota.lab.test
FAKE=$(mktemp -d "${TMPDIR:-/tmp}/e2e-fakeca.XXXXXX")
if "$ROOT/tests/make-fakeca.sh" "$FAKE" >/dev/null 2>&1; then
  refused A2 "client cert from a fake CA (same names)" +tls-certfile="$FAKE/fake-esp32-1.crt" +tls-keyfile="$FAKE/fake-esp32-1.key" A ota.lab.test
else
  row A2 "client cert from a fake CA (same names)" "refused" "could not build the fake CA" 0
fi
refused A3 "gateway's own cert used as a client cert" "${GWCERT[@]}" A ota.lab.test

# 3. a client cannot switch validation off with the CD bit
ask "${OKCERT[@]}" +cd A ota.badsig.lab.test
got="${STATUS:-no answer}"; ok=0; [ "$got" = SERVFAIL ] && ok=1
row C1 "CD bit set on the bogus ota.badsig" "SERVFAIL" "$got" "$ok"

# 4. the back door: Unbound must only be reachable through stunnel
if [ "$MODE" = lab ]; then
  pub=$(docker port clab-dnssec-unbound 2>/dev/null)
  if [ -z "$pub" ]; then row B1 "Unbound publishes no host port" "no published port" "no published port" 1
  else row B1 "Unbound publishes no host port" "no published port" "published: $(tr '\n' ' ' <<<"$pub")" 0; fi

  ub_ip=$(docker inspect -f '{{(index .NetworkSettings.Networks "clab1").IPAddress}}' clab-dnssec-unbound 2>/dev/null)
  if [ -z "$ub_ip" ]; then
    row B2 "non-stunnel client inside the lab network" "REFUSED" "Unbound not found on clab1" 0
  else
    out=$(docker run --rm --network clab1 docker.io/cznic/knot:latest kdig @"$ub_ip" +timeout=5 A ota.lab.test 2>&1)
    parse "$out"; got="${STATUS:-no answer}"; [ -n "$EDE" ] && got="$got EDE $EDE"
    ok=0; [ "${STATUS:-}" = REFUSED ] && ok=1
    row B2 "non-stunnel client inside the lab network" "REFUSED" "$got" "$ok"
  fi
else
  if sock=$(ssh -o BatchMode=yes -o ConnectTimeout=8 "$PI" 'ss -tuln' 2>&1) && grep -q 'LISTEN\|UNCONN' <<<"$sock"; then
    bad=$(awk '$5 ~ /:53$/ && $5 !~ /^127\.0\.0\.1:/ && $5 !~ /^\[::1\]:/ {print $5}' <<<"$sock" | tr '\n' ' ')
    if [ -z "$bad" ]; then row B1 "port 53 listens on localhost only (ssh)" "127.0.0.1 / ::1 only" "127.0.0.1 / ::1 only" 1
    else row B1 "port 53 listens on localhost only (ssh)" "127.0.0.1 / ::1 only" "also on: $bad" 0; fi
  else
    row B1 "port 53 listens on localhost only (ssh)" "127.0.0.1 / ::1 only" "ssh to $PI failed" 0
  fi
  if [ "$GW_UP" != 1 ]; then
    row B2 "port 53 not reachable over the network" "closed" "gateway not answering: cannot judge" 0
  elif timeout 4 bash -c "echo > /dev/tcp/$GW_HOST/53" 2>/dev/null; then
    row B2 "port 53 not reachable over the network" "closed" "OPEN" 0
  else
    row B2 "port 53 not reachable over the network" "closed" "closed" 1
  fi
fi

echo
echo "mode: $MODE   gateway: $GW_HOST:$GW_PORT   passed: $PASSN   failed: $FAILN"
echo "note: Unbound caches answers (300 s, bogus 60 s). After changing a zone or key, a FAIL can be stale: wait or restart Unbound, then re-run."
[ "$FAILN" -eq 0 ]
