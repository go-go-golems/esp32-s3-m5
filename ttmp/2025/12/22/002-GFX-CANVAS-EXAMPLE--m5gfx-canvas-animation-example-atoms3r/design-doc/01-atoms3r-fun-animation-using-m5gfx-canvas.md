---
Title: 'AtomS3R: Fun animation using M5GFX Canvas'
Ticket: 002-GFX-CANVAS-EXAMPLE
Status: active
Topics:
    - esp32s3
    - esp-idf
    - display
    - m5gfx
    - atoms3r
    - animation
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-23T00:00:00Z
WhatFor: ""
WhenToUse: ""
---

# AtomS3R: Fun animation using M5GFX Canvas

## Executive Summary

Build a small, **fun, smooth, reproducible animation demo** for AtomS3R using **M5GFX’s `M5Canvas` (sprite) API** under ESP-IDF. The demo renders each frame into an offscreen canvas, then pushes it to the GC9107 panel with a transfer strategy that avoids the known “frame-rate flutter” (DMA + `waitDMA()` or otherwise fully synchronous updates).

The result is a “known-good” reference that new contributors can flash to validate: display init, backlight init, canvas rendering, and high-FPS SPI transfers.

## Problem Statement

We currently have:

- An ESP-IDF `esp_lcd` GC9107 example (`esp32-s3-m5/0008-atoms3r-display-animation`) that can show content but exhibits **frame-rate flutter** under animation.
- An ESP-IDF + M5GFX baseline (`esp32-s3-m5/0009-atoms3r-m5gfx-display-animation`) that can show content, but also exhibits **frame-rate flutter** unless we explicitly synchronize transfers.

We need a single tutorial-quality example that:

- Demonstrates **M5GFX canvas rendering** (not immediate-mode drawing) on AtomS3R.
- Provides a **stable animation** that is visually fun and obviously “alive”.
- Encodes the **hardware bring-up contract** (GC9107 offsets, backlight I2C init, SPI host/pins) so future work doesn’t regress.

## Proposed Solution

### Overview

Create a new tutorial project (e.g. `esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation`) which:

- Initializes display/backlight exactly as the known-good AtomS3R path does.
- Allocates one offscreen **`M5Canvas`** at 128×128 RGB565.
- Renders a deterministic animation per frame into the canvas.
- Pushes the canvas to the panel each frame in a transfer-safe way.

### Animation concept (fun but simple)

Pick one of these as the default animation (easy to implement, low CPU, looks good at 30–60 FPS):

- **Plasma gradient**: palette-cycled RGB565 generated from precomputed sine tables.
- **Starfield + parallax**: ~64–128 stars, additive trails, simple physics.
- **Metaballs / “lava lamp”**: 3–5 moving blobs, thresholded field, palette map.

Plasma is the best first choice: tiny code, great visuals, deterministic.

### Rendering & presentation pipeline

#### Canvas

- Construct canvas on top of `M5GFX` display:
  - `M5Canvas canvas(&display);`
  - `canvas.setColorDepth(16);`
  - `canvas.createSprite(128, 128);`
- Render into the sprite buffer using fast primitives or direct pixel writes.

#### Present

Prefer one of the following presentation modes:

- **Synchronous push** (simple, stable):
  - `canvas.pushSprite(0, 0);`
  - or `display.pushImage(..., canvas.getBuffer())` style.

- **DMA + explicit wait** (recommended if we see flutter at frame rate):
  - Ensure the backing buffer is DMA-safe (either by M5GFX allocation source, or by using a DMA-safe buffer path).
  - After pushing, call `display.waitDMA()` (or `canvas.waitDMA()` if available).

The key invariant: **do not modify the frame buffer while SPI is still transferring it**.

#### Note from the known-good UserDemo (why PSRAM matters)

`M5AtomS3-UserDemo` uses this idiom:

- Persistent `M5Canvas canvas(&M5.Display);`
- `canvas.setColorDepth(16); canvas.createSprite(...);`
- Draw into the canvas, then call `canvas.pushSprite(...)` when needed.

In M5GFX, `M5Canvas(LovyanGFX*)` defaults `_psram=true`, which means sprite buffers tend to land in **PSRAM** and thus **disable DMA** in `pushSprite` (M5GFX comments: “DMA disable with use SPIRAM”). That’s great for UI workloads; for a full-screen animation we usually want `_psram=false` so the sprite buffer is **AllocationSource::Dma** and we can safely do DMA pushes (plus an explicit `waitDMA()` to avoid overlap).

