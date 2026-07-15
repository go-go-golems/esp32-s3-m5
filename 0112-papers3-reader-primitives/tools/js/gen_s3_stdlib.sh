#!/usr/bin/env bash
# Regenerates the reader's JS stdlib table and atom header. The atom header
# is stdlib-specific: this project's engine component must not be shared.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ENGINE_DIR="${ROOT_DIR}/components/mquickjs"
GEN="${ROOT_DIR}/tools/js/s3_stdlib_gen"

gcc -O2 -Wall -I"${ENGINE_DIR}" -I"${ROOT_DIR}/tools/js" \
    "${ROOT_DIR}/tools/js/s3_stdlib.c" "${ENGINE_DIR}/mquickjs_build.c" \
    -lm -o "${GEN}"

"${GEN}" -m32 > "${ROOT_DIR}/main/js_stdlib.h"
"${GEN}" -m32 -a > "${ENGINE_DIR}/mquickjs_atom.h"
echo "wrote main/js_stdlib.h and components/mquickjs/mquickjs_atom.h"
