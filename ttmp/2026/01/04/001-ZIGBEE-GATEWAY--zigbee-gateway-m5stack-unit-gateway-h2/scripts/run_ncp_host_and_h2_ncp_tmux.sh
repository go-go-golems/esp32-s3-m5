#!/usr/bin/env bash
set -euo pipefail

TICKET_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${OUT_DIR:-$TICKET_DIR/various/logs}"
mkdir -p "$OUT_DIR"

SESSION="${SESSION:-001-zigbee-ncp}"
CORES3_PORT="${CORES3_PORT:-/dev/ttyACM0}"
H2_PORT="${H2_PORT:-/dev/ttyACM1}"
IDF_PATH="${IDF_PATH:-$HOME/esp/esp-5.4.1}"

HOST_DIR="${HOST_DIR:-$HOME/esp/esp-zigbee-sdk/examples/esp_zigbee_host}"
NCP_DIR="${NCP_DIR:-$HOME/esp/esp-zigbee-sdk/examples/esp_zigbee_ncp}"

DURATION_SECONDS="${DURATION_SECONDS:-}"

host_cmd="source \"$IDF_PATH/export.sh\" >/dev/null 2>&1; cd \"$HOST_DIR\"; idf.py -p \"$CORES3_PORT\" monitor"
ncp_cmd="source \"$IDF_PATH/export.sh\" >/dev/null 2>&1; cd \"$NCP_DIR\"; idf.py -p \"$H2_PORT\" monitor"

if tmux has-session -t "$SESSION" 2>/dev/null; then
  echo "tmux session already exists: $SESSION"
  echo "Kill it first: tmux kill-session -t $SESSION"
  exit 1
fi

tmux new-session -d -s "$SESSION" -n ncp
tmux split-window -h -t "$SESSION":0

tmux send-keys -t "$SESSION":0.0 "bash -lc $(printf '%q' "$host_cmd")" C-m
tmux send-keys -t "$SESSION":0.1 "bash -lc $(printf '%q' "$ncp_cmd")" C-m

tmux set-option -t "$SESSION" remain-on-exit on >/dev/null
tmux select-layout -t "$SESSION":0 even-horizontal >/dev/null

echo "Session: $SESSION"
echo "CoreS3 port: $CORES3_PORT (host)"
echo "H2 port: $H2_PORT (ncp)"
echo "Attach: tmux attach -t $SESSION"

if [[ -n "${DURATION_SECONDS}" ]]; then
  stamp="$(date -u +%Y%m%d-%H%M%S)"
  echo "Capturing for ${DURATION_SECONDS}s -> $OUT_DIR/${stamp}-*.log"
  sleep "$DURATION_SECONDS"

  tmux capture-pane -pt "$SESSION":0.0 -S -5000 > "$OUT_DIR/${stamp}-cores3-host.log" || true
  tmux capture-pane -pt "$SESSION":0.1 -S -5000 > "$OUT_DIR/${stamp}-h2-ncp.log" || true

  tmux kill-session -t "$SESSION" || true

  echo "Wrote:"
  echo "  $OUT_DIR/${stamp}-cores3-host.log"
  echo "  $OUT_DIR/${stamp}-h2-ncp.log"
fi

