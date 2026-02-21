#!/usr/bin/env bash
set -euo pipefail

PORT="${PORT:-/dev/serial/by-id/usb-1a86_USB_Single_Serial_575E072431-if00}"
PROJECT_DIR="${PROJECT_DIR:-0067-esp-c3-led-matrix-http}"

idf.py -C "$PROJECT_DIR" -p "$PORT" flash
