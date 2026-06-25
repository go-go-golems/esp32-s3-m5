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
    echo "  git submodule update --init 0100-esp32-p4-quickjs-wasm/wasm-src/quickjs" >&2
    echo "  make -C $ROOT/0100-esp32-p4-quickjs-wasm/wasm-src/quickjs qjs" >&2
    exit 1
  fi
fi

tmp="$(mktemp)"
trap 'rm -f "$tmp"' EXIT

for name in hello-api calc; do
  "$JS_DIR/tests/bundle-example.sh" "$name" > "$tmp"
  out="$($QJS -I "$JS_DIR/host-shim.js" "$tmp")"
  case "$name" in
    hello-api) grep -q "picoOS DSL" <<<"$out" ;;
    calc) grep -q "1.41421356" <<<"$out" ;;
  esac
  echo "PASS bundle $name"
done
