#!/usr/bin/env bash
# Builds the whole lab on a CLEAN clone: your own PKI, the two images, the Containerlab lab,
# then the two DS values that Knot's new keys require, and finally tests/e2e.sh.
#
#   tests/setup-lab.sh --dry-run     print every step, change nothing (safe anywhere, also on a live lab)
#   tests/setup-lab.sh --self-test   unit-test the config patchers on temp COPIES, change nothing
#   tests/setup-lab.sh               do it
#   options: --force (overwrite existing keys / lab), --skip-e2e
#
# DO NOT run it for real on a machine that already has the lab: it creates NEW keys and
# new DS values. It refuses when gateway/pki/*.key, the knot-storage volume or the lab
# containers already exist, unless you pass --force.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PKI="$ROOT/gateway/pki"
UNBOUND_CONF="$ROOT/gateway/unbound/unbound.conf"
LAB_ZONE="$ROOT/gateway/knot/lab.test.zone"
KNOT=clab-dnssec-knot
UNBOUND=clab-dnssec-unbound

DRY=0; FORCE=0; E2E=1; SELFTEST=0
while [ $# -gt 0 ]; do
  case "$1" in
    --dry-run)   DRY=1 ;;
    --force)     FORCE=1 ;;
    --skip-e2e)  E2E=0 ;;
    --self-test) SELFTEST=1 ;;
    -h|--help)   sed -n '2,12p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) echo "setup-lab: unknown argument: $1 (try --help)" >&2; exit 2 ;;
  esac
  shift
done

say()  { printf '\n== %s\n' "$*"; }
run()  { printf '+'; printf ' %q' "$@"; printf '\n'; [ "$DRY" = 1 ] || "$@"; }   # printed lines are copy-pasteable
fail() { echo "setup-lab: $*" >&2; exit 1; }

DS_RE='^[a-z.]+\. DS [0-9]+ [0-9]+ 2 [0-9a-fA-F]{64}$'

# ---- pure helpers (also used by --self-test) ----------------------------------------

# keymgr "<zone> list" on stdin -> tag of the first KSK.  Line: <id> <tag> KSK <algorithm> ...
ksk_tag_from_list() { awk '$3=="KSK" && !d {print $2; d=1}'; }   # reads all input: no SIGPIPE for the writer

# keymgr "<zone> ds <tag>" on stdin -> the digest-type-2 (SHA-256) line:  <zone> DS <tag> <alg> 2 <digest>
ds2_from_output() { awk '$2=="DS" && $5==2 && !d {print $1, $2, $3, $4, $5, $6; d=1}'; }

# replace the value of the trust-anchor line in an unbound.conf (touches that one line only)
patch_trust_anchor() { # file "lab.test. DS tag alg 2 digest"
  local f="$1" ds="$2"
  [[ "$ds" =~ $DS_RE ]] && [[ "$ds" == lab.test.\ DS\ * ]] || { echo "bad DS line for the trust anchor: $ds" >&2; return 1; }
  grep -qE '^[[:space:]]*trust-anchor:[[:space:]]*"lab\.test\. DS ' "$f" || { echo "no trust-anchor line for lab.test in $f" >&2; return 1; }
  sed -E "s|^([[:space:]]*trust-anchor:[[:space:]]*\")lab\.test\. DS [^\"]*(\".*)\$|\1${ds}\2|" "$f" > "$f.new" && mv "$f.new" "$f"
}

# replace the signed.lab.test. DS line in the parent zone file (that one line only)
patch_signed_ds() { # file "signed.lab.test. DS tag alg 2 digest"
  local f="$1" ds="$2" n
  [[ "$ds" =~ $DS_RE ]] && [[ "$ds" == signed.lab.test.\ DS\ * ]] || { echo "bad DS line for signed.lab.test: $ds" >&2; return 1; }
  n=$(grep -cE '^signed\.lab\.test\.[[:space:]]+(IN[[:space:]]+)?DS[[:space:]]' "$f" || true)
  [ "$n" = 1 ] || { echo "expected exactly one signed.lab.test. DS line in $f, found $n" >&2; return 1; }
  sed -E "s|^signed\.lab\.test\.[[:space:]]+(IN[[:space:]]+)?DS[[:space:]].*\$|${ds}|" "$f" > "$f.new" && mv "$f.new" "$f"
}

