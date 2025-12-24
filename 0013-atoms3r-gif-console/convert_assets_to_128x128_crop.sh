#!/usr/bin/env bash
set -euo pipefail

# Convert all GIFs in ./assets to square 128x128 GIFs by **cropping** (no padding).
#
# Output naming:
#   input.gif -> input_128x128.gif
#
# Strategy:
# - Prefer ImageMagick (`magick` or `convert`) because it preserves GIF frame delays/looping well.
# - Fallback to ffmpeg (uses palettegen/paletteuse; FPS is configurable).
#
# Usage:
#   ./convert_assets_to_128x128_crop.sh
#   ./convert_assets_to_128x128_crop.sh --dir ./assets --force
#   ./convert_assets_to_128x128_crop.sh --fps 15

ASSETS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/assets"
FORCE=0
FPS=15

while [[ $# -gt 0 ]]; do
  case "$1" in
    --dir)
      ASSETS_DIR="$2"
      shift 2
      ;;
    --force)
      FORCE=1
      shift
      ;;
    --fps)
      FPS="$2"
      shift 2
      ;;
    -h|--help)
      echo "usage: $0 [--dir <assets_dir>] [--force] [--fps <n>]"
      exit 0
      ;;
    *)
      echo "unknown arg: $1" >&2
      exit 2
      ;;
  esac
done

if [[ ! -d "$ASSETS_DIR" ]]; then
  echo "ERROR: assets dir not found: $ASSETS_DIR" >&2
  exit 1
fi

have_magick=0
if command -v magick >/dev/null 2>&1; then
  have_magick=1
fi
have_convert=0
if command -v convert >/dev/null 2>&1; then
  have_convert=1
fi
have_ffmpeg=0
if command -v ffmpeg >/dev/null 2>&1; then
  have_ffmpeg=1
fi

if [[ "$have_magick" -eq 0 && "$have_convert" -eq 0 && "$have_ffmpeg" -eq 0 ]]; then
  echo "ERROR: need ImageMagick (magick/convert) or ffmpeg on PATH" >&2
  exit 1
fi

echo "convert_assets: dir=$ASSETS_DIR force=$FORCE fps=$FPS"

shopt -s nullglob
inputs=("$ASSETS_DIR"/*.gif "$ASSETS_DIR"/*.GIF)
shopt -u nullglob

if [[ ${#inputs[@]} -eq 0 ]]; then
  echo "no GIFs found in $ASSETS_DIR"
  exit 0
fi

for in_path in "${inputs[@]}"; do
  base="${in_path%.*}"
  out_path="${base}_128x128.gif"

  if [[ "$FORCE" -eq 0 && -f "$out_path" ]]; then
    echo "skip (exists): $out_path"
    continue
  fi

  echo "convert: $(basename "$in_path") -> $(basename "$out_path")"

  if [[ "$have_magick" -eq 1 ]]; then
    # -coalesce: expand partial frames to full frames (important for resize/crop correctness)
    # -resize 128x128^: fill the box, preserving aspect ratio (will exceed in one dimension)
    # -extent 128x128: center-crop down to 128x128 (no padding because we filled first)
    # -layers Optimize: reduce output size
    magick "$in_path" \
      -coalesce \
      -resize 128x128^ \
      -gravity center -extent 128x128 \
      -layers Optimize \
      "$out_path"
    continue
  fi

  if [[ "$have_convert" -eq 1 ]]; then
    convert "$in_path" \
      -coalesce \
      -resize 128x128^ \
      -gravity center -extent 128x128 \
      -layers Optimize \
      "$out_path"
    continue
  fi

  # ffmpeg fallback (can lose original per-frame delays; uses a fixed FPS)
  # Use palettegen/paletteuse for reasonable quality.
  tmp_palette="$(mktemp)"
  ffmpeg -v error -y -i "$in_path" \
    -vf "fps=${FPS},scale=128:128:force_original_aspect_ratio=increase,crop=128:128,palettegen=reserve_transparent=1" \
    "$tmp_palette"
  ffmpeg -v error -y -i "$in_path" -i "$tmp_palette" \
    -lavfi "fps=${FPS},scale=128:128:force_original_aspect_ratio=increase,crop=128:128[x];[x][1:v]paletteuse=dither=bayer:bayer_scale=5:alpha_threshold=128" \
    -loop 0 \
    "$out_path"
  rm -f "$tmp_palette"
done

echo "done"


