#!/usr/bin/env bash
set -euo pipefail

TICKET_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
REPO_ROOT=$(git -C "$TICKET_ROOT" rev-parse --show-toplevel)
PROJECT="$REPO_ROOT/0107-papers3-epd-painter-control"
IDF_ROOT='/home/manuel/esp/esp-idf-5.4.2'
BUILD_DIR="$PROJECT/build-ticket"
SDKCONFIG="$PROJECT/sdkconfig.ticket"
DEFAULT_PORT='/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00'
REPORT="$TICKET_ROOT/scripts/output/12-epd-painter-build-latest.md"
AUDIT="$TICKET_ROOT/scripts/output/13-built-control-audit-latest.md"
MODE='check'
PORT="$DEFAULT_PORT"
CONFIRM=''

usage() {
  cat <<'EOF'
Usage: 15-flash-epd-control.sh [--check|--execute] [--port PATH] [--confirm FLASH-P0.17]

--check is non-destructive. --execute requires the exact confirmation token,
refuses a serial port with an owner, and writes a timestamped flash log.
EOF
}

while (($#)); do
  case "$1" in
    --check) MODE='check'; shift ;;
    --execute) MODE='execute'; shift ;;
    --port) PORT=${2:?missing port}; shift 2 ;;
    --confirm) CONFIRM=${2:?missing token}; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "error: unknown argument: $1" >&2; usage >&2; exit 2 ;;
  esac
done

for required in "$IDF_ROOT/export.sh" "$REPORT" "$AUDIT" \
  "$BUILD_DIR/papers3_epd_painter_control.bin" "$SDKCONFIG"; do
  [[ -f "$required" ]] || { echo "error: missing $required" >&2; exit 2; }
done
[[ -e "$PORT" ]] || { echo "error: serial port not present: $PORT" >&2; exit 2; }
REAL_PORT=$(readlink -f "$PORT")

owners=$(lsof -t "$PORT" "$REAL_PORT" 2>/dev/null | sort -u || true)
if [[ -n "$owners" ]]; then
  echo "error: serial port already owned by PID(s): $owners" >&2
  lsof "$PORT" "$REAL_PORT" 2>/dev/null || true
  exit 3
fi

APP_BIN="$BUILD_DIR/papers3_epd_painter_control.bin"
ACTUAL_SHA=$(sha256sum "$APP_BIN" | awk '{print $1}')
REPORT_SHA=$(grep -E 'Application BIN:.*SHA-256' "$REPORT" | grep -Eo '[0-9a-f]{64}' | tail -1)
[[ "$ACTUAL_SHA" == "$REPORT_SHA" ]] || {
  echo "error: application hash differs from latest build report" >&2
  echo "actual=$ACTUAL_SHA report=$REPORT_SHA" >&2
  exit 4
}
grep -q 'Gate: \*\*PASS\*\*' "$AUDIT" || {
  echo "error: latest binary audit is not PASS" >&2
  exit 4
}
grep -q 'Hardware modified: \*\*no\*\*' "$REPORT" || {
  echo "error: latest build report does not identify a no-flash build" >&2
  exit 4
}

# shellcheck disable=SC1091
source "$IDF_ROOT/export.sh" >/dev/null 2>&1
[[ "$(idf.py --version)" == 'ESP-IDF v5.4.2' ]] || {
  echo "error: exact ESP-IDF 5.4.2 is not active" >&2
  exit 4
}
python -m esptool --chip esp32s3 image_info "$APP_BIN" >/dev/null

printf 'mode=%s\nport=%s\nreal_port=%s\napp_sha256=%s\naudit=PASS\nserial_owner=none\n' \
  "$MODE" "$PORT" "$REAL_PORT" "$ACTUAL_SHA"

if [[ "$MODE" == 'check' ]]; then
  echo 'hardware_modified=no'
  exit 0
fi
if [[ "$CONFIRM" != 'FLASH-P0.17' ]]; then
  echo "error: --execute requires --confirm FLASH-P0.17" >&2
  exit 5
fi

STAMP=$(date -u +%Y%m%dT%H%M%SZ)
LOG="$TICKET_ROOT/scripts/output/15-epd-control-flash-$STAMP.log"
normalize_log() { [[ ! -f "$LOG" ]] || perl -pi -e 's/[ \t]+$//' "$LOG"; }
trap normalize_log EXIT
{
  echo "flash_utc=$STAMP"
  echo "port=$PORT"
  echo "real_port=$REAL_PORT"
  echo "app_sha256=$ACTUAL_SHA"
  echo "idf=$(idf.py --version)"
  cd "$BUILD_DIR"
  # Flash exactly the audited artifacts. `idf.py flash` may reconfigure and
  # relink the app descriptor when unrelated repository state changes, which
  # would invalidate the preflight SHA after it was checked.
  python -m esptool --chip esp32s3 -p "$PORT" -b 460800 \
    --before default_reset --after hard_reset write_flash @flash_args
  POST_FLASH_SHA=$(sha256sum "$APP_BIN" | awk '{print $1}')
  [[ "$POST_FLASH_SHA" == "$ACTUAL_SHA" ]] || {
    echo "error: application artifact changed during flash" >&2
    exit 6
  }
  echo "post_flash_app_sha256=$POST_FLASH_SHA"
} 2>&1 | tee "$LOG"

echo "flash_log=$LOG"
echo 'hardware_modified=yes'
