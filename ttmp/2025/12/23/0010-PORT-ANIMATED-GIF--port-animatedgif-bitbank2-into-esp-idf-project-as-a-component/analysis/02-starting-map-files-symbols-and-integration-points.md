---
Title: 'Starting map: files, symbols, and integration points'
Ticket: 0010-PORT-ANIMATED-GIF
Status: active
Topics:
    - esp32s3
    - esp-idf
    - atoms3r
    - display
    - animation
    - gif
    - m5gfx
    - assets
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ATOMS3R-CAM-M12-UserDemo/main/usb_webcam_main.cpp
      Note: Reference esp_partition_mmap input pattern
    - Path: M5Cardputer-UserDemo/components/M5GFX
      Note: Vendored M5GFX/LovyanGFX component used by tutorials
    - Path: esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation/CMakeLists.txt
      Note: M5GFX component wiring via EXTRA_COMPONENT_DIRS
    - Path: esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation/main/hello_world_main.cpp
      Note: Known-good AtomS3R M5GFX canvas init + waitDMA present pattern
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/main/hello_world_main.cpp
      Note: Scaffold project (avoid integrating AnimatedGIF here yet)
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-23T17:32:08.296550121-05:00
WhatFor: ""
WhenToUse: ""
---


# Starting map: files, symbols, and integration points

## Goal

Create a “where to look / what to touch” map for porting **bitbank2/AnimatedGIF** into an **ESP-IDF component** and proving it works with an AtomS3R **M5GFX RGB565 canvas** in a minimal **single-GIF playback harness** (no serial console UX yet).

This doc is intentionally pragmatic: filenames, key symbols, and the expected call graph.

## Non-goals (for ticket 0010)

- Building the serial console UI / command set (belongs to ticket 008)
- Building the full host-side asset pipeline (belongs to ticket 008)
- Supporting a “directory of many GIFs” (this ticket proves *one* GIF works)

## Repo landmarks (files to read first)

### Ticket docs (decision + constraints)

- `echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/23/0010-PORT-ANIMATED-GIF--port-animatedgif-bitbank2-into-esp-idf-project-as-a-component/tasks.md`
  - Procedural checklist for this ticket (component packaging, `GIFDraw`, correctness/perf, etc.)
- `echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/23/0010-PORT-ANIMATED-GIF--port-animatedgif-bitbank2-into-esp-idf-project-as-a-component/analysis/01-plan-port-animatedgif-into-esp-idf-component-atoms3r-playback-harness.md`
  - Current scoped deliverables + non-goals
- `echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/23/009-INVESTIGATE-GIF-LIBRARIES--investigate-gif-decoding-libraries-for-esp-idf-esp32-atoms3r/sources/research-gif-decoder.md`
  - Decision record: **use AnimatedGIF**, plus the key APIs we expect to call
- `echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/23/008-ATOMS3R-GIF-CONSOLE--atoms3r-serial-console-controlled-gif-display-flash-bundled-animations/analysis/01-brainstorm-architecture-serial-controlled-animation-playback-on-atoms3r.md`
  - Ground truth for AtomS3R display constraints + the mmap pattern we’ll eventually mirror
- `echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/23/008-ATOMS3R-GIF-CONSOLE--atoms3r-serial-console-controlled-gif-display-flash-bundled-animations/reference/01-asset-pipeline-gif-flash-bundled-animation-playback.md`
  - Why “store raw RGB565 frames” explodes flash; motivates “decode GIF bytes from flash/memory”

### Known-good display bring-up + present pattern (what we will reuse)

- `esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation/main/hello_world_main.cpp`
  - **Key idea**: draw into an `M5Canvas` (sprite) and present with `pushSprite()` + `waitDMA()` to avoid tearing/flutter.
  - **Backlight gotcha**: this file intentionally sticks to the **legacy I2C driver** (`driver/i2c.h`) because M5GFX links legacy; mixing with `driver_ng` aborts on ESP-IDF 5.x.
- `esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation/CMakeLists.txt`
  - Shows how tutorials pull in M5GFX without duplicating it:
    - `EXTRA_COMPONENT_DIRS` points at `M5Cardputer-UserDemo/components/M5GFX`
- `M5Cardputer-UserDemo/components/M5GFX`
  - The **actual** vendored M5GFX/LovyanGFX component compiled by the tutorial projects.

