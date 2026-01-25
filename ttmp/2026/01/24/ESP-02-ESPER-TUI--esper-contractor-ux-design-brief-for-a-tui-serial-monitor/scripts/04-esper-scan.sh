#!/usr/bin/env bash
set -euo pipefail

# By default this does NOT probe with esptool (probing can reset the device).
# Set PROBE=1 to enable probing.

PROBE="${PROBE:-0}"

cd esper

args=(scan --all)
if [[ "${PROBE}" == "1" ]]; then
  args+=(--probe-esptool)
fi

go run ./cmd/esper "${args[@]}"

