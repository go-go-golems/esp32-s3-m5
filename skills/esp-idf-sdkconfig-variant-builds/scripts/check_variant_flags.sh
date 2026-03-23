#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 2 ]; then
  echo "usage: $0 <build-dir> <KCONFIG_KEY> [<KCONFIG_KEY> ...]" >&2
  exit 2
fi

BUILD_DIR="$1"
shift

JSON_FILE="${BUILD_DIR}/config/sdkconfig.json"
CMAKE_FILE="${BUILD_DIR}/config/sdkconfig.cmake"
HEADER_FILE="${BUILD_DIR}/config/sdkconfig.h"

for path in "${JSON_FILE}" "${CMAKE_FILE}" "${HEADER_FILE}"; do
  if [ ! -f "${path}" ]; then
    echo "missing generated config file: ${path}" >&2
    exit 1
  fi
done

for raw_key in "$@"; do
  key="${raw_key#CONFIG_}"
  cmake_key="CONFIG_${key}"

  echo "== ${key} =="

  found=0

  if rg -n "\"${key}\"" "${JSON_FILE}" >/dev/null 2>&1; then
    found=1
    echo "-- sdkconfig.json"
    rg -n "\"${key}\"" "${JSON_FILE}"
  fi

  if rg -n "^set\\(${cmake_key}( |\\))" "${CMAKE_FILE}" >/dev/null 2>&1; then
    found=1
    echo "-- sdkconfig.cmake"
    rg -n "^set\\(${cmake_key}( |\\))" "${CMAKE_FILE}"
  fi

  if rg -n "^#define ${cmake_key}\\b" "${HEADER_FILE}" >/dev/null 2>&1; then
    found=1
    echo "-- sdkconfig.h"
    rg -n "^#define ${cmake_key}\\b" "${HEADER_FILE}"
  fi

  if [ "${found}" -eq 0 ]; then
    echo "key not found in generated config artifacts: ${key}" >&2
    exit 1
  fi

  echo
done
