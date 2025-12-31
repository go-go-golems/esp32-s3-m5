#!/usr/bin/env bash
set -euo pipefail

# One-command build helper for 0021 (CLINTS-MEMO-WEBSITE).
#
# - Sources ESP-IDF only when needed (based on ESP_IDF_VERSION).
# - Then runs `idf.py build` (or any idf.py command you pass).
#
# Usage:
#   ./build.sh                      # == idf.py build
#   ./build.sh flash                # == idf.py flash
#   ./build.sh -p /dev/ttyACM0 flash monitor
#
# tmux helper (optional):
#   ./build.sh tmux-flash-monitor -p /dev/ttyACM0
#
# Override ESP-IDF export path if needed:
#   IDF_EXPORT_SH=/path/to/esp-idf/export.sh ./build.sh

REQUIRED_ESP_IDF_VERSION_PREFIX="${REQUIRED_ESP_IDF_VERSION_PREFIX:-5.4}"
IDF_EXPORT_SH="${IDF_EXPORT_SH:-$HOME/esp/esp-idf-5.4.1/export.sh}"
DEFAULT_IDF_TARGET="${DEFAULT_IDF_TARGET:-esp32s3}"

if [[ "${ESP_IDF_VERSION:-}" != ${REQUIRED_ESP_IDF_VERSION_PREFIX}* ]]; then
  if [[ ! -f "${IDF_EXPORT_SH}" ]]; then
    echo "ERROR: ESP-IDF export script not found at: ${IDF_EXPORT_SH}" >&2
    echo "Set IDF_EXPORT_SH to the correct path, e.g.:" >&2
    echo "  IDF_EXPORT_SH=\$HOME/esp/esp-idf-5.4.1/export.sh ./build.sh" >&2
    exit 1
  fi
  # shellcheck disable=SC1090
  source "${IDF_EXPORT_SH}" >/dev/null
fi

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${PROJECT_DIR}"

if [[ $# -eq 0 ]]; then
  set -- build
fi

if [[ "${1:-}" == "tmux-flash-monitor" ]]; then
  shift
  if ! command -v tmux >/dev/null 2>&1; then
    echo "ERROR: tmux not found in PATH" >&2
    exit 1
  fi

  # Reuse the same idf.py args for both panes (e.g. -p /dev/ttyACM0 -b 1500000).
  # Left pane: flash; Right pane: monitor.
  session="atoms3-memo-0021"
  tmux new-session -d -s "${session}" "idf.py $* flash" || true
  tmux split-window -h -t "${session}" "idf.py $* monitor"
  tmux select-layout -t "${session}" even-horizontal
  exec tmux attach -t "${session}"
fi

ensure_target() {
  # If target isn't set (or is set to plain esp32), switch to esp32s3 by default.
  # This avoids surprising "IDF_TARGET not set, using default target: esp32".
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

# Avoid running set-target for commands that don't touch build config.
case "${1:-}" in
  set-target|fullclean|help|--help|-h)
    ;;
  *)
    ensure_target
    ;;
esac

exec idf.py "$@"


