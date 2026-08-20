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

# 3. Concatenate the OS core + app sections into ONE image (ESP-55 P1:
#    mechanical split of pulp.js; the concatenation preserves the original
#    semantics — os/[0-3]* first, app sections, then launcher + boot last,
#    so the only executing statement, boot's home(), runs after every
#    definition; function declarations hoist across the whole file).
BUILD_DIR="${JS_DIR}/build"
mkdir -p "${BUILD_DIR}"
ALL="${BUILD_DIR}/pulp_all.js"
cat "${JS_DIR}"/os/[0-3]*.js > "${ALL}"
# Each app file is a bare descriptor expression (ESP-55 P2); the registry
# glue is generated here so the files stay in load()-ready form.
for app in "${JS_DIR}"/apps/*.js; do
    name="$(basename "${app}" .js)"
    {
        printf "APPS['%s'] =\n" "${name}"
        cat "${app}"
        printf ";\n"
    } >> "${ALL}"
done
cat "${JS_DIR}"/os/[4-9]*.js >> "${ALL}"
"${HOST_DIR}/pulpjsc" "${ALL}" "${ROOT_DIR}/main/js_pulp.h" \
    "kJsBytecode_pulp"
echo "bytecode image written to main/js_pulp.h"
