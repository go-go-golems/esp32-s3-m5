#!/usr/bin/env bash
# Start the PicoCalc serial crash logger in tmux and write logs to a file.
#
# Note: this uses `tmux -L repro` intentionally. In this environment an older
# default tmux server was observed without the current /dev/serial/by-id view,
# which caused false "No such file or directory" errors for the by-id port.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SESSION="${SESSION:-picocalc-0102-physlog}"
LOG="${LOG:-/tmp/picoos_physical_crash.log}"
PORT="${PORT:-/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00}"
TMUX_SOCKET="${TMUX_SOCKET:-repro}"

if [[ ! -e "$PORT" ]]; then
  echo "Port not found: $PORT" >&2
  echo "Visible by-id ports:" >&2
  ls -l /dev/serial/by-id >&2 || true
  exit 1
fi

if command -v lsof >/dev/null 2>&1; then
  lsof "$PORT" || true
fi

tmux -L "$TMUX_SOCKET" kill-session -t "$SESSION" 2>/dev/null || true
tmux -L "$TMUX_SOCKET" new-session -d -s "$SESSION" \
  "\"$SCRIPT_DIR/07-serial-crash-logger.py\" --port \"$PORT\" --wake 2>&1 | tee -a \"$LOG\""

echo "Started tmux logger."
echo "  tmux: tmux -L $TMUX_SOCKET attach -t $SESSION"
echo "  log:  $LOG"
