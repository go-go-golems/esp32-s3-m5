#!/usr/bin/env bash
set -euo pipefail

SESSION="z2m-test"
WORKDIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/scripts/zigbee2mqtt-test"

if tmux has-session -t "${SESSION}" 2>/dev/null; then
  tmux send-keys -t "${SESSION}:0.0" C-c
  tmux send-keys -t "${SESSION}:0.1" C-c
  sleep 1
  tmux kill-session -t "${SESSION}"
  echo "Stopped tmux session ${SESSION}"
else
  echo "tmux session ${SESSION} not running"
fi

# Optional cleanup: stop containers if still running.
( cd "${WORKDIR}" && docker compose down )
