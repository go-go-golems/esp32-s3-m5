---
Title: 'Guide: AtomS3R GC9107 bring-up (esp_lcd)'
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - esp32
    - firmware
    - documentation
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: 'Expanded research and writing guide for AtomS3R GC9107 bring-up using esp_lcd.'
LastUpdated: 2026-01-11T19:44:01-05:00
WhatFor: 'Help a ghostwriter draft the AtomS3R GC9107 esp_lcd bring-up doc.'
WhenToUse: 'Use before writing the esp_lcd display bring-up document.'
---

# Guide: AtomS3R GC9107 bring-up (esp_lcd)

## Purpose

This guide enables a ghostwriter to create a single-topic doc about the AtomS3R GC9107 display using ESP-IDF `esp_lcd`. The doc should be practical: exact pin map, known offsets, backlight control modes, and the key menuconfig knobs to fix black screens or color issues.

## Assignment Brief

Write a tutorial-style reference that walks a developer from a blank screen to a working animation. The emphasis is on the GC9107 panel quirks: 128x160 RAM with a 128x128 visible area, a Y offset, and backlight control that varies by hardware revision.

## Environment Assumptions

- ESP-IDF 5.4.1
- AtomS3R hardware (GC9107 panel)
- USB Serial/JTAG console access

## Source Material to Review

- `esp32-s3-m5/0008-atoms3r-display-animation/README.md`
- `esp32-s3-m5/0008-atoms3r-display-animation/main/idf_component.yml` (GC9107 component)
- `esp32-s3-m5/0008-atoms3r-display-animation/animation-preview.html`
- `esp32-s3-m5/ttmp/2025/12/22/001-MAKE-GFX-EXAMPLE-WORK--make-gc9107-m5gfx-style-display-example-work-atoms3r/reference/01-diary.md` (contrast with M5GFX if needed)

## Technical Content Checklist

- Wiring pin map (CS/SCK/MOSI/RS/RST) from the README
- GC9107 128x160 RAM vs 128x128 visible area; Y offset 32
- Backlight control options:
  - I2C brightness device (`0x30`, reg `0x0e`, SCL=GPIO0, SDA=GPIO45)
  - GPIO gate (GPIO7 active-low) if present
- Menuconfig knobs: color order, RGB565 byte swap, SPI clock, offsets, frame delay
- Component dependency: `espressif/esp_lcd_gc9107`

## Pseudocode Sketch

Show a minimal init and draw loop so readers can spot where offsets and color order are applied.

```c
// Pseudocode: esp_lcd bring-up on GC9107
init_spi_bus(SCK, MOSI, CS, DC, RST)
init_panel_gc9107()
set_panel_offset(x=0, y=32)
set_color_order(RGB)
set_byte_swap(true)
set_backlight_mode(i2c_or_gpio)
loop:
  draw_frame(buffer_rgb565)
  esp_lcd_panel_draw_bitmap(buffer)
```

## Pitfalls to Call Out

- Wrong byte order or color order yields noise or swapped colors.
- Offsets must match the visible window; otherwise the image is shifted.
- Panel init can look correct while the screen stays black; backlight is often the culprit.

## Suggested Outline

1. Hardware assumptions and pin map
2. Panel geometry and offsets
3. Backlight modes and how to select them
4. `esp_lcd` init sequence and draw loop
5. Validation tips (expected animation)
6. Troubleshooting checklist

## Commands

```bash
cd esp32-s3-m5/0008-atoms3r-display-animation
idf.py set-target esp32s3
idf.py build flash monitor
```

## Exit Criteria

- Doc includes pin map and offset explanation.
- Doc includes a short menuconfig checklist with key options.
- Doc includes a black screen troubleshooting section.

## Notes

Keep the focus on `esp_lcd` bring-up. Mention M5GFX only to explain why this path exists.
