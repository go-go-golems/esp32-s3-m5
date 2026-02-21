#!/usr/bin/env bash
set -euo pipefail

BASE_URL="${BASE_URL:-http://192.168.3.119}"
PROJECT_DIR="${PROJECT_DIR:-0067-esp-c3-led-matrix-http}"
EXAMPLES_DIR="$PROJECT_DIR/examples/js"

usage() {
  cat <<'EOF'
Usage:
  0067_play_js_example.sh <example-name-or-path>
  0067_play_js_example.sh --list

Examples:
  0067_play_js_example.sh 01-plasma-ribbon
  0067_play_js_example.sh 02-life-torus.js
  0067_play_js_example.sh 0067-esp-c3-led-matrix-http/examples/js/03-comet-trails.js
EOF
}

if [[ "${1:-}" == "--list" ]]; then
  ls -1 "$EXAMPLES_DIR"
  exit 0
fi

if [[ $# -lt 1 ]]; then
  usage
  exit 1
fi

arg="$1"
file=""

if [[ -f "$arg" ]]; then
  file="$arg"
elif [[ -f "$EXAMPLES_DIR/$arg" ]]; then
  file="$EXAMPLES_DIR/$arg"
elif [[ -f "$EXAMPLES_DIR/$arg.js" ]]; then
  file="$EXAMPLES_DIR/$arg.js"
else
  echo "example not found: $arg" >&2
  echo "run --list to view available examples" >&2
  exit 1
fi

echo "Playing: $file"
curl -sS -X POST "$BASE_URL/api/js/eval" --data-binary "@$file"
echo
