---
Title: M5Unified + M5GFX Deep Dive (Display Stack and Low-Level LCD Communication)
Ticket: 003-ANALYZE-ATOMS3R-USERDEMO
Status: active
Topics:
    - embedded-systems
    - esp32
    - m5stack
    - arduino
    - analysis
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Deep dive into M5Unified + M5GFX (LovyanGFX) internals with a focus on ESP32-S3 LCD bring-up, SPI transactions, backlight control, sprites/canvas, and board auto-detection (AtomS3 / AtomS3R)"
LastUpdated: 2025-12-22T09:16:37.20232382-05:00
WhatFor: "Provide an implementation-level reference for how M5Unified uses M5GFX to initialize and drive the LCD, including the exact pins/driver used on AtomS3/AtomS3R and the low-level SPI write path"
WhenToUse: "When debugging display issues, replicating M5Stack’s LCD bring-up in pure ESP-IDF, or correcting assumptions about panel controllers/pinouts in this repository’s docs"
---

# M5Unified + M5GFX Deep Dive (Display Stack and Low-Level LCD Communication)

## Executive Summary

M5Stack’s “it just works” experience on boards like **AtomS3** comes from a layered stack:

- **`M5Unified`** owns *device lifecycle*: board detection, pin mapping tables, I2C setup, power/speaker/IMU, and the global `M5` singleton used by sketches.
- **`M5GFX`** owns *graphics + panel bring-up*: it wraps **LovyanGFX** (`lgfx`) and contains the board-specific display wiring and auto-detection logic. It also provides `M5.Display.*` drawing APIs (rects, fonts, PNG draw) and sprite/canvas support.
- **LovyanGFX (`lgfx`)** owns the *fast path to hardware*: panel classes implement LCD controller command sequences (e.g. GC9107 init table), and bus classes implement the transport (SPI, I2C, parallel). On ESP32-S3, SPI writes are optimized with **direct SPI register programming** for small command/data operations and DMA-backed bulk transfers.

Most importantly for this ticket: for **AtomS3 / AtomS3R**, the panel is not a “likely ST7789” guess — **M5GFX explicitly auto-detects GC9107** using the `RDDID` (0x04) command and then configures a `Panel_GC9107` instance with the correct geometry and offsets. This document records the exact pins and the low-level SPI write mechanism that makes that happen.

## Where the library source comes from (in this repo workspace)

This repository includes the full M5Stack library sources as **PlatformIO-resolved dependencies** for `M5AtomS3-UserDemo`.

You can find the exact sources used for this project at:

- `M5AtomS3-UserDemo/.pio/libdeps/ATOMS3/M5Unified/`
- `M5AtomS3-UserDemo/.pio/libdeps/ATOMS3/M5GFX/`

These are the authoritative references for “what pins/driver does M5 actually use?” in the context of this demo.

## How M5Unified uses M5GFX at runtime

M5Unified exposes a global singleton `M5` and a `M5GFX Display` instance at `M5.Display`. The key design idea is that **display initialization happens as part of `M5.begin()`**, and M5Unified uses the display itself to help determine the board type (auto-detection) before enabling other subsystems.

### `M5.begin()` flow (high level)

At a high level, `M5Unified::begin(config_t)` does:

- **Force display brightness to 0** (avoid flash/garbage during bring-up)
- **Call `Display.init_without_reset()`** (M5GFX auto-detect + panel init)
- **Derive the “real” board type** using `_check_boardtype(Display.getBoard())`
- **Install a board-specific pin map** (`_setup_pinmap(board)`)
- **Initialize I2C pins/ports for internal/external buses** (`_setup_i2c(board)`)
- **Continue initializing peripherals** (power, speaker, IMU/RTC, etc.)
- **Restore display brightness** once a real display was detected

This is the reason sketches can simply do `M5.begin()` and then immediately call `M5.Display.*` drawing APIs.

### Key snippet: M5Unified calls M5GFX `init_without_reset()`

The inline implementation in `M5Unified.hpp` shows the exact ordering and intent:

- brightness is captured
- brightness is set to 0
- `Display.init_without_reset()`
- board type resolution and pinmap setup
- brightness restored if display was detected

This ordering matters when diagnosing display power sequencing and “flash of garbage” issues.

## M5GFX board auto-detection and AtomS3 / AtomS3R display bring-up

M5GFX’s `init_impl()` performs board/panel detection. On ESP32-S3, it sets up an SPI bus, toggles reset pins, and probes likely chip-select lines by issuing `RDDID` (0x04) and reading 32 bits.

The probe function `_read_panel_id()`:
- selects CS
- sends a command byte (default 0x04)
- reads back 32 bits
- compares the value to known patterns

### Confirmed: AtomS3 and AtomS3R use GC9107 (not ST7789)

Both AtomS3 and AtomS3R detection blocks check for:

- `(id & 0xFFFFFF) == 0x079100` → **GC9107**

Once this matches, M5GFX constructs `Panel_GC9107` (from LovyanGFX) and applies a config that sets:

