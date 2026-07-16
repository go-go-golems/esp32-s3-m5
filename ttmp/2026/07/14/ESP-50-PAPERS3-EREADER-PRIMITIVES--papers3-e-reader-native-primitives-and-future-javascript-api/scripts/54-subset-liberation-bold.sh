#!/usr/bin/env bash
# Reproducible Liberation Sans Bold subsetting for PULP OS display faces.
# Source: the system-installed Liberation fonts (SIL OFL 1.1 since v2.0),
# same pinned repertoire as PT Serif (scripts/53).
set -euo pipefail
TICKET=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
ROOT=$(git -C "$TICKET" rev-parse --show-toplevel)
DEST="$ROOT/0112-papers3-reader-primitives/components/s3paper_core/fonts"
SRC="/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf"
UNICODES="U+0020-007E,U+00A0-00FF,U+0400-045F,U+0490-0491,U+2010-2027,U+20AC,U+2116"
pyftsubset "$SRC" \
  --unicodes="$UNICODES" \
  --layout-features='kern' \
  --hinting \
  --output-file="$DEST/LibSansBoldUkr.ttf"
cp /usr/share/doc/fonts-liberation/copyright "$DEST/LiberationSans-LICENSE.txt" 2>/dev/null || \
cp /usr/share/doc/fonts-liberation2/copyright "$DEST/LiberationSans-LICENSE.txt" 2>/dev/null || \
echo "WARNING: copy the Liberation license manually"
ls -la "$DEST/LibSansBoldUkr.ttf"
sha256sum "$DEST/LibSansBoldUkr.ttf"
