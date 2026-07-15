#!/usr/bin/env bash
set -euo pipefail

TICKET=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
ROOT=$(git -C "$TICKET" rev-parse --show-toplevel)
EXP="$TICKET/scripts/experiments/EXP-20260715-013-factory-f2-esptool-reset-density"
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
    *) echo "usage: $0 [--check|--execute --confirm RUN-DENS-F2-AUTORESET]" >&2; exit 2 ;;
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
[[ "$CONFIRM" == RUN-DENS-F2-AUTORESET ]] || { echo 'execute requires --confirm RUN-DENS-F2-AUTORESET' >&2; exit 5; }

STAGE="$EXP/f2-stage-no-reset.log"
RESET="$EXP/f2-esptool-hard-reset.log"
RAW="$EXP/raw-dynamic-f2.jsonl"
COLLECTOR="$EXP/collector-console.log"
HOST="$EXP/host-events.jsonl"
TRANSCRIPT="$EXP/firmware-transcript.log"
RING="$EXP/ring.jsonl"
ALIGNED="$EXP/ring-host-aligned.jsonl"
ALIGNMENT="$EXP/ring-alignment.json"
for path in "$STAGE" "$RESET" "$RAW" "$COLLECTOR" "$HOST" "$TRANSCRIPT" "$RING" "$ALIGNED" "$ALIGNMENT" "$EXP/evidence.sha256"; do
  [[ ! -e "$path" ]] || { echo "refusing to replace evidence: $path" >&2; exit 6; }
done

# Phase 1: esptool is the sole owner; leave the verified F2 image unbooted.
{
  printf 'utc=%s\nmode=flash-after-no-reset\nbinary=%s\nsha256=%s\nidf=%s\n' "$(date -u +%Y%m%dT%H%M%SZ)" "$BIN" "$actual" "$(idf.py --version)"
  cd "$BUILD"
  python -m esptool --chip esp32s3 -p "$PORT" -b 460800 --before default_reset --after no_reset write_flash @flash_args
} >"$STAGE" 2>&1
grep -q 'Hash of data verified.' "$STAGE" || { echo "F2 flash failed; inspect $STAGE" >&2; exit 7; }

# Phase 2: esptool remains sole owner, verifies the staged bootloader link,
# applies its ESP32-S3 USB-JTAG hard-reset sequence, then exits and closes ACM0.
python - "$HOST" activation_begin <<'PY'
import json, os, socket, sys, time
path, event = sys.argv[1:]
with open(path, 'a', encoding='utf-8') as stream:
    stream.write(json.dumps({'schema':'esp50.host-marker.v1', 'event':event, 'host_monotonic_ns':time.monotonic_ns(), 'host_realtime_ns':time.time_ns(), 'pid':os.getpid(), 'hostname':socket.gethostname()}, sort_keys=True, separators=(',', ':')) + '\n')
PY
python -m esptool --chip esp32s3 -p "$PORT" -b 460800 --before no_reset --after hard_reset --no-stub chip_id >"$RESET" 2>&1
grep -q 'Chip is ESP32-S3' "$RESET" || { echo "F2 esptool hard reset failed; inspect $RESET" >&2; exit 8; }
python - "$HOST" activation_hard_reset_complete <<'PY'
import json, os, socket, sys, time
path, event = sys.argv[1:]
with open(path, 'a', encoding='utf-8') as stream:
    stream.write(json.dumps({'schema':'esp50.host-marker.v1', 'event':event, 'host_monotonic_ns':time.monotonic_ns(), 'host_realtime_ns':time.time_ns(), 'pid':os.getpid(), 'hostname':socket.gethostname()}, sort_keys=True, separators=(',', ':')) + '\n')
PY

# Phase 3: the Python collector opens PaperS3 read-only, without pyserial or
# DTR/RTS ioctls. The F2 ring is deliberately dumped post-idle, well after boot.
"$TICKET/scripts/29-capture-synchronized-serial.py" \
  --execute --dens-raw-stream --confirm ENABLE-DENS-RAW-STREAM \
  --gain 2 --integration 0 --light-duty 128 --duration 60 --output "$RAW" >"$COLLECTOR" 2>&1
grep -q '"event":"capture_end"' "$RAW" || { echo 'capture_end missing' >&2; exit 9; }
grep -q '"result":"ok"' "$RAW" || { echo 'collector result not ok' >&2; exit 9; }
"$TICKET/scripts/38-extract-factory-f2-ring.py" \
  --capture "$RAW" --transcript "$TRANSCRIPT" --ring "$RING" \
  --aligned-ring "$ALIGNED" --alignment "$ALIGNMENT"
(cd "$EXP" && sha256sum f2-stage-no-reset.log f2-esptool-hard-reset.log raw-dynamic-f2.jsonl collector-console.log host-events.jsonl firmware-transcript.log ring.jsonl ring-host-aligned.jsonl ring-alignment.json > evidence.sha256 && sha256sum -c evidence.sha256)
printf 'stage_log=%s\nreset_log=%s\nring=%s\nevidence=%s/evidence.sha256\nhardware_modified=yes\n' "$STAGE" "$RESET" "$RING" "$EXP"
