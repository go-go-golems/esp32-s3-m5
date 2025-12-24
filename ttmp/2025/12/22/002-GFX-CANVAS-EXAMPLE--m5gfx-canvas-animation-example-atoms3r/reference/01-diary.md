---
Title: Diary
Ticket: 002-GFX-CANVAS-EXAMPLE
Status: active
Topics:
    - esp32s3
    - esp-idf
    - display
    - m5gfx
    - atoms3r
    - animation
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-23T00:00:00Z
WhatFor: ""
WhenToUse: ""
---

# Diary — 002-GFX-CANVAS-EXAMPLE

## Goal

Build a **tutorial-quality AtomS3R example** that renders a fun animation using **M5GFX’s canvas/sprite API** (`M5Canvas`) under ESP-IDF, with stable full-frame presentation (no “frame-rate flutter”).

## Step 1: Create ticket + design doc; implement `0010` canvas tutorial scaffold

This step turns the “canvas animation” idea into a concrete, buildable tutorial under `esp32-s3-m5`, while also capturing the design intent in a living document. The key strategy is to **base `0010` on the known-good AtomS3R M5GFX bring-up** from `0009`, then switch rendering to an **offscreen sprite (`M5Canvas`)**.

We also cross-checked the PlatformIO “ground truth” (`M5AtomS3-UserDemo`) to understand how M5 uses `M5Canvas` in practice: draw into a persistent canvas and call `pushSprite(...)` to present, with DMA implicitly disabled when the sprite is in PSRAM. For our full-screen animation we prefer a **DMA-capable sprite buffer** plus an explicit `waitDMA()` to avoid overlapping transfers.

**Commit (esp32-s3-m5):** 75c0ecc3fc5f40134f0c59cf6652fa76f097107c — "0010: AtomS3R M5GFX canvas plasma animation"

### What I did

- Created ticket `002-GFX-CANVAS-EXAMPLE` and wrote the initial design doc:
  - `.../design-doc/01-atoms3r-fun-animation-using-m5gfx-canvas.md`
- Created a new ESP-IDF tutorial by cloning `0009` → `0010`:
  - `esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation`
- Updated `0010` to:
  - use `project(atoms3r_m5gfx_canvas_animation)`
  - rename Kconfig namespace to `CONFIG_TUTORIAL_0010_*`
  - add canvas/present Kconfig toggles:
    - `CONFIG_TUTORIAL_0010_CANVAS_USE_PSRAM`
    - `CONFIG_TUTORIAL_0010_PRESENT_USE_DMA`
  - implement a “plasma” animation rendered directly into the canvas buffer
  - present frames in the UserDemo style (`canvas.pushSprite(...)`) and, when DMA present is enabled, call `s_display.waitDMA()` each frame
- Updated `0010` `sdkconfig.defaults` + `sdkconfig` to match the new Kconfig symbols and provide sensible defaults (16ms frame delay, DMA present on, PSRAM off).
- Verified the project builds:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation && \
idf.py set-target esp32s3 && idf.py build
```

### Why

- `0009` already encodes the AtomS3R-specific display/backlight contract; copying it avoids re-deriving pins/init again.
- `M5Canvas` matches how M5’s own demo code is structured, and reduces SPI chatter by rendering offscreen.
- The explicit `waitDMA()` is a defensive measure against the previously observed “flutter” caused by overlapping transfers at animation frame rate.

### What worked

- Found the “ground truth” canvas flow in `M5AtomS3-UserDemo`:
  - persistent `M5Canvas`
  - draw → `pushSprite(...)` to present
- `0010` builds cleanly under ESP-IDF 5.4.1.

### What didn’t work

- N/A (this step focused on scaffolding + build; device flash/visual validation is the next step).

### What I learned

- In M5GFX, `pushSprite(...)` decides whether to use DMA based on the sprite buffer allocation:
  - `M5Canvas(LovyanGFX*)` defaults to PSRAM usage, which **disables DMA** in `pushSprite` (“DMA disable with use SPIRAM”).
  - For full-frame animation, forcing PSRAM off allows the sprite buffer to come from **AllocationSource::Dma**, enabling fast DMA pushes.

### What warrants a second pair of eyes

- Whether `waitDMA()` is strictly required after every `pushSprite` for our animation loop (it is likely necessary to avoid overlap, but worth confirming against LovyanGFX/M5GFX transaction semantics).

### Code review instructions

- Start at:
  - `esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation/main/hello_world_main.cpp`
  - `esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation/main/Kconfig.projbuild`
- Validate by building:
  - `idf.py build`

