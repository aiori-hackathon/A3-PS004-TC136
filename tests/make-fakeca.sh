#!/usr/bin/env bash
# Attacker's PKI: a CA also called "LAB CA" with its own key, plus a gateway
# and an esp32-1 cert signed by it. Same names and rules as gateway/pki/,
# only the signing key differs. The real gateway and ESP32 must refuse both.
#
# usage: tests/make-fakeca.sh [out_dir]    (default: a new temp dir)
# Output stays outside the repo on purpose; keys never go in git.
set -euo pipefail

pki="$(cd "$(dirname "$0")/../gateway/pki" && pwd)"
out="${1:-$(mktemp -d /tmp/fakeca.XXXX)}"
mkdir -p "$out"
cd "$out"
umask 077

newkey() { openssl genpkey -algorithm ec -pkeyopt ec_paramgen_curve:prime256v1 -out "$1"; }

# fake CA, same subject as the real one
newkey fakeca.key
openssl req -new -x509 -key fakeca.key -days 360 -subj "/CN=LAB CA" \
  -addext "basicConstraints=critical,CA:TRUE" \
  -addext "keyUsage=critical,keyCertSign,cRLSign" \
  -out fakeca.crt

# fake gateway + fake esp32-1, reusing the real .ext files so the rules match
for name in gateway esp32-1; do
  newkey "fake-$name.key"
  openssl req -new -key "fake-$name.key" -subj "/CN=$name" -out "fake-$name.csr"
  openssl x509 -req -in "fake-$name.csr" -CA fakeca.crt -CAkey fakeca.key \
    -days 360 -extfile "$pki/$name.ext" -out "fake-$name.crt" 2>/dev/null
  rm "fake-$name.csr"
done

# sanity: good for the fake CA, bad for the real one
openssl verify -CAfile fakeca.crt fake-gateway.crt fake-esp32-1.crt
if openssl verify -CAfile "$pki/ca.crt" fake-esp32-1.crt >/dev/null 2>&1; then
  echo "real CA accepted a fake cert, something is wrong" >&2
  exit 1
fi
echo "real CA rejects them (as it should)"
echo "fake pki in $out"
