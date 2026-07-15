#!/usr/bin/env bash
set -euo pipefail

TICKET=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
EXP="$TICKET/scripts/experiments/EXP-20260715-011-factory-f2-ring-density"
RAW="$EXP/raw-dynamic-f2.jsonl"
HOST="$EXP/host-events.jsonl"
COLLECTOR_LOG="$EXP/collector-console.log"
FLASH_RUNNER_LOG="$EXP/flash-runner-console.log"
FLASH_FULL_LOG="$EXP/flash-full.log"
RING="$EXP/ring.jsonl"
ALIGNED_RING="$EXP/ring-host-aligned.jsonl"
ALIGNMENT="$EXP/ring-alignment.json"
TRANSCRIPT="$EXP/firmware-transcript.log"
MODE=check
CONFIRM=''

while (($#)); do
  case "$1" in
    --check) MODE=check; shift ;;
    --execute) MODE=execute; shift ;;
    --confirm) CONFIRM=${2:?missing confirmation}; shift 2 ;;
    *) echo "usage: $0 [--check|--execute --confirm RUN-DENS-F2]" >&2; exit 2 ;;
  esac
done

[[ -d "$EXP" ]] || { echo "missing experiment: $EXP" >&2; exit 3; }
(cd "$EXP" && sha256sum -c preregistration.sha256 >/dev/null)
"$TICKET/scripts/27-run-factory-comparison-flash.sh" --check --treatment f2 >/dev/null
"$TICKET/scripts/29-capture-synchronized-serial.py" \
  --check --dens-raw-stream \
  --confirm ENABLE-DENS-RAW-STREAM --gain 2 --integration 0 --light-duty 128 \
  --duration 60 --output "$RAW" >/dev/null

printf 'mode=%s\nexperiment=%s\nraw=%s\nhost_events=%s\n' "$MODE" "$(basename "$EXP")" "$RAW" "$HOST"
if [[ "$MODE" == check ]]; then
  echo 'hardware_modified=no'
  exit 0
fi
[[ "$CONFIRM" == RUN-DENS-F2 ]] || { echo 'execute requires --confirm RUN-DENS-F2' >&2; exit 4; }
for path in "$RAW" "$HOST" "$COLLECTOR_LOG" "$FLASH_RUNNER_LOG" "$FLASH_FULL_LOG" "$RING" "$ALIGNED_RING" "$ALIGNMENT" "$TRANSCRIPT"; do
  [[ ! -e "$path" ]] || { echo "refusing to replace evidence: $path" >&2; exit 5; }
done

collector_pid=''
cleanup() {
  if [[ -n "$collector_pid" ]] && kill -0 "$collector_pid" 2>/dev/null; then
    kill -INT "$collector_pid" 2>/dev/null || true
    wait "$collector_pid" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM

emit_host_event() {
  python3 - "$HOST" "$1" <<'PY'
import json, os, socket, sys, time
path, event = sys.argv[1:]
record = {
    "schema": "esp50.host-marker.v1",
    "event": event,
    "host_monotonic_ns": time.monotonic_ns(),
    "host_realtime_ns": time.time_ns(),
    "pid": os.getpid(),
    "hostname": socket.gethostname(),
}
with open(path, "a", encoding="utf-8") as stream:
    stream.write(json.dumps(record, sort_keys=True, separators=(",", ":")) + "\n")
    stream.flush()
    os.fsync(stream.fileno())
PY
}

emit_host_event orchestrator_begin
"$TICKET/scripts/29-capture-synchronized-serial.py" \
  --execute --dens-raw-stream \
  --confirm ENABLE-DENS-RAW-STREAM --gain 2 --integration 0 --light-duty 128 \
  --duration 60 --output "$RAW" >"$COLLECTOR_LOG" 2>&1 &
collector_pid=$!

for _ in $(seq 1 100); do
  if [[ -f "$RAW" ]] && grep -q '"event":"densitometer_raw_stream_begin"' "$RAW"; then break; fi
  kill -0 "$collector_pid" 2>/dev/null || { tail -n 50 "$COLLECTOR_LOG" >&2; exit 6; }
  sleep 0.1
done
grep -q '"event":"densitometer_raw_stream_begin"' "$RAW" || { echo 'raw stream did not start' >&2; exit 6; }
emit_host_event raw_stream_confirmed
sleep 2
emit_host_event flash_begin
"$TICKET/scripts/27-run-factory-comparison-flash.sh" \
  --execute --treatment f2 --confirm RUN-F2 >"$FLASH_RUNNER_LOG" 2>&1
flash_source=$(awk -F= '/^flash_log=/{print $2}' "$FLASH_RUNNER_LOG")
[[ -n "$flash_source" && -f "$flash_source" ]] || { echo 'full flash log path missing' >&2; exit 7; }
cp "$flash_source" "$FLASH_FULL_LOG"
emit_host_event flash_runner_complete
wait "$collector_pid"
collector_pid=''
"$TICKET/scripts/38-extract-factory-f2-ring.py" --capture "$RAW" --transcript "$TRANSCRIPT" --ring "$RING" --aligned-ring "$ALIGNED_RING" --alignment "$ALIGNMENT"
emit_host_event orchestrator_complete
trap - EXIT INT TERM

grep -q '"event":"capture_end"' "$RAW" || { echo 'capture_end missing' >&2; exit 7; }
grep -q '"result":"ok"' "$RAW" || { echo 'collector result not ok' >&2; exit 7; }
printf 'result=ok\nraw_sha256=%s\nhost_sha256=%s\nring_sha256=%s\nhardware_modified=yes\n' \
  "$(sha256sum "$RAW" | awk '{print $1}')" "$(sha256sum "$HOST" | awk '{print $1}')" "$(sha256sum "$RING" | awk '{print $1}')"
