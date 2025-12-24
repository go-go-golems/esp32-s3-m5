---
Title: 'Brainstorm/architecture: serial-controlled animation playback on AtomS3R'
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
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ATOMS3R-CAM-M12-UserDemo/main/hal_config.h
      Note: Example vendor button pin definitions (HAL_PIN_BUTTON_A)
    - Path: ATOMS3R-CAM-M12-UserDemo/main/usb_webcam_main.cpp
      Note: Concrete AssetPool mmap injection flow (esp_partition_find_first + esp_partition_mmap)
    - Path: ATOMS3R-CAM-M12-UserDemo/main/utils/assets/images/types.h
      Note: AssetPool struct layout defining asset fields and lengths
    - Path: esp32-s3-m5/0003-gpio-isr-queue-demo/main/hello_world_main.c
      Note: Button interrupt + queue + debounce pattern
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-23T15:00:49.861246899-05:00
WhatFor: ""
WhenToUse: ""
---


## Executive summary

Build an AtomS3R firmware that can **list** a set of flash-bundled animations and **play** one selected by a serial console command. Source assets start as real GIF files, but the on-device playback can either:

- **Decode GIF on-device** (small flash footprint, more CPU and library work), or
- **Play a pre-converted frame pack** (very simple playback, potentially large flash usage unless compressed).

This repo already contains the building blocks to make this straightforward:

- **AtomS3R display bring-up + stable animation present**:
  - `esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation/` (M5GFX + canvas + `waitDMA()` anti-flutter)
  - `esp32-s3-m5/0008-atoms3r-display-animation/` (esp_lcd GC9107 init + backlight control)
- **Asset “bundle into flash partition” pattern**:
  - `ATOMS3R-CAM-M12-UserDemo/` uses an `assetpool` partition and flashes `AssetPool.bin` via `parttool.py`.
- **Serial command infrastructure (optional)**:
  - `echo-base--openai-realtime-embedded-sdk/components/esp-protocols/components/console_simple_init/` can create an `esp_console` REPL over USB Serial/JTAG.

## Requirements (MVP)

- **Flash-bundled animations**: a fixed set of animations are shipped in the device image (or flashed to a known partition).
- **Serial console control**:
  - `list` → prints available animation IDs/names
  - `play <name|id>` → starts playback
  - `stop` → stops playback
  - `info` → prints active animation details (fps/delay/frames, storage size)
  - optional: `brightness <0..255>` (AtomS3R backlight)
- **Smooth playback** on AtomS3R GC9107 (avoid tearing/flutter; don’t scribble into buffers while DMA is active).

## Ground truth: AtomS3R display contract

From our existing AtomS3R chapters:

- **Panel**: GC9107 (memory 128×160), visible window 128×128 at **y-offset 32**
- **SPI wiring (fixed)**:
  - `CS=GPIO14`, `SCK=GPIO15`, `MOSI=GPIO21`, `DC=GPIO42`, `RST=GPIO48`
- **Backlight ambiguity** (revision-dependent):
  - Some boards: GPIO gate (GPIO7, active-low)
  - Some boards: I2C brightness device (addr `0x30`, reg `0x0e`, SCL=GPIO0, SDA=GPIO45)

Best “known-good” approach for animation workloads is M5GFX canvas + explicit DMA sync (see `0010`).

## Architecture sketch

### Components

- **Display subsystem**
  - M5GFX init + backlight init (mirror `0010`)
  - full-screen `M5Canvas` (sprite) 128×128, RGB565
  - present path: `canvas.pushSprite(0, 0); display.waitDMA();` when DMA is used
- **Asset subsystem**
  - loads an “animation directory” (either from a raw pack in a custom partition, or from files in a FS partition)
  - exposes: `ListAnimations()`, `OpenAnimation(name)`, `ReadFrame(i)` or `DecodeNextFrame()`
- **Serial control subsystem**
  - reads lines from console and parses commands
  - updates playback state machine (select/stop)
- **Playback subsystem**
  - a single task/loop that:
    - waits for `PLAYING` state
    - renders next frame into the canvas
    - presents and schedules the next frame according to delay metadata

### Concurrency model (simple + robust)