# ---- self-test: temp copies and sample data only ------------------------------------
self_test() {
  local t bad=0 tmp
  tmp="$(mktemp -d "${TMPDIR:-/tmp}/setup-lab-test.XXXXXX")"
  trap 'rm -rf "$tmp"' RETURN
  ok()  { printf '  ok    %s\n' "$1"; }
  nok() { printf '  FAIL  %s\n' "$1"; bad=$((bad+1)); }

  local ds_lab="lab.test. DS 11111 13 2 $(printf 'ab%.0s' {1..32})"
  local ds_sig="signed.lab.test. DS 22222 13 2 $(printf 'cd%.0s' {1..32})"
  cp "$UNBOUND_CONF" "$tmp/unbound.conf"; cp "$LAB_ZONE" "$tmp/lab.test.zone"

  echo "patchers on temp copies of the real config files"
  patch_trust_anchor "$tmp/unbound.conf" "$ds_lab" && ok "trust-anchor patched" || nok "trust-anchor patch failed"
  grep -qF "trust-anchor: \"$ds_lab\"" "$tmp/unbound.conf" && ok "new trust-anchor line present" || nok "new trust-anchor line missing"
  t=$(diff "$UNBOUND_CONF" "$tmp/unbound.conf" | grep -c '^[<>]' || true)
  [ "$t" = 2 ] && ok "only that one line changed (1 removed, 1 added)" || nok "unbound.conf: $t changed lines, expected 2"

  patch_signed_ds "$tmp/lab.test.zone" "$ds_sig" && ok "signed DS patched" || nok "signed DS patch failed"
  grep -qxF "$ds_sig" "$tmp/lab.test.zone" && ok "new signed.lab.test. DS line present" || nok "new signed DS line missing"
  t=$(diff "$LAB_ZONE" "$tmp/lab.test.zone" | grep -c '^[<>]' || true)
  [ "$t" = 2 ] && ok "only that one line changed (1 removed, 1 added)" || nok "lab.test.zone: $t changed lines, expected 2"

  cp "$tmp/unbound.conf" "$tmp/u1"; cp "$tmp/lab.test.zone" "$tmp/z1"
  patch_trust_anchor "$tmp/unbound.conf" "$ds_lab"; patch_signed_ds "$tmp/lab.test.zone" "$ds_sig"
  cmp -s "$tmp/u1" "$tmp/unbound.conf" && cmp -s "$tmp/z1" "$tmp/lab.test.zone" && ok "idempotent (a second run changes nothing)" || nok "second run changed the files"

  echo "bad input is rejected and leaves the file alone"
  cp "$tmp/unbound.conf" "$tmp/u2"
  patch_trust_anchor "$tmp/unbound.conf" "lab.test. DS 1 13 2 nothex" 2>/dev/null && nok "accepted a bad digest" || ok "bad digest rejected"
  patch_trust_anchor "$tmp/unbound.conf" "other.test. DS 1 13 2 $(printf 'ab%.0s' {1..32})" 2>/dev/null && nok "accepted the wrong zone" || ok "wrong zone rejected"
  printf 'server:\n  verbosity: 1\n' > "$tmp/no-anchor.conf"
  patch_trust_anchor "$tmp/no-anchor.conf" "$ds_lab" 2>/dev/null && nok "patched a file with no trust-anchor line" || ok "file without a trust-anchor line rejected"
  printf 'dns1 A 10.0.0.1\n' > "$tmp/no-ds.zone"
  patch_signed_ds "$tmp/no-ds.zone" "$ds_sig" 2>/dev/null && nok "patched a zone with no signed DS line" || ok "zone without a signed DS line rejected"
  cmp -s "$tmp/u2" "$tmp/unbound.conf" && ok "rejected input left the file untouched" || nok "rejected input changed the file"

  echo "parsers on sample keymgr output (public metadata only)"
  local list ds3
  list=$'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa 21965 ZSK ECDSAP256SHA256 created=1 publish=1 active=1\nbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb 62736 KSK ECDSAP256SHA256 created=1 publish=1 ready=1'
  [ "$(ksk_tag_from_list <<<"$list")" = 62736 ] && ok "KSK tag picked out of keymgr list (the KSK, not the ZSK)" || nok "wrong KSK tag"
  ds3=$'lab.test. DS 62736 13 1 6e16bfe8672048cb585e077fe8b8439db0354607\nlab.test. DS 62736 13 2 69fe724c197fa676c0b8c603a24a274ec62098dfac6d8903cee04627fb4a9b8d\nlab.test. DS 62736 13 4 1114bb2e1bfbc55b69db4751a5300bbf9089bfe0aa80fd293421610c7d778eda95a6f7b045dc723059516a85df1dffbd'
  [ "$(ds2_from_output <<<"$ds3")" = "lab.test. DS 62736 13 2 69fe724c197fa676c0b8c603a24a274ec62098dfac6d8903cee04627fb4a9b8d" ] && ok "digest-type-2 line picked out of three DS lines" || nok "wrong DS line"
  [[ "$(ds2_from_output <<<"$ds3")" =~ $DS_RE ]] && ok "picked DS line passes the same format check the patchers use" || nok "picked DS line fails the format check"

  if [ "$bad" -eq 0 ]; then echo "self-test: all passed"; else echo "self-test: $bad FAILED"; return 1; fi
}

