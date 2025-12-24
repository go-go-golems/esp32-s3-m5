#!/usr/bin/env bash
set -euo pipefail

# Create a 128x128 square GIF by center-cropping (no padding) and limiting to N frames.
#
# This is useful when a GIF has hundreds of frames and becomes too large for flash storage.
#
# Usage:
#   ./trim_gif_128x128_frames.sh input.gif output.gif 30
#
# Notes:
# - Uses ImageMagick (IM6) `convert`/`identify` style tooling (available as /usr/bin/convert here).
# - We keep original per-frame delays for the selected frames (no re-timing).
# - We force infinite looping in the output (`-loop 0`).

IN_PATH="${1:?input gif required}"
OUT_PATH="${2:?output gif required}"
MAX_FRAMES="${3:-30}"

if [[ ! -f "$IN_PATH" ]]; then
  echo "ERROR: input not found: $IN_PATH" >&2
  exit 1
fi

if ! command -v convert >/dev/null 2>&1; then
  echo "ERROR: ImageMagick 'convert' not found on PATH" >&2
  exit 1
fi

if ! [[ "$MAX_FRAMES" =~ ^[0-9]+$ ]] || [[ "$MAX_FRAMES" -lt 1 ]]; then
  echo "ERROR: MAX_FRAMES must be a positive integer (got: $MAX_FRAMES)" >&2
  exit 1
fi

last=$((MAX_FRAMES - 1))
echo "trim_gif: in=$IN_PATH out=$OUT_PATH frames=0..$last"

# Select first N frames, then coalesce (expand partial frames), crop/resize to 128x128, optimize.
convert "${IN_PATH}[0-${last}]" \
  -coalesce \
  -resize 128x128^ \
  -gravity center -extent 128x128 \
  -layers Optimize \
  -loop 0 \
  "$OUT_PATH"

echo "wrote: $OUT_PATH"


