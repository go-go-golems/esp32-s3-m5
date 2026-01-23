#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-/dev/ttyACM0}"
SESSION="${2:-m0066}"

PROJECT_DIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim"

if ! command -v tmux >/dev/null 2>&1; then
  echo "tmux not found" >&2
  exit 2
fi

if ! command -v idf.py >/dev/null 2>&1; then
  echo "idf.py not found in PATH" >&2
  exit 2
fi

tmux has-session -t "${SESSION}" 2>/dev/null && {
  echo "tmux session already exists: ${SESSION}" >&2
  echo "attach with: tmux attach -t ${SESSION}" >&2
  exit 1
}

tmux new-session -d -s "${SESSION}" -c "${PROJECT_DIR}" "idf.py -p \"${PORT}\" monitor"
tmux set-option -t "${SESSION}" remain-on-exit on
tmux display-message -t "${SESSION}" "0066 monitor on ${PORT}; attach: tmux attach -t ${SESSION}"

echo "started tmux session: ${SESSION}"
echo "attach: tmux attach -t ${SESSION}"