### Timing model

Use a fixed `frame_delay_ms` (default 16 or 33) and compute `t` from a monotonic clock to keep animation speed stable even if frame timing varies.

Example:

- `uint32_t t_ms = (xTaskGetTickCount() * portTICK_PERIOD_MS);`
- Derive phase = `t_ms * speed`.

### AtomS3R bring-up contract (must match known-good)

The tutorial should bake in the AtomS3R contract as defaults (with menuconfig overrides):

- **Panel**: GC9107, visible 128×128, memory 128×160, `offset_y=32`
- **SPI**:
  - host `SPI3_HOST`
  - SCLK=15, MOSI=21, DC=42, CS=14, RST=48
  - `spi_mode=0`, `spi_3wire=true`
  - `freq_write=40MHz` (fallback: 26MHz if SI issues)
- **Backlight**:
  - I2C addr `0x30`
  - init writes: `0x00←0x40`, delay, `0x08←0x01`, `0x70←0x00`
  - brightness write: `0x0e←brightness`
  - optional gate GPIO7 (active-low) if hardware variant requires it

### Logging & debug UX

Keep a small amount of high-signal logging:

- Boot banner: heap, DMA free, config summary (pclk, offset, invert, rgb order)
- Backlight init logs (addr, regs written)
- Heartbeat every N frames: frame count + dt + free heap

Provide a “visual sanity sequence” (solid fills) behind a Kconfig option for quick diagnosis.

## Design Decisions

### Use `M5Canvas` as the primary rendering API

- **Why**: It’s the best “friendly” abstraction in M5GFX to demonstrate offscreen composition and reduce per-primitive SPI chatter.
- **Tradeoff**: Sprite buffer memory must be managed carefully (and ideally DMA-safe).

### Default to RGB565, 128×128, full-screen

- **Why**: Matches the panel’s visible area; avoids dealing with partial update math and offsets for the first example.

### Explicitly synchronize transfers when animating

- **Why**: We’ve observed frame-rate flutter in our environment when transfers overlap; design explicitly prevents that.

## Alternatives Considered

### Immediate-mode drawing (no canvas)

- **Rejected**: Too easy to end up SPI-bound with lots of small primitives; doesn’t demonstrate the canvas API.

### `esp_lcd` with a framebuffer

- **Rejected for this ticket**: This ticket is specifically “M5GFX + canvas”. `esp_lcd` remains valuable elsewhere.

### LVGL

- **Rejected**: Great for UI, not for the “tiny fun animation” demo; higher footprint and setup complexity.

## Implementation Plan

1. Create the new tutorial project folder.
2. Copy the known-good AtomS3R M5GFX bring-up (SPI pins, backlight init) into the tutorial.
3. Implement `M5Canvas` allocation and a simple animation (plasma).
4. Implement present path with a “safe transfer” policy (DMA+wait or strict synchronous push).
5. Add Kconfig toggles:
   - frame delay
   - SPI freq
   - backlight enable + gate pin
   - optional “solid colors” diagnostic
6. Validate on-device:
   - solid colors stable
   - plasma smooth at target FPS
   - no frame-rate flutter under prolonged run

## Testing / Validation

- **Visual sanity**: solid fill sequence then animation.
- **Stability**: run for 5 minutes; no flicker regressions, no heap leak.
- **Performance**: log dt every N frames; confirm stable average.

## Open Questions

- Should the default present path be strictly synchronous (`pushSprite`) or DMA+wait? (We can keep both and choose via Kconfig.)
- Do all AtomS3R units require GPIO7 gating for backlight, or only some revisions?
- Can we directly force M5Canvas sprite allocation to DMA memory in ESP-IDF mode, or should we keep an explicit DMA framebuffer and use `pushImageDMA`?

## References

- Ticket 001 diary + RCA (flutter + I2C conflict): `ttmp/2025/12/22/001-MAKE-GFX-EXAMPLE-WORK--make-gc9107-m5gfx-style-display-example-work-atoms3r/reference/01-diary.md`
- PlatformIO-resolved AtomS3R M5GFX bring-up:
  - `M5AtomS3-UserDemo/.pio/libdeps/ATOMS3/M5GFX/src/M5GFX.cpp`
  - `M5AtomS3-UserDemo/.pio/libdeps/ATOMS3/M5GFX/src/lgfx/v1/panel/Panel_GC9A01.hpp` (contains `Panel_GC9107`)
