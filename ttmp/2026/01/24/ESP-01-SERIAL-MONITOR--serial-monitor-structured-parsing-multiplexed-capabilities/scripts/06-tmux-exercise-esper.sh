#!/usr/bin/env bash
set -euo pipefail

# Tmux harness to exercise the esper monitor + esp32s3 test firmware.
#
# Usage:
#   ./scripts/06-tmux-exercise-esper.sh \
#     --port '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*'
#
# Notes:
# - We intentionally gate starting `esper` until after flashing is done, to avoid port contention.

ROOT_DIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5"
ESPER_DIR="${ROOT_DIR}/esper"
FW_DIR="${ESPER_DIR}/firmware/esp32s3-test"

IDF_EXPORT_SH="${IDF_EXPORT_SH:-$HOME/esp/esp-idf-5.4.1/export.sh}"
PORT_GLOB=""
BAUD="${BAUD:-115200}"
TOOLCHAIN_PREFIX="${TOOLCHAIN_PREFIX:-xtensa-esp32s3-elf-}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --port)
      PORT_GLOB="${2:-}"; shift 2 ;;
    --idf-export-sh)
      IDF_EXPORT_SH="${2:-}"; shift 2 ;;
    --baud)
      BAUD="${2:-}"; shift 2 ;;
    --toolchain-prefix)
      TOOLCHAIN_PREFIX="${2:-}"; shift 2 ;;
    *)
      echo "unknown arg: $1" >&2
      exit 2 ;;
  esac
done

if [[ -z "${PORT_GLOB}" ]]; then
  echo "missing --port (use a stable by-id glob)" >&2
  exit 2
fi

SESSION="esper-demo"
tmux has-session -t "${SESSION}" 2>/dev/null && tmux kill-session -t "${SESSION}"

tmux new-session -d -s "${SESSION}" -n flash

tmux send-keys -t "${SESSION}:flash" "source \"${IDF_EXPORT_SH}\" && cd \"${FW_DIR}\" && idf.py set-target esp32s3 && idf.py build && idf.py flash" C-m
tmux send-keys -t "${SESSION}:flash" "echo && echo \"FLASH DONE. Switch to window 'monitor' and press Enter to start esper.\" && bash" C-m

tmux new-window -t "${SESSION}" -n monitor
tmux send-keys -t "${SESSION}:monitor" "cd \"${ESPER_DIR}\"" C-m
tmux send-keys -t "${SESSION}:monitor" "echo \"Waiting for flash. Press Enter to start esper...\"; read -r _" C-m
tmux send-keys -t "${SESSION}:monitor" "go run ./cmd/esper -port \"${PORT_GLOB}\" -baud \"${BAUD}\" -elf \"${FW_DIR}/build/esp32s3-test.elf\" -toolchain-prefix \"${TOOLCHAIN_PREFIX}\"" C-m

tmux attach -t "${SESSION}"

