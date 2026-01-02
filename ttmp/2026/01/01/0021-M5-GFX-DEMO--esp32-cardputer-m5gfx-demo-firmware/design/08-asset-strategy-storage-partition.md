---
Title: 'Asset strategy: storage partition (FATFS) + embedded icons'
Ticket: 0021-M5-GFX-DEMO
Status: active
Topics:
    - cardputer
    - m5gfx
    - esp32s3
    - esp-idf
    - assets
    - fatfs
    - debugging
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/partitions.csv
      Note: Defines the `storage` FAT partition used for demo assets
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0013-atoms3r-gif-console/flash_storage.sh
      Note: Prior art for flashing a FATFS image into the `storage` partition with parttool.py
ExternalSources: []
Summary: Decision on how the demo-suite should ship and iterate on images/animations/fonts: use a flash-backed FATFS `storage` partition for large assets, plus embedded tiny icons for UI chrome.
LastUpdated: 2026-01-02T02:20:00Z
WhatFor: ""
WhenToUse: ""
---

# Asset strategy: `storage` partition (FATFS) + embedded icons

This doc decides how the demo-suite firmware should handle assets (images, animations, test patterns, tiny icons).

## Decision (what we will do)

1. **Large / frequently-changed assets** (JPG/PNG/GIF/QOI test images, sample screenshots, demo datasets):
   - stored in the existing `storage` **FATFS** partition (flash-backed)
   - mounted at runtime (e.g. `/storage`)
   - flashed from the host as a prebuilt FATFS image (`storage.bin`)

2. **Tiny UI icons** (battery/wifi glyphs, small 1-bit symbols used in header/footer):
   - embedded directly in firmware as small bitmaps (XBM / 1-bit arrays)

## Why this approach

- **Fast iteration loop**: asset changes don’t require rebuilding firmware; update `storage.bin` and flash the partition.
- **Size realism**: image/animation demos need more than a handful of embedded arrays.
- **Keeps the firmware lean**: avoid bloating the `.bin` with large binary assets.
- **Matches repo prior art**: `0013-atoms3r-gif-console/flash_storage.sh` already uses `parttool.py` to write `storage.bin` to the `storage` partition.

## Partition and tooling contract

### Partition

The demo-suite uses a project-local `partitions.csv` with:

- `storage, data, fat, , 1M,`

This partition is the contract for “asset-bearing” demos.

### Flashing assets

Host workflow (pattern copied from other projects):

```bash
parttool.py --port /dev/serial/by-id/... write_partition --partition-name=storage --input storage.bin
```

We should keep a small `storage.bin` checked into git only if it’s truly stable and small; otherwise, we generate it locally (and keep it out of git).

## File layout inside `/storage`

Suggested layout (mirror the demo catalog):

```text
/storage/
  images/
    test/
    photos/
  fonts/
  gifs/
  qoi/
  README.txt
```

## Constraints / risks

- FATFS mount code must be stable (wear leveling, mount failures need a clear on-screen error).
- Asset demos must degrade gracefully when `/storage` is empty or missing.
- PNG/JPG decode paths should avoid allocating giant temporary buffers; prefer streaming decode APIs when possible.

## Follow-ups implied by this decision

- Add a `tools/flash_storage.sh` in `0022-cardputer-m5gfx-demo-suite/` (copy `0013` pattern) once we add real assets.
- Add a “Storage status” screen that shows mount status and lists `/storage` contents (helps debug missing assets).

