---
Title: Investigation Diary
Ticket: ESP32-P4-PICOCALC-LVGL
Status: active
Topics:
    - esp32-p4
    - picocalc
    - lvgl
    - display
    - firmware-port
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Diary for LVGL demo investigation on ESP32-P4 PicoCalc"
LastUpdated: 2026-06-01T23:15:00-04:00
WhatFor: "Track LVGL demo design and investigation"
WhenToUse: "Resume or review the LVGL demo effort"
---

# Diary

## Goal

Track the creation of a design guide for running an LVGL demo on the ESP32-P4 PicoCalc, covering the display driver integration path, what carries over from existing M5Stack LVGL projects, and the ST7365P/ILI9488-specific challenges.

## Step 1: Ticket Created and Design Guide Written

Created the ticket and wrote a detailed intern-facing design guide. Key findings:

- Two viable integration paths exist: the `esp_lcd` panel IO path (recommended) and the manual `spi_master` path.
- The `esp_lcd` path uses `esp_lcd_panel_io_spi` which manages the DC GPIO internally and supports queued transactions — a major advantage over the manual path where DC must be managed explicitly.
- An ILI9488 panel driver component (`atanisoft/esp_lcd_ili9488`) exists on the ESP Component Registry.
- The ST7365P vendor unlock sequence (`0xF0` + keys) is the main risk — standard ILI9488 drivers may not include it.
- The existing Cardputer LVGL projects (0025, 0047) provide architectural patterns but M5GFX cannot be reused. A new `lvgl_port_picocalc` module is needed.
- The 40-line single-buffered display buffer strategy from the Cardputer projects carries over directly.

### What I did

- Created ticket ESP32-P4-PICOCALC-LVGL.
- Added design doc and diary doc.
- Gathered LVGL ESP-IDF integration docs, `esp_lcd` API references, ILI9488 component info, and the ESP-IDF SPI LCD+LVGL example.
- Stored research sources in `sources/`.
- Analyzed existing Cardputer LVGL projects (0025, 0047) for reusable patterns.
- Wrote the design guide with two integration paths, buffer strategy, keyboard input device, phase plan, and risk analysis.

### What was tricky to build

The main uncertainty is the ST7365P vendor unlock. The standard ILI9488 init sequence assumes RGB565 works after `COLMOD 0x55`, but the ST7365P silently ignores this without the vendor unlock (`0xF0` with keys `0xC3` and `0x96`). If the standard ILI9488 component does not include this, a pre-init hook or a custom panel driver fork will be needed.

The second uncertainty is whether `esp_lcd_panel_io_spi` can drive the PicoCalc at 80 MHz with `SPI_CLK_SRC_SPLL`. The current manual driver works at this speed, but `esp_lcd_panel_io_spi` may have different internal clock source handling.

### What should be done in the future

- Create the `0100-esp32-p4-picocalc-lvgl-demo` project.
- Test the `atanisoft/esp_lcd_ili9488` component and check if it includes the ST7365P vendor unlock.
- If not, add the vendor unlock via `esp_lcd_panel_io_tx_param()` before panel init.
- Implement the LVGL flush callback and keyboard input device.
- Run a visual demo.
