#!/usr/bin/env bash
set -euo pipefail

# Firmware runner for tutorial 0019 (Cardputer BLE temp logger).
# - sources ESP-IDF 5.4.1 toolchain
# - selects a reasonable serial port (by-id preferred)
# - runs idf.py flash monitor (or custom ACTION)
#
# Usage:
#   bash tools/run_fw_flash_monitor.sh
#
# Optional env vars:
#   PORT=/dev/ttyACM0                         # override serial port
#   ACTION="flash monitor"                    # idf.py action(s)
#   IDF_EXPORT=/home/manuel/esp/esp-idf-5.4.1/export.sh

export DIRENV_DISABLE=1

PROJ_DIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0019-cardputer-ble-temp-logger"
IDF_EXPORT="${IDF_EXPORT:-/home/manuel/esp/esp-idf-5.4.1/export.sh}"
ACTION="${ACTION:-flash monitor}"

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

main() {
  local port
  port="$(pick_port)"

  if [[ ! -f "$IDF_EXPORT" ]]; then
    echo "ERROR: IDF export.sh not found at: $IDF_EXPORT" >&2
    return 2
  fi

  # shellcheck disable=SC1090
  source "$IDF_EXPORT"

  # Prefer running the IDF launcher via the exact python from IDF's venv, to avoid
  # PATH/direnv quirks in interactive shells (tmux panes).
  local idf_py="${IDF_PATH}/tools/idf.py"
  local py="${IDF_PYTHON_ENV_PATH:-}/bin/python"
  if [[ -z "${IDF_PYTHON_ENV_PATH:-}" || ! -x "$py" ]]; then
    py="$(command -v python || true)"
  fi
  if [[ -z "$py" ]]; then
    echo "ERROR: could not find python after sourcing ESP-IDF env" >&2
    return 2
  fi
  if [[ ! -f "$idf_py" ]]; then
    echo "ERROR: IDF tool not found: $idf_py" >&2
    return 2
  fi

  # If monitor is missing in the ESP-IDF python env, flash/monitor will fail with:
  #   No module named 'esp_idf_monitor'
  # Provide a crisp hint, but don't auto-fix (interactive env choice varies).
  if ! "$py" -c "import esp_idf_monitor" >/dev/null 2>&1; then
    echo "ERROR: Python module 'esp_idf_monitor' is missing in the active ESP-IDF python env." >&2
    echo "This breaks: idf.py monitor / idf.py flash monitor" >&2
    echo "" >&2
    echo "Suggested fix (run once in a shell):" >&2
    echo "  source '$IDF_EXPORT' && python -m pip install --upgrade esp-idf-monitor" >&2
    echo "" >&2
    echo "Or reinstall ESP-IDF tools/python env per the ESP-IDF get-started docs." >&2
    return 2
  fi

  cd "$PROJ_DIR"
  echo "PORT=$port"
  echo "ACTION=$ACTION"
  "$py" "$idf_py" -p "$port" $ACTION
}

main "$@"


