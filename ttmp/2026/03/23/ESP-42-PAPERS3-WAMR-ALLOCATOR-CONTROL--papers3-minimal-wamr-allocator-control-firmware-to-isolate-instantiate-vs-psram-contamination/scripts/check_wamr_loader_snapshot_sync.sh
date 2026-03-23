#!/usr/bin/env bash
set -euo pipefail

ROOT="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5"
LIVE_BASE="$ROOT/0082-papers3-wamr-allocator-control/managed_components/espressif__wasm-micro-runtime"
SNAP_BASE="$ROOT/ttmp/2026/03/23/ESP-42-PAPERS3-WAMR-ALLOCATOR-CONTROL--papers3-minimal-wamr-allocator-control-firmware-to-isolate-instantiate-vs-psram-contamination/scripts/wamr-local-debug-snapshots"

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
  "$LIVE_BASE/core/iwasm/interpreter/wasm_loader.c" \
  "$SNAP_BASE/wasm_loader.c"
