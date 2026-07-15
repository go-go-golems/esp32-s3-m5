#!/usr/bin/env bash
# Regenerates the spike stdlib table and atom header from tools/spike_stdlib.c.
# The atom header lives in the PROJECT-LOCAL engine copy: atoms depend on the
# stdlib contents, so this component must never be shared with other stdlibs.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ENGINE_DIR="${ROOT_DIR}/components/mquickjs"
GEN="${ROOT_DIR}/tools/spike_stdlib_gen"

gcc -O2 -Wall -I"${ENGINE_DIR}" -I"${ROOT_DIR}/tools" \
    "${ROOT_DIR}/tools/spike_stdlib.c" "${ENGINE_DIR}/mquickjs_build.c" \
    -lm -o "${GEN}"

"${GEN}" -m32 > "${ROOT_DIR}/main/spike_stdlib.h"
"${GEN}" -m32 -a > "${ENGINE_DIR}/mquickjs_atom.h"
echo "wrote main/spike_stdlib.h and components/mquickjs/mquickjs_atom.h"
