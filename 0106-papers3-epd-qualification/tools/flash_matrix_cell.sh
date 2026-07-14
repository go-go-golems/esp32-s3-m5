#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 CELL /dev/serial/by-id/DEVICE" >&2
  exit 2
fi

CELL="${1^^}"
PORT="$2"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ROW="$(awk -F '\t' -v cell="$CELL" '$1 == cell { print; exit }' "$ROOT/matrix/cells.tsv")"
if [[ -z "$ROW" ]]; then
  echo "error: unknown matrix cell '$CELL'" >&2
  exit 2
fi
IFS=$'\t' read -r _ IDF_VERSION _ _ _ _ <<<"$ROW"
IDF_DIR="$HOME/esp/esp-idf-$IDF_VERSION"

if [[ ! -e "$PORT" ]]; then
  echo "error: serial device does not exist: $PORT" >&2
  exit 1
fi
if command -v fuser >/dev/null 2>&1 && fuser "$PORT" >/dev/null 2>&1; then
  echo "error: serial device already has an owner; stop monitor/probe processes first: $PORT" >&2
  fuser -v "$PORT" >&2 || true
  exit 1
fi

"$ROOT/tools/build_matrix_cell.sh" "$CELL"
# shellcheck disable=SC1090
source "$IDF_DIR/export.sh" >/dev/null
cd "$ROOT"
idf.py -B "$ROOT/build-cell-$CELL" -p "$PORT" flash

echo "Flashed cell $CELL. Do not start a second monitor while run_qualification.py owns $PORT."