- `panel_width = 128`
- `panel_height = 128`
- `offset_y = 32` (crucial; the controller’s native memory height is 160)
- `readable = false`

### AtomS3: confirmed display wiring + backlight

From `M5GFX.cpp` auto-detect path:

- **SPI MOSI**: GPIO21
- **SPI SCLK**: GPIO17
- **LCD DC**: GPIO33
- **LCD CS**: GPIO15
- **LCD RST**: GPIO34
- **SPI host**: final bus uses `SPI3_HOST`
- **SPI write clock**: `freq_write = 40_000_000` (40MHz)
- **Backlight**: PWM (`_set_pwm_backlight(GPIO_NUM_16, ch=7, freq=256, invert=false, offset=48)`)

This means AtomS3’s backlight is controlled via LEDC PWM on GPIO16 (via `lgfx::Light_PWM`).

### AtomS3R: confirmed display wiring + backlight control mechanism

From `M5GFX.cpp` auto-detect path:

- **SPI MOSI**: GPIO21
- **SPI SCLK**: GPIO15
- **LCD DC**: GPIO42
- **LCD CS**: GPIO14
- **LCD RST**: GPIO48
- **SPI host**: final bus uses `SPI3_HOST`
- **SPI write clock**: `freq_write = 40_000_000` (40MHz)
- **Backlight**: I2C-controlled light (`Light_M5StackAtomS3R`)
  - initializes I2C on **GPIO45 + GPIO0**
  - writes to device address **48 (0x30)**, setting brightness via register **0x0e**

So, unlike AtomS3, AtomS3R backlight is not PWM in the default M5GFX stack: it’s managed by an I2C-controlled device.

## The low-level LCD communication path (SPI)

This section focuses on how a high-level draw call (e.g. `M5.Display.fillRect()` or `M5.Display.drawPng()`) eventually becomes SPI transactions on the ESP32-S3.

### Layers

The typical call chain is:

1. **User code** calls `M5.Display.*` (M5Unified exposes `M5GFX Display`)
2. `M5GFX` inherits LovyanGFX base classes and routes primitives to the panel
3. `Panel_*` class implements controller-specific commands (CASETRASET/RAMWR, init sequences)
4. `Bus_SPI` performs the actual bus operations

### `Bus_SPI` on ESP32: direct-register command/data writes

For small writes like LCD commands and short parameters, `Bus_SPI::writeCommand()` and `Bus_SPI::writeData()` do **not** call `spi_device_transmit()`. Instead, they:

- wait for SPI engine idle
- set MOSI bit length register
- write the data to `SPI_W0`
- set D/C by writing to a cached GPIO register pointer
- kick the SPI engine by writing `SPI_EXECUTE`

This direct register approach is a key performance technique: it reduces per-transaction overhead and keeps command/data toggling extremely cheap.

### D/C control: precomputed register pointers

During `Bus_SPI::config()`, the code precomputes:

- `_mask_reg_dc` = bit mask for D/C pin
- `_gpio_reg_dc[0]` = pointer used to drive D/C “command”
- `_gpio_reg_dc[1]` = pointer used to drive D/C “data”

Then:
- `writeCommand()` uses `_gpio_reg_dc[0]` (command)
- `writeData()` uses `_gpio_reg_dc[1]` (data)

This avoids calling `gpio_set_level()` in hot paths.

### Bulk pixel writes: DMA-friendly buffers

Pixel-heavy operations (drawing images, sprites, filled rectangles) are implemented with paths that can use:

- DMA-capable buffers (`AllocationSource::Dma`)
- preformatted pixel buffers (`pixelcopy_t`)
- queued DMA transactions (`addDMAQueue` / `execDMAQueue`)

For example, LovyanGFX sprite/canvas buffers are allocated with DMA-capability by default (unless configured for PSRAM), and `pushSprite()` copies that buffer to the panel.

## Sprite / Canvas: how `M5Canvas` works

The `M5AtomS3-UserDemo` uses `M5Canvas` to build a sprite and then `pushSprite()` it to the real display. Under the hood, the sprite is a `Panel_Sprite` that owns a pixel buffer and implements drawing primitives by writing into memory; then `pushSprite()` pushes pixels to the panel via the bus.

Two practical implications:

- **RAM footprint** is proportional to sprite size and color depth (RGB565 → 2 bytes/pixel).
- **Display update cost** is mostly “copy buffer over SPI,” which is why the bus and DMA strategy matter.

## What this means for our AtomS3 demo analysis

This deep dive gives us enough hard evidence to fix earlier “likely ST7789” statements and to document the actual pins/backlight mechanisms used by the standard M5Stack stack. In particular:

- The LCD controller for AtomS3/AtomS3R is **GC9107** in M5GFX’s auto-detect path.
- The visible area is 128×128 but the controller memory height is 160; M5GFX uses **`offset_y = 32`**.
- AtomS3 backlight is PWM on GPIO16; AtomS3R backlight is controlled via I2C (GPIO45/GPIO0 + device 0x30).

Those details should replace speculative wording in the primary analysis document for this ticket.
