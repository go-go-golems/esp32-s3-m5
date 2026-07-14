---
title: "Scripts README"
doc_type: reference
status: active
intent: long-term
topics: [m5stackchan, animation, imagemagick, esp32]
---

# Face Animation Studio — Scripts

Scripts for tile normalization, conversion, export, and pair extraction.
Numbered by execution order.

## 01-normalize_tiles.sh

**Purpose**: Normalize face expression tiles from 3 sprite sheets.

**Pipeline**: `crop 4x4@` → `shave 1x1` → `black-threshold 2%` → `trim` → per-sheet scale → global scale (50.19%) → bottom-align 135×240 → `black-threshold 1%`

**Input**: `assets/sheets/sheet{1,2,3}.png`
**Output**: `assets/tiles/sheet{1,2,3}_{00-15}.png` (135×240)

## 02-normalize_tiles.py

**Purpose**: Python/PIL version of the normalization pipeline. Superseded by `01-normalize_tiles.sh`.

## 03-tile_to_cpp.py

**Purpose**: Convert PNG tiles to C++ RGB565 PROGMEM arrays for ESP32.

**Usage**: `python3 03-tile_to_cpp.py <tile.png|dir/> --output <file.h|dir/>`

**Note**: 48 tiles × 32400 px × 2B = ~26MB raw. Add RLE compression for production.

## 05-extract_sprite_pairs.py

**Purpose**: Extract aligned sprite pairs from matching grid sheets using **weighted cross-correlation**.

**Algorithm**: Crops grid cells at identical positions → scales to target width → computes binary masks → finds optimal vertical offset via weighted cross-correlation of row-sum profiles → places both tiles on canvas at aligned positions.

**Why cross-correlation?** Simple bottom-alignment fails for paired sprites when different variants have different content extents (e.g., red variant has extra features above the clock body). The weighted correlation emphasizes the common wide parts (the clock body) and de-emphasizes narrow features (raised eyebrows, horns).

**Usage**:
```bash
python3 05-extract_sprite_pairs.py white_sheet.png red_sheet.png \
  --output-dir assets/tiles_clock \
  --cols 4 --rows 4 \
  --variant-a white --variant-b red
```

**Requirements**: ImageMagick, Python 3.7+, numpy

## General Principles

1. Match tile dimensions to display dimensions (135×240 for M5StackChan)
2. Bottom-align faces — the chin is the natural reference point
3. Use deterministic CV — black-threshold + trim for monochrome on black
4. Apply black-threshold after resize to kill interpolation artifacts
5. For paired sprites, use cross-correlation alignment — not bottom-align
6. Weighted correlation by min(A,B) row sums emphasizes the common body
