#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GENERATOR="${ROOT_DIR}/tools/esp_stdlib_gen/esp_stdlib_gen"
OUT_FILE="${ROOT_DIR}/main/esp32_stdlib.h"
ATOM_FILE="${ROOT_DIR}/components/mquickjs/mquickjs_atom.h"

if [[ ! -x "${GENERATOR}" ]]; then
  echo "error: missing generator binary: ${GENERATOR}" >&2
  exit 1
fi

tmp_file="$(mktemp)"
trap 'rm -f "${tmp_file}"' EXIT

"${GENERATOR}" -m32 >"${tmp_file}"

if ! rg -q 'static const uint32_t' "${tmp_file}"; then
  echo "error: generator output doesn't look like a 32-bit stdlib header" >&2
  exit 1
fi

mv "${tmp_file}" "${OUT_FILE}"
trap - EXIT
echo "wrote ${OUT_FILE}"

tmp_atom_file="$(mktemp)"
trap 'rm -f "${tmp_atom_file}"' EXIT

"${GENERATOR}" -m32 -a >"${tmp_atom_file}"

if ! rg -q '^#define JS_ATOM_var ' "${tmp_atom_file}"; then
  echo "error: generator output doesn't look like an atom header (missing JS_ATOM_var)" >&2
  exit 1
fi

mv "${tmp_atom_file}" "${ATOM_FILE}"
trap - EXIT
echo "wrote ${ATOM_FILE}"
