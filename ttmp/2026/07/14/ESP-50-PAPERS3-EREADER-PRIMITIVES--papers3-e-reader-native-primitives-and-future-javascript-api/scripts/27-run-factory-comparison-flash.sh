#!/usr/bin/env bash
set -euo pipefail

TICKET_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
REPO_ROOT=$(git -C "$TICKET_ROOT" rev-parse --show-toplevel)
PROJECT="$REPO_ROOT/0109-papers3-factory-v0.5-runtime-trace"
IDF='/home/manuel/esp/esp-idf-5.3.3'
PORT=${PAPERS3_PORT:-/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00}
MODE=check
TREATMENT=''
CONFIRM=''

while (($#)); do
  case "$1" in
    --check) MODE=check; shift ;;
    --execute) MODE=execute; shift ;;
    --treatment) TREATMENT=${2:?missing treatment}; shift 2 ;;
    --confirm) CONFIRM=${2:?missing token}; shift 2 ;;
    *) echo "usage: $0 --treatment f0|f1|f2 [--check|--execute --confirm RUN-F0|RUN-F1|RUN-F2]" >&2; exit 2 ;;
  esac
done
case "$TREATMENT" in f0|f1|f2) ;; *) echo 'error: treatment must be f0, f1, or f2' >&2; exit 2 ;; esac

case "$TREATMENT" in
  f0)
    EXP='EXP-20260714-001-factory-v05-exact-f0'
    BIN="$TICKET_ROOT/sources/hardware/factory-v0.5/C139-PaperS3-FactoryTest-V0.5_0x0.bin"
    EXPECTED='d6733a0ca378f95335fa5fba4d4d992fb1dd97c17557b20e9aebfca08ba6d624'
    ;;
  f1)
    EXP='EXP-20260714-002-factory-v05-source-f1-off'
    BUILD="$PROJECT/build-off"; BIN="$BUILD/papers3_factory_v05_trace.bin"
    EXPECTED='3d9bf37a5c5faa120fa1dccf357e8d0676a77495359754d062a5fa654dd2d2b3'
    ;;
  f2)
    EXP='EXP-20260714-003-factory-v05-source-f2-trace'
    BUILD="$PROJECT/build-trace"; BIN="$BUILD/papers3_factory_v05_trace.bin"
    EXPECTED='95334c261762205ab95d3f578a5d3d0a0eac4fe7fffdfd1ada0e836ba8a2d755'
    ;;
esac
EXP_DIR="$TICKET_ROOT/scripts/experiments/$EXP"
AUDIT="$TICKET_ROOT/scripts/output/25-factory-v0.5-trace-audit-latest.md"
[[ -d "$EXP_DIR" && -f "$BIN" && -f "$AUDIT" ]] || { echo 'error: missing preregistration, binary, or audit' >&2; exit 3; }
[[ -e "$PORT" ]] || { echo "error: port missing: $PORT" >&2; exit 3; }
REAL=$(readlink -f "$PORT")
owners=$(lsof -t "$PORT" "$REAL" 2>/dev/null | sort -u || true)
[[ -z "$owners" ]] || { echo "error: serial owned by PID(s): $owners" >&2; exit 3; }
actual=$(sha256sum "$BIN" | awk '{print $1}')
[[ "$actual" == "$EXPECTED" ]] || { echo "error: $TREATMENT binary hash mismatch expected=$EXPECTED actual=$actual" >&2; exit 4; }
grep -q 'Gate: \*\*PASS\*\*' "$AUDIT" || { echo 'error: audit is not PASS' >&2; exit 4; }
# shellcheck disable=SC1091
unset IDF_PATH IDF_VERSION IDF_PYTHON_ENV_PATH
source "$IDF/export.sh" >/dev/null 2>&1
[[ "$(idf.py --version)" == 'ESP-IDF v5.3.3' ]] || { echo 'error: wrong IDF environment' >&2; exit 4; }
python -m esptool --chip esp32s3 image_info "$BIN" >/dev/null

printf 'mode=%s\ntreatment=%s\nexperiment=%s\nport=%s\nreal_port=%s\napp_sha256=%s\naudit=PASS\nserial_owner=none\n' \
  "$MODE" "$TREATMENT" "$EXP" "$PORT" "$REAL" "$actual"
if [[ "$MODE" == check ]]; then echo 'hardware_modified=no'; exit 0; fi
expected_token="RUN-${TREATMENT^^}"
[[ "$CONFIRM" == "$expected_token" ]] || { echo "error: execute requires --confirm $expected_token" >&2; exit 5; }

STAMP=$(date -u +%Y%m%dT%H%M%SZ)
LOG="$EXP_DIR/flash-$STAMP.log"
echo "[flash] treatment=$TREATMENT; full output -> $LOG"
set +e
{
  printf 'utc=%s\ntreatment=%s\nbinary=%s\nsha256=%s\nidf=%s\n' "$STAMP" "$TREATMENT" "$BIN" "$actual" "$(idf.py --version)"
  if [[ "$TREATMENT" == f0 ]]; then
    python -m esptool --chip esp32s3 -p "$PORT" -b 115200 --before default_reset --after hard_reset write_flash 0x0 "$BIN"
  else
    cd "$BUILD"
    python -m esptool --chip esp32s3 -p "$PORT" -b 460800 --before default_reset --after hard_reset write_flash @flash_args
  fi
} >"$LOG" 2>&1
status=$?
set -e
perl -pi -e 's/[ \t]+$//' "$LOG"
if [[ $status -ne 0 ]]; then
  echo "error: flash failed (exit $status); filtered tail:" >&2
  tail -n 60 "$LOG" | cut -c1-500 >&2
  exit "$status"
fi
printf 'flash_log=%s\nhardware_modified=yes\n' "$LOG"
