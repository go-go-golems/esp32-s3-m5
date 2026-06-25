#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
JS_DIR="$ROOT/0102-esp32-p4-visual-quickjs-repl/js"
HOST_DIR="$JS_DIR/tools/native-host"

make -C "$HOST_DIR" all >/dev/null
export PICO_JS_DIR="$JS_DIR"
exec "$HOST_DIR/build/picojs-host" "${1:-hello-native}"
