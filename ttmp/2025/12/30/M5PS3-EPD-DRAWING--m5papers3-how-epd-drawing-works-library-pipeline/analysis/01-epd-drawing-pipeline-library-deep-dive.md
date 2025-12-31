---
Title: EPD drawing pipeline + library deep dive
Ticket: M5PS3-EPD-DRAWING
Status: active
Topics:
    - epd
    - display
    - m5gfx
    - m5unified
    - esp-idf
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: build/compile_commands.json
      Note: Build evidence for which panel sources are compiled
    - Path: components/M5GFX/docs/M5PaperS3.md
      Note: States EPDiy no longer used since v0.2.7
    - Path: components/M5GFX/src/M5GFX.cpp
      Note: Shows PaperS3 selecting Panel_EPD + Bus_EPD
    - Path: components/M5GFX/src/lgfx/v1/platforms/esp32/Bus_EPD.cpp
      Note: Defines scanline write mechanism over esp_lcd_i80
    - Path: components/M5GFX/src/lgfx/v1/platforms/esp32/Bus_EPD.h
      Note: Signal naming (XSTL/XOE/XLE/XCL) and pin config structure
    - Path: components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp
      Note: Defines framebuffer format + async LUT/step refresh task
    - Path: main/main.cpp
      Note: Shows how the app triggers full refresh and batches drawing
ExternalSources: []
Summary: Explains how this firmware draws to the PaperS3 EPD via M5Unified/M5GFX, identifying the active panel backend (Panel_EPD + Bus_EPD) and its LUT/step-based refresh algorithm.
LastUpdated: 2025-12-30T22:49:08.033537247-05:00
WhatFor: 'Research and maintenance: understand/modify EPD refresh behavior (speed/ghosting/partial updates) and locate the responsible code paths.'
WhenToUse: When debugging display artifacts, adding new UI drawing patterns, or changing EPD update strategy on PaperS3.
---



# EPD drawing pipeline + library deep dive (PaperS3)

## Executive summary

This firmware draws to the PaperS3 e-ink panel through the standard M5Stack stack:

- **App code** calls `GetHAL().display.*` which is **`M5.Display`** from **`M5Unified`** (`main/hal/hal.h`).
- `M5.Display` is a **`M5GFX`** instance (LovyanGFX-derived).
- For **M5PaperS3**, `M5GFX` selects an in-tree EPD backend:
  - **Panel**: `lgfx::Panel_EPD` (`components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp`)
  - **Bus**: `lgfx::Bus_EPD` (`components/M5GFX/src/lgfx/v1/platforms/esp32/Bus_EPD.cpp`)
  - It uses ESP-IDF’s **`esp_lcd_i80`** underneath for scanline transfers.

Despite the “EPDiy” rumor: this repo’s `M5GFX` is `0.2.15` (see `repos.json`) and the M5GFX docs explicitly state **EPDiy is no longer used since v0.2.7** (`components/M5GFX/docs/M5PaperS3.md`). `Panel_EPDiy` still exists, but it is gated behind `__has_include(<epdiy.h>)` and **no `epdiy.h` is present** in this workspace or the build.

## Build confirmation (source-of-truth)

Running `idf.py build` in this repo succeeds (ESP-IDF `v5.3.4` appears in the compile command lines).

`build/compile_commands.json` confirms the PaperS3 EPD driver is compiled from:

- `components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp`
- `components/M5GFX/src/lgfx/v1/platforms/esp32/Bus_EPD.cpp`

It also includes `components/M5GFX/src/lgfx/v1/panel/Panel_EPDiy.cpp`, but this unit is effectively inert unless `<epdiy.h>` is available.

## App-level drawing and refresh behavior

### What the app draws with

Apps use normal M5GFX APIs:

- primitives: `fillRect`, `fillScreen`
- text: `loadFont`, `setTextDatum`, `setTextColor`, `drawString`
- images: `drawPng`

These are called through `GetHAL().display` (which is `M5.Display`).

### How “refresh” is orchestrated in this repo

There are two “refresh concepts”:

1) **App-level redraw request flag**

- `Hal::requestRefresh()` just sets a boolean (`main/hal/hal.h`).
- Apps check `GetHAL().isRefreshRequested()` to decide to redraw their own regions (e.g., RTC app).

2) **Panel-level EPD refresh/update**

