---
Title: XIAO ESP32C6 Pin Mapping
Ticket: 0065-GPIO-WEB-SERVER
Status: active
Topics:
    - esp-idf
    - esp32c6
    - wifi
    - webserver
    - http
    - gpio
    - console
    - usb-serial-jtag
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0065-xiao-esp32c6-gpio-web-server/main/Kconfig.projbuild
      Note: Firmware pin defaults depend on this mapping
ExternalSources:
    - https://wdcdn.qpic.cn/MTY4ODg1Nzc0ODUwMjM3NA_318648_dMoXitoaQiq2N3-a_1711678067?w=1486&h=1228
Summary: Full XIAO ESP32C6 board pin map (XIAO pin ↔ ESP32-C6 GPIO + common functions).
LastUpdated: 2026-01-21T22:53:50.404456772-05:00
WhatFor: ""
WhenToUse: ""
---


# XIAO ESP32C6 Pin Mapping

## Goal

Capture a single source-of-truth mapping from “XIAO pin labels” (D0..D10, etc.) to ESP32‑C6 GPIO numbers, so firmware defaults and documentation don’t drift.

## Context

The XIAO ESP32C6 board silkscreen uses Arduino-like labels (`D0`, `D1`, …). ESP-IDF uses GPIO numbers. This doc bridges the two.

## Quick Reference

Transcribed from the provided pin map image (see ExternalSources).

### Digital/analog pins (XIAO header)

| XIAO Pin | Function | Chip Pin | Alternate Functions | Description |
|---|---|---:|---|---|
| 5V | VBUS |  |  | Power Input/Output |
| GND |  |  |  |  |
| 3V3 | 3V3_OUT |  |  | Power Output |
| D0 | Analog | GPIO0 | LP_GPIO0 | GPIO, ADC |
| D1 | Analog | GPIO1 | LP_GPIO1 | GPIO, ADC |
| D2 | Analog | GPIO2 | LP_GPIO2 | GPIO, ADC |
| D3 | Digital | GPIO21 | SDIO_DATA1 | GPIO |
| D4 | SDA | GPIO22 | SDIO_DATA2 | GPIO, I2C Data |
| D5 | SCL | GPIO23 | SDIO_DATA3 | GPIO, I2C Clock |
| D6 | TX | GPIO16 |  | GPIO, UART Transmit |
| D7 | RX | GPIO17 |  | GPIO, UART Receive |
| D8 | SCK | GPIO19 | SDIO_CLK | GPIO, SPI Clock |
| D9 | MISO | GPIO20 | SDIO_DATA0 | GPIO, SPI Data |
| D10 | MOSI | GPIO18 | SDIO_CMD | GPIO, SPI Data |

### JTAG + control pins

| XIAO Pin | Chip Pin | Description |
|---|---:|---|
| MTDO | GPIO7 | JTAG |
| MTDI | GPIO5 | JTAG, ADC |
| MTCK | GPIO6 | JTAG, ADC |
| MTMS | GPIO4 | JTAG, ADC |
| EN | CHIP_PU | Reset |
| Boot | GPIO9 | Enter Boot Mode |

### RF + onboard light

| Label | Chip Pin | Description |
|---|---:|---|
| RF Switch Port Select | GPIO14 | Switch onboard antenna and the UFL antenna |
| RF Switch Power | GPIO3 | Power |
| Light | GPIO15 | User Light |

## Usage Examples

For this ticket’s firmware (`0065-xiao-esp32c6-gpio-web-server`):

- D2 defaults to `GPIO2` (active-low)
- D3 defaults to `GPIO21` (active-low)

## Related

- `ttmp/2026/01/21/0065-GPIO-WEB-SERVER--esp32-c6-wi-fi-webserver-frontend-for-d2-d3/design-doc/01-analysis-esp32-c6-gpio-web-server-d2-d3-toggle.md`
- `0065-xiao-esp32c6-gpio-web-server/main/Kconfig.projbuild`
