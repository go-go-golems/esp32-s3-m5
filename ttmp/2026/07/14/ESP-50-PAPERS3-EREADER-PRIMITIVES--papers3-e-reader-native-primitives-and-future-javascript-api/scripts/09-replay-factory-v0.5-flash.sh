#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
BIN="$ROOT/sources/hardware/factory-v0.5/C139-PaperS3-FactoryTest-V0.5_0x0.bin"
EXPECTED_SHA='d6733a0ca378f95335fa5fba4d4d992fb1dd97c17557b20e9aebfca08ba6d624'
PORT=${PAPERS3_PORT:-/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00}
PYTHON=${ESPTOOL_PYTHON:-/home/manuel/.espressif/python_env/idf5.3_py3.13_env/bin/python}
OUT_DIR="$ROOT/scripts/output"
mkdir -p "$OUT_DIR"

usage() {
  cat <<'EOF'
Usage: 09-replay-factory-v0.5-flash.sh [--check|--execute]

--check    Verify the official merged binary, esptool image, serial path, and
           current port ownership without changing the device. This is default.
--execute  Perform the flash after all checks pass. The caller must first exit
           any monitor that owns the same port.

Environment:
  PAPERS3_PORT     Override the stable USB Serial/JTAG by-id path.
  ESPTOOL_PYTHON   Override the Python interpreter containing esptool.
EOF
}

mode=check
case "${1:---check}" in
  --check) mode=check ;;
  --execute) mode=execute ;;
  -h|--help) usage; exit 0 ;;
  *) usage >&2; exit 2 ;;
esac

[[ -f "$BIN" ]] || { echo "error: missing factory binary: $BIN" >&2; exit 1; }
actual_sha=$(sha256sum "$BIN" | awk '{print $1}')
[[ "$actual_sha" == "$EXPECTED_SHA" ]] || {
  echo "error: factory SHA mismatch: expected=$EXPECTED_SHA actual=$actual_sha" >&2
  exit 1
}
[[ -x "$PYTHON" ]] || { echo "error: esptool Python is not executable: $PYTHON" >&2; exit 1; }
[[ -e "$PORT" ]] || { echo "error: serial path does not exist: $PORT" >&2; exit 1; }

stamp=$(date -u +%Y%m%dT%H%M%SZ)
log="$OUT_DIR/09-factory-v0.5-${mode}-${stamp}.txt"
{
  echo "mode=$mode"
  echo "utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "binary=$BIN"
  echo "sha256=$actual_sha"
  echo "port=$PORT"
  echo "resolved_port=$(readlink -f "$PORT")"
  echo "python=$PYTHON"
  "$PYTHON" -m esptool image_info "$BIN"

  if fuser "$PORT" >/dev/null 2>&1; then
    echo "serial_owner=present"
    fuser -v "$PORT" 2>&1
    if [[ "$mode" == execute ]]; then
      echo "error: refusing to flash while the serial port has an owner" >&2
      exit 1
    fi
  else
    echo "serial_owner=none"
  fi

  if [[ "$mode" == execute ]]; then
    "$PYTHON" -m esptool \
      --chip esp32s3 --port "$PORT" --baud 115200 \
      --before default_reset --after hard_reset \
      write_flash 0x0 "$BIN"
  else
    echo "result=check-only; device unchanged"
  fi
} 2>&1 | tee "$log"

echo "log=$log"
