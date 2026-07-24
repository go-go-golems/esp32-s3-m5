#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 || $# -gt 3 ]]; then
  echo "usage: $0 OUTPUT_DIR LAN_IP [DNS_NAME]" >&2
  exit 2
fi
out=$1
ip=$2
dns=${3:-pulp-auth.local}
mkdir -p "$out"
chmod 700 "$out"

openssl genrsa -out "$out/ca.key" 3072 >/dev/null 2>&1
chmod 600 "$out/ca.key"
openssl req -x509 -new -sha256 -days 30 \
  -key "$out/ca.key" -out "$out/ca.crt" \
  -subj "/CN=PULP ESP-54 Development CA"

openssl genrsa -out "$out/server.key" 2048 >/dev/null 2>&1
chmod 600 "$out/server.key"
cat >"$out/server.ext" <<EOF
subjectAltName=IP:${ip},IP:127.0.0.1,DNS:${dns},DNS:localhost
extendedKeyUsage=serverAuth
keyUsage=digitalSignature,keyEncipherment
basicConstraints=CA:FALSE
EOF
openssl req -new -key "$out/server.key" -out "$out/server.csr" \
  -subj "/CN=${ip}"
openssl x509 -req -sha256 -days 30 \
  -in "$out/server.csr" -CA "$out/ca.crt" -CAkey "$out/ca.key" \
  -CAcreateserial -out "$out/server.crt" -extfile "$out/server.ext"
rm -f "$out/server.csr" "$out/server.ext" "$out/ca.srl"
chmod 644 "$out/ca.crt" "$out/server.crt"
echo "generated development CA and server certificate in $out"
echo "CA certificate for curl/firmware: $out/ca.crt"
