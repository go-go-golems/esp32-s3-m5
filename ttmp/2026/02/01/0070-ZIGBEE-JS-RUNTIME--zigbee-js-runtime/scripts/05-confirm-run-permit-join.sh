#!/usr/bin/env bash
set -euo pipefail

ROOT="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5"
SCRIPT="$ROOT/ttmp/2026/02/01/0070-ZIGBEE-JS-RUNTIME--zigbee-js-runtime/scripts/04-permit-join-watch-yaml.js"

BROKER="${1:-mqtt://localhost:1884}"
BASE_TOPIC="${2:-zigbee2mqtt}"
SECONDS="${3:-120}"
DEVICE="${4:-}"
TIMEOUT="${5:-60s}"

APPROVED=$(plz-confirm confirm \
  --title "Run zigctl JS permit-join watcher?" \
  --message "This will run the JS permit-join watch script for ${SECONDS}s. Pair the plug during the window if desired." \
  --approve-text "Run" \
  --reject-text "Cancel" \
  --output json | python3 -c 'import json,sys
try:
    data=json.load(sys.stdin)
    if isinstance(data, list):
        if data and isinstance(data[0], dict):
            print(str(data[0].get("approved")).lower())
        else:
            print("false")
    elif isinstance(data, dict):
        print(str(data.get("approved")).lower())
    else:
        print("false")
except Exception:
    print("false")
')

if [ "$APPROVED" != "true" ]; then
  echo "User cancelled."
  exit 1
fi

cd "$ROOT/zigctl"

go run ./ js run "$SCRIPT" \
  --arg "broker=$BROKER" \
  --arg "baseTopic=$BASE_TOPIC" \
  --arg "seconds=$SECONDS" \
  --arg "device=$DEVICE" \
  --arg "timeout=$TIMEOUT"
