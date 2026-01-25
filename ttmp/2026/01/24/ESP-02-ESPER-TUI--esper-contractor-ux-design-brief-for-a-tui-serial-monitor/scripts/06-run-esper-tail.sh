#!/usr/bin/env bash
set -euo pipefail

ESPPORT="${ESPPORT:-}"
ESPER_ELF="${ESPER_ELF:-$PWD/esper/firmware/esp32s3-test/build/esp32s3-test.elf}"
TOOLCHAIN_PREFIX="${TOOLCHAIN_PREFIX:-xtensa-esp32s3-elf-}"
BAUD="${BAUD:-115200}"
TIMEOUT="${TIMEOUT:-5s}"

if [[ -z "${ESPPORT}" ]]; then
  ESPPORT="$(./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/01-detect-port.sh)"
fi

cd esper

exec go run ./cmd/esper tail \
  --port "${ESPPORT}" \
  --baud "${BAUD}" \
  --timeout "${TIMEOUT}" \
  --timestamps \
  --prefix-port \
  --elf "${ESPER_ELF}" \
  --toolchain-prefix "${TOOLCHAIN_PREFIX}"

