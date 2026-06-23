#!/usr/bin/env bash
# build-quickjs-wasm.sh — build quickjs.wasm (wasm32-wasi reactor) on the host PC.
# See design doc §5.2. Run from wasm-src/.
set -euo pipefail

: "${WASI_SDK_PATH:=/opt/wasi-sdk}"
QJS_DIR="$(pwd)/quickjs"
OUT_DIR="$(pwd)/../wasm-build"
mkdir -p "$OUT_DIR"

CLANG="$WASI_SDK_PATH/bin/clang"
QJS_SRCS=( quickjs.c libregexp.c libunicode.c cutils.c libbf.c )

echo "Building quickjs.wasm with $CLANG ..."
"$CLANG" \
    --target=wasm32-wasi \
    -O3 -flto \
    -I"$QJS_DIR" \
    "${QJS_SRCS[@]/#/$QJS_DIR/}" \
    wasm_main.c \
    -o "$OUT_DIR/quickjs.wasm" \
    -Wl,--no-entry \
    -Wl,--export=qjs_init \
    -Wl,--export=qjs_eval \
    -Wl,--allow-undefined \
    -Wl,--export=__heap_base

echo "Done: $OUT_DIR/quickjs.wasm"
echo "--- verify ---"
command -v wasm-objdump >/dev/null 2>&1 && {
    wasm-objdump -x "$OUT_DIR/quickjs.wasm" | grep -E 'Import|Export|qjs_(init|eval)|env\.host_|wasi_' || true
} || echo "(install wabt for wasm-objdump verification)"
echo "Next: cp $OUT_DIR/quickjs.wasm ../main/quickjs.wasm"
