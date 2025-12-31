#!/usr/bin/env bash
set -euo pipefail

# Host-side runner for tutorial 0019 BLE client.
#
# Key gotcha: `ble_client.py` defines `--timeout` as a *global* option, which
# argparse requires to appear BEFORE the subcommand, e.g.:
#   python3 tools/ble_client.py --timeout 5 scan
# not:
#   python3 tools/ble_client.py scan --timeout 5
#
# This wrapper accepts either ordering and normalizes to the global-first form.
#
# Usage examples:
#   bash tools/run_host_ble_client.sh --timeout 5 scan
#   bash tools/run_host_ble_client.sh scan --timeout 5      # also OK (reordered)
#   bash tools/run_host_ble_client.sh run --name CP_TEMP_LOGGER --read --notify --duration 20
#
# Optional env vars:
#   PYTHON=/path/to/python

export DIRENV_DISABLE=1

PROJ_DIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0019-cardputer-ble-temp-logger"

pick_python() {
  if [[ -n "${PYTHON:-}" ]]; then
    echo "$PYTHON"
    return 0
  fi

  # Prefer system python3 if it already has bleak.
  if command -v python3 >/dev/null 2>&1 && python3 -c "import bleak" >/dev/null 2>&1; then
    echo "python3"
    return 0
  fi

  # Fallback to the ESP-IDF 5.4.1 venv python if present (often used in tmux panes).
  local idf_py="${HOME}/.espressif/python_env/idf5.4_py3.11_env/bin/python"
  if [[ -x "$idf_py" ]] && "$idf_py" -c "import bleak" >/dev/null 2>&1; then
    echo "$idf_py"
    return 0
  fi

  # Last resort.
  if command -v python3 >/dev/null 2>&1; then
    echo "python3"
    return 0
  fi

  echo "ERROR: could not find python3" >&2
  return 2
}

normalize_timeout_args() {
  # Normalize:
  #   <cmd> --timeout N ...  =>  --timeout N <cmd> ...
  # Only handles the "--timeout N" form (not --timeout=N).
  local -a in=("$@")
  local -a out=()
  local timeout_val=""
  local i=0
  while [[ $i -lt ${#in[@]} ]]; do
    if [[ "${in[$i]}" == "--timeout" && $((i+1)) -lt ${#in[@]} ]]; then
      timeout_val="${in[$((i+1))]}"
      i=$((i+2))
      continue
    fi
    out+=("${in[$i]}")
    i=$((i+1))
  done

  if [[ -n "$timeout_val" ]]; then
    printf '%s\0' "--timeout" "$timeout_val" "${out[@]}"
  else
    printf '%s\0' "${out[@]}"
  fi
}

main() {
  cd "$PROJ_DIR"

  local py
  py="$(pick_python)"

  if ! "$py" -c "import bleak" >/dev/null 2>&1; then
    echo "ERROR: python package 'bleak' is not installed for: $py" >&2
    echo "Install (system python):" >&2
    echo "  python3 -m pip install --user --upgrade bleak" >&2
    echo "" >&2
    echo "If using an ESP-IDF virtualenv (user-site disabled), install without --user:" >&2
    echo "  python3 -m pip install --upgrade bleak" >&2
    return 2
  fi

  # Reorder timeout args if needed.
  local -a norm=()
  while IFS= read -r -d '' x; do norm+=("$x"); done < <(normalize_timeout_args "$@")

  exec "$py" tools/ble_client.py "${norm[@]}"
}

main "$@"


