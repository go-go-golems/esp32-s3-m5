#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0051-tab5-boot-logo"
SDKCONFIG="$PROJECT_DIR/sdkconfig"
DEFAULTS="$PROJECT_DIR/sdkconfig.defaults"

python3 - <<'PY'
from pathlib import Path
paths = [
    Path('/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0051-tab5-boot-logo/sdkconfig'),
    Path('/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0051-tab5-boot-logo/sdkconfig.defaults'),
]
for p in paths:
    text = p.read_text()
    if 'CONFIG_IDF_EXPERIMENTAL_FEATURES=y' not in text:
        text += '\nCONFIG_IDF_EXPERIMENTAL_FEATURES=y\n'
    text = text.replace('CONFIG_SPIRAM_SPEED_20M=y', '# CONFIG_SPIRAM_SPEED_20M is not set')
    if 'CONFIG_SPIRAM_SPEED_200M=y' not in text:
        text += 'CONFIG_SPIRAM_SPEED_200M=y\n'
    if 'CONFIG_SPIRAM_SPEED=20' in text:
        text = text.replace('CONFIG_SPIRAM_SPEED=20', 'CONFIG_SPIRAM_SPEED=200')
    p.write_text(text)
PY

echo "Updated: $SDKCONFIG"
echo "Updated: $DEFAULTS"
rg -n "CONFIG_IDF_EXPERIMENTAL_FEATURES|CONFIG_SPIRAM_SPEED_" "$SDKCONFIG" -S || true
