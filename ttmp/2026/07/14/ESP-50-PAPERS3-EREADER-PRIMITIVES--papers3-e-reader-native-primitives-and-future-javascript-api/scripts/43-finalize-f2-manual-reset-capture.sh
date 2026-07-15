#!/usr/bin/env bash
set -euo pipefail

TICKET=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
EXP="$TICKET/scripts/experiments/EXP-20260715-012-factory-f2-manual-reset-density"
RAW="$EXP/raw-dynamic-f2.jsonl"
CONSOLE="$EXP/collector-console.log"
SESSION='esp50-f2-manual-reset'

[[ -f "$RAW" && -f "$CONSOLE" ]] || { echo 'manual-reset capture was not armed' >&2; exit 3; }
if tmux has-session -t "$SESSION" 2>/dev/null; then
  echo 'capture still active; wait for the 75-second window before finalizing' >&2
  exit 4
fi
grep -q '"event":"capture_end"' "$RAW" || { echo 'capture_end missing' >&2; exit 5; }
grep -q '"result":"ok"' "$RAW" || { echo 'collector result not ok' >&2; exit 5; }
for path in "$EXP/firmware-transcript.log" "$EXP/ring.jsonl" "$EXP/ring-host-aligned.jsonl" "$EXP/ring-alignment.json" "$EXP/evidence.sha256"; do
  [[ ! -e "$path" ]] || { echo "refusing to replace evidence: $path" >&2; exit 6; }
done
"$TICKET/scripts/38-extract-factory-f2-ring.py" \
  --capture "$RAW" \
  --transcript "$EXP/firmware-transcript.log" \
  --ring "$EXP/ring.jsonl" \
  --aligned-ring "$EXP/ring-host-aligned.jsonl" \
  --alignment "$EXP/ring-alignment.json"
(cd "$EXP" && sha256sum raw-dynamic-f2.jsonl collector-console.log f2-stage-no-reset.log firmware-transcript.log ring.jsonl ring-host-aligned.jsonl ring-alignment.json > evidence.sha256 && sha256sum -c evidence.sha256)
printf 'result=ok\nevidence=%s/evidence.sha256\nhardware_modified=yes\n' "$EXP"
