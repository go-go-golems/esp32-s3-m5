#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=../upstream.env
source "$PROJECT_DIR/upstream.env"
WORK_ROOT="${STACKCHAN_WORK_ROOT:-$PROJECT_DIR/.work/StackChan}"
PORT="${ESPPORT:-/dev/ttyACM0}"
MODE="${1:-app}"

if [[ -z "${IDF_PATH:-}" ]]; then
    echo "IDF_PATH is unset; source ~/esp/esp-idf-$ESP_IDF_VERSION/export.sh" >&2
    exit 1
fi
if [[ ! -f "$WORK_ROOT/firmware/build/stack-chan.bin" ]]; then
    echo "firmware is not built; run ./scripts/build.sh first" >&2
    exit 1
fi

cd "$WORK_ROOT/firmware"
case "$MODE" in
    --full|full)
        idf.py -p "$PORT" flash
        ;;
    app|--app)
        idf.py -p "$PORT" app-flash
        ;;
    *)
        echo "usage: $0 [app|--full]" >&2
        exit 2
        ;;
esac
