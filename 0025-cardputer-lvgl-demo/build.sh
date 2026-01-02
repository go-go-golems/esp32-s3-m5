#!/usr/bin/env bash
set -euo pipefail

# One-command build + flash helper for the Cardputer LVGL demo.
#
# Usage:
#   ./build.sh                          # == idf.py build
#   ./build.sh flash                    # == idf.py flash
#   ./build.sh -p /dev/ttyACM1 flash monitor
#
# tmux helper (recommended):
#   ./build.sh tmux-flash-monitor       # auto-picks PORT (by-id preferred)
#   PORT=/dev/ttyACM1 ./build.sh tmux-flash-monitor
#
# Override ESP-IDF export path if needed:
#   IDF_EXPORT_SH=/path/to/esp-idf/export.sh ./build.sh

export DIRENV_DISABLE=1

REQUIRED_ESP_IDF_VERSION_PREFIX="${REQUIRED_ESP_IDF_VERSION_PREFIX:-5.4}"
IDF_EXPORT_SH="${IDF_EXPORT_SH:-$HOME/esp/esp-idf-5.4.1/export.sh}"
DEFAULT_IDF_TARGET="${DEFAULT_IDF_TARGET:-esp32s3}"

pick_port() {
  if [[ -n "${PORT:-}" ]]; then
    echo "$PORT"
    return 0
  fi

  local by_id
  by_id="$(ls -1 /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_* 2>/dev/null | head -1 || true)"
  if [[ -n "$by_id" ]]; then
    echo "$by_id"
    return 0
  fi

  local acm
  acm="$(ls -1 /dev/ttyACM* 2>/dev/null | head -1 || true)"
  if [[ -n "$acm" ]]; then
    echo "$acm"
    return 0
  fi

  echo "ERROR: no serial port found (set PORT=...)" >&2
  return 2
}

if [[ "${ESP_IDF_VERSION:-}" != ${REQUIRED_ESP_IDF_VERSION_PREFIX}* ]]; then
  if [[ ! -f "${IDF_EXPORT_SH}" ]]; then
    echo "ERROR: ESP-IDF export script not found at: ${IDF_EXPORT_SH}" >&2
    echo "Set IDF_EXPORT_SH to the correct path, e.g.:" >&2
    echo "  IDF_EXPORT_SH=$HOME/esp/esp-idf-5.4.1/export.sh ./build.sh" >&2
    exit 1
  fi
  # shellcheck disable=SC1090
  source "${IDF_EXPORT_SH}" >/dev/null
fi

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${PROJECT_DIR}"

idf_py="${IDF_PATH}/tools/idf.py"
py="${IDF_PYTHON_ENV_PATH:-}/bin/python"
if [[ -z "${IDF_PYTHON_ENV_PATH:-}" || ! -x "$py" ]]; then
  py="$(command -v python || true)"
fi
if [[ -z "$py" ]]; then
  echo "ERROR: could not find python after sourcing ESP-IDF env" >&2
  exit 2
fi
if [[ ! -f "$idf_py" ]]; then
  echo "ERROR: IDF tool not found: $idf_py" >&2
  exit 2
fi

if [[ $# -eq 0 ]]; then
  set -- build
fi

if [[ "${1:-}" == "tmux-flash-monitor" ]]; then
  shift
  if ! command -v tmux >/dev/null 2>&1; then
    echo "ERROR: tmux not found in PATH" >&2
    exit 1
  fi

  port="$(pick_port)"
  echo "PORT=$port"

  session="cardputer-lvgl-demo"
  tmux new-session -d -s "${session}" "'${py}' '${idf_py}' -p '${port}' $* flash monitor" || true
  tmux split-window -h -t "${session}" "bash -lc 'source \"${IDF_EXPORT_SH}\" >/dev/null; cd \"${PROJECT_DIR}\"; exec bash'"
  tmux select-layout -t "${session}" even-horizontal
  exec tmux attach -t "${session}"
fi

ensure_target() {
  if [[ -f "sdkconfig" ]]; then
    if grep -q "^CONFIG_IDF_TARGET_ESP32=y" sdkconfig && ! grep -q "^CONFIG_IDF_TARGET_ESP32S3=y" sdkconfig; then
      echo "NOTE: sdkconfig target is esp32; switching to ${DEFAULT_IDF_TARGET}" >&2
      idf.py set-target "${DEFAULT_IDF_TARGET}"
    fi
    return 0
  fi

  echo "NOTE: no sdkconfig yet; setting target to ${DEFAULT_IDF_TARGET}" >&2
  idf.py set-target "${DEFAULT_IDF_TARGET}"
}

case "${1:-}" in
  set-target|fullclean|help|--help|-h)
    ;;
  *)
    ensure_target
    ;;
esac

exec "$py" "$idf_py" "$@"

