---
Title: 'Research plan: GIF decoding options for AtomS3R'
Ticket: 009-INVESTIGATE-GIF-LIBRARIES
Status: active
Topics:
    - esp32s3
    - esp-idf
    - atoms3r
    - display
    - animation
    - gif
    - assets
    - serial
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/23/009-INVESTIGATE-GIF-LIBRARIES--investigate-gif-decoding-libraries-for-esp-idf-esp32-atoms3r/reference/02-research-execution-guide-gif-decoder-library-investigation.md
      Note: Step-by-step execution guide for library investigation
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-23T16:41:19.000814529-05:00
WhatFor: ""
WhenToUse: ""
---


## Executive summary

This ticket exists to answer one question for the AtomS3R “serial-controlled GIF display” project (ticket 008):

**Which GIF decoding approach/library should we use under ESP-IDF (ESP32-S3) so that playback is correct enough and simple enough to ship as a tutorial?**

The hard part is not “can we decode bytes”. The hard parts are:

- correctness (disposal modes, transparency, local palettes)
- I/O model (flash-mapped blob vs filesystem vs embedded C array)
- output model (paletted indices vs RGB888 vs RGB565 conversion costs)
- resource budgeting (how many animations fit on flash; how much RAM we burn)

## What we already know (from this repo)

Before evaluating any library, we already have strong ground truth in-repo:

- AtomS3R display stability patterns:
  - `esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation/` (canvas + `waitDMA()`; avoid “flutter”)
- Asset “blob in partition” pattern:
  - `ATOMS3R-CAM-M12-UserDemo/main/usb_webcam_main.cpp` (custom partition + `esp_partition_mmap`)
- “AssetPool is field-addressed, not name-addressed”:
  - `ATOMS3R-CAM-M12-UserDemo/main/utils/assets/images/types.h`
  - `ATOMS3R-CAM-M12-UserDemo/main/utils/assets/assets.*`

This means the evaluation harness should reuse the `0010` display/present pattern and should explicitly test a flash-mapped input mode (because ticket 008’s storage story likely starts there).

## Evaluation criteria (write these down first)

### Correctness checklist

- Global palette
- Local palette
- Transparency
- Disposal modes
- Per-frame delays
- Looping behavior

### Integration checklist

- ESP-IDF 5.4.1 compatibility
- Packaging (clean ESP-IDF component vs Arduino-only)
- Input source flexibility:
  - file path / fd
  - memory buffer (ideal for flash-mapped assets)
- Output flexibility:
  - can render line-by-line (good for streaming to SPI)
  - can render full frame (simpler, but needs RAM)

### Performance checklist

- CPU per frame at 128×128
- RAM requirements:
  - minimal mode (no full canvas) vs full-canvas mode
- Flash footprint impact (if storing raw vs compressed)

## Candidate shortlist (starting point)

This list is intentionally small; add candidates only when they clearly improve the tradeoff.

- `bitbank2/AnimatedGIF`
- `UncleRus/esp-idf-libnsgif` (NetSurf libnsgif wrapped for ESP-IDF)
- LVGL GIF support (`lv_gif` / `lv_lib_gif`) **only if** we accept LVGL as a dependency
- `gifdec` (tiny C decoder; must validate limitations and I/O suitability)

## Recommended next step: minimal harness

Implement a tiny “decode and display” harness (separate from serial UI):

- Boot
- Bring up AtomS3R display using the stable `0010` canvas pattern
- Decode one known GIF (stored in flash-mapped blob or embedded C array)
- Render into canvas buffer (RGB565) and `pushSprite()` + `waitDMA()`
- Respect per-frame delay

This harness turns library evaluation into measurable facts: does it look correct on the real LCD, and does it fit within our resource budgets?

