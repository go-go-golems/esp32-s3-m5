---
Title: ATOMS3R Display (schematics) — questions for specialist
Ticket: 002-M5-ESPS32-TUTORIAL
Status: active
Topics:
    - esp32
    - m5stack
    - freertos
    - esp-idf
    - tutorial
    - grove
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-21T18:34:16.734167721-05:00
WhatFor: ""
WhenToUse: ""
---

# ATOMS3R Display (schematics) — questions for specialist

## Goal



## Context

- We want a tutorial that **renders a looping animation** on **M5Stack ATOMS3R**.
- We have two sources of truth:
  - The ATOMS3R schematic (rendered PNGs in `RelatedFiles`)
  - An Arduino demo using `M5Unified` (`M5AtomS3-UserDemo/src/main.cpp`) that successfully draws primitives (useful for resolution/orientation hints).
- From the schematic we can already identify most **control pins** for the display connector, but some important details (panel controller, backlight drive) still need confirmation.

## Quick Reference

### What we can already read from the ATOMS3R schematic

Display connector `J1` signals (from `atoms3r_display_pins_crop.png`):

| J1 pin | Net name | ESP32-S3 GPIO | Notes |
| ------ | -------- | ------------- | ----- |
| 1 | `LED_BL` | (not shown at J1) | Backlight control net; **needs confirmation** how it’s driven |
| 2 | `GND` | — | — |
| 3 | `DISP_RST` | `GPIO48` | Panel reset |
| 4 | `DISP_RS` | `GPIO42` | Data/Command (D/C) line (often called RS) |
| 5 | `SPI_MOSI` | `GPIO21` | SPI MOSI to panel |
| 6 | `SPI_SCK` | `GPIO15` | SPI clock to panel |
| 7 | `VDD_3V3` | — | Panel power |
| 8 | `DISP_CS` | `GPIO14` | SPI chip-select to panel |

### Helpful hint from the Arduino demo

The ATOMS3R Arduino demo uses `M5Unified` drawing calls like `M5.Display.fillArc(...)` and uses `M5.Display.width()/height()` when creating a canvas (`M5AtomS3-UserDemo/src/main.cpp`). This suggests the board display is initialized and functional under M5’s default board config; we can use it to cross-check **resolution** and **rotation/origin** once the panel controller is confirmed.

## Usage Examples

### Send to schematics specialist (copy/paste)

Please answer/confirm the questions in the next section and (if possible) annotate the schematic with the relevant nets/pins.

## Questions for schematics specialist

### A) Panel identification / capabilities (highest priority)

1. **What is the display panel controller IC?** (e.g., ST7789/GC9A01/etc) and exact panel part (if known).
2. **What is the native resolution?** (e.g., 128×128) and is there any **controller RAM window offset** (common on ST77xx panels).
3. **What is the required pixel format / color order?**
   - RGB565 vs other
   - RGB vs BGR order
4. **What SPI mode / max SPI clock** is expected for reliable operation?

### B) Backlight control (needs clarification)

5. `LED_BL` (J1 pin 1): **How is backlight driven?**
   - Is `LED_BL` directly from an ESP32 GPIO? If yes, which GPIO?
   - Or is it driven via a transistor / LED driver (e.g. `LP5562`)? If yes, what’s the control input?
6. Is backlight control **active-high or active-low**? Any default pull-ups/pull-downs?
7. Is backlight **PWM capable** (intended), or only on/off?

### C) SPI bus topology / other signals

8. Is the display interface strictly **4-wire SPI** (SCK/MOSI/CS/D-C/RST), or is there also:
   - `MISO` (SPI readback)
   - `TE` (tearing effect sync)
   - any other aux signal
9. Are `GPIO14/15/21/42/48` **shared with any other peripheral** on the board?
10. Any reason these pins have constraints (bootstraps, reserved, etc.) for ATOMS3R specifically?

### D) Power/reset sequencing

11. Any requirements for **power sequencing** (e.g., backlight after panel init)?
12. Recommended **reset timing** (how long to hold `DISP_RST`, any RC delay, etc.)?

### E) Sanity checks vs known working software

13. In the official M5 board support (M5Unified), what’s the **expected rotation** and origin (0,0) relative to the physical device?
14. Any known **x/y offsets** used by M5’s config that we should replicate in an ESP-IDF `esp_lcd_panel` init?

## Related

- Schematic PNG:
  - `atoms3r_schematic_v0.4.1.png`
  - `atoms3r_display_pins_crop.png`
- Arduino reference:
  - `M5AtomS3-UserDemo/src/main.cpp`