- The EPD backend refreshes when `Panel_EPD::display(...)` is called.
- `Panel_EPD` sets `_auto_display = true`, so `endWrite()` triggers `display(0,0,0,0)` automatically when a write transaction ends (`components/M5GFX/src/lgfx/v1/Panel.hpp`).

In `main/main.cpp`, the firmware also forces a **full-screen background redraw** every 15 seconds:

- It sets mode `epd_quality`, then `drawPng(img_bg, 0,0)` (full-screen).
- It calls `GetHAL().requestRefresh()` so each app redraws its overlays next loop.

### How EPD mode is used by the app

The app selects modes frequently:

- `epd_fastest`: for small, frequent updates like time/date (`main/apps/app_rtc.cpp`)
- `epd_text`: for text-heavy UI portions (`main/apps/app_sd_card.cpp`, `main/apps/app_wifi.cpp`)
- `epd_quality`: for full-screen images and “clean” redraws (`main/main.cpp`, various apps)

Mode selection is via `GetHAL().display.setEpdMode(epd_mode_t::...)`, which routes to the underlying panel’s `setEpdMode(...)` (`components/M5GFX/src/lgfx/v1/Panel.hpp`).

## Which panel backend PaperS3 uses (and where it is wired)

PaperS3-specific wiring is in `components/M5GFX/src/M5GFX.cpp`:

- It detects `board_M5PaperS3` (via I2C probing a GT911 touch controller).
- It checks PSRAM config (requires OPI PSRAM).
- It constructs:
  - `auto bus_epd = new Bus_EPD();`
  - `auto p = new lgfx::Panel_EPD();`
- It assigns the PaperS3 EPD pinout to `Bus_EPD::config()` and sets panel geometry:
  - `cfg.panel_width = 960`, `cfg.panel_height = 540`
  - `cfg.memory_width = 960`, `cfg.memory_height = 540`

So: **the PaperS3 EPD driver is in `M5GFX`** and is not “some missing esp-idf component”.

## How drawing becomes pixels (Panel_EPD framebuffer format)

`Panel_EPD` is a **buffered** panel:

- `_buf` is allocated in PSRAM: `panel_w * panel_h / 2` bytes (`components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp`).
- It stores **4 bits per pixel** (16 grayscale levels), packed **2 pixels per byte**.

Drawing entry points:

- `writeFillRectPreclipped(...)` converts the requested grayscale into 4-bit pixels using a Bayer matrix and writes packed nibbles into `_buf`.
- `writeImage(...)` and `writePixels(...)` ultimately call `_draw_pixels(...)`.
- `_draw_pixels(...)` applies rotation mapping, does dithering/quantization, and writes 4-bit values into `_buf`.

Mode affects quantization:

- If `epd_fast`/`epd_fastest`: pixels are thresholded to `0x0` or `0xF` (effectively 1-bit) using Bayer.
- Otherwise: pixels are quantized to `0..15` with Bayer dithering.

## How drawing becomes waveforms (Panel_EPD update engine)

### High-level model

EPD refresh is not “copy framebuffer to display”. Instead, the driver advances pixels through a **multi-step LUT waveform sequence**, and it can keep driving the panel until all pixels have finished transitioning.

Key structures allocated in `Panel_EPD::init_intenal()`:

- `_buf` (PSRAM): current target grayscale pixels (4bpp packed).
- `_step_framebuf` (PSRAM): per-pixel-pair state machine buffer (stores current+pending state).
- `_lut_2pixel` (DMA memory): precomputed lookup table mapping:
  - `(mode/LUT step index, desired grayscale byte)` → “2-pixel drive pattern”
- `_dma_bufs[2]` (DMA): scanline staging buffers.

### How a refresh is triggered

`Panel_EPD` tracks “touched regions” via `_range_mod`:

- Draw operations call `_update_transferred_rect(...)` which expands `_range_mod`.

Then `Panel_EPD::display(x,y,w,h)`:

- Optionally expands `_range_mod` again if `w/h` are non-zero.
- Aligns coordinates (notably `xs/xe` are even-aligned).
- Packages `update_data_t {x,y,w,h,mode}`.
- `xQueueSend(...)` pushes it to `_update_queue_handle`.
- Sets `_display_busy = true`.

### Background task: `task_update`

`Panel_EPD::task_update(...)` runs forever on a pinned core and does two jobs:

