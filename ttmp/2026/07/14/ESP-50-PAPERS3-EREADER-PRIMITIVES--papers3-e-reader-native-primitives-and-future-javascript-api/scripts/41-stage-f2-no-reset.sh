#!/usr/bin/env bash
set -euo pipefail

TICKET=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
ROOT=$(git -C "$TICKET" rev-parse --show-toplevel)
EXP="$TICKET/scripts/experiments/EXP-20260715-012-factory-f2-manual-reset-density"
PROJECT="$ROOT/0109-papers3-factory-v0.5-runtime-trace"
BUILD="$PROJECT/build-trace"
BIN="$BUILD/papers3_factory_v05_trace.bin"
IDF='/home/manuel/esp/esp-idf-5.3.3'
PORT=${PAPERS3_PORT:-/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00}
EXPECTED='95334c261762205ab95d3f578a5d3d0a0eac4fe7fffdfd1ada0e836ba8a2d755'
AUDIT="$TICKET/scripts/output/25-factory-v0.5-trace-audit-latest.md"
MODE=check
CONFIRM=''

while (($#)); do
  case "$1" in
    --check) MODE=check; shift ;;
    --execute) MODE=execute; shift ;;
    --confirm) CONFIRM=${2:?missing confirmation}; shift 2 ;;
    *) echo "usage: $0 [--check|--execute --confirm STAGE-F2-NO-RESET]" >&2; exit 2 ;;
  esac
done

[[ -d "$EXP" && -f "$BIN" && -f "$AUDIT" ]] || { echo 'missing experiment, F2 binary, or audit' >&2; exit 3; }
(cd "$EXP" && sha256sum -c preregistration.sha256 >/dev/null)
[[ -e "$PORT" ]] || { echo "port missing: $PORT" >&2; exit 3; }
REAL=$(readlink -f "$PORT")
owners=$(lsof -t "$PORT" "$REAL" 2>/dev/null | sort -u || true)
[[ -z "$owners" ]] || { echo "serial owned by PID(s): $owners" >&2; exit 3; }
actual=$(sha256sum "$BIN" | awk '{print $1}')
[[ "$actual" == "$EXPECTED" ]] || { echo "F2 hash mismatch expected=$EXPECTED actual=$actual" >&2; exit 4; }
grep -q 'Gate: \*\*PASS\*\*' "$AUDIT" || { echo 'F2 audit is not PASS' >&2; exit 4; }
unset IDF_PATH IDF_VERSION IDF_PYTHON_ENV_PATH
# shellcheck disable=SC1090
source "$IDF/export.sh" >/dev/null 2>&1
[[ "$(idf.py --version)" == 'ESP-IDF v5.3.3' ]] || { echo 'wrong IDF environment' >&2; exit 4; }
python -m esptool --chip esp32s3 image_info "$BIN" >/dev/null

printf 'mode=%s\nexperiment=%s\nport=%s\nreal_port=%s\napp_sha256=%s\naudit=PASS\n' \
  "$MODE" "$(basename "$EXP")" "$PORT" "$REAL" "$actual"
if [[ "$MODE" == check ]]; then
  echo 'hardware_modified=no'
  exit 0
fi
[[ "$CONFIRM" == STAGE-F2-NO-RESET ]] || { echo 'execute requires --confirm STAGE-F2-NO-RESET' >&2; exit 5; }
LOG="$EXP/f2-stage-no-reset.log"
[[ ! -e "$LOG" ]] || { echo "refusing to replace evidence: $LOG" >&2; exit 6; }
{
  printf 'utc=%s\nmode=stage-no-reset\nbinary=%s\nsha256=%s\nidf=%s\n' "$(date -u +%Y%m%dT%H%M%SZ)" "$BIN" "$actual" "$(idf.py --version)"
  cd "$BUILD"
  python -m esptool --chip esp32s3 -p "$PORT" -b 460800 --before default_reset --after no_reset write_flash @flash_args
} >"$LOG" 2>&1
perl -pi -e 's/[ \t]+$//' "$LOG"
grep -q 'Hash of data verified.' "$LOG" || { echo "F2 stage failed; inspect $LOG" >&2; exit 7; }
printf 'stage_log=%s\nnext_state=awaiting_manual_reset\nhardware_modified=yes\n' "$LOG"
