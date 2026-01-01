#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
test_repeat_repl_qemu_uart_stdio.sh

Runs QEMU with UART on stdio (no idf_monitor, no tcp socket) and checks that the
REPL echoes input. This isolates "UART RX in QEMU" vs "idf_monitor/socket path".

Usage:
  ./tools/test_repeat_repl_qemu_uart_stdio.sh [--timeout SECONDS]

What it does:
  1) Builds firmware
  2) Creates build/qemu_flash.bin via esptool merge_bin
  3) Runs qemu-system-xtensa with -serial stdio -monitor none
  4) Waits for 'repeat>' then writes ':mode' and expects 'mode: repeat'

EOF
}

TIMEOUT_SECS="45"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --timeout)
      TIMEOUT_SECS="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown arg: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${PROJECT_DIR}"

REQUIRED_ESP_IDF_VERSION_PREFIX="${REQUIRED_ESP_IDF_VERSION_PREFIX:-5.4}"
IDF_EXPORT_SH="${IDF_EXPORT_SH:-$HOME/esp/esp-idf-5.4.1/export.sh}"
if [[ "${ESP_IDF_VERSION:-}" != ${REQUIRED_ESP_IDF_VERSION_PREFIX}* ]]; then
  # shellcheck disable=SC1090
  source "${IDF_EXPORT_SH}" >/dev/null
fi

idf.py build >/dev/null

mkdir -p build
LOG_PATH="build/repeat_repl_qemu_uart_stdio.log"

FLASH_BIN="build/qemu_flash.bin"
EFUSE_BIN="build/qemu_efuse.bin"

if [[ ! -f "${EFUSE_BIN}" ]]; then
  dd if=/dev/zero of="${EFUSE_BIN}" bs=1 count=4096 status=none
fi

python -m esptool --chip=esp32s3 merge_bin \
  --output="${FLASH_BIN}" \
  --fill-flash-size=8MB \
  --flash_mode dio \
  --flash_freq 80m \
  --flash_size 8MB \
  0x0 build/bootloader/bootloader.bin \
  0x8000 build/partition_table/partition-table.bin \
  0x10000 build/mqjs-repl.bin >/dev/null

if ! command -v qemu-system-xtensa >/dev/null 2>&1; then
  echo "qemu-system-xtensa not found on PATH (did you install qemu-xtensa via idf_tools?)" >&2
  exit 1
fi

python - <<PY
import os
import pty
import selectors
import subprocess
import sys
import time

timeout_secs = int(os.environ.get("TIMEOUT_SECS", "${TIMEOUT_SECS}"))
log_path = "${LOG_PATH}"
flash_bin = "${FLASH_BIN}"
efuse_bin = "${EFUSE_BIN}"

qemu_cmd = [
    "qemu-system-xtensa",
    "-M", "esp32s3",
    "-drive", f"file={flash_bin},if=mtd,format=raw",
    "-drive", f"file={efuse_bin},if=none,format=raw,id=efuse",
    "-global", "driver=nvram.esp32c3.efuse,property=drive,value=efuse",
    "-global", "driver=timer.esp32s3.timg,property=wdt_disable,value=true",
    "-nographic",
    "-monitor", "none",
    "-serial", "stdio",
]

master_fd, slave_fd = pty.openpty()
proc = subprocess.Popen(
    qemu_cmd,
    stdin=slave_fd,
    stdout=slave_fd,
    stderr=slave_fd,
    close_fds=True,
)
os.close(slave_fd)

buf = b""
start = time.time()
sent_mode = False

def write_line(line: str) -> None:
    os.write(master_fd, (line + "\r\n").encode("utf-8"))

try:
    sel = selectors.DefaultSelector()
    sel.register(master_fd, selectors.EVENT_READ)

    while True:
        if time.time() - start > timeout_secs:
            raise TimeoutError("timeout")

        events = sel.select(timeout=0.2)
        if not events:
            continue
        chunk = os.read(master_fd, 4096)
        if not chunk:
            continue
        buf += chunk

        if (b"repeat>" in buf) and (not sent_mode):
            write_line(":mode")
            sent_mode = True

        if b"mode: repeat" in buf:
            with open(log_path, "wb") as f:
                f.write(buf)
            print(f"OK: QEMU UART stdio smoke test passed (log: {log_path})", file=sys.stderr)
            proc.terminate()
            break

except Exception as e:
    with open(log_path, "wb") as f:
        f.write(buf)
    print(f"FAIL: {e} (log: {log_path})", file=sys.stderr)
    proc.terminate()
    sys.exit(1)
finally:
    try:
        os.close(master_fd)
    except Exception:
        pass
PY
