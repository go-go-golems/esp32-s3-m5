#!/usr/bin/env bash
set -euo pipefail

TICKET_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
REPO_ROOT=$(git -C "$TICKET_ROOT" rev-parse --show-toplevel)
PROJECT="$REPO_ROOT/0107-papers3-epd-painter-control"
IDF_ROOT='/home/manuel/esp/esp-idf-5.4.2'
BUILD_DIR="$PROJECT/build-ticket"
SDKCONFIG="$PROJECT/sdkconfig.ticket"
DEFAULT_PORT='/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00'
DEFAULT_PANE='papers3-0106:0.0'
STATE="$TICKET_ROOT/scripts/output/16-monitor-current.env"
ACTIONS="$TICKET_ROOT/scripts/output/16-command-actions.log"
MODE=${1:-status}
shift || true
PORT="$DEFAULT_PORT"
PANE="$DEFAULT_PANE"

normalize_monitor_log() {
  local log=$1
  [[ -f "$log" ]] || return 0
  python3 - "$log" <<'PY'
import re
import sys
from pathlib import Path

path = Path(sys.argv[1])
data = path.read_bytes().replace(b"\r", b"").replace(b"\x00", b"")
data = re.sub(rb"\x1b\[[0-?]*[ -/]*[@-~]", b"", data)
lines = [line.rstrip(b" \t") for line in data.split(b"\n")]
path.write_bytes(b"\n".join(lines).rstrip(b"\n") + b"\n")
PY
}

while (($#)); do
  case "$1" in
    --port) PORT=${2:?missing port}; shift 2 ;;
    --pane) PANE=${2:?missing pane}; shift 2 ;;
    --command) COMMAND=${2:?missing command}; shift 2 ;;
    *) echo "error: unknown argument $1" >&2; exit 2 ;;
  esac
done

tmux display-message -p -t "$PANE" '#{session_name}:#{window_index}.#{pane_index}' >/dev/null
mkdir -p "$TICKET_ROOT/scripts/output"

case "$MODE" in
  start)
    REAL_PORT=$(readlink -f "$PORT")
    owners=$(lsof -t "$PORT" "$REAL_PORT" 2>/dev/null | sort -u || true)
    [[ -z "$owners" ]] || { echo "error: serial port owned by $owners" >&2; exit 3; }
    STAMP=$(date -u +%Y%m%dT%H%M%SZ)
    LOG="$TICKET_ROOT/scripts/output/16-epd-control-monitor-$STAMP.log"
    printf 'PANE=%q\nPORT=%q\nLOG=%q\nSTARTED_UTC=%q\n' "$PANE" "$PORT" "$LOG" "$STAMP" > "$STATE"
    tmux pipe-pane -t "$PANE" "cat >> '$LOG'"
    tmux send-keys -t "$PANE" C-c
    tmux send-keys -t "$PANE" -l "cd '$PROJECT' && source '$IDF_ROOT/export.sh' >/dev/null 2>&1 && idf.py -B '$BUILD_DIR' -D SDKCONFIG='$SDKCONFIG' -p '$PORT' monitor"
    tmux send-keys -t "$PANE" Enter
    printf '%s action=start pane=%s port=%s log=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$PANE" "$PORT" "$LOG" >> "$ACTIONS"
    echo "monitor_log=$LOG"
    ;;
  send)
    [[ -f "$STATE" ]] || { echo "error: monitor state missing" >&2; exit 2; }
    # shellcheck disable=SC1090
    source "$STATE"
    : "${COMMAND:?send requires --command TEXT}"
    case "$COMMAND" in
      'epd status'|'epd heap'|'epd cleanup CONFIRM'|'epd target full white'|'epd target full black'|'epd target area 1'|'epd target area 10'|'epd target area 25'|'epd target area 50'|'epd target area 100'|'epd target checker a'|'epd target checker b'|'epd target page'|'epd wait') ;;
      *) echo "error: command is outside the audited grammar: $COMMAND" >&2; exit 4 ;;
    esac
    printf '%s action=send pane=%s command=%q\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$PANE" "$COMMAND" >> "$ACTIONS"
    tmux send-keys -t "$PANE" -l "$COMMAND"
    tmux send-keys -t "$PANE" Enter
    echo "sent=$COMMAND"
    ;;
  stop)
    [[ -f "$STATE" ]] && source "$STATE"
    printf '%s action=stop pane=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$PANE" >> "$ACTIONS"
    tmux send-keys -t "$PANE" C-]
    sleep 1
    tmux pipe-pane -t "$PANE"
    normalize_monitor_log "$LOG"
    echo "stopped=$PANE"
    ;;
  status)
    tmux capture-pane -p -t "$PANE" -S -80
    ;;
  *)
    echo "Usage: $0 start|send|stop|status [--pane TARGET] [--port PATH] [--command TEXT]" >&2
    exit 2
    ;;
esac
