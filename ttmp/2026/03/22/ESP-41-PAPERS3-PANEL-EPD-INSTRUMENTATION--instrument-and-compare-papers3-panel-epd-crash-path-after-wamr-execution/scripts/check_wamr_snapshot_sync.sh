#!/usr/bin/env bash
set -euo pipefail

ROOT="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5"
LIVE_BASE="$ROOT/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime"
SNAP_BASE="$ROOT/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/wamr-local-debug-snapshots"

compare_file() {
  local live="$1"
  local snap="$2"

  if diff -u "$live" "$snap"; then
    printf 'snapshot-sync ok %s\n' "$snap"
  else
    printf 'snapshot-sync mismatch %s\n' "$snap" >&2
    return 1
  fi
}

compare_file \
  "$LIVE_BASE/core/iwasm/common/wasm_runtime_common.c" \
  "$SNAP_BASE/wasm_runtime_common.c"

compare_file \
  "$LIVE_BASE/core/iwasm/interpreter/wasm_runtime.c" \
  "$SNAP_BASE/wasm_runtime.c"

compare_file \
  "$LIVE_BASE/core/iwasm/common/wasm_memory.c" \
  "$SNAP_BASE/wasm_memory.c"
