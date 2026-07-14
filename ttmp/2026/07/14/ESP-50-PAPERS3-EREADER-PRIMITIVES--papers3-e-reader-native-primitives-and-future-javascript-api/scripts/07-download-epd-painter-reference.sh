#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
OWNER_REPO='tonywestonuk/EPD_Painter'
COMMIT='753c521da8aef59756df07c1a4eb88f1c64c8227'
DEST="$ROOT/sources/code/epd-painter-$COMMIT"
mkdir -p "$DEST/src" "$DEST/tools" "$DEST/examples/adafruit/page_text"

# Capture the complete build-relevant source directory, not only the files that
# were initially selected for reading. This keeps every audit reproducible from
# ticket-owned evidence and avoids temporary firmware/source checkouts.
files=(
  README.md
  How_It_Works.md
  reference_manual.md
  library.properties
  src/build_opt.h
  src/EPD_Painter.cpp
  src/EPD_Painter.h
  src/EPD_Painter.S
  src/EPD_Painter_Adafruit.h
  src/EPD_Painter_LVGL.h
  src/EPD_Painter_presets.h
  src/epd_painter_bootctl.cpp
  src/epd_painter_bootctl.h
  src/epd_painter_powerctl.cpp
  src/epd_painter_powerctl.h
  src/epd_pin_driver.cpp
  src/epd_pin_driver.h
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

(
  cd "$DEST"
  {
    echo "# repository=https://github.com/$OWNER_REPO"
    echo "# commit=$COMMIT"
    echo "# retrieved_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo
    sha256sum "${files[@]}"
  } > MANIFEST.txt
)

printf '%s\n' "$DEST"
