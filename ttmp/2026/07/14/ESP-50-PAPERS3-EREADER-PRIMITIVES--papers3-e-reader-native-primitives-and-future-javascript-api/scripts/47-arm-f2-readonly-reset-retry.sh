#!/usr/bin/env bash
set -euo pipefail

TICKET=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
EXP="$TICKET/scripts/experiments/EXP-20260715-015-factory-f2-readonly-reconnect-ring-retry"
RAW="$EXP/read-only-reset-capture.jsonl"
CONSOLE="$EXP/capture-console.log"
SESSION='esp50-f2-readonly-reset-retry'
MODE=check
CONFIRM=''
while (($#)); do
  case "$1" in
    --check) MODE=check; shift ;;
    --execute) MODE=execute; shift ;;
    --confirm) CONFIRM=${2:?missing confirmation}; shift 2 ;;
    *) echo "usage: $0 [--check|--execute --confirm ARM-F2-READONLY-RESET-RETRY]" >&2; exit 2 ;;
  esac
done
for path in "$RAW" "$CONSOLE"; do [[ ! -e "$path" ]] || { echo "refusing to replace evidence: $path" >&2; exit 3; }; done
if tmux has-session -t "$SESSION" 2>/dev/null; then echo "capture session already exists: $SESSION" >&2; exit 3; fi
printf 'mode=%s\nexperiment=%s\nraw=%s\n' "$MODE" "$(basename "$EXP")" "$RAW"
if [[ "$MODE" == check ]]; then echo 'hardware_modified=no'; exit 0; fi
[[ "$CONFIRM" == ARM-F2-READONLY-RESET-RETRY ]] || { echo 'execute requires --confirm ARM-F2-READONLY-RESET-RETRY' >&2; exit 4; }
tmux new-session -d -s "$SESSION" \
  "'$TICKET/scripts/45-capture-papers3-readonly-reconnect.py' --execute --confirm CAPTURE-F2-READONLY-RESET --duration 180 --output '$RAW' >'$CONSOLE' 2>&1"
for _ in $(seq 1 100); do
  if [[ -f "$RAW" ]] && grep -q '"event":"source_open"' "$RAW"; then
    printf 'capture_armed=yes\nmanual_action=press_PaperS3_Reset_once_now\nhardware_modified=no\n'
    exit 0
  fi
  tmux has-session -t "$SESSION" 2>/dev/null || { echo 'observer exited before arming' >&2; exit 5; }
  sleep 0.1
done
echo 'observer did not arm within ten seconds' >&2; exit 5
