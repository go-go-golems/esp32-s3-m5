---
Title: 'AtomS3R GC9107 black screen: retracing steps vs M5GFX and esp_lcd'
Ticket: 001-MAKE-GFX-EXAMPLE-WORK
Status: active
Topics:
    - esp32s3
    - esp-idf
    - display
    - gc9107
    - m5gfx
    - atoms3r
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-22T18:19:58.946538402-05:00
WhatFor: ""
WhenToUse: ""
---

## Goal

Explain why our ESP-IDF `esp_lcd` + `espressif__esp_lcd_gc9107` tutorial (`esp32-s3-m5/0008-atoms3r-display-animation`) can log a “successful” GC9107 bring-up (panel created/reset/inited, gap set, display on, backlight I2C write OK) yet still render a **black screen** on AtomS3R hardware — while the PlatformIO UserDemo keeps working.

This document is deliberately “lots of background and thinking”: it captures assumptions, what changed, what we learned from M5GFX, and what we should test next.

## Executive summary (current best hypothesis)

Even with `gap=(0,32)` and RGB565 byte-swapping enabled, **two likely missing compatibility details remain**:

- **Backlight enable is not purely I2C on all AtomS3R revisions**: schematics show `LED_BL` gated by a **GPIO-controlled FET (GPIO7, active-low)**. If that gate is off, you can see “nothing” even if I2C writes succeed (or appear to).
- **GC9107 vendor init sequence differences can leave the panel black**: M5GFX’s GC9107 init includes an “unlock” preamble (`0xFE`, `0xEF`) and different gamma tables than `esp_lcd_gc9107`’s default list. Some modules require the unlock sequence (or specific gamma/power settings) to actually light pixels.

We implemented both as toggles in the tutorial:
- **I2C mode + gate GPIO** (default GPIO7 active-low) to satisfy the “FET gated” hardware model
- **M5GFX-style vendor init sequence** via `gc9107_vendor_config_t`

## What we observed (on-device)

Manuel’s log shows a clean bring-up path:

- I2C writes to backlight device appear to succeed: `addr=0x30 reg=0x0e value=0/255`
- GC9107 panel created: `gc9107: LCD panel create success, version: 2.0.0`
- Reset/init completed, `disp_on_off(true)` called
- Gap correctly set: `gap=(0,32)`
- RGB565 byte swap enabled

Yet the screen is visually black (no animation), while UserDemo (PIO/Arduino/M5GFX) works.

### Update: backlight gate + M5GFX init enabled (backlight returns, still black)

After enabling:
- backlight gate GPIO (GPIO7 active-low), and
- M5GFX-style vendor init commands (`0xFE/0xEF` + M5GFX gamma tables),

the backlight reliably turns on again and the render loop heartbeat confirms frames are being produced — but the display content is still reported as black.

This is strong evidence that “black screen” is now **not** simply “backlight is off”, and pushes suspicion toward:
- SPI command/data semantics (DC polarity, CS behavior, or command/data framing)
- or a remaining init-state mismatch (invert/BGR, or a required additional vendor command not captured)

## Ground truth: what M5GFX does (and what it implies)

### GC9107 memory geometry and offset

In M5GFX, `Panel_GC9107` is configured as **128×160 memory**, while Atom boards set the visible window with an offset.

In `M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.cpp` (GC9107 detection block), M5GFX explicitly sets:

- `cfg.panel_width = 128`
- `cfg.panel_height = 128`
- `cfg.offset_y = 32`

This matches our `esp_lcd_panel_set_gap(..., 0, 32)` approach.

### Vendor init sequence differences (M5GFX vs esp_lcd_gc9107)

M5GFX `Panel_GC9107` init sequence (in `Panel_GC9A01.hpp`) includes:

- `0xFE` (delay)
- `0xEF` (delay)
- then the vendor list (`0xB0`, `0xB2`, ... `0xF0`, `0xF1`, ...)

Espressif’s managed driver `managed_components/espressif__esp_lcd_gc9107/esp_lcd_gc9107.c`:

