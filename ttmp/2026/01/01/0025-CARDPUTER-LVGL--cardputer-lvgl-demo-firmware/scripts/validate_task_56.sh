#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-/dev/ttyACM0}"

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../../../" && pwd)"
PROJECT="${REPO_ROOT}/0025-cardputer-lvgl-demo"

OUT_DIR="${2:-/tmp/0025-cardputer-lvgl-task-56}"
mkdir -p "${OUT_DIR}"

cd "${PROJECT}"

python3 tools/capture_screenshot_png_from_console.py "${PORT}" "${OUT_DIR}/menu.png" --timeout-s 60 --cmd $'menu\nscreenshot'
python3 tools/capture_screenshot_png_from_console.py "${PORT}" "${OUT_DIR}/files_root.png" --timeout-s 60 --cmd $'sdmount\nfiles\nscreenshot'
python3 tools/capture_screenshot_png_from_console.py "${PORT}" "${OUT_DIR}/saved_shot.png" --timeout-s 60 --cmd $'sdmount\nsaveshot\nfiles\nscreenshot'
python3 tools/capture_screenshot_png_from_console.py "${PORT}" "${OUT_DIR}/shots_dir.png" --timeout-s 60 --cmd $'sdmount\nfiles\nkeys down down down\nwaitms 250\nkeys enter\nwaitms 250\nscreenshot'

if command -v pinocchio >/dev/null 2>&1; then
  pinocchio code professional --images "${OUT_DIR}/menu.png,${OUT_DIR}/files_root.png,${OUT_DIR}/shots_dir.png" \
    "OCR all screenshots and summarize the visible UI text and any truncation/overlap you can see."
fi

echo "Wrote screenshots to ${OUT_DIR}"

