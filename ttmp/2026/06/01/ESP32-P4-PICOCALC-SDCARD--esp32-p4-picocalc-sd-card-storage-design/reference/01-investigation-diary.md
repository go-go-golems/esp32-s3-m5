---
Title: Investigation Diary
Ticket: ESP32-P4-PICOCALC-SDCARD
Status: active
Topics:
    - esp32-p4
    - picocalc
    - sd-card
    - storage
    - firmware-port
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Diary for SD card storage design on ESP32-P4 PicoCalc"
LastUpdated: 2026-06-01T22:30:00-04:00
WhatFor: "Track SD card design and investigation"
WhenToUse: "Resume or review the SD card design effort"
---

# Diary

## Goal

Track the creation of a design guide for SD card storage on the ESP32-P4 PicoCalc, covering both the Waveshare onboard SDMMC path and the PicoCalc SPI SD path.

## Step 1: Ticket Created and Design Guide Written

Created the ticket and wrote a detailed intern-facing design guide covering the two SD card paths available to the ESP32-P4 PicoCalc: the Waveshare onboard SDMMC TF slot (4-bit, high-speed, GPIO39-44) and the PicoCalc full-size SD slot via SPI through the same-position adapter (RP2040 GP16-19, GP22).

### What I did

- Created ticket ESP32-P4-PICOCALC-SDCARD.
- Added design doc and diary doc.
- Gathered research from ESP-IDF SDMMC/SDSPI driver docs, Waveshare board docs, and PicoCalc hardware references.
- Stored research sources in `sources/` directory.
- Wrote the design guide with API sketch, phases, and testing strategy.

### What was tricky to build

The PicoCalc SPI SD pin mapping through the same-position adapter is unknown. The Waveshare ESP32-P4-WIFI6 (SKU 32020) has a different 40-pin header GPIO assignment than the DEV-KIT B (SKU 30843), so the DEV-KIT B pinout reference cannot be used directly.

### What should be done in the future

- Discover the PicoCalc SPI SD GPIO mapping.
- Check if 0098 webserver firmware has reusable SDMMC init code.
- Implement Phase 1: Waveshare SDMMC slot.
