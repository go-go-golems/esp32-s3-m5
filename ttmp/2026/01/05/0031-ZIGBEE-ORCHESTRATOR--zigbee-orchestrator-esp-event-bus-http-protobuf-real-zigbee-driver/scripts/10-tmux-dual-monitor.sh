#!/usr/bin/env bash
set -euo pipefail

# Dual monitor helper for 0031:
# - pane 0: H2 NCP monitor
# - pane 1: Cardputer host monitor (gw console)
#
# Usage:
#   H2_PORT=/dev/ttyACM0 HOST_PORT=/dev/ttyACM1 ./scripts/10-tmux-dual-monitor.sh
#
# Defaults try to use stable /dev/serial/by-id/ names.
#
# IMPORTANT:
# - Always pass `--no-reset` when monitoring the H2 NCP. `idf.py monitor` toggles
#   DTR/RTS by default and can reset the H2; if the H2 reboots while the host is
#   connected over the Grove UART, the H2 boot ROM logs can spill onto the ZNSP
#   SLIP link and corrupt in-flight requests.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
HOST_PROJ="${ROOT_DIR}/0031-zigbee-orchestrator"
H2_PROJ_DEFAULT="${ROOT_DIR}/thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp"

H2_PROJ="${H2_PROJ:-$H2_PROJ_DEFAULT}"

H2_PORT="${H2_PORT:-/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_48:31:B7:CA:45:5B-if00}"
HOST_PORT="${HOST_PORT:-/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00}"

SESSION="${SESSION:-0031}"
TMUX_SOCK="${TMUX_SOCK:-$HOME/.tmux-0031.sock}"

if [[ ! -e "$H2_PORT" ]]; then
  echo "ERROR: H2_PORT not found: $H2_PORT" >&2
  echo "Set H2_PORT=/dev/ttyACM? or a /dev/serial/by-id/ path." >&2
  exit 2
fi

if [[ ! -e "$HOST_PORT" ]]; then
  echo "ERROR: HOST_PORT not found: $HOST_PORT" >&2
  echo "Set HOST_PORT=/dev/ttyACM? or a /dev/serial/by-id/ path." >&2
  exit 2
fi

if [[ ! -d "$HOST_PROJ" ]]; then
  echo "ERROR: Host project not found: $HOST_PROJ" >&2
  exit 2
fi

if [[ ! -d "$H2_PROJ" ]]; then
  echo "ERROR: H2 project not found: $H2_PROJ" >&2
  echo "Set H2_PROJ=/abs/path/to/esp_zigbee_ncp" >&2
  exit 2
fi

mkdir -p "$(dirname "$TMUX_SOCK")"

tmux -S "$TMUX_SOCK" has-session -t "$SESSION" 2>/dev/null && {
  echo "Attaching existing session: $SESSION"
  exec tmux -S "$TMUX_SOCK" attach -t "$SESSION"
}

tmux -S "$TMUX_SOCK" new-session -d -s "$SESSION" -c "$H2_PROJ" \
  "idf.py -C \"$H2_PROJ\" -p \"$H2_PORT\" monitor --no-reset"

tmux -S "$TMUX_SOCK" split-window -h -t "$SESSION:0" -c "$HOST_PROJ" \
  "idf.py -C \"$HOST_PROJ\" -p \"$HOST_PORT\" monitor --no-reset"

tmux -S "$TMUX_SOCK" select-layout -t "$SESSION:0" even-horizontal
tmux -S "$TMUX_SOCK" set -t "$SESSION" remain-on-exit on

exec tmux -S "$TMUX_SOCK" attach -t "$SESSION"
