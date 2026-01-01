#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"

ROOT="${SCRIPT_DIR}"
while [[ "${ROOT}" != "/" && ! -d "${ROOT}/.git" ]]; do
  ROOT="$(dirname -- "${ROOT}")"
done
if [[ ! -d "${ROOT}/.git" ]]; then
  echo "error: could not find repo root (.git)" >&2
  exit 1
fi

PROJECT_DIR="${PROJECT_DIR:-${ROOT}/0020-cardputer-ble-keyboard-host}"
EXPORT_SH="${EXPORT_SH:-/home/manuel/esp/esp-idf-5.4.1/export.sh}"
PORT="${PORT:-/dev/ttyACM0}"
SESSION="${SESSION:-blekbd}"

bash -lc "source '${EXPORT_SH}' >/dev/null 2>&1 && idf.py -C '${PROJECT_DIR}' -p '${PORT}' flash"

tmux has-session -t "${SESSION}" 2>/dev/null && tmux kill-session -t "${SESSION}" || true
tmux new-session -d -s "${SESSION}" "bash -lc 'source \"${EXPORT_SH}\" >/dev/null 2>&1 && idf.py -C \"${PROJECT_DIR}\" -p \"${PORT}\" monitor'"

echo "tmux attach -t ${SESSION}"

