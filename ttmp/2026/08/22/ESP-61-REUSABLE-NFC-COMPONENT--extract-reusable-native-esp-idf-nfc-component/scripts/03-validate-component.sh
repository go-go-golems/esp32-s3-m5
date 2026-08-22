#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# ESP-61 reusable validation: host tests, core-component hygiene guard, and
# the ESP-IDF 5.5.4 smoke build. Run from the repository root.
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"  # esp32-s3-m5 repo root

echo "=== 1. gogolem_nfc host tests ==="
components/gogolem_nfc/test_host/build.sh

echo "=== 2. core hygiene (no app policy in component) ==="
if rg -n 'printf|ESP_LOG|nvs_|esp_restart|GPIO_NUM|i2c_new_master_bus' \
        components/gogolem_nfc/src components/gogolem_nfc/include; then
  echo "FAIL: application policy leaked into the core component"
  exit 1
fi
echo "OK: no printf/ESP_LOG/nvs_/esp_restart/GPIO/i2c bus creation in core"

echo "=== 3. ESP-IDF 5.5.4 smoke build ==="
source ~/esp/esp-idf-5.5.4/export.sh
( cd examples/nfc_types_smoke && idf.py build >/tmp/esp61-smoke-build.log 2>&1 )
if rg -n 'warning:|error:' /tmp/esp61-smoke-build.log; then
  echo "FAIL: smoke build produced warnings/errors"
  exit 1
fi
rg -n 'Project build complete' /tmp/esp61-smoke-build.log | tail -1
echo "=== validation complete ==="
