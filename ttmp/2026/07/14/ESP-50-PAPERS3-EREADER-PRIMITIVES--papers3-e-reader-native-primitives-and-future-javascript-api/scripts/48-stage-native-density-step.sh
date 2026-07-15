#!/usr/bin/env bash
set -euo pipefail
TICKET=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
ROOT=$(git -C "$TICKET" rev-parse --show-toplevel)
EXP="$TICKET/scripts/experiments/EXP-20260715-016-native-epd-density-step-response"
BUILD="$ROOT/0110-papers3-epd-density-step-response/build"
BIN="$BUILD/papers3_epd_density_step_response.bin"
PORT=${PAPERS3_PORT:-/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00}
EXPECTED=eb77a34c7073e0dce54725d2d10ae134168e843825013861f5dfaf8d37134bce
[[ ${1:-} == --execute && ${2:-} == --confirm && ${3:-} == STAGE-DENSITY-STEP ]] || { echo 'usage: --execute --confirm STAGE-DENSITY-STEP' >&2; exit 2; }
[[ -f "$BIN" ]] || { echo 'missing binary' >&2; exit 3; }
[[ $(sha256sum "$BIN" | awk '{print $1}') == "$EXPECTED" ]] || { echo 'binary SHA mismatch' >&2; exit 4; }
[[ ! -e "$EXP/flash-no-reset.log" ]] || { echo 'refusing to replace flash evidence' >&2; exit 5; }
[[ -z $(lsof -t "$PORT" "$(readlink -f "$PORT")" 2>/dev/null | sort -u || true) ]] || { echo 'PaperS3 port already owned' >&2; exit 6; }
unset IDF_PATH IDF_VERSION IDF_PYTHON_ENV_PATH
source /home/manuel/esp/esp-idf-5.4.2/export.sh >/dev/null 2>&1
{
  printf 'binary_sha256=%s\nidf=%s\n' "$EXPECTED" "$(idf.py --version)"
  cd "$BUILD"
  python -m esptool --chip esp32s3 -p "$PORT" -b 460800 --before default_reset --after no_reset write_flash @flash_args
} >"$EXP/flash-no-reset.log" 2>&1
grep -q 'Hash of data verified.' "$EXP/flash-no-reset.log" || { echo 'flash failed' >&2; exit 7; }
echo "result=ok next=arm-readonly-and-density-then-physical-reset"
