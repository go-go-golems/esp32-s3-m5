#!/usr/bin/env bash
# build-quickjs-wasm.sh — build quickjs.wasm (wasm32-wasip1 reactor) on the host PC.
# See design doc §5.2. Run from wasm-src/.
#
# Produces a reactor (library) module that exports qjs_init / qjs_eval and
# imports host_* from module "env" (satisfied at runtime by WAMR native symbols).
# quickjs-libc.c is intentionally NOT included (we provide our own `print`,
# and we want a small module with minimal WASI dependencies).
set -euo pipefail

: "${WASI_SDK_PATH:=/home/manuel/tools/wasi-sdk-33.0-x86_64-linux}"
HERE="$(pwd)"
QJS_DIR="$HERE/quickjs"
OUT_DIR="$HERE/../wasm-build"
mkdir -p "$OUT_DIR"

CLANG="$WASI_SDK_PATH/bin/clang"
QJS_VERSION="$(cat "$QJS_DIR/VERSION" 2>/dev/null || echo unknown)"

# Minimal QuickJS engine sources (no libbf — bignum was removed upstream;
# no quickjs-libc — avoids pulling in WASI file/socket stdlib we don't need).
QJS_SRCS=( quickjs.c cutils.c dtoa.c libregexp.c libunicode.c )

echo "Building quickjs.wasm (QuickJS $QJS_VERSION) with $CLANG ..."
"$CLANG" \
    --target=wasm32-wasip1 \
    -O2 \
    -std=gnu11 \
    -DCONFIG_VERSION="\"$QJS_VERSION\"" \
    -include "$HERE/wasm_shim.h" \
    -I"$HERE/wasm_overrides" -I"$HERE" -I"$QJS_DIR" \
    "${QJS_SRCS[@]/#/$QJS_DIR/}" \
    wasm_main.c \
    -o "$OUT_DIR/quickjs.wasm" \
    -Wl,--no-entry \
    -Wl,--export=qjs_init \
    -Wl,--export=qjs_eval \
    -Wl,--export=malloc \
    -Wl,--export=free \
    -Wl,--allow-undefined \
    -Wl,--export=__heap_base \
    -Wl,--export=__data_end \
    -Wl,--initial-memory=8388608 \
    -Wl,--max-memory=16777216

echo "Done: $OUT_DIR/quickjs.wasm ($(du -h "$OUT_DIR/quickjs.wasm" | cut -f1))"
echo "--- verify (Python wasm section parser) ---"
"$HERE/wasm_inspect.py" "$OUT_DIR/quickjs.wasm" || true
echo "Next: cp $OUT_DIR/quickjs.wasm ../main/quickjs.wasm"
