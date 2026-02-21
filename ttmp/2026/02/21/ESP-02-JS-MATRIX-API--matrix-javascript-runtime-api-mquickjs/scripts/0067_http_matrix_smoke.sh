#!/usr/bin/env bash
set -euo pipefail

BASE_URL="${BASE_URL:-http://192.168.3.119}"

echo "== matrix status =="
curl -sS "$BASE_URL/api/matrix/status"
echo

echo "== matrix text =="
curl -sS -X POST "$BASE_URL/api/matrix/text" \
  -H 'content-type: application/json' \
  --data '{"text":"MATRIX OK"}'
echo

echo "== matrix anim scroll =="
curl -sS -X POST "$BASE_URL/api/matrix/anim" \
  -H 'content-type: application/json' \
  --data '{"mode":"scroll","text":"SCROLL","fps":20,"pause_ms":150,"repeat_count":1}'
echo

echo "== matrix anim wave =="
curl -sS -X POST "$BASE_URL/api/matrix/anim" \
  -H 'content-type: application/json' \
  --data '{"mode":"wave","text":"WAVE","fps":20,"pause_ms":100,"repeat_count":1}'
echo

echo "== matrix stop =="
curl -sS -X POST "$BASE_URL/api/matrix/stop"
echo

echo "== matrix status (final) =="
curl -sS "$BASE_URL/api/matrix/status"
echo
