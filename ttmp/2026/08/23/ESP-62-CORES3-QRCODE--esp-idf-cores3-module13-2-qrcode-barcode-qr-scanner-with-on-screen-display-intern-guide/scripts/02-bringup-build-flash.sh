#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# ESP-62 — 02-bringup-build-flash.sh
#
# Documented, reusable bring-up helper for the CoreS3 + Module13.2 QRCode
# firmware. Follows the AGENTS.md rules: pin IDF 5.3.4 for ESP32-S3, set
# target once, USB Serial/JTAG console.
#
# Usage:
#   ./02-bringup-build-flash.sh            # build only
#   ./02-bringup-build-flash.sh flash       # build + flash
#   ./02-bringup-build-flash.sh monitor      # build + flash + monitor
#
# Flash port is auto-detected from /dev/serial/by-id (CoreS3 USB Serial/JTAG).
set -euo pipefail

FW_DIR="${FW_DIR:-0118-cores3-qrcode-scanner}"   # expected firmware dir name (created in Phase 1)
IDF_VER="${IDF_VER:-5.3.4}"
IDF_PATH="$HOME/esp/esp-idf-${IDF_VER}"

echo "==> sourcing ESP-IDF ${IDF_VER}"
# shellcheck disable=SC1091
source "${IDF_PATH}/export.sh"

cd "$(dirname "$0")/../../../${FW_DIR}" 2>/dev/null || {
  echo "ERROR: firmware dir ${FW_DIR} not found yet (create it in Phase 1)."
  exit 1
}

echo "==> target: esp32s3"
idf.py set-target esp32s3

echo "==> build"
idf.py build

PORT="$(ls /dev/serial/by-id/*M5Stack*CoreS3* 2>/dev/null | head -1 || true)"
[ -z "$PORT" ] && PORT="$(ls /dev/serial/by-id/*usb-esp* 2>/dev/null | head -1 || true)"
[ -z "$PORT" ] && PORT="$(ls /dev/ttyACM* 2>/dev/null | head -1 || true)"

case "${1:-build}" in
  build) ;;
  flash|monitor)
    [ -z "$PORT" ] && { echo "ERROR: no CoreS3 serial port found"; exit 1; }
    echo "==> flash port: ${PORT}"
    idf.py -p "${PORT}" flash
    [ "${1}" = "monitor" ] && idf.py -p "${PORT}" monitor
    ;;
  *) echo "unknown action: $1"; exit 1 ;;
esac
