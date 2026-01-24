#!/usr/bin/env bash
set -euo pipefail

HOST="${HOST:-http://192.168.4.1}"

echo "== status =="
curl -fsS "$HOST/api/status" | jq .
echo

echo "== apply: chase =="
curl -fsS -XPOST "$HOST/api/apply" \
  -H 'content-type: application/json' \
  -d '{"type":"chase","brightness_pct":35,"frame_ms":16,"chase":{"speed":128,"tail":40,"gap":8,"trains":2,"fg":"#FF8800","bg":"#001020","dir":0,"fade":true}}' \
  | jq .
echo

echo "== frame (RGB bytes) =="
curl -fsS -D /tmp/0066-headers.txt -o /tmp/0066-frame.bin "$HOST/api/frame"
wc -c /tmp/0066-frame.bin
head -n 20 /tmp/0066-headers.txt
echo

echo "== gpio status =="
curl -fsS "$HOST/api/gpio/status" | jq .
echo

echo "== js eval =="
curl -fsS -XPOST "$HOST/api/js/eval" \
  -H 'content-type: text/plain; charset=utf-8' \
  --data-binary $'emit(\"log\", { msg: \"hello\" });\\nsim.setPattern(\"rainbow\");\\n' \
  | jq .
echo

echo "== js reset =="
curl -fsS -XPOST "$HOST/api/js/reset" | jq .
echo

echo "Open in browser: $HOST/"

