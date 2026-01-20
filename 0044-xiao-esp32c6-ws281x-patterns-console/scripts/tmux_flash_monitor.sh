#!/usr/bin/env bash
set -euo pipefail

PORT=${1:-/dev/ttyACM0}
SESSION=${SESSION:-mo032-0044}

PROJECT_DIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console"
IDF_EXPORT="${IDF_EXPORT:-$HOME/esp/esp-idf-5.4.1/export.sh}"

if ! command -v tmux >/dev/null 2>&1; then
  echo "tmux not found" >&2
  exit 1
fi

if tmux has-session -t "$SESSION" 2>/dev/null; then
  echo "tmux session already exists: $SESSION" >&2
  echo "attach with: tmux attach -t $SESSION" >&2
  exit 2
fi

tmux new-session -d -s "$SESSION" -c "$PROJECT_DIR" "bash -lc 'source \"$IDF_EXPORT\" && idf.py -p \"$PORT\" flash monitor'"

tmux split-window -h -t "$SESSION" -c "$PROJECT_DIR" "bash -lc 'echo \"Paste commands from scripts/led_smoke_commands.txt\"; echo; sed -n \"1,200p\" scripts/led_smoke_commands.txt; echo; echo \"Tip: to enable periodic logs: led log on\"; exec bash'"

tmux select-pane -t "$SESSION":0.0

echo "Started tmux session: $SESSION"
echo "- Left pane: idf.py flash monitor on $PORT"
echo "- Right pane: smoke command list"
echo "Attach: tmux attach -t $SESSION"