if [ "$SELFTEST" = 1 ]; then self_test; exit $?; fi

# ---- the real thing (or its dry run) ------------------------------------------------
[ "$DRY" = 1 ] && echo "DRY RUN: nothing below is executed (lines starting with '+' are what would run)"

say "0. checks"
for c in openssl docker containerlab kdig; do
  command -v "$c" >/dev/null || { if [ "$DRY" = 1 ]; then echo "  warning: $c not found (a real run needs it)"; else fail "$c not found"; fi; }
done
existing=()
compgen -G "$PKI/*.key" >/dev/null && existing+=("keys in gateway/pki")
docker volume inspect knot-storage >/dev/null 2>&1 && existing+=("docker volume knot-storage")
docker ps -a --format '{{.Names}}' 2>/dev/null | grep -q '^clab-dnssec-' && existing+=("lab containers clab-dnssec-*")
if [ "${#existing[@]}" -gt 0 ]; then
  echo "  found: ${existing[*]}"
  if [ "$DRY" = 1 ]; then
    echo "  a real run would ABORT here (this looks like a live lab; it would create NEW keys). Use --force only if you mean it."
  elif [ "$FORCE" = 1 ]; then
    echo "  --force: continuing, existing keys WILL be overwritten (stop the old lab first: containerlab destroy)"
  else
    fail "refusing to run: a lab or keys already exist (see above). --dry-run shows the steps; --force overrides."
  fi
fi

say "1. PKI: your own CA, gateway and esp32-1 certificates (ECDSA P-256)"
old_umask=$(umask); umask 077
run openssl genpkey -algorithm ec -pkeyopt ec_paramgen_curve:prime256v1 -out "$PKI/ca.key"
run openssl req -new -x509 -key "$PKI/ca.key" -days 360 -subj "/CN=LAB CA" \
  -addext "basicConstraints=critical,CA:TRUE" -addext "keyUsage=critical,keyCertSign,cRLSign" -out "$PKI/ca.crt"
