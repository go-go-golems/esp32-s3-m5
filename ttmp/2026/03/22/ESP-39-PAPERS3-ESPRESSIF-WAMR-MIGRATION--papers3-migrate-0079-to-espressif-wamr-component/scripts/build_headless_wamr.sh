#!/usr/bin/env bash
set -euo pipefail

ROOT="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5"
PROJECT="${ROOT}/0079-papers3-wamr-assemblyscript-console"
BUILD_DIR="${PROJECT}/build-headless"
SDKCONFIG_FILE="${BUILD_DIR}/sdkconfig"

mkdir -p "${BUILD_DIR}"
rm -f "${SDKCONFIG_FILE}"

unset IDF_PYTHON_ENV_PATH IDF_PATH
source /home/manuel/esp/esp-idf-5.3.4/export.sh >/dev/null

idf.py \
  -C "${PROJECT}" \
  -B "${BUILD_DIR}" \
  -D SDKCONFIG="${SDKCONFIG_FILE}" \
  -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.headless" \
  reconfigure build
