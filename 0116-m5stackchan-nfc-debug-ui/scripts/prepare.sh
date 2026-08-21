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
import sys

root = Path(sys.argv[1])
apps = root / "firmware/main/apps/apps.h"
main = root / "firmware/main/main.cpp"

apps_text = apps.read_text()
include = '#include "app_nfc_debug/app_nfc_debug.h"'
if include not in apps_text:
    apps_text = apps_text.rstrip() + "\n" + include + "\n"
    apps.write_text(apps_text)

main_text = main.read_text()
install = "        GetMooncake().installApp(std::make_unique<AppNfcDebug>());"
anchor = "        GetMooncake().installApp(std::make_unique<AppSetup>());"
if install not in main_text:
    if anchor not in main_text:
        raise SystemExit(f"registration anchor missing in {main}")
    main_text = main_text.replace(anchor, anchor + "\n" + install)
    main.write_text(main_text)
PY

printf 'Prepared StackChan %s at %s\n' "$STACKCHAN_COMMIT" "$WORK_ROOT"
printf 'Next: source ~/esp/esp-idf-%s/export.sh && %s/scripts/build.sh\n' "$ESP_IDF_VERSION" "$PROJECT_DIR"
