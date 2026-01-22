#!/usr/bin/env bash
set -euo pipefail

SESSION="${SESSION:-mled-server-test}"

if ! command -v tmux >/dev/null 2>&1; then
  echo "tmux not found" >&2
  exit 1
fi

tmux kill-session -t "$SESSION" 2>/dev/null || true
echo "Stopped tmux session: $SESSION"
