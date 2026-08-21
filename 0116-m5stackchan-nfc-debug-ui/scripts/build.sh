#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=../upstream.env
source "$PROJECT_DIR/upstream.env"
WORK_ROOT="${STACKCHAN_WORK_ROOT:-$PROJECT_DIR/.work/StackChan}"
FIRMWARE_DIR="$WORK_ROOT/firmware"

if [[ ! -d "$WORK_ROOT/.git" ]]; then
    echo "missing composed checkout; run ./scripts/prepare.sh first" >&2
    exit 1
fi
if [[ "$(git -C "$WORK_ROOT" rev-parse HEAD)" != "$STACKCHAN_COMMIT" ]]; then
    echo "composed checkout is not at pinned commit $STACKCHAN_COMMIT" >&2
    exit 1
fi
if [[ -z "${IDF_PATH:-}" ]]; then
    echo "IDF_PATH is unset; source ~/esp/esp-idf-$ESP_IDF_VERSION/export.sh" >&2
    exit 1
fi

cd "$FIRMWARE_DIR"
if [[ ! -d xiaozhi-esp32 ]]; then
    python3 ./fetch_repos.py
fi
idf.py build
