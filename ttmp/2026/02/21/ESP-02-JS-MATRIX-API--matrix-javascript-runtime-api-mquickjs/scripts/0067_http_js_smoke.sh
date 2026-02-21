#!/usr/bin/env bash
set -euo pipefail

BASE_URL="${BASE_URL:-http://192.168.3.119}"

echo "== matrix status =="
curl -sS "$BASE_URL/api/matrix/status"
echo

echo "== js status =="
curl -sS "$BASE_URL/api/js/status"
echo

echo "== js eval: 1+1 =="
curl -sS -X POST "$BASE_URL/api/js/eval" --data "1+1"
echo

echo "== js eval: matrix text =="
curl -sS -X POST "$BASE_URL/api/js/eval" --data "matrix.setText('JS OK')"
echo

echo "== js mem (first 20 lines) =="
curl -sS "$BASE_URL/api/js/mem" | sed -n '1,20p'
echo

echo "== js stop =="
curl -sS -X POST "$BASE_URL/api/js/stop"
echo

echo "== js reset =="
curl -sS -X POST "$BASE_URL/api/js/reset"
echo

echo "== js status (after reset) =="
curl -sS "$BASE_URL/api/js/status"
echo

echo "== js eval (after reset): 3+4 =="
curl -sS -X POST "$BASE_URL/api/js/eval" --data "3+4"
echo

echo "== js hard reset =="
curl -sS -X POST "$BASE_URL/api/js/reset-hard"
echo

echo "== js eval (after hard reset): 5+6 =="
curl -sS -X POST "$BASE_URL/api/js/eval" --data "5+6"
echo
