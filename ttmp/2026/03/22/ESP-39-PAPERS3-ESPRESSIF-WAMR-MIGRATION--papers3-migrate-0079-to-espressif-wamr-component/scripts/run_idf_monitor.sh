#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-/dev/ttyACM0}"
PROJECT_DIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console"
IDF_EXPORT="/home/manuel/esp/esp-idf-5.3.4/export.sh"

unset IDF_PYTHON_ENV_PATH IDF_PATH
source "${IDF_EXPORT}" >/dev/null
idf.py -C "${PROJECT_DIR}" -p "${PORT}" monitor
