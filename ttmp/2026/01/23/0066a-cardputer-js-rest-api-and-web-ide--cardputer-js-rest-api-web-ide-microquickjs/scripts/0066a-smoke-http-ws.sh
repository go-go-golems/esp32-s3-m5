#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "usage: $0 <device-ip> [ws]"
  echo "examples:"
  echo "  $0 192.168.1.123"
  echo "  $0 192.168.1.123 ws   # requires websocat"
  exit 2
fi

IP="$1"
BASE="http://${IP}"

echo "[status] GET ${BASE}/api/status"
curl -fsS "${BASE}/api/status" | python3 -m json.tool || true
echo

echo "[eval] POST ${BASE}/api/js/eval"
curl -fsS -X POST "${BASE}/api/js/eval" \
  -H 'content-type: text/plain; charset=utf-8' \
  --data-binary $'emit("log", { msg: "hello from smoke", ts: Date.now() });\n1+1\n' \
  | python3 -m json.tool || true
echo

if [[ "${2:-}" == "ws" ]]; then
  if ! command -v websocat >/dev/null 2>&1; then
    echo "websocat not found; install it or omit the 'ws' arg"
    exit 1
  fi
  echo "[ws] connect ws://${IP}/ws (Ctrl-C to stop)"
  websocat "ws://${IP}/ws"
fi

