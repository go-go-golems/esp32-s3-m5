#!/usr/bin/env bash
set -euo pipefail

SESSION="${SESSION:-mled-server-e2e}"
HTTP_ADDR="${HTTP_ADDR:-localhost:18765}"
DATA_DIR="${DATA_DIR:-/tmp/mled-server-e2e-var}"
LOG_LEVEL="${LOG_LEVEL:-info}"
MCAST_PORT="${MCAST_PORT:-14626}"

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"

cleanup() {
  if [[ "${KEEP_RUNNING:-0}" == "1" ]]; then
    echo "KEEP_RUNNING=1; leaving tmux session running: $SESSION"
    return
  fi
  SESSION="$SESSION" "$SCRIPT_DIR/tmux-stop-mled-server.sh" >/dev/null 2>&1 || true
}
trap cleanup EXIT

SESSION="$SESSION" HTTP_ADDR="$HTTP_ADDR" DATA_DIR="$DATA_DIR" LOG_LEVEL="$LOG_LEVEL" MCAST_PORT="$MCAST_PORT" "$SCRIPT_DIR/tmux-run-mled-server.sh"

wait_for_server() {
  local url="http://${HTTP_ADDR}/api/status"
  local i
  for i in {1..60}; do
    if curl -fsS "$url" --max-time 1 >/dev/null 2>&1; then
      return 0
    fi
    sleep 0.5
  done
  echo "Server did not become ready: $url" >&2
  return 1
}

wait_for_server

SERVER_URL="http://${HTTP_ADDR}" "$SCRIPT_DIR/e2e-rest-verbs.sh"
