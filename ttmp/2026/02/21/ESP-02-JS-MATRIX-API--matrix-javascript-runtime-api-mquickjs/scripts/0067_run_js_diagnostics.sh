#!/usr/bin/env bash
set -euo pipefail

BASE_URL="${BASE_URL:-http://192.168.3.119}"
PLAY_SCRIPT="ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_play_js_example.sh"
DIAG_DIR="0067-esp-c3-led-matrix-http/examples/js/diag"

AUTO="${AUTO:-0}"

steps=(
  "00-env-status.js|Read JS+matrix status (no visual pattern expected)"
  "01-all-on.js|All LEDs should turn ON"
  "02-all-off.js|All LEDs should turn OFF"
  "03-single-pixel.js|Single pixel at top-left logical origin"
  "04-border.js|Full rectangular border"
  "05-checkerboard.js|Checkerboard pattern"
  "06-walk-dot.js|Moving dot left-to-right"
  "07-text-test.js|Static text TEST"
  "08-scroll-test.js|Scrolling text HELLO 123"
  "09-wave-test.js|Wave-scrolling text"
  "10-stop-reset.js|Clear/off"
)

echo "Base URL: $BASE_URL"
for row in "${steps[@]}"; do
  file="${row%%|*}"
  expect="${row#*|}"
  path="$DIAG_DIR/$file"

  echo
  echo "== STEP $file =="
  echo "Expected: $expect"
  BASE_URL="$BASE_URL" "$PLAY_SCRIPT" "$path"
  echo "Status:"
  curl -sS -m 4 "$BASE_URL/api/js/status"; echo
  curl -sS -m 4 "$BASE_URL/api/matrix/status"; echo

  if [[ "$AUTO" != "1" ]]; then
    read -r -p "Press Enter to continue to next step (or Ctrl+C to stop)... " _
  else
    sleep 1
  fi
done

echo
echo "Diagnostics sequence complete."