- **Playback task** owns the display + canvas (single writer).
- **Console task** only mutates a small shared state (atomic/queue) to request changes:
  - `RequestedAnimation`, `RequestedAction` (play/stop)

Using an event queue (`QueueHandle_t`) is fine; for MVP even a single atomic “current selection” works.

## Serial console: two implementation options

This section is intentionally explicit about `esp_console`, because it’s one of those ESP-IDF facilities that can save you a lot of time (line editing + command parsing + help) if you embrace it early. It also clarifies what you don’t get if you go with a hand-rolled “read bytes until newline” loop.

### Option 1: `esp_console` REPL over USB Serial/JTAG (recommended)

Pros:

- Built-in line editing + command registration
- Easy `help` and argument parsing
- Can target USB Serial/JTAG directly (`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG`)

The repo already vendors a helper that creates a REPL:

- `console_simple_init.c` (supports UART / USB CDC / USB Serial/JTAG)

#### What `esp_console` gives you (practically)

`esp_console` is more than “a place to `printf`”. It is a small REPL framework that provides:

- **Command registration**: register commands like `list`, `play`, `stop`, `info` with help strings.
- **Line editing**: backspace, history, and a prompt (depending on build/config).
- **Argument parsing**: parse `play 3` or `play nyan_cat` without writing your own tokenizer.
- **Built-in help**: `help` and per-command usage help is easy to wire up.
- **Multiple transports**: UART, USB CDC, or USB Serial/JTAG depending on `sdkconfig` (see `console_simple_init.c`).

In practice this means the “serial control plane” can stay small and declarative: you implement command handlers, not a terminal emulator.

### Option 2: simple line reader over USB Serial/JTAG driver

Pros:

- Smaller dependency surface
- Full control of buffering and non-blocking read

Cons:

- You must implement line buffering, backspace, and prompt output (unless you accept “dumb” input).

If you do this, reuse the “direct USB-Serial-JTAG write” approach from tutorial `0007` (Cardputer), adapted for AtomS3R.

## Input UX: “next animation” button

Serial control is great for development and automation, but for a demo it’s also useful to have a single physical control: **press button → next animation**. This also acts as a sanity test that the playback loop can switch animations safely without tearing or crashing.

For AtomS3R, the safest default is to treat the **BOOT button** (often `GPIO0`) as the “next” button, because it’s physically present on most ESP32-S3 dev boards. If the board exposes a different “display click” button (for example in some vendor firmware, a `HAL_PIN_BUTTON_A`), we should make the GPIO configurable via Kconfig so we can pick the right pin per board revision.

Recommended implementation pattern (from our existing repo code):

- Use GPIO interrupts + a FreeRTOS queue (see `esp32-s3-m5/0003-gpio-isr-queue-demo/main/hello_world_main.c`)
- Debounce in the consumer task (e.g. ignore events within 50ms)
- On “pressed edge”, enqueue a `NEXT_ANIMATION` request
- The playback task receives that request and switches animations (preferably at a frame boundary)

The key invariant: the button handler must never touch the display buffer directly; it should only signal the playback controller.

## Assets on flash: how to bundle animations

### Option A: custom raw partition (AssetPool-style)

There is an existing pattern in `ATOMS3R-CAM-M12-UserDemo`:

- Partition table includes `assetpool` (custom type `233/0x23`)
- Host flashes assets via:
  - `parttool.py ... write_partition --partition-name=assetpool --input AssetPool.bin`

For this ticket, we can reuse the same concept:

- create an `animpack` partition (or reuse `assetpool`)
- flash a single packed binary containing:
  - directory (names + offsets)
  - per-animation metadata
  - frame data (compressed or not)

#### AssetPool deep dive: what it is and how “individual assets” are addressed

This subsection writes down the AssetPool pattern as it exists in this repo so we can reuse the good parts without inheriting accidental complexity.

In `ATOMS3R-CAM-M12-UserDemo`, “AssetPool” is not a filesystem and it’s not a directory of individually addressable files. It is a **single C struct** that is:

- generated on the host into a flat `AssetPool.bin`, and
- memory-mapped on the device from a custom flash partition, and
- accessed by **field offsets** (C struct layout), not by names/paths.

