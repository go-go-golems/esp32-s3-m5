#!/usr/bin/env bash
# Builds and runs the ESP-55 host eval harness against the firmware's
# vendored engine + host stdlib (tools/js/host must exist: run
# tools/js/build_bytecode_apps.sh first). Args are passed to the harness.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT=/home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0114-papers3-pulp-os
ENGINE="$ROOT/components/mquickjs"; JS="$ROOT/tools/js"; HOST="$JS/host"
OUT="${HARNESS_OUT:-/tmp/claude-1000/-home-manuel-code-wesen-go-go-golems-esp32-s3-m5/6356eb91-e077-4a86-a666-db446c46efc0/scratchpad}"
mkdir -p "$OUT"
gcc -O2 -w -I"$HOST" -I"$ENGINE" -I"$JS" "$HERE/02-host-eval-harness.c" \
    "$HOST/mquickjs.c" "$ENGINE/cutils.c" "$ENGINE/dtoa.c" "$ENGINE/libm.c" \
    -lm -o "$OUT/host-eval-harness"
exec "$OUT/host-eval-harness" "$@"
