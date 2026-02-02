#!/usr/bin/env bash
set -euo pipefail

SESSION="z2m-test"
WORKDIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/scripts/zigbee2mqtt-test"

if tmux has-session -t "${SESSION}" 2>/dev/null; then
  echo "tmux session ${SESSION} already exists"
  exit 0
fi

tmux new-session -d -s "${SESSION}" -c "${WORKDIR}"
tmux split-window -h -t "${SESSION}:0" -c "${WORKDIR}"

# Left pane: mosquitto
 tmux send-keys -t "${SESSION}:0.0" "docker compose up mosquitto" C-m

# Right pane: zigbee2mqtt
 tmux send-keys -t "${SESSION}:0.1" "docker compose up zigbee2mqtt" C-m

echo "Started tmux session ${SESSION} in ${WORKDIR}"