- sends `SLPOUT` then `MADCTL` then `COLMOD`
- then uses a default vendor init list **that differs**:
  - it includes `0x21` (INVON) by default
  - it does **not** include `0xFE/0xEF`
  - gamma tables differ

**Implication**: “successfully initialized” in logs does not mean “panel in the expected power/gamma state”. This can produce black output, washed output, or unstable output depending on module variant.

### Backlight model ambiguity (I2C vs gate GPIO)

We previously assumed AtomS3R backlight was an I2C brightness device at `0x30`/`0x0e` (derived from M5GFX deep dive notes and earlier research).

However, schematic analysis (`reference/03-schematic-claude-answers.md`) indicates:

- `LED_BL` is gated by a PMOS FET controlled by **GPIO7** (active-low)

**Implication**: It’s plausible the actual board requires **both**:
- enable the gate GPIO, and
- optionally set brightness via I2C (if present)

This ambiguity explains “backlight used to work, now it doesn’t” across config changes and power cycles: if the physical backlight enable path is GPIO-gated and we don’t drive it, the screen remains black regardless of SPI correctness.

## What we changed in the ESP-IDF tutorial (0008) while debugging

### 1) Fixed the obvious mapping issues

- set default `y_offset=32`
- added RGB565 per-pixel byte swap (default enabled)
- added heartbeat logs so we can tell the render loop is alive

These were necessary but not sufficient.

### 2) Added M5GFX compatibility toggles

We added:

- `TUTORIAL_0008_USE_M5GFX_INIT_CMDS`
  - uses `gc9107_vendor_config_t` to pass the M5GFX vendor init list (including `0xFE/0xEF`)
- `TUTORIAL_0008_BL_I2C_USE_GATE_GPIO` + `TUTORIAL_0008_BL_I2C_GATE_GPIO` (default 7 active-low)
  - in I2C mode, we also toggle a gate GPIO OFF during init and ON after init

## How to proceed (recommended next experiments)

### Experiment A: “Force backlight gate ON and confirm physical light”

- Enable the I2C mode gate GPIO option.
- In logs, confirm you see `backlight gate gpio: ... -> ON`.
- If the LCD stays black, shine a flashlight at the panel:
  - **If you can faintly see pixels**: backlight path is still off or brightness control differs.
  - **If you can’t see pixels at all**: SPI/init sequence is likely still wrong.

### Experiment B: “Use M5GFX init sequence”

- Enable `TUTORIAL_0008_USE_M5GFX_INIT_CMDS`.
- Keep `gap=32`, `byteswap=1`, SPI mode 0.
- If still black, try toggling:
  - `invert` (some panels require INVON, others require INVOFF)
  - `BGR/RGB`
  - lower SPI clock to 26MHz (signal integrity)

### Experiment C: sanity check we are actually writing to GRAM

Add a hard-coded full-screen fill after init:

- Fill framebuffer with all-white (`0xFFFF`) for 1s, then all-red, then all-green, etc.
- If you never see a solid color, you’re not writing to GRAM (address window, DC, CS, or init state mismatch).

## What warrants a second pair of eyes

- The correct “truth” about AtomS3R backlight hardware: is it I2C-only, GPIO-only, or both?
- Whether `esp_lcd_gc9107` already expects pre-swapped RGB565 and whether our swap duplicates or is required.
- Whether the M5GFX vendor init list should replace or augment `MADCTL/COLMOD` handling in the managed driver (we currently keep those in the driver).

## Sources / key files

- M5GFX init reference: `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/panel/Panel_GC9A01.hpp` (contains `Panel_GC9107`)
- M5GFX board configuration: `M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.cpp` (shows `cfg.offset_y = 32` for GC9107)
- ESP-IDF managed driver: `esp32-s3-m5/0008-atoms3r-display-animation/managed_components/espressif__esp_lcd_gc9107/esp_lcd_gc9107.c`
- Our tutorial: `esp32-s3-m5/0008-atoms3r-display-animation/main/hello_world_main.c`
- GC9107 datasheet: `M5AtomS3-UserDemo/datasheets/GC9107_DataSheet_V1.2.pdf`