1) **Incorporate queued rectangles into the per-pixel state (`_step_framebuf`)**

For each queued update rectangle:

- It reads packed grayscale bytes from `_buf`.
- It writes/updates corresponding entries in `_step_framebuf`, setting the **sign bit (`0x8000`)** to indicate “this pixel pair is active / needs further driving”.
- Depending on `mode`, it may insert an “eraser” stage (see `lut_eraser`) before the final mode LUT to reduce ghosting.

2) **Emit scanlines until no pixels remain “in transition”**

Each scanline iteration:

- Calls `blit_dmabuf(...)` to:
  - read `_step_framebuf` for that scanline,
  - consult `_lut_2pixel`,
  - update step state,
  - write scanline bytes into the DMA buffer.
- Uses `Bus_EPD` to send the scanline:
  - `bus->beginTransaction()` once at the start
  - `bus->writeScanLine(...)` for each scanline
  - `bus->endTransaction()` at the end
- Powers the panel on/off via `bus->powerControl(...)`.

The task keeps looping while `remain == true` (meaning at least one pixel pair still needs waveform steps).

### The LUTs and modes

`Panel_EPD` has built-in LUT arrays for:

- `lut_quality`
- `lut_text`
- `lut_fast`
- `lut_fastest`
- plus `lut_eraser` (used as a pre-pass)

`Panel_EPD::init(...)` loads defaults into `config_detail` if none are provided.

### Concurrency + cache coherence (important)

There is an explicit warning in `Panel_EPD::init_intenal()`:

> If the task core differs from the main core, PSRAM cache sync is needed or framebuffer updates may not be reflected immediately.

Mitigations present in code:

- `Panel_EPD::display(...)` calls `cacheWriteBack(...)` on the PSRAM buffer region before queueing.

This is a likely hotspot if you ever see “updates sometimes don’t apply” on real hardware.

## Panel_EPD.cpp deep dive (how to read it)

`components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp` is doing three big things:

1) **Store what you want to see** (a 4bpp target framebuffer in PSRAM, `_buf`)
2) **Track what the panel still needs** (a per-pixel-pair “waveform progress” buffer, `_step_framebuf`)
3) **Continuously emit scanlines** (a background task that produces per-line drive patterns and pushes them over an I80-parallel engine via `Bus_EPD`)

If you keep those roles in your head, most of the file becomes readable: you are either (a) writing target pixels, (b) translating target changes into “work items”, or (c) performing that work by repeatedly applying LUT steps.

### What hardware assumption this driver makes

This is not a typical “EPD controller IC with SPI commands” design.

PaperS3’s panel is driven more like a raw panel:

- The MCU shifts per-line source data (what to do to each pixel) into a source driver using a parallel bus.
- The MCU also toggles gate-driver timing pins (vertical scanning) and latch/enable pins (horizontal scanning).
- The *time axis* (the waveform steps) is implemented in software by repeating scanout passes with different 2-bit “drive actions” per pixel, pulled from the LUT.

So: the “controller” is the code in `Panel_EPD.cpp` + `Bus_EPD.cpp`.

## Pin map / signal naming (PaperS3)

The PaperS3 wiring is set in `components/M5GFX/src/M5GFX.cpp` when it constructs `Bus_EPD` + `Panel_EPD`.

From that code:

- `bus_cfg.pin_data[0..7] = { GPIO6,14,7,12,9,11,8,10 }`
- `bus_cfg.pin_sph = GPIO13`
- `bus_cfg.pin_le  = GPIO15`
- `bus_cfg.pin_spv = GPIO17`
- `bus_cfg.pin_ckv = GPIO18`
- `bus_cfg.pin_oe  = GPIO45`
- `bus_cfg.pin_pwr = GPIO46`
- `bus_cfg.pin_cl  = GPIO16`

The bus header comments describe naming correspondence:

- `pin_sph`: “start pulse source driver (XSTL)”
- `pin_oe`: “output enable source driver (XOE)”
- `pin_le`: “latch enable source driver (XLE)”
- `pin_cl`: “clock source driver (XCL)”

Your screenshot pin map (EPD_ED047TC1 ↔ ESP32S3R8) matches the *source data* and several control signals:

| EPD_ED047TC1 | ESP32S3R8 (from screenshot) | PaperS3 `Bus_EPD` signal (from code) |
|---|---:|---|
| DB0 | G6  | `pin_data[0]` |
| DB1 | G14 | `pin_data[1]` |
| DB2 | G7  | `pin_data[2]` |
| DB3 | G12 | `pin_data[3]` |
| DB4 | G9  | `pin_data[4]` |
| DB5 | G11 | `pin_data[5]` |
| DB6 | G8  | `pin_data[6]` |
| DB7 | G10 | `pin_data[7]` |
| XSTL | G13 | `pin_sph` (XSTL / source start pulse) |
| XLE | G15 | `pin_le` (latch enable) |
| SPV | G17 | `pin_spv` (gate start pulse) |
| CKV | G18 | `pin_ckv` (gate clock) |
| PWR | G45 | likely `pin_oe` on PaperS3 (code uses GPIO45 as XOE) |

Important discrepancy:

- The screenshot lists **PWR=G45**, while PaperS3 code uses **GPIO45 as `pin_oe` (XOE)** and uses **GPIO46 as `pin_pwr`**.
- This suggests the screenshot’s “PWR” label may refer to the panel module’s “output enable / power gating” concept, while M5GFX splits this into:
  - `pin_oe` (XOE) and
  - `pin_pwr` (a separate panel power enable line).

When debugging or reasoning about timing, treat the code as the authoritative mapping for this firmware.

## Data structures and formats (the “state machine”)

### `_buf`: what you want the display to look like

Allocated in `init_intenal()`:

- `_buf = heap_caps_aligned_alloc(..., (panel_w * panel_h) / 2, MALLOC_CAP_SPIRAM)`
- Comment: “1Byte = 2pixel”

So `_buf` stores **two 4-bit grayscale pixels per byte** (16 gray levels).

All drawing primitives end up writing 4-bit pixels into `_buf`.

### `_step_framebuf`: what the panel still needs to do

Allocated in `init_intenal()`:

- `_step_framebuf = heap_caps_aligned_alloc(..., (memory_w * memory_h / 2) * 2 * sizeof(uint16_t), MALLOC_CAP_SPIRAM)`

There are two key implications:

1) The buffer is indexed at “pixel-pair” granularity: `memory_w * memory_h / 2` entries correspond to bytes in `_buf` (each byte = 2 pixels).
2) It is multiplied by 2 (`*2*sizeof(uint16_t)`), because the driver maintains **two slots per pixel-pair**:
   - **even index**: currently-being-processed waveform state (the one `blit_dmabuf` consumes),
   - **odd index**: a “reserved / next” state (used when a new draw request arrives while a previous waveform is still running).

This “double buffering per pixel-pair” is what allows:

- “insert an eraser pass, then apply new target”
- “replace the queued target while the current pass is still running”

### `_lut_2pixel`: how software turns “state” into drive patterns

Allocated in DMA-capable memory:

- `_lut_2pixel = heap_caps_malloc(lut_total_step * 256 * sizeof(uint16_t), MALLOC_CAP_DMA)`

Despite the `sizeof(uint16_t)` in the allocation size, it is typed as `uint8_t*` and is indexed as bytes.

Conceptually:

- Each LUT *step* has **256 entries** corresponding to all possible 2-pixel grayscale combinations (the 8-bit value stored in `_buf`).
- Each entry is a **4-bit nibble** representing two 2-bit actions:
  - upper 2 bits: action for pixel0
  - lower 2 bits: action for pixel1

Actions are encoded as:

- `0`: end-of-data (stop)
- `1`: drive “towards black”
- `2`: drive “towards white”
- `3`: no operation

These come from the LUT tables in the file (e.g., `lut_quality`, `lut_text`, etc.). Those LUT tables are defined as time sequences (rows) of per-gray-level actions (columns).

### How the “current step” is represented

Each even-slot element in `_step_framebuf` is treated as a signed 16-bit integer in `blit_dmabuf`.

The value is used as an index into `_lut_2pixel`:

- **low 8 bits**: the 2-pixel grayscale byte (copied from `_buf`)
- **high 8 bits**: which step block you are in (0,1,2,...) plus an offset that selects which LUT family (eraser/quality/text/fast/fastest)

Every time the driver emits a scanline, it advances time for that pixel-pair by doing:

- `s_n += 256` (i.e., advance to the next 256-entry block = next waveform step).

And it considers the per-pixel waveform complete when:

- `lut[s_n] == 0` (the LUT says “end of data” for those two pixels at that step).

At end-of-data, it “swaps in” the reserved slot:

- set current (even) = reserved (odd)
- mark reserved (odd) as inactive (bit15 set, via `| 0x8000`)

### The meaning of the sign bit (0x8000)

`blit_dmabuf` loads the current slot as a signed 16-bit:

- if current value is **negative** (bit15 set), it is treated as **inactive / no work** and skipped.
- if current value is **>= 0**, it is treated as **active** and converted via LUT into output drive bits.

This is why you’ll see `task_update` carefully controlling which slot is modified:

- It primarily writes **odd slots** as “the next target”.
- It selectively forces **even slots** to be non-negative to schedule actual waveform work (either directly in fast modes, or by inserting eraser work first).

## `blit_dmabuf`: the “waveform stepper”

`blit_dmabuf` exists in two versions:

- an Xtensa-optimized assembly version (`#if defined(__XTENSA__)`)
- a C fallback (`#else`) which is easier to read

The C version is the clearest explanation of the algorithm:

- It processes a scanline in chunks of 16 pixels:
  - each 32-bit output word packs **8 nibbles**
  - each nibble represents **2 pixels**
  - 8 nibbles × 2 pixels = 16 pixels
- For each nibble (2 pixels), it:
  1) reads current state from an even slot (`src[0], src[2], ...`)
  2) looks up drive actions: `tmp = lut[s_n]`
  3) advances the step pointer: `s_n += 256`
  4) packs that nibble into the output word
  5) if `tmp == 0`, it “ends” and swaps in the reserved slot (odd slot) and marks the odd slot inactive.
  6) writes the updated current state back to the even slot.

It returns `true` if any pixel pair produced non-zero drive data (meaning there is still work remaining somewhere), and `false` if everything is idle.

This is the heart of “timing”:

- The LUT is a time sequence.
- Advancing time is done by scanning out the panel repeatedly, and incrementing `s_n` by 256 each pass.

## `task_update`: the scheduler + scanline engine

`task_update` runs forever as a FreeRTOS task created in `init_intenal()`.

It has two phases per loop:

### Phase A: incorporate new update rectangles

It waits on `_update_queue_handle` for an `update_data_t {x,y,w,h,mode}`.

When it receives one, it updates the `_step_framebuf` entries for that rectangle.

Key details:

- The mode determines whether the driver tries to:
  - update immediately (fast/fastest), or
  - insert an eraser pass first (quality/text).
- The “eraser insertion” works by forcing the **current slot (even)** to be an eraser-state value (high byte = 0), while writing the **target LUT state** into the reserved slot (odd). When the eraser completes, `blit_dmabuf` swaps in the reserved target state automatically.

### Phase B: emit scanlines until the panel reaches steady state

After incorporating pending updates, it:

- powers the panel on: `bus->powerControl(true)`
- for each scanline:
  - runs `blit_dmabuf` to fill `dma_buf` and to advance per-pixel step state
  - writes that scanline (sometimes repeated for vertical magnification)
  - sets `remain = true` if any pixels still need more steps
- ends the transaction and powers off when no work remains (`remain == false`)

`_display_busy` is controlled here:

- it is set to `true` when there are queued updates or remaining work
- it becomes `false` only when the step engine reaches idle and the queue is empty

This is what `waitDisplay()` is waiting on.

## Bus timing: what the delays and pin toggles are doing

The “timing stuff” is primarily in `components/M5GFX/src/lgfx/v1/platforms/esp32/Bus_EPD.cpp`.

### 1) The I80 engine is used as a “pixel shifter”

`Bus_EPD::init()` creates:

- `esp_lcd_new_i80_bus(...)` with:
  - `wr_gpio_num = pin_cl` (XCL)
  - `data_gpio_nums[] = pin_data[]` (DB0..DB7)
  - `dc_gpio_num = pin_pwr` (dummy; this is *not* a command/data split design)
- `esp_lcd_new_panel_io_i80(...)` with:
  - `cs_gpio_num = pin_sph` (XSTL / source start pulse)
  - `pclk_hz = bus_speed` (PaperS3 sets this to 16 MHz)
  - `on_color_trans_done = notify_line_done` callback

Meaning: the ESP-IDF LCD peripheral becomes a DMA-driven parallel shifter.

### 2) Per-scanline handshake

To send one scanline:

- `writeScanLine(...)`:
  - waits until `_bus_busy` is false
  - sets `LE` low (prepare latch)
  - sets `CKV` high
  - starts background DMA transfer: `esp_lcd_panel_io_tx_color(...)`
  - marks `_bus_busy=true`

When the DMA transfer completes, ESP-IDF invokes `notify_line_done(...)`:

- pulls `CKV` low
- pulls `LE` high (latch the shifted source data)
- clears `_bus_busy`

This establishes a reliable ordering:

- shift data → (interrupt) latch it → allow next scanline

### 3) Frame / vertical scanning pulses

`beginTransaction()` manipulates `SPV` and `CKV` with short microsecond delays:

- `SPV` is a gate-start pulse (start of vertical scan).
- `CKV` is a gate clock (advance to next row).

The precise waveform requirements are panel-dependent; without the panel datasheet you can treat these as:

- “initialize vertical scan”
- “clock a few times to align internal state”

The short delays (`delayMicroseconds(1/3)`) are there to satisfy minimum pulse widths / setup times (the comments are approximate).

### 4) Power sequencing

`powerControl(true/false)` toggles:

- `pin_oe` (XOE)
- `pin_pwr` (power enable)
- `pin_spv` (gate start pulse line is also driven to a known state)

With small delays (`100us`, `1ms`, etc.) between steps.

That is typical of panels that require the source/gate drivers and/or high-voltage rails to stabilize before you drive waveforms, and to discharge safely when powering down.

### 5) Why `vTaskDelay(1)` exists in the panel loop

There is a `vTaskDelay(1)`:

- after queueing in `Panel_EPD::display(...)` (very short yield)
- once per full scanout loop in `task_update` (after sending all scanlines)

These serve two practical purposes:

- yield to the scheduler (keep other tasks alive, avoid watchdog issues)
- allow hardware/DMA callbacks to run in a timely manner

They are not “the LUT timing” itself; the LUT timebase is the repeated scanout passes. The delays just keep the system healthy and allow edges/rails to settle.

## How scanlines are physically sent (Bus_EPD)

`Bus_EPD` uses ESP-IDF’s “LCD I80” engine as a generic parallel write engine:

- `esp_lcd_new_i80_bus(...)`
- `esp_lcd_new_panel_io_i80(...)`
- `esp_lcd_panel_io_tx_color(...)`

It uses a callback (`notify_line_done`) to toggle EPD control signals per scanline and to clear an internal `_bus_busy` flag, allowing the next scanline transfer.

The critical methods:

- `Bus_EPD::beginTransaction()` / `endTransaction()`: manage line/frame control pins (CKV/SPV/LE/etc).
- `Bus_EPD::writeScanLine(...)`: starts a background `esp_lcd_panel_io_tx_color(...)` transfer.
- `Bus_EPD::powerControl(...)`: toggles power-related pins (`pin_pwr`, `pin_oe`, `pin_spv`).

## Where “EPDiy” fits (and why you can’t find it here)

This repo’s PaperS3 path does **not** require the external EPDiy library:

- `components/M5GFX/docs/M5PaperS3.md` documents EPDiy as legacy and states it is no longer used from `v0.2.7`.
- The `Panel_EPDiy` source in `components/M5GFX/src/lgfx/v1/panel/Panel_EPDiy.*` is guarded with `#if __has_include(<epdiy.h>)`.
- This workspace does not include `epdiy.h`, and the build compilation database does not reference it.

So the “can’t find EPDiy source” issue is expected: **it’s not a dependency of this firmware**.

If you still need EPDiy for other projects, the upstream is `https://github.com/vroland/epdiy/` (as referenced in the M5GFX docs), but it is not required to understand or modify the EPD drawing path in this repo.

## Practical pointers (where to look next)

If you want to change EPD behavior (ghosting vs speed, partial update rules, etc.), the leverage points are:

- `components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp`: LUTs, “eraser” insertion, queue behavior, scanline emission.
- `components/M5GFX/src/lgfx/v1/platforms/esp32/Bus_EPD.cpp`: timing/handshake and bus speed limits.
- `components/M5GFX/src/M5GFX.cpp`: PaperS3 pin mapping and panel config (memory size, padding, task core).
- App code: mode selection and batching (`startWrite`/`endWrite`) in `main/main.cpp` and `main/apps/*`.