That design is excellent for a fixed set of web assets (an HTML gzip blob and a logo), because it avoids a filesystem and avoids runtime allocation. It is less convenient for “a growing list of GIFs” because there is no self-describing directory unless you build one into the blob.

##### How the AssetPool is created (host-side)

The generator creates a `StaticAsset_t` struct whose members are fixed-size byte arrays (see `main/utils/assets/images/types.h`). It then copies raw bytes from files into those arrays and writes the entire struct to disk:

- `AssetPool::CreateStaticAsset()` fills the struct by copying file bytes into fields
- `AssetPool::CreateStaticAssetBin()` writes the struct to `AssetPool.bin`

##### How the AssetPool is loaded (device-side)

At runtime, the firmware:

- finds the partition by custom type/subtype (233 / 0x23),
- memory-maps the partition (`esp_partition_mmap`), and
- treats the base address as a `StaticAsset_t *`, which is injected into the singleton via `AssetPool::InjectStaticAsset(...)`.

From there, code uses the struct fields directly, e.g. `AssetPool::GetImage().index_html_gz`.

##### How to “find an individual asset”

You “find” an asset by:

- the **field name** in the struct (e.g. `index_html_gz`, `m5_logo`), and
- the **field length** (the array size in `types.h`).

There is no runtime lookup table. The offset is the compiler’s struct layout; the size is the array length in the struct definition.

##### Real-world implications / gotchas

- **Size coupling is fragile**: if you update the source file and forget to update the struct field size, you will truncate or overflow.
- **Consumers can accidentally use the wrong length**: callers must pass the correct field length (not a stale hard-coded number), otherwise they risk out-of-bounds reads.
- **No directory**: to support `list`/`play` by name, you’ll want a directory table (names + offsets) either in a purpose-built animation pack format, or embedded into the blob.

For ticket 008, AssetPool is a useful *pattern reference* (flash a blob to a partition; mmap it; treat it as readonly), but we should likely build a purpose-built **animation pack format** rather than a monolithic C struct full of raw arrays.

### Option B: file system partition (SPIFFS/FATFS)

Pros:

- “Natural” to store `*.gif` or `*.anim` files
- Easy to add/remove animations without rebuilding firmware (only re-flash FS image)

Cons:

- Need FS image build step (and choose FATFS vs SPIFFS vs LittleFS)
- Slightly more runtime complexity (file I/O)

## GIF decoding strategy: what to do on-device

### Strategy 1: decode GIF on-device (flash-efficient)

Store the original GIF bytes (often much smaller than raw frames) and decode on-device with a small GIF decoder library (e.g. `gifdec`-style). This is attractive if you want a “list of GIFs” without blowing flash.

Tradeoff: you need to integrate a decoder and manage palette + transparency + per-frame disposal correctly.

### Strategy 2: pre-convert GIF to a device-friendly frame pack (CPU-efficient)

Convert GIFs offline to a predictable format (e.g., RGB565 frames + delays). Playback becomes:

- copy/decode frame into canvas buffer
- push to LCD

Tradeoff: raw RGB565 is large. A single 128×128 RGB565 frame is \(128 \cdot 128 \cdot 2 = 32768\) bytes, so even a “small” animation can consume megabytes unless compressed.

## Recommended MVP path (minimize risk)

- Use **M5GFX canvas** (start from `0010`) for stable display + present semantics.
- Use **esp_console REPL** over **USB Serial/JTAG** for commands (so we don’t hand-roll line editing).
- Start with **pre-converted frame packs**, but require one of:
  - paletted frames (8-bit indices + palette)
  - or per-frame compression (RLE/LZ4)
so we can actually fit multiple animations into flash.

Once the workflow is stable, consider switching to true GIF decode if “many GIFs” is the priority.

## Suggested next implementation chapter

Create a new tutorial project (likely):

- `esp32-s3-m5/0013-atoms3r-gif-console/`

Milestones:

- bring-up display + backlight (copy `0010`)
- start console and implement `list/play/stop/info`
- play a built-in tiny animation (2–3 frames) from flash to validate the end-to-end loop
- integrate the real pipeline from GIFs (see the asset pipeline doc)

