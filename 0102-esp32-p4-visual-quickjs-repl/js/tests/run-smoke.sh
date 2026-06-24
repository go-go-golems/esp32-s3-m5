#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
JS_DIR="$ROOT/0102-esp32-p4-visual-quickjs-repl/js"

if command -v qjs >/dev/null 2>&1; then
  QJS=qjs
else
  QJS="$ROOT/0100-esp32-p4-quickjs-wasm/wasm-src/quickjs/qjs"
  if [[ ! -x "$QJS" ]]; then
    echo "qjs not found. Either install qjs or build vendored QuickJS:" >&2
    echo "  make -C $ROOT/0100-esp32-p4-quickjs-wasm/wasm-src/quickjs qjs" >&2
    exit 1
  fi
fi

"$QJS" "$JS_DIR/host-shim.js" "$JS_DIR/examples/smoke.js"
