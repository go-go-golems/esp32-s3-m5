---
Title: 'AnimatedGIF support plan: runtime integration + flash asset bundling (parttool)'
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
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-23T18:57:58.597456513-05:00
WhatFor: ""
WhenToUse: ""
---

## Executive summary

Ticket 009/0010 establishes **AnimatedGIF (bitbank2)** as the decoder we want to use for AtomS3R GIF playback under ESP-IDF. We now have:

- a working reference “truth harness” project (`esp32-s3-m5/0014-atoms3r-animatedgif-single`) that proves decode → `GIFDraw` → M5GFX `M5Canvas` present works, and
- a control-plane MVP (`esp32-s3-m5/0013-atoms3r-gif-console`) that proves serial console + button + playback state machine works (with mock animations).

This document describes the next integration step: **replace the mock animations with real GIF playback**, and define an asset workflow to **bundle multiple GIFs into flash** and select them via serial console. The asset workflow is intentionally designed around **`parttool.py write_partition`**, matching existing repo precedent (`ATOMS3R-CAM-M12-UserDemo/upload_asset_pool.sh`) and the “custom partition” pattern used across these firmwares.

## Current repo “ground truth” (don’t re-derive)

### Decoder + draw contract

The canonical code path we will reuse lives in the AnimatedGIF harness:

- `esp32-s3-m5/0014-atoms3r-animatedgif-single/main/hello_world_main.cpp`
  - `static void GIFDraw(GIFDRAW *pDraw)` — scanline draw callback
  - `AnimatedGIF::begin(GIF_PALETTE_RGB565_LE)`
  - `AnimatedGIF::open((uint8_t*)gif_bytes, gif_size, GIFDraw)`
  - `AnimatedGIF::playFrame(false, &delay_ms, &ctx)` + `getLastError()` nuance
  - `present_canvas(canvas)` uses `pushSprite()` + `waitDMA()` for correctness

The core decoder API and callback types live in:

- `esp32-s3-m5/0014-atoms3r-animatedgif-single/components/animatedgif/src/AnimatedGIF.h`
  - `GIFDRAW` fields: `pPixels`, `pPalette`, `ucTransparent`, `ucHasTransparency`, `ucDisposalMethod`, `iX/iY/y/iWidth`
  - pixel palette modes: `GIF_PALETTE_RGB565_LE`, `GIF_PALETTE_RGB565_BE`
  - draw types: `GIF_DRAW_RAW`, `GIF_DRAW_COOKED`
  - file callback types: `GIF_OPEN_CALLBACK`, `GIF_READ_CALLBACK`, `GIF_SEEK_CALLBACK`, `GIF_CLOSE_CALLBACK`

### Control plane we will plug into

The console + button skeleton that already works:

- `esp32-s3-m5/0013-atoms3r-gif-console/main/hello_world_main.cpp`
  - `esp_console_new_repl_usb_serial_jtag(...)` + `esp_console_start_repl(repl)`
  - command handlers: `list`, `play`, `stop`, `next`, `info` (and `brightness`)
  - button ISR → queue → playback state machine

This is where AnimatedGIF playback should be integrated, rather than building a parallel second control plane.

## Architectural plan (Phase B integration)

### Playback pipeline (at runtime)

The playback loop becomes:

1. **Asset selection** chooses a GIF by name/index (from the console or button cycling).
2. **Open GIF bytes**:
   - memory-backed: `gif.open(ptr, size, GIFDraw)` (preferred), where `ptr` points into a memory-mapped partition or a buffer loaded from FS.
3. **Decode frame**:
   - `playFrame(false, &delay_ms, &ctx)` repeatedly
4. **Present**:
   - `canvas.pushSprite(0, 0); display.waitDMA();`
5. **Pace**:
   - `vTaskDelay(pdMS_TO_TICKS(delay_ms))` with a floor (e.g. `>= 10ms`)
6. **EOF/reset**:
   - `gif.reset()` and restart, with a 1-tick yield on the EOF path

We already have a correct implementation of steps (2)–(6) in `0014`. The main integration work is steps (1) and the “asset open” sub-choices.

### Asset bundling into flash (two viable approaches)

We want “bundle multiple GIFs on flash and list/select them”. There are two viable approaches; both can be flashed with `parttool.py`.

#### Option A (recommended MVP): FATFS `storage` partition containing `*.gif` files

This mirrors the partition strategy found in the Cardputer demo:

- `M5Cardputer-UserDemo/partitions.csv` defines:
  - `storage, data, fat, , 1M`

We can apply the same idea to AtomS3R:

- add a `storage` (FATFS) partition to the chapter’s `partitions.csv`
- generate a FATFS image on the host containing:
  - `/gifs/0.gif`, `/gifs/1.gif`, ...
  - and optionally `/gifs/index.txt` (names/metadata)
- flash that image using:
  - `parttool.py --port <port> write_partition --partition-name=storage --input storage.bin`

Runtime:

- mount FATFS at `/storage`
- enumerate `/storage/gifs/*.gif` for `list`
- on `play`, either:
  - read file into memory and call memory-backed `open(ptr, size, GIFDraw)` (simplest), or
  - implement AnimatedGIF file callbacks (`GIF_OPEN_CALLBACK` etc.) using `fopen`/`fread`/`fseek` (lower RAM, more glue code)

Why this is a good MVP:

- easiest to iterate (swap GIF files by reflashing only the partition image)
- keeps firmware small and avoids inventing a pack format too early

#### Option B (later / “production style”): raw `gifpack` partition (custom directory + concatenated bytes)

This matches the existing “raw blob partition” patterns in the repo (AssetPool) and is ideal if we want pure memory-mapped reads:

- create a custom `gifpack` partition (data/custom type)
- host tool produces `GifPack.bin`:
  - header + directory (names + offsets + sizes)
  - concatenated raw GIF bytes
- flash with:
  - `parttool.py --port <port> write_partition --partition-name=gifpack --input GifPack.bin`

Runtime:

- `esp_partition_find_first(...)` + `esp_partition_mmap(...)`
- parse directory in-place
- `gif.open(mapped + offset, size, GIFDraw)`

This is a better long-term story, but it’s more work: we must define and maintain a pack format and build tool.

## Recommendation (what to build next)

Implement AnimatedGIF support as:

1. **Integrate AnimatedGIF playback into the existing console+button chapter (`0013`)**, gated behind an “asset provider” interface.
2. Use **Option A (FATFS `storage` partition)** for bundling GIF files initially, flashed using **`parttool.py`**.
3. Only after the end-to-end workflow is stable, consider migrating to a raw `gifpack` partition (Option B) if we need the mmap/no-FS properties.

This staging reduces risk: we validate the full selection/playback loop without committing to a custom asset format.

## Implementation notes (symbols to reuse)

- **Draw callback**:
  - Start from `0014`’s `GIFDraw(GIFDRAW *pDraw)` and its `GifRenderCtx` (scaling + optional byte swap).
- **Present correctness**:
  - Use `present_canvas()` pattern from `0014` (DMA present + `waitDMA()`).
- **Control-plane events**:
  - Reuse `0013`’s queue/event loop for `PlayIndex`, `Stop`, `Next`, `Info`.
  - Replace the “mock animation registry” with a “GIF asset registry”.
- **Decoder return semantics**:
  - Follow the contract called out in the usage guide (`reference/02-animatedgif-bitbank2-on-esp-idf-usage-guide.md`):
    - `open()` returns 1/0 (not a GIF_* error code)
    - `playFrame()` can return 0 at EOF even if a frame was rendered; use `getLastError()`.

