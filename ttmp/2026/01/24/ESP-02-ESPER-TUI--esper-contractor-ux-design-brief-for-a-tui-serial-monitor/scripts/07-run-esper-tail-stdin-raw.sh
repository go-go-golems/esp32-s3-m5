#!/usr/bin/env bash
set -euo pipefail

ESPPORT="${ESPPORT:-}"
BAUD="${BAUD:-115200}"

if [[ -z "${ESPPORT}" ]]; then
  ESPPORT="$(./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/01-detect-port.sh)"
fi

cd esper

echo "Starting esper tail (stdin raw). Ctrl-] exits." >&2
exec go run ./cmd/esper tail \
  --port "${ESPPORT}" \
  --baud "${BAUD}" \
  --stdin-raw

