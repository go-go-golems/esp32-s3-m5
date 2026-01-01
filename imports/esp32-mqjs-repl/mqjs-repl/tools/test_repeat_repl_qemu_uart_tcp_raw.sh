#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
test_repeat_repl_qemu_uart_tcp_raw.sh

Starts QEMU with UART0 exposed as a raw TCP server and drives it via a plain TCP
socket (no idf_monitor). This isolates "UART RX in QEMU" vs "idf_monitor issues".

Usage:
  ./tools/test_repeat_repl_qemu_uart_tcp_raw.sh [--port TCP_PORT] [--timeout SECONDS]

EOF
}

TCP_PORT="5556"
TIMEOUT_SECS="45"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --port)
      TCP_PORT="$2"
      shift 2
      ;;
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
LOG_PATH="build/repeat_repl_qemu_uart_tcp_raw.log"

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

QEMU_LOG="build/qemu_uart_tcp_raw_${TCP_PORT}.log"

qemu-system-xtensa \
  -M esp32s3 \
  -drive "file=${FLASH_BIN},if=mtd,format=raw" \
  -drive "file=${EFUSE_BIN},if=none,format=raw,id=efuse" \
  -global "driver=nvram.esp32c3.efuse,property=drive,value=efuse" \
  -global "driver=timer.esp32s3.timg,property=wdt_disable,value=true" \
  -nic user,model=open_eth \
  -nographic \
  -monitor none \
  -serial "tcp:127.0.0.1:${TCP_PORT},server,nowait" \
  >"${QEMU_LOG}" 2>&1 &

QEMU_PID="$!"
cleanup() {
  kill "${QEMU_PID}" >/dev/null 2>&1 || true
  wait "${QEMU_PID}" >/dev/null 2>&1 || true
}
trap cleanup EXIT

python - <<PY
import socket
import time
import sys

tcp_port = int("${TCP_PORT}")
timeout_secs = float("${TIMEOUT_SECS}")
log_path = "${LOG_PATH}"
qemu_log = "${QEMU_LOG}"

start = time.time()
buf = b""

def deadline() -> bool:
  return (time.time() - start) > timeout_secs

while True:
  try:
    s = socket.create_connection(("127.0.0.1", tcp_port), timeout=1.0)
    break
  except Exception:
    if deadline():
      raise SystemExit(f"FAIL: could not connect to QEMU TCP UART on {tcp_port}")
    time.sleep(0.2)

s.settimeout(0.2)

def read_some():
  global buf
  try:
    chunk = s.recv(4096)
    if chunk:
      buf += chunk
  except socket.timeout:
    pass

def wait_for(needle: bytes) -> bool:
  while needle not in buf:
    if deadline():
      return False
    read_some()
  return True

if not wait_for(b"repeat>"):
  open(log_path, "wb").write(buf)
  raise SystemExit(f"FAIL: timeout waiting for prompt (serial log: {log_path}, qemu log: {qemu_log})")

s.sendall(b":mode\r\n")

if not wait_for(b"mode: repeat"):
  open(log_path, "wb").write(buf)
  raise SystemExit(f"FAIL: no response to :mode (serial log: {log_path}, qemu log: {qemu_log})")

open(log_path, "wb").write(buf)
print(f"OK: QEMU UART TCP raw smoke test passed (serial log: {log_path}, qemu log: {qemu_log})", file=sys.stderr)
PY

