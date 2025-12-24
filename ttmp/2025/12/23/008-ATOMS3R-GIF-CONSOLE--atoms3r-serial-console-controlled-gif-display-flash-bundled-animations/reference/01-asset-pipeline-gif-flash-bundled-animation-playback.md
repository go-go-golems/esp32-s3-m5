---
Title: 'Asset pipeline: GIF -> flash-bundled animation -> playback'
Ticket: 008-ATOMS3R-GIF-CONSOLE
Status: active
Topics:
    - esp32s3
    - esp-idf
    - atoms3r
    - display
    - m5gfx
    - animation
    - gif
    - assets
    - serial
    - usb-serial-jtag
    - ui
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-23T15:00:50.055029916-05:00
WhatFor: ""
WhenToUse: ""
---

# Asset pipeline: GIF -> flash-bundled animation -> playback

## Goal

Provide a practical, repeatable pipeline to take **real GIF files** and turn them into **on-device playable animations** on AtomS3R, bundled into flash (either as a packed binary in a custom partition or as files in a FS partition).

## Context

Key constraints:

- AtomS3R display is **128×128 visible** (GC9107 window with y-offset 32); most animations should be normalized to 128×128.
- Raw frames are expensive in flash:
  - 128×128 RGB565 is \(128 \cdot 128 \cdot 2 = 32768\) bytes per frame
  - 2MB flash partition stores only ~64 raw frames (≈ 2 seconds at 30fps)
- Therefore, to store “a list of GIFs”, you almost always want **compressed storage**:
  - store original GIF and decode on-device, or
  - store paletted/compressed frames in a custom pack format.

Existing repo pattern we can reuse:

- `ATOMS3R-CAM-M12-UserDemo/partitions.csv` defines an `assetpool` partition (custom type `233/0x23`, size 2MB)
- `ATOMS3R-CAM-M12-UserDemo/upload_asset_pool.sh` uses `parttool.py write_partition` to flash a binary blob to that partition.

One subtle but important point: “AssetPool” in this repo is a **memory-mapped blob**, not a filesystem. That has consequences for how you “list assets”, how you evolve formats, and how you validate sizes. The architecture doc has a deeper explanation; here we focus on the pipeline implications.

## Quick Reference

### Recommended pipeline choices

- **If you want “many GIFs”**: store original `*.gif` files in a FS partition (SPIFFS/LittleFS) and decode on-device with a small GIF decoder library.
- **If you want “minimal CPU + simplest playback”**: pre-convert to a custom packed format (but use palette/compression, not raw RGB565).

### A simple packed animation format (sketch)

This is not implemented yet; it’s a strawman format to guide future implementation.

- File/partition begins with:
  - magic: `"ANPK"` (4 bytes)
  - version: `uint16`
  - animation_count: `uint16`
  - directory_offset: `uint32`
- Directory entry (repeated `animation_count`):
  - name: 32 bytes (NUL-terminated)
  - width: `uint16` (expect 128)
  - height: `uint16` (expect 128)
  - frame_count: `uint16`
  - palette_mode: `uint8` (0=rgb565, 1=pal8)
  - reserved: `uint8`
  - frames_offset: `uint32`
  - delays_offset: `uint32` (array of `uint16` ms)
  - compressed: `uint8` (0=none, 1=rle, 2=lz4)
  - reserved2: padding

Frame encoding options:

- **pal8**: 256-color palette (512 bytes RGB565) + 8-bit indices (`w*h` bytes), optionally compressed
- **rgb565**: `w*h*2` bytes, optionally compressed

### Flashing assets to a custom partition (AssetPool-style)

Example (pattern from repo):

- `parttool.py --port "/dev/ttyACM0" write_partition --partition-name=assetpool --input "AssetPool.bin"`

For this ticket we’d likely use:

- partition name: `animpack` (or reuse `assetpool`)
- input: `AnimPack.bin`

### AssetPool vs an animation directory (how to think about “list/play”)

AssetPool-style blobs are accessed by **field offsets** (C struct layout). That’s great for “one fixed asset set”, but it’s not enough for an animation selector UI because there is no directory. If you want `list` and `play <name>`, you need one of:

- a **directory table** embedded into the blob (names + offsets), or
- a filesystem partition where files are enumerable by path.

In other words: for ticket 008, the critical evolution from “AssetPool” is adding a self-describing directory.

## Usage Examples

### Example 1: Normalize a GIF to 128×128 and reduce FPS (host-side)

Using `ffmpeg` (example):

- Scale to 128×128, keep aspect by padding, reduce to 15fps:

```bash
ffmpeg -i input.gif -vf "fps=15,scale=128:128:force_original_aspect_ratio=decrease,pad=128:128:(ow-iw)/2:(oh-ih)/2" normalized.gif
```

### Example 2: Extract frames as PNGs (host-side)

```bash
ffmpeg -i normalized.gif frames/frame_%04d.png
```

### Example 3: Convert frames to RGB565 (host-side, Python sketch)

Pseudo-code:

```python
from PIL import Image
import struct, glob

def rgb565(r,g,b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

with open("frames.rgb565", "wb") as f:
    for path in sorted(glob.glob("frames/frame_*.png")):
        im = Image.open(path).convert("RGB")
        im = im.resize((128,128))
        for (r,g,b) in im.getdata():
            f.write(struct.pack("<H", rgb565(r,g,b)))
```

This is intentionally “dumb”; in practice you should use paletted frames and/or compression to fit more animations.

## Related

- Ticket 008 architecture doc:
  - `analysis/01-brainstorm-architecture-serial-controlled-animation-playback-on-atoms3r.md`
- AssetPool pattern we can reuse:
  - `ATOMS3R-CAM-M12-UserDemo/partitions.csv`
  - `ATOMS3R-CAM-M12-UserDemo/upload_asset_pool.sh`
  - `ATOMS3R-CAM-M12-UserDemo/main/utils/assets/assets.*`
