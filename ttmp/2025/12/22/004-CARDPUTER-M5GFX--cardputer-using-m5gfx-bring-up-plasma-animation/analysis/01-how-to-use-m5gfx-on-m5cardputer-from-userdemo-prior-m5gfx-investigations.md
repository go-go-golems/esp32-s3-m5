---
Title: How to use M5GFX on M5Cardputer (from UserDemo + prior M5GFX investigations)
Ticket: 004-CARDPUTER-M5GFX
Status: active
Topics:
    - esp32s3
    - esp-idf
    - cardputer
    - display
    - m5gfx
    - st7789
    - spi
    - animation
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-22T22:06:59.497366005-05:00
WhatFor: ""
WhenToUse: ""
---

# How to use M5GFX on M5Cardputer (from UserDemo + prior M5GFX investigations)

## Executive summary

M5Cardputer’s LCD is **ST7789** on **SPI3**, with a non-zero **visible window offset** (`offset_x=52`, `offset_y=40`) and a portrait panel rotated to landscape via `rotation=1`. The simplest “known-good” way to bring it up under ESP-IDF is:

- Vendor M5GFX (`M5Cardputer-UserDemo/components/M5GFX`)
- `M5GFX display; display.init();` (autodetect selects `board_M5Cardputer`)
- Render into an offscreen sprite/canvas (`LGFX_Sprite` / `M5Canvas`) and `pushSprite()` to the display.

For full-screen animation (e.g., plasma), avoid “flutter/tearing” by ensuring the transfer is **DMA-safe** and the bus is **flushed** between frames (same principle we learned on AtomS3R: overlapping transfers cause frame-rate flutter). In M5GFX, `pushSprite()` will use DMA when the sprite buffer is not in PSRAM; you can also explicitly call `waitDMA()` on the display.

## What we’re building next (goal)

Create a Cardputer tutorial analogous to AtomS3R `0010`:

- `esp32-s3-m5/00xx-cardputer-m5gfx-plasma` (name TBD)
- Full-screen plasma animation at ~30–60 FPS
- Uses M5GFX autodetect for display wiring and backlight
- Uses a sprite/canvas backbuffer and a present path that does not overlap transfers

## Ground truth: Cardputer display wiring and panel config (M5GFX autodetect)

The most important “ground truth” is in M5GFX autodetection logic:

- **File**: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.cpp`
- **Symbol/area**: autodetect branch for `board_t::board_M5Cardputer` (shares logic with `board_M5VAMeter`)

Key configuration values (as used by M5GFX for Cardputer):

- **SPI bus**
  - `spi_host = SPI3_HOST`
  - `spi_mode = 0`
  - `spi_3wire = true`
  - `freq_write = 40 MHz`
  - `freq_read = 16 MHz`
  - `pin_mosi = GPIO35`
  - `pin_sclk = GPIO36`
  - `pin_dc   = GPIO34`
  - `pin_cs   = GPIO37`
  - `pin_rst  = GPIO33`
  - `miso = NC`
- **Panel**
  - `Panel_ST7789`
  - `panel_height = 240`
  - **Cardputer only**:
    - `panel_width = 135`
    - `offset_x = 52`
    - `offset_y = 40`
    - `rotation = 1`
  - `invert = true`
  - `readable = true`
- **Backlight**
  - PWM backlight on **GPIO38**
  - via `_set_pwm_backlight(GPIO_NUM_38, ch=7, freq=256, invert=false, offset=16)`

Why these numbers matter:

- If you draw at (0,0) with the wrong offsets/rotation you can get a “black screen” or clipped output even when SPI transactions succeed.
- Treat M5GFX’s autodetect + config as the “golden path” and avoid hardcoding until necessary.

## Minimal bring-up pattern (ESP-IDF)

### 1) Bring up M5GFX as an ESP-IDF component

Copy the approach from AtomS3R `0009` / `0010`:

- Use `EXTRA_COMPONENT_DIRS` to point at the vendored M5GFX component.
  - Example (from `esp32-s3-m5/0009-atoms3r-m5gfx-display-animation/CMakeLists.txt`):
    - `EXTRA_COMPONENT_DIRS = ../../M5Cardputer-UserDemo/components/M5GFX`

### 2) Initialize display

Use the vendor `M5GFX` class:

- `M5GFX display;`
- `display.init();`

This will autodetect Cardputer and apply the config above.

### 3) Use a sprite/canvas

Vendor Cardputer firmware uses sprites extensively:

- **File**: `M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp`
- **Symbol**: `HalCardputer::_display_init()`

It allocates:

- `_display = new M5GFX; _display->init();`
- `_canvas = new LGFX_Sprite(_display); _canvas->createSprite(204, 109);`

For a full-screen plasma animation, create a sprite sized to the visible area after rotation:

- Expect something like **240×135** (landscape) depending on `rotation`.
  - Verify by printing `display.width()` / `display.height()` after `init()`.

## Animation (plasma) approach

Borrow the “plasma” implementation pattern from AtomS3R `0010`:

- **File**: `esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation/main/hello_world_main.cpp`
- **Symbols**:
  - `build_plasma_tables(...)`
  - `draw_plasma(...)`
  - present loop uses `canvas.pushSprite(...)` and `s_display.waitDMA()`

Porting steps for Cardputer:

1. Compute `w = display.width()`, `h = display.height()` after rotation/autodetect.
2. Create `M5Canvas canvas(&display)` (or `LGFX_Sprite canvas(&display)`).
3. Prefer **DMA-backed sprite** (disable PSRAM for the sprite buffer if using `M5Canvas`).
4. Render plasma into `canvas.getBuffer()` as RGB565.
5. Present with `canvas.pushSprite(0,0)` and call `display.waitDMA()` each frame.

## “Flutter” / tearing lessons from AtomS3R (why flush matters)

We observed on AtomS3R that animation can “flutter” at the frame rate while solid colors are stable. The underlying lesson:

- Don’t start rendering the next frame into a buffer that is still being transferred over SPI.

Mitigation pattern we used:

- Use DMA-capable buffer (`MALLOC_CAP_DMA` or M5GFX sprite AllocationSource::Dma)
- Present via DMA
- Explicitly `waitDMA()` between frames

This same pattern is recommended for Cardputer plasma.

## Suggested file layout for the future Cardputer plasma tutorial

- `esp32-s3-m5/00xx-cardputer-m5gfx-plasma/`
  - `CMakeLists.txt` (sets `EXTRA_COMPONENT_DIRS` for M5GFX)
  - `main/hello_world_main.cpp` (init display + canvas + plasma loop)
  - `main/Kconfig.projbuild` (frame delay, optional heartbeat logs)
  - `README.md` (build/flash/expected output)

## Practical validation checklist

- **Bring-up**
  - `display.init()` returns and `display.width()/height()` match expectations.
  - `fillScreen(TFT_RED)` etc show stable colors.
- **Animation**
  - Plasma is visibly “blobby” (not a static texture scrolling).
  - No frame-rate flutter; if present, force `waitDMA()` and ensure sprite buffer isn’t PSRAM-backed.

## Related docs / prior work to cross-reference

- AtomS3R M5GFX baseline: `esp32-s3-m5/0009-atoms3r-m5gfx-display-animation`
- AtomS3R M5GFX canvas plasma: `esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation`
- Ticket 002 docs (canvas + DMA rationale): `ttmp/2025/12/22/002-GFX-CANVAS-EXAMPLE--m5gfx-canvas-animation-example-atoms3r/`
- Ticket 001 docs (M5GFX vs esp_lcd + I2C conflict RCA): `ttmp/2025/12/22/001-MAKE-GFX-EXAMPLE-WORK--make-gc9107-m5gfx-style-display-example-work-atoms3r/`
