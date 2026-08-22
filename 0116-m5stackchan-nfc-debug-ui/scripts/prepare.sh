#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=../upstream.env
source "$PROJECT_DIR/upstream.env"
WORK_ROOT="${STACKCHAN_WORK_ROOT:-$PROJECT_DIR/.work/StackChan}"
SOURCE="${STACKCHAN_SOURCE:-$STACKCHAN_REPOSITORY}"

if [[ ! -d "$WORK_ROOT/.git" ]]; then
    mkdir -p "$(dirname "$WORK_ROOT")"
    git clone "$SOURCE" "$WORK_ROOT"
fi

git -C "$WORK_ROOT" fetch --all --tags --prune
git -C "$WORK_ROOT" checkout --detach "$STACKCHAN_COMMIT"
git -C "$WORK_ROOT" reset --hard "$STACKCHAN_COMMIT"
git -C "$WORK_ROOT" clean -ffd firmware/main/apps/app_nfc_debug

cp -a "$PROJECT_DIR/overlay/." "$WORK_ROOT/"

python3 - "$WORK_ROOT" <<'PY'
from pathlib import Path
import re
import sys

root = Path(sys.argv[1])
apps = root / "firmware/main/apps/apps.h"
main = root / "firmware/main/main.cpp"
cmake = root / "firmware/main/CMakeLists.txt"

# NFC-only app registry: standard app headers and implementations are omitted.
apps.write_text('''/* NFC-only generated app registry. */
#pragma once
#include "app_nfc_debug/app_nfc_debug.h"
''')

main_text = main.read_text()
main_text = re.sub(
    r"    const bool skip_mooncake =\n        GetHAL\(\)\.getXiaozhiConfig\(\)\.startAiAgentOnBoot && GetHAL\(\)\.getWarmRebootTarget\(\) < 0;",
    "    const bool skip_mooncake = false;  // NFC-only diagnostic firmware",
    main_text,
)
main_text = "\n".join(
    line for line in main_text.splitlines()
    if "GetMooncake().installApp" not in line
) + "\n"
install_anchor = "        // Install apps"
install = "        GetMooncake().installApp(std::make_unique<AppNfcDebug>());"
if install_anchor not in main_text:
    raise SystemExit(f"install anchor missing in {main}")
main_text = main_text.replace(install_anchor, install_anchor + "\n" + install)
main.write_text(main_text)

# Upstream globs every app source and links main WHOLE_ARCHIVE. Filter standard
# apps before they enter SOURCES so disabling installation also reduces binary.
cmake_text = cmake.read_text()
filter_anchor = "set(STACK_CHAN_INCLUDE_DIRS"
filter_block = '''# NFC-only diagnostic build: exclude every standard Mooncake app.
list(FILTER STACK_CHAN_SOURCES EXCLUDE REGEX
    "/apps/(app_ai_agent|app_app_center|app_avatar|app_dance|app_espnow_ctrl|app_ezdata|app_launcher|app_setup|app_template)/"
)

'''
if filter_anchor not in cmake_text:
    raise SystemExit(f"source-filter anchor missing in {cmake}")
cmake.write_text(cmake_text.replace(filter_anchor, filter_block + filter_anchor, 1))

# ESP-61: add reusable NFC components to the firmware build.
# 1. EXTRA_COMPONENT_DIRS in firmware CMakeLists.txt
fw_cmake = root / "firmware/CMakeLists.txt"
fw_text = fw_cmake.read_text()
if 'gogolem_nfc' not in fw_text:
    # Components are at esp32-s3-m5/components; firmware is at
    # 0116/.work/StackChan — root is .work/StackChan, so 3 levels up.
    comp_base = str(root.parent.parent.parent / "components")
    extra = f'''# ESP-61: reusable NFC components
set(EXTRA_COMPONENT_DIRS
    "{comp_base}/gogolem_nfc"
    "{comp_base}/gogolem_nfc_engine"
    ${{EXTRA_COMPONENT_DIRS}})
'''
    fw_cmake.write_text(fw_text.replace('include($ENV{IDF_PATH}/tools/cmake/project.cmake)',
        extra + 'include($ENV{IDF_PATH}/tools/cmake/project.cmake)'))

# 2. Add gogolem_nfc and gogolem_nfc_engine to main CMakeLists PRIV_REQUIRES
main_cmake = root / "firmware/main/CMakeLists.txt"
mc_text = main_cmake.read_text()
if 'gogolem_nfc' not in mc_text:
    mc_text = mc_text.replace(
        'PRIV_REQUIRES\n                        esp_pm',
        'PRIV_REQUIRES\n                        gogolem_nfc\n                        gogolem_nfc_engine\n                        esp_pm', 1)
    main_cmake.write_text(mc_text)

# 3. Add M5Unit-NFC to main/idf_component.yml
comp_yml = root / "firmware/main/idf_component.yml"
yml_text = comp_yml.read_text()
if 'M5Unit-NFC' not in yml_text:
    yml_text += '''
  m5stack/M5Unit-NFC:
    git: https://github.com/m5stack/M5Unit-NFC.git
    version: 93745b547364f310cd64b5155a870103a7800a5d
'''
    comp_yml.write_text(yml_text)
PY

printf 'Prepared StackChan %s at %s\n' "$STACKCHAN_COMMIT" "$WORK_ROOT"
printf 'Next: source ~/esp/esp-idf-%s/export.sh && %s/scripts/build.sh\n' "$ESP_IDF_VERSION" "$PROJECT_DIR"
