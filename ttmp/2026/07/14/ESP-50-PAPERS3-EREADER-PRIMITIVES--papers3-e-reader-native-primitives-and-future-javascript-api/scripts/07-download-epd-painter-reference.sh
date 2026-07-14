#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
OWNER_REPO='tonywestonuk/EPD_Painter'
COMMIT='753c521da8aef59756df07c1a4eb88f1c64c8227'
DEST="$ROOT/sources/code/epd-painter-$COMMIT"
mkdir -p "$DEST/src" "$DEST/tools" "$DEST/examples/adafruit/page_text"

files=(
  README.md
  How_It_Works.md
  reference_manual.md
  src/EPD_Painter.cpp
  src/EPD_Painter.h
  src/EPD_Painter_presets.h
  src/epd_painter_powerctl.cpp
  tools/waveform_calibrator.html
  examples/adafruit/page_text/page_text.ino
)

for path in "${files[@]}"; do
  destination="$DEST/$path"
  mkdir -p "$(dirname "$destination")"
  curl --fail --location --silent --show-error \
    "https://raw.githubusercontent.com/$OWNER_REPO/$COMMIT/$path" \
    --output "$destination"
done

# Preserve source semantics while keeping the ticket corpus clean for git diff --check.
find "$DEST" -type f ! -name MANIFEST.txt -print0 | xargs -0 perl -pi -e 's/[ \t]+$//'

{
  echo "repository=https://github.com/$OWNER_REPO"
  echo "commit=$COMMIT"
  echo "retrieved_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo
  sha256sum "${files[@]/#/$DEST/}"
} > "$DEST/MANIFEST.txt"

printf '%s\n' "$DEST"
