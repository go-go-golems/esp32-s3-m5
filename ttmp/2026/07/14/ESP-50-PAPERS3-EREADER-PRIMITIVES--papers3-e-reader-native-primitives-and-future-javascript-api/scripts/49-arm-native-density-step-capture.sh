#!/usr/bin/env bash
set -euo pipefail
TICKET=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
EXP="$TICKET/scripts/experiments/EXP-20260715-016-native-epd-density-step-response"
DENS="$EXP/density.jsonl"; FIRM="$EXP/firmware.jsonl"
[[ ${1:-} == --execute && ${2:-} == --confirm && ${3:-} == ARM-DENSITY-STEP ]] || { echo 'usage: --execute --confirm ARM-DENSITY-STEP' >&2; exit 2; }
[[ -f "$EXP/flash-no-reset.log" ]] || { echo 'stage F2-equivalent step image first' >&2; exit 3; }
for f in "$DENS" "$FIRM" "$EXP/density-console.log" "$EXP/firmware-console.log"; do [[ ! -e "$f" ]] || { echo "refusing to replace $f" >&2; exit 4; }; done
tmux has-session -t esp50-density-step-dens 2>/dev/null && { echo 'density capture already running' >&2; exit 5; }
tmux new-session -d -s esp50-density-step-dens "'$TICKET/scripts/29-capture-synchronized-serial.py' --execute --no-firmware --dens-raw-stream --confirm ENABLE-DENS-RAW-STREAM --gain 2 --integration 0 --light-duty 128 --duration 150 --output '$DENS' >'$EXP/density-console.log' 2>&1"
tmux new-session -d -s esp50-density-step-firm "'$TICKET/scripts/45-capture-papers3-readonly-reconnect.py' --execute --confirm CAPTURE-F2-READONLY-RESET --duration 150 --output '$FIRM' >'$EXP/firmware-console.log' 2>&1"
for _ in $(seq 1 100); do
  if [[ -f "$DENS" && -f "$FIRM" ]] && grep -q '"event":"source_open"' "$FIRM" && grep -q '"event":"densitometer_raw_stream_begin"' "$DENS"; then
    echo 'capture_armed=yes manual_action=press_PaperS3_Reset_once_now'
    exit 0
  fi
  sleep 0.1
done
echo 'capture arm failed' >&2; exit 6
