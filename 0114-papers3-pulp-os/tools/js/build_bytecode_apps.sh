#!/usr/bin/env bash
# Host authoring pipeline: builds pulpjsc against the vendored engine + the
# pulp stdlib (host word size), then compiles every app under tools/js/apps/
# into an embedded bytecode header in main/.
#
# Bytecode is atom-coupled to the stdlib: rerun this after ANY change to
# tools/js/pulp_stdlib.c or mqjs_stdlib_pulp.c (and regenerate the device
# headers too via gen_pulp_stdlib.sh).
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ENGINE_DIR="${ROOT_DIR}/components/mquickjs"
JS_DIR="${ROOT_DIR}/tools/js"
HOST_DIR="${JS_DIR}/host"
mkdir -p "${HOST_DIR}"

# 1. Host-word-size stdlib table + atom header from the same definition.
gcc -O2 -Wall -I"${ENGINE_DIR}" -I"${JS_DIR}" \
    "${JS_DIR}/pulp_stdlib.c" "${ENGINE_DIR}/mquickjs_build.c" \
    -lm -o "${HOST_DIR}/pulp_stdlib_gen_host"
"${HOST_DIR}/pulp_stdlib_gen_host" > "${HOST_DIR}/pulp_stdlib_host.h"
"${HOST_DIR}/pulp_stdlib_gen_host" -a > "${HOST_DIR}/mquickjs_atom.h"

# 2. pulpjsc: host compiler linked against the vendored engine. The engine
#    source is COPIED into the host dir: a quoted #include searches the
#    including file's own directory first, so compiling it in place would
#    pick up the device (-m32) atom header and silently mismatch the host
#    stdlib table (symptom: every keyword becomes a parse error).
cp "${ENGINE_DIR}/mquickjs.c" "${HOST_DIR}/mquickjs.c"
gcc -O2 -Wall -I"${HOST_DIR}" -I"${ENGINE_DIR}" \
    "${JS_DIR}/pulpjsc.c" "${HOST_DIR}/mquickjs.c" \
    "${ENGINE_DIR}/cutils.c" "${ENGINE_DIR}/dtoa.c" \
    "${ENGINE_DIR}/libm.c" \
    -lm -o "${HOST_DIR}/pulpjsc"

# 3. Compile every app to an embedded header.
for app in "${JS_DIR}"/apps/*.js; do
    name="$(basename "${app}" .js)"
    "${HOST_DIR}/pulpjsc" "${app}" "${ROOT_DIR}/main/js_${name}.h" \
        "kJsBytecode_${name}"
done
echo "bytecode apps written to main/"
