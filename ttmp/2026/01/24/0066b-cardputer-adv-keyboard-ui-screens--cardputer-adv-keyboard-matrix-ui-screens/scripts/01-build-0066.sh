#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
PROJ_DIR="$ROOT_DIR/0066-cardputer-adv-ledchain-gfx-sim"

exec idf.py -C "$PROJ_DIR" build

