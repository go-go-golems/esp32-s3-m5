#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
set_repl_console.sh

Edits local (gitignored) `sdkconfig` to choose which REPL console transport to build with.

Usage:
  ./tools/set_repl_console.sh uart0
  ./tools/set_repl_console.sh usb-serial-jtag

Notes:
  - `sdkconfig` is gitignored in this repo; this script is safe to run locally.
  - After running, build/flash as usual (idf.py will reconfigure as needed).
EOF
}

if [[ $# -ne 1 ]]; then
  usage >&2
  exit 2
fi

MODE="$1"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SDKCONFIG="${PROJECT_DIR}/sdkconfig"

if [[ ! -f "${SDKCONFIG}" ]]; then
  echo "sdkconfig not found at ${SDKCONFIG} (run ./build.sh build once first)" >&2
  exit 1
fi

set_symbol() {
  local key="$1"
  local value="$2"
  if rg -q "^${key}=" "${SDKCONFIG}"; then
    sed -i "s/^${key}=.*/${key}=${value}/" "${SDKCONFIG}"
  else
    printf "%s=%s\n" "${key}" "${value}" >>"${SDKCONFIG}"
  fi
}

unset_symbol() {
  local key="$1"
  sed -i "s/^${key}=y/# ${key} is not set/" "${SDKCONFIG}"
}

case "${MODE}" in
  uart0)
    set_symbol "CONFIG_MQJS_REPL_CONSOLE_UART0" "y"
    unset_symbol "CONFIG_MQJS_REPL_CONSOLE_USB_SERIAL_JTAG"
    ;;
  usb-serial-jtag)
    set_symbol "CONFIG_MQJS_REPL_CONSOLE_USB_SERIAL_JTAG" "y"
    unset_symbol "CONFIG_MQJS_REPL_CONSOLE_UART0"
    ;;
  *)
    echo "Unknown mode: ${MODE}" >&2
    usage >&2
    exit 2
    ;;
esac

echo "Updated sdkconfig for REPL console: ${MODE}" >&2

