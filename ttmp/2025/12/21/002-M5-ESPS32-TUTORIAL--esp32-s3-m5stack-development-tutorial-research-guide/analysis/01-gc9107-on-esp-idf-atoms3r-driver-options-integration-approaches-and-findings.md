---
Title: 'GC9107 on ESP-IDF (AtomS3R): driver options, integration approaches, and findings'
Ticket: 002-M5-ESPS32-TUTORIAL
Status: active
Topics:
    - esp32
    - m5stack
    - freertos
    - esp-idf
    - tutorial
    - grove
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-22T09:37:43.56476044-05:00
WhatFor: ""
WhenToUse: ""
---

# GC9107 on ESP-IDF (AtomS3R): driver options, integration approaches, and findings

## Executive summary

For **AtomS3R** (GC9107 LCD), “pure ESP-IDF `esp_lcd`” is viable **without** adopting M5Unified/M5GFX: Espressif publishes an official, component-manager installable driver component: **`espressif/esp_lcd_gc9107`** (exposes an `esp_lcd`-style panel constructor). This avoids writing a panel driver from scratch and keeps the tutorial series aligned with ESP-IDF.

However, to make graphics “match the real deal,” you still must apply the **geometry quirks** that M5GFX knows about:

- The visible area is **128×128**, but the controller memory geometry is **128×160**.
- M5GFX uses a **Y gap/offset of 32** so that the visible 128×128 window maps correctly.

This doc lays out the options and a recommended integration plan.

## What we know about AtomS3R LCD from the “real deal” M5 stack

From the deep dive docs in ticket `003-ANALYZE-ATOMS3R-USERDEMO`:

- **LCD controller**: GC9107 (auto-detected via `RDDID` / 0x04).
- **Visible resolution**: 128×128.
- **Controller memory**: 128×160, with **`offset_y = 32`** for visible region.
- **SPI pins (AtomS3R)** (per M5GFX auto-detect config):
  - MOSI = GPIO21
  - SCLK = GPIO15
  - DC   = GPIO42
  - CS   = GPIO14
  - RST  = GPIO48
  - SPI host: `SPI3_HOST`
- **Backlight (per M5GFX)**:
  - Controlled by `Light_M5StackAtomS3R` via I2C (GPIO45 + GPIO0)
  - Device address `0x30`, brightness register `0x0e`

Important: the schematic interpretation doc (`03-schematic-claude-answers.md`) also proposes a GPIO-driven backlight path. Treat this as a hardware-revision risk: prefer “real deal” M5GFX behavior unless confirmed otherwise on the physical device.

## The core problem in “pure ESP-IDF”

ESP-IDF’s built-in `esp_lcd` component (IDF 5.4.1) includes panel drivers like ST7789/SSD1306/etc, but does **not** ship GC9107 in-tree.

So you must choose one of:

1. Install **an external `esp_lcd`-compatible GC9107 panel driver component** (best).
2. Port a GC9107 driver yourself (more work).
3. Vendor M5GFX/LovyanGFX into IDF and use their proven stack (works, but not “pure IDF”).

## Recommended approach (AtomS3R tutorial series)

### Option A (recommended): Use Espressif `esp_lcd_gc9107` component

Install via ESP-IDF component manager:

```bash
idf.py add-dependency "espressif/esp_lcd_gc9107^2.0.0"
```

Then use the normal `esp_lcd` flow:

- `spi_bus_initialize(...)`
- `esp_lcd_new_panel_io_spi(...)`
- `esp_lcd_new_panel_gc9107(...)`
- `esp_lcd_panel_init(...)`
- `esp_lcd_panel_set_gap(0, 32)` to match M5GFX’s visible window mapping

Pros:
- Stays in ESP-IDF world; clean for tutorials.
- Uses Espressif-maintained driver code.

Cons / open items:
- You still need to validate color order, rotation, and any additional panel quirks.
- Backlight control is board-specific; driver only covers the panel.

### Option B: Use ESP-IoT-Solution display support

Espressif’s ESP-IoT-Solution user guide lists GC9107 as supported and contains related driver code.

Pros:
- “Batteries included” for some display types.

Cons:
- Additional dependency surface area; less minimal for our tutorial repo.

### Option C: Vendor LovyanGFX/M5GFX (the exact M5 path)

M5GFX already:
- detects GC9107 with `RDDID`,
- applies `offset_y = 32`,
- uses a highly optimized SPI write path.

Pros:
- Highest confidence of matching M5 behavior.

Cons:
- Heavier dependency tree.
- Less “ESP-IDF-native” for tutorial goals.

## Practical integration checklist for AtomS3R

- **Pins**: ensure your `spi_bus_config_t` and `esp_lcd_panel_io_spi_config_t` match AtomS3R wiring.
- **SPI clock**: M5 uses 40MHz; start at 40MHz and drop if unstable.
- **Y gap**: set `y_gap = 32` to map 128×128 visible area into 128×160 controller RAM.
- **Color order**: be ready to toggle RGB/BGR.
- **Backlight**:
  - Implement as a board-specific layer:
    - I2C brightness (0x30 / reg 0x0e) per M5GFX, or
    - GPIO-driven enable if your specific schematic/hardware revision proves that.

## Sources (online)

- Espressif component: `esp_lcd_gc9107`:
  - `https://components.espressif.com/components/espressif/esp_lcd_gc9107`
- ESP-IoT-Solution user guide (mentions GC9107):
  - `https://docs.espressif.com/projects/esp-iot-solution/en/latest/esp-iot-solution-en-master.pdf`
- GC9107 datasheet mirrors:
  - `https://files.waveshare.com/wiki/0.85inch-LCD-Module/GC9107_DataSheet_V1.2.pdf`
  - `https://www.buydisplay.com/download/ic/GC9107.pdf`
- Arduino_GFX (community GC9107 usage, not ESP-IDF-native but useful for controller behavior comparisons):
  - `https://github.com/moononournation/Arduino_GFX`

## Related documents in this repo

- Ticket 002 schematic pin crop (AtomS3R):
  - `.../reference/atoms3r_display_pins_crop.png`
- Schematic interpretation summary:
  - `.../reference/03-schematic-claude-answers.md`
- M5 stack deep dive (authoritative for “real deal” behavior):
  - `.../003-ANALYZE-ATOMS3R-USERDEMO.../analysis/02-m5unified-m5gfx-deep-dive-display-stack-and-low-level-lcd-communication.md`
