#!/bin/bash
# normalize_tiles.sh
# Normalize face expression tiles: crop → clean → trim → per-sheet scale → global scale → bottom-align
# Output: 135×240 tiles matching M5StackChan display, bottom-aligned, same scale across sheets.

set -euo pipefail

BASEDIR="$(cd "$(dirname "$0")/.." && pwd)"
SHEETS_DIR="$BASEDIR/assets/sheets"
OUT_DIR="$BASEDIR/assets/tiles"
TMPDIR=$(mktemp -d)

echo "=== Face Tile Normalizer ==="
echo "Input:  $SHEETS_DIR"
echo "Output: $OUT_DIR"
echo ""

# Step 1: Slice sheets into 4×4 grids with 1px border shave to remove bleed
echo "Step 1: Slicing sprite sheets..."
for sheet in 1 2 3; do
  convert "$SHEETS_DIR/sheet${sheet}.png" -crop 4x4@ +repage -shave 1x1 \
    "$TMPDIR/raw_sheet${sheet}_%02d.png"
done

# Step 2: Clean near-black to pure black, then auto-trim
echo "Step 2: Cleaning and trimming..."
for sheet in 1 2 3; do
  for idx in $(seq 0 15); do
    src="$TMPDIR/raw_sheet${sheet}_$(printf '%02d' $idx).png"
    dst="$TMPDIR/clean_sheet${sheet}_$(printf '%02d' $idx).png"
    convert "$src" -black-threshold 2% -fuzz 5% -trim +repage "$dst"
  done
done

# Step 3: Per-sheet scaling to normalize face sizes
# Base face heights (median of non-outlier trimmed heights per sheet):
#   Sheet 1: 246px, Sheet 2: 257px, Sheet 3: 260px
# Reference: 257 (Sheet 2)
echo "Step 3: Per-sheet scaling..."
REF_H=257
declare -A BASE_H=([1]=246 [2]=257 [3]=260)

for sheet in 1 2 3; do
  scale=$(python3 -c "print(f'{${REF_H}/${BASE_H[$sheet]}*100:.4f}')")
  echo "  Sheet $sheet: scale=${scale}%"
  for idx in $(seq 0 15); do
    src="$TMPDIR/clean_sheet${sheet}_$(printf '%02d' $idx).png"
    dst="$TMPDIR/scaled_sheet${sheet}_$(printf '%02d' $idx).png"
    convert "$src" -resize "${scale}%" "$dst"
  done
done

# Step 4: Find global scale factor (widest face → 135px)
echo "Step 4: Computing global scale..."
MAX_W=0
for f in "$TMPDIR"/scaled_sheet*.png; do
  w=$(identify -format "%[width]" "$f")
  [ "$w" -gt "$MAX_W" ] && MAX_W=$w
done
GLOBAL_SCALE=$(python3 -c "print(f'{135/${MAX_W}*100:.4f}')")
echo "  Max width: ${MAX_W}px, Global scale: ${GLOBAL_SCALE}%"

# Step 5: Apply global scale + bottom-align on 135×240 canvas
echo "Step 5: Generating final tiles..."
mkdir -p "$OUT_DIR"
for sheet in 1 2 3; do
  for idx in $(seq 0 15); do
    src="$TMPDIR/scaled_sheet${sheet}_$(printf '%02d' $idx).png"
    dst="$OUT_DIR/sheet${sheet}_$(printf '%02d' $idx).png"
    convert "$src" \
      -resize "${GLOBAL_SCALE}%" \
      -gravity south \
      -background black \
      -extent "135x240" \
      "$dst"
  done
done

# Step 6: Verify — check for top-edge artifacts
echo ""
echo "Step 6: Verification..."
dirty=0
for f in "$OUT_DIR"/sheet*.png; do
  name=$(basename "$f")
  nonblack=$(convert "$f" -depth 8 -crop "135x1+0+0" txt:- 2>/dev/null | \
    tail -n +2 | grep -v "(0,0,0)" | wc -l)
  if [ "$nonblack" -gt 0 ]; then
    echo "  ARTIFACT: $name — $nonblack non-black pixels on top row"
    dirty=$((dirty+1))
  fi
done

echo ""
echo "Done! $((48 - dirty))/48 tiles clean, all 135×240 bottom-aligned"
echo "Output: $OUT_DIR"

# Cleanup
rm -rf "$TMPDIR"
