---
Title: Investigation Diary
Ticket: ESP32-P4-PICOCALC-BACKLIGHT
Status: active
Topics:
    - esp32-p4
    - picocalc
    - backlight
    - lcd
    - i2c
    - firmware-port
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Diary for LCD backlight control design on ESP32-P4 PicoCalc"
LastUpdated: 2026-06-01T22:30:00-04:00
WhatFor: "Track backlight control design and investigation"
WhenToUse: "Resume or review the backlight control design effort"
---

# Diary

## Goal

Track the creation of a design guide for LCD backlight control on the ESP32-P4 PicoCalc.

## Step 1: Ticket Created and Design Guide Written

Created the ticket and wrote a detailed intern-facing design guide. Key discovery: the PicoCalc LCD backlight is NOT a direct GPIO PWM signal. It is controlled through the STM32 southbridge I2C register 0x05 (values 0-255). The ESP32-P4 only needs to write a single I2C register byte to control brightness.

### What I did

- Created ticket ESP32-P4-PICOCALC-BACKLIGHT.
- Added design doc and diary doc.
- Gathered LEDC PWM docs (for reference only), PicoCalc STM32 register map.
- Stored research sources in `sources/`.
- Wrote the design guide with backlight API, console commands, idle dimming policy, and safety considerations.

### What was tricky to build

The biggest insight is that this is NOT a GPIO PWM task. The STM32 southbridge owns the backlight PWM output. The ESP32-P4 writes an I2C register, and the STM32 handles the actual PWM generation.

### What should be done in the future

- Add a southbridge I2C register write function to the keyboard driver.
- Test register 0x05 and verify visible brightness changes.
- Test register 0x0A for keyboard backlight (may not be present).
