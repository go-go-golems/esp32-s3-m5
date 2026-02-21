#!/usr/bin/env bash
set -euo pipefail

PORT="${PORT:-/dev/serial/by-id/usb-1a86_USB_Single_Serial_575E072431-if00}"
PROJECT_DIR="${PROJECT_DIR:-0067-esp-c3-led-matrix-http}"
SECONDS_TO_RUN="${SECONDS_TO_RUN:-30}"
LOG_FILE="${1:-}"

if [[ -n "$LOG_FILE" ]]; then
  timeout "${SECONDS_TO_RUN}s" idf.py -C "$PROJECT_DIR" -p "$PORT" monitor | tee "$LOG_FILE"
else
  timeout "${SECONDS_TO_RUN}s" idf.py -C "$PROJECT_DIR" -p "$PORT" monitor
fi