for n in gateway esp32-1; do
  run openssl genpkey -algorithm ec -pkeyopt ec_paramgen_curve:prime256v1 -out "$PKI/$n.key"
  run openssl req -new -key "$PKI/$n.key" -subj "/CN=$n" -out "$PKI/$n.csr"
  run openssl x509 -req -in "$PKI/$n.csr" -CA "$PKI/ca.crt" -CAkey "$PKI/ca.key" -days 360 -extfile "$PKI/$n.ext" -out "$PKI/$n.crt"
  run rm -f "$PKI/$n.csr"
done
umask "$old_umask"

say "2. the two small images"
run docker build -t lab-unbound -f "$ROOT/gateway/unbound/Containerfile" "$ROOT/gateway/unbound"
run docker build -t lab-stunnel -f "$ROOT/gateway/stunnel/Containerfile" "$ROOT/gateway/stunnel"

say "3. start the lab (Knot generates new DNSSEC keys on its first start)"
( cd "$ROOT/lab" && run containerlab deploy )

say "4. wait for Knot to create its keys"
if [ "$DRY" = 1 ]; then
  echo "+ poll: docker exec $KNOT keymgr -c /config/knot.conf <zone> list   (up to 90 s, for lab.test. and signed.lab.test.)"
else
  for i in $(seq 1 45); do
    a=$(docker exec "$KNOT" keymgr -c /config/knot.conf lab.test. list 2>/dev/null | ksk_tag_from_list || true)
    b=$(docker exec "$KNOT" keymgr -c /config/knot.conf signed.lab.test. list 2>/dev/null | ksk_tag_from_list || true)
    [ -n "$a" ] && [ -n "$b" ] && break
    sleep 2
  done
  [ -n "${a:-}" ] && [ -n "${b:-}" ] || fail "Knot did not create its keys in 90 s (docker logs $KNOT)"
fi

say "5. read the KSK tags and the SHA-256 DS of both zones"
ds_of() { # zone
  local tag
  if [ "$DRY" = 1 ]; then echo "+ keymgr $1 list -> KSK tag;  keymgr $1 ds <tag> -> digest-2 line" >&2; echo "$1 DS <tag> 13 2 <digest>"; return; fi
  tag=$(docker exec "$KNOT" keymgr -c /config/knot.conf "$1" list | ksk_tag_from_list)
  [ -n "$tag" ] || fail "no KSK found for $1"
  docker exec "$KNOT" keymgr -c /config/knot.conf "$1" ds "$tag" | ds2_from_output
}
DS_LAB=$(ds_of lab.test.);        echo "  lab.test.        $DS_LAB"
DS_SIGNED=$(ds_of signed.lab.test.); echo "  signed.lab.test. $DS_SIGNED"

say "6. put them where they belong"
echo "+ trust-anchor line in gateway/unbound/unbound.conf  <- $DS_LAB"
echo "+ signed.lab.test. DS line in gateway/knot/lab.test.zone <- $DS_SIGNED"
if [ "$DRY" != 1 ]; then
  patch_trust_anchor "$UNBOUND_CONF" "$DS_LAB"
  patch_signed_ds "$LAB_ZONE" "$DS_SIGNED"
fi

say "7. tell Knot, then restart Unbound (it caches, and reads its anchor at start)"
run docker exec "$KNOT" knotc zone-reload lab.test.
for z in lab.test. signed.lab.test.; do
  if [ "$DRY" = 1 ]; then echo "+ docker exec $KNOT knotc zone-ksk-submitted $z"
  else docker exec "$KNOT" knotc zone-ksk-submitted "$z" || echo "  ($z: key already active, fine)"; fi
done
run docker restart "$UNBOUND"

say "8. end-to-end checks"
if [ "$E2E" = 1 ]; then
  if [ "$DRY" = 1 ]; then echo "+ $ROOT/tests/e2e.sh --mode lab"; else sleep 5; "$ROOT/tests/e2e.sh" --mode lab; fi
else
  echo "  skipped (--skip-e2e)"
fi
[ "$DRY" = 1 ] && echo && echo "DRY RUN finished: nothing was changed."
exit 0
