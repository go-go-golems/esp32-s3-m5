#!/usr/bin/env bash
set -euo pipefail

TICKET=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
EXP="$TICKET/scripts/experiments/EXP-20260715-012-factory-f2-manual-reset-density"
RAW="$EXP/raw-dynamic-f2.jsonl"
CONSOLE="$EXP/collector-console.log"
SESSION='esp50-f2-manual-reset'
MODE=check
CONFIRM=''

while (($#)); do
  case "$1" in
    --check) MODE=check; shift ;;
    --execute) MODE=execute; shift ;;
    --confirm) CONFIRM=${2:?missing confirmation}; shift 2 ;;
    *) echo "usage: $0 [--check|--execute --confirm ARM-F2-CAPTURE]" >&2; exit 2 ;;
  esac
done

[[ -e /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00 ]] || { echo 'PaperS3 port missing' >&2; exit 3; }
for path in "$RAW" "$CONSOLE"; do
  [[ ! -e "$path" ]] || { echo "refusing to replace evidence: $path" >&2; exit 4; }
done
if tmux has-session -t "$SESSION" 2>/dev/null; then
  echo "capture session already exists: $SESSION" >&2
  exit 4
fi

staged=no
[[ -f "$EXP/f2-stage-no-reset.log" ]] && staged=yes
printf 'mode=%s\nexperiment=%s\nraw=%s\nstaged=%s\n' "$MODE" "$(basename "$EXP")" "$RAW" "$staged"
if [[ "$MODE" == check ]]; then
  echo 'hardware_modified=no'
  exit 0
fi
[[ "$staged" == yes ]] || { echo 'F2 is not staged with no-reset flash' >&2; exit 5; }
[[ "$CONFIRM" == ARM-F2-CAPTURE ]] || { echo 'execute requires --confirm ARM-F2-CAPTURE' >&2; exit 5; }

# This opens PaperS3 only through script 29's read-only, non-modem-control fd.
# It sends no PaperS3 input. Operator Reset is the only boot trigger.
tmux new-session -d -s "$SESSION" \
  "'$TICKET/scripts/29-capture-synchronized-serial.py' --execute --dens-raw-stream --confirm ENABLE-DENS-RAW-STREAM --gain 2 --integration 0 --light-duty 128 --duration 75 --output '$RAW' >'$CONSOLE' 2>&1"

for _ in $(seq 1 100); do
  if [[ -f "$RAW" ]] && grep -q '"event":"densitometer_raw_stream_begin"' "$RAW"; then
    printf 'capture_armed=yes\nmanual_action=press_PaperS3_Reset_once_now\nhardware_modified=transient_densitometer_state_only\n'
    exit 0
  fi
  tmux has-session -t "$SESSION" 2>/dev/null || { echo 'collector session ended before arming' >&2; exit 6; }
  sleep 0.1
done
echo 'collector did not arm within 10 seconds' >&2
exit 6
