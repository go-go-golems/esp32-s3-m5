#!/usr/bin/env bash
# Reproducible PT Serif subsetting for the s3paper reader (ticket 3r0u).
#
# Downloads PT Serif Regular (SIL OFL) from google/fonts, subsets it to the
# reader's pinned character repertoire (Latin + Ukrainian Cyrillic +
# punctuation), keeping kern/GPOS pair kerning, and installs the result plus
# the OFL license into 0112-papers3-reader-primitives/components/s3paper_core/fonts/.
set -euo pipefail

TICKET=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
ROOT=$(git -C "$TICKET" rev-parse --show-toplevel)
DEST="$ROOT/0112-papers3-reader-primitives/components/s3paper_core/fonts"
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

SRC_URL="https://github.com/google/fonts/raw/main/ofl/ptserif/PT_Serif-Web-Regular.ttf"
OFL_URL="https://github.com/google/fonts/raw/main/ofl/ptserif/OFL.txt"

# Pinned repertoire:
#  0020-007E  ASCII
#  00A0-00FF  Latin-1 supplement (accented Latin, guillemets, NBSP)
#  0400-045F  Cyrillic core (covers Ukrainian А-Я а-я, Єє U+0404/0454,
#             Іі U+0406/0456, Її U+0407/0457)
#  0490-0491  Ґґ (ghe with upturn)
#  2010-2027  hyphens, dashes, quotes incl. U+2019 apostrophe, ellipsis
#  20AC,2116  euro, numero sign
UNICODES="U+0020-007E,U+00A0-00FF,U+0400-045F,U+0490-0491,U+2010-2027,U+20AC,U+2116"

curl -sL -m 60 -o "$WORK/PTSerif-Regular.ttf" "$SRC_URL"
curl -sL -m 60 -o "$WORK/OFL.txt" "$OFL_URL"

pyftsubset "$WORK/PTSerif-Regular.ttf" \
    --output-file="$WORK/PTSerifUkr.ttf" \
    --unicodes="$UNICODES" \
    --layout-features='kern' \
    --legacy-kern \
    --name-IDs='0,1,2,13,14' \
    --drop-tables+=DSIG

mkdir -p "$DEST"
cp "$WORK/PTSerifUkr.ttf" "$DEST/PTSerifUkr.ttf"
cp "$WORK/OFL.txt" "$DEST/PTSerif-OFL.txt"

echo "installed: $DEST/PTSerifUkr.ttf ($(stat -c%s "$DEST/PTSerifUkr.ttf") bytes)"
echo "sha256: $(sha256sum "$DEST/PTSerifUkr.ttf" | awk '{print $1}')"

# Coverage verification: every Ukrainian-critical codepoint must be present.
# Uses the ttx shim (same fonttools install as pyftsubset).
ttx -q -t cmap -o "$WORK/cmap.ttx" "$DEST/PTSerifUkr.ttf"
missing=0
for cp in 0x490 0x491 0x404 0x454 0x406 0x456 0x407 0x457 0x2019 0xab 0xbb; do
    if ! grep -qi "code=\"$cp\"" "$WORK/cmap.ttx"; then
        echo "MISSING: $cp"
        missing=1
    fi
done
[[ $missing == 0 ]] || exit 1
echo "coverage ok: all Ukrainian-critical glyphs present"
