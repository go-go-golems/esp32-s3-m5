#!/usr/bin/env bash
set -euo pipefail

SESSION="${SESSION:-mled-server-test}"
HTTP_ADDR="${HTTP_ADDR:-localhost:18765}"
DATA_DIR="${DATA_DIR:-/tmp/mled-server-test-var}"
LOG_LEVEL="${LOG_LEVEL:-info}"
MCAST_GROUP="${MCAST_GROUP:-}"
MCAST_PORT="${MCAST_PORT:-}"

REPO_ROOT="/home/manuel/workspaces/2025-12-21/echo-base-documentation"
MLED_SERVER_DIR="$REPO_ROOT/esp32-s3-m5/mled-server"

if ! command -v tmux >/dev/null 2>&1; then
  echo "tmux not found" >&2
  exit 1
fi

tmux kill-session -t "$SESSION" 2>/dev/null || true

EXTRA_ARGS=()
if [[ -n "${MCAST_GROUP}" ]]; then
  EXTRA_ARGS+=(--mcast-group "${MCAST_GROUP}")
fi
if [[ -n "${MCAST_PORT}" ]]; then
  EXTRA_ARGS+=(--mcast-port "${MCAST_PORT}")
fi

tmux new-session -d -s "$SESSION" \
  "cd \"$MLED_SERVER_DIR\" && go run ./cmd/mled-server serve --http-addr \"$HTTP_ADDR\" --data-dir \"$DATA_DIR\" --log-level \"$LOG_LEVEL\" ${EXTRA_ARGS[*]}"

echo "Started tmux session: $SESSION"
echo "HTTP_ADDR=$HTTP_ADDR"
echo "DATA_DIR=$DATA_DIR"
if [[ -n "${MCAST_GROUP}" ]]; then
  echo "MCAST_GROUP=$MCAST_GROUP"
fi
if [[ -n "${MCAST_PORT}" ]]; then
  echo "MCAST_PORT=$MCAST_PORT"
fi
echo "Tail logs: tmux attach -t $SESSION"
