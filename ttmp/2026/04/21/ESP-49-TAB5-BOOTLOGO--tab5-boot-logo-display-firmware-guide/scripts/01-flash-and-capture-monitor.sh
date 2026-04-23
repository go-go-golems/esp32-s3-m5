#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0051-tab5-boot-logo"
IDF_EXPORT_SH="${IDF_EXPORT_SH:-/home/manuel/esp/esp-idf-5.4.2/export.sh}"
PORT="${PORT:-/dev/ttyACM0}"
SESSION="${SESSION:-tab5_boot_logo}"
LOG_PATH="${LOG_PATH:-/tmp/tab5_boot_logo_monitor.log}"
WAIT_SECS="${WAIT_SECS:-8}"

if [[ ! -f "$IDF_EXPORT_SH" ]]; then
  echo "ERROR: missing ESP-IDF export script: $IDF_EXPORT_SH" >&2
  exit 1
fi

# shellcheck disable=SC1090
source "$IDF_EXPORT_SH" >/dev/null 2>&1

screen -S "$SESSION" -X quit 2>/dev/null || true
fuser -k "$PORT" 2>/dev/null || true

cd "$PROJECT_DIR"
idf.py -p "$PORT" flash

screen -dmS "$SESSION" bash -lc "cd '$PROJECT_DIR' && source '$IDF_EXPORT_SH' >/dev/null 2>&1 && exec idf.py -p '$PORT' monitor"
sleep "$WAIT_SECS"
screen -S "$SESSION" -X hardcopy -h "$LOG_PATH"

echo "Captured monitor log to: $LOG_PATH"
tail -n 200 "$LOG_PATH"
