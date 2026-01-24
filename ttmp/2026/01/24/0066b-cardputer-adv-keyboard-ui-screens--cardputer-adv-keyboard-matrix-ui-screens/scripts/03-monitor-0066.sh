#!/usr/bin/env bash
set -euo pipefail

PORT="${PORT:-/dev/ttyACM0}"

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
PROJ_DIR="$ROOT_DIR/0066-cardputer-adv-ledchain-gfx-sim"

echo "NOTE: idf.py monitor needs a real TTY. Run this script in a terminal (or tmux pane)."
exec idf.py -C "$PROJ_DIR" -p "$PORT" monitor