### Flash-mapped input reference (for later)

- `ATOMS3R-CAM-M12-UserDemo/main/usb_webcam_main.cpp`
  - Concrete usage of `esp_partition_find_first` + `esp_partition_mmap`.
  - This is the “shape” we’ll mirror once the “embedded byte array” harness works.

### Existing “gif-console” project (do not integrate yet)

- `esp32-s3-m5/0013-atoms3r-gif-console/`
  - Currently appears to be a **mock/scaffold**: the `main/hello_world_main.cpp` is still essentially the `0010` canvas demo (tagged `"atoms3r_m5gfx_canvas"`).
  - We should **not** integrate AnimatedGIF here yet (per ticket instruction); this project remains for ticket 008.

## Symbols / search terms (fast navigation)

### In our code (AtomS3R + M5GFX)

- **Display init + DMA sync**:
  - `M5GFX`, `M5Canvas`, `pushSprite`, `waitDMA`
- **Timing**:
  - `vTaskDelay`, `pdMS_TO_TICKS`, `esp_timer_get_time`
- **Flash mapping**:
  - `esp_partition_find_first`, `esp_partition_mmap`, `esp_partition_munmap`

### In AnimatedGIF (upstream)

We’ll confirm exact names once vendored, but the plan assumes the API surface from ticket 009:

- `AnimatedGIF`
- `GIFDRAW` and the `GIF_DRAW_CALLBACK`
- `open(uint8_t *pData, int iDataSize, GIF_DRAW_CALLBACK *pfnDraw)`
- `playFrame(bool bSync, int *delayMilliseconds)`
- `getInfo(GIFINFO*)`
- `begin(iEndian)` (RGB565 palette endianness selection)
- `allocFrameBuf(...)` + `setDrawType(GIF_DRAW_COOKED)` (optional “correctness first” mode)

Search for Arduino-isms we may need to shim:

- `Arduino.h`, `PROGMEM`, `pgm_read_*`, `memcpy_P`
- `yield()`, `millis()`, `delay()`

## Expected call graph (minimal single-GIF harness)

1. **Init display** (copy/paste from tutorial `0010`)
2. **Prepare canvas**:
   - `M5Canvas canvas(&display); canvas.createSprite(128, 128);`
3. **Init AnimatedGIF**:
   - `gif.begin(/* endian */);`
   - Optional: `gif.allocFrameBuf(...)` + `gif.setDrawType(GIF_DRAW_COOKED)` for simpler draw correctness
4. **Open GIF from memory** (first milestone):
   - `gif.open((uint8_t*)gif_bytes, gif_size, GIFDraw);`
5. **Frame loop**:
   - `while (gif.playFrame(false, &delay_ms)) { canvas.pushSprite(0, 0); display.waitDMA(); vTaskDelay(pdMS_TO_TICKS(delay_ms)); }`

## `GIFDraw` contract (what we think we’ll implement)

We will validate against upstream headers once vendored. The basic intent is:

- The callback is invoked per **scanline** or per **region**.
- `pDraw->pPixels` contains **palette indices** for the line.
- `pDraw->pPalette` contains **RGB565** palette entries (endianness controlled by `gif.begin(...)`).

Two planned implementations:

- **Indexed → RGB565 scanline**: translate indices to RGB565 and write into the `M5Canvas` buffer at `(x,y)` respecting transparency.
- **Cooked mode (optional)**: let the library handle disposal/transparency inside its framebuffer; draw callback becomes a simpler blit.

## Where the new “single GIF” project likely lives

Create a new standalone tutorial project under `esp32-s3-m5/` (separate from `0013-atoms3r-gif-console/`):

- Candidate: `esp32-s3-m5/0014-atoms3r-animatedgif-single/`

This project will:

- reuse the `0010` display+canvas pattern
- add `components/animatedgif/` (vendored + pinned AnimatedGIF)
- embed one small test GIF as a `const uint8_t[]` (first milestone)
- optionally add an `esp_partition_mmap` path later (closer to ticket 008 architecture)

## Immediate next steps

- Vendor/pin AnimatedGIF into an ESP-IDF component and confirm it compiles under ESP-IDF 5.4.1.
- Create the new `esp32-s3-m5/0014-...` project and get it building with M5GFX + AnimatedGIF.
- Implement the minimal `GIFDraw` + `playFrame()` loop to play a single embedded GIF.
