---
Title: 'Bring-up checklist: I2C OLED (GME12864-11)'
Ticket: 065a-ADD-OLED
Status: active
Topics:
    - esp-idf
    - esp32c6
    - xiao
    - i2c
    - oled
    - display
    - wifi
    - ui
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-22T20:49:02.231235668-05:00
WhatFor: ""
WhenToUse: ""
---

# Bring-up checklist: I2C OLED (GME12864-11)

## Purpose

Provide a repeatable, step-by-step bring-up procedure to:
- wire a 128×64 OLED module (GME12864-11 class) to a Seeed Studio XIAO ESP32C6 over I2C
- confirm the module is actually responding on the I2C bus (address probe)
- validate basic SSD1306 initialization and framebuffer writes using ESP-IDF’s `esp_lcd` driver
- validate the intended UX: show Wi‑Fi state + IP for firmware `0065-xiao-esp32c6-gpio-web-server`

## Environment Assumptions

- Host has ESP-IDF installed at `/home/manuel/esp/esp-idf-5.4.1`
- You will use the USB Serial/JTAG console (`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`), not UART
- Target firmware workspace: `0065-xiao-esp32c6-gpio-web-server/`
- Hardware: Seeed Studio XIAO ESP32C6 + I2C OLED breakout
- Wiring is short and stable; pull-ups exist (either on breakout or external)

## Commands

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5

# Optional: inventory what ESP-IDF already provides for SSD1306
ttmp/2026/01/22/065a-ADD-OLED--add-oled-gme12864-11-on-esp32-c6-i2c-for-wi-fi-status-ip/scripts/idf_inventory_ssd1306.sh \
  /home/manuel/esp/esp-idf-5.4.1

# Optional: re-run the "research breadcrumbs" for the display module naming
python3 ttmp/2026/01/22/065a-ADD-OLED--add-oled-gme12864-11-on-esp32-c6-i2c-for-wi-fi-status-ip/scripts/lookup_gme12864_11_sources.py \
  search --limit 5

# Build the baseline firmware (or your modified OLED-enabled version)
cd 0065-xiao-esp32c6-gpio-web-server
idf.py set-target esp32c6
idf.py build

# Flash and monitor (adjust port as needed)
idf.py -p /dev/ttyACM0 flash monitor
```

## Exit Criteria

- On boot, the OLED powers on and shows a stable image (no rapid flicker, no random garbage).
- I2C address is confirmed (device ACKs on the expected address, typically `0x3C` or `0x3D`).
- The OLED screen content changes when Wi‑Fi state changes:
  - before connect: “IDLE” and “IP -”
  - while connecting: “CONNECTING”
  - after DHCP: “CONNECTED” and an actual IPv4 address
- The USB Serial/JTAG console remains usable (no log spam from OLED refresh).

## Notes

### Wiring reminder (XIAO ESP32C6)

Per Seeed Studio XIAO ESP32C6 pinout (as provided in the ticket request):
- `SDA` is on **GPIO22** (board “pin 4”)
- `SCL` is on **GPIO23** (board “pin 5”)

### Quick I2C address probe (recommended early)

If you don’t yet have an I2C scan/probe in the firmware, add a temporary probe right after I2C bus init (using the new I2C master driver):

```c
for (int addr = 0x08; addr <= 0x77; addr++) {
    if (i2c_master_probe(bus_handle, addr, 50) == ESP_OK) {
        ESP_LOGI(TAG, "I2C ACK at 0x%02X", addr);
    }
}
```

Expected outcome:
- Most SSD1306 I2C OLEDs ACK at `0x3C` or `0x3D`.
- If nothing ACKs:
  - re-check wiring (SDA/SCL swapped is common)
  - confirm the module is actually an I2C breakout (not SPI)
  - confirm pull-ups exist

### Common failure modes

- **No ACK on scan**: wrong address, wrong interface variant (SPI module), missing pull-ups, swapped SDA/SCL, bad ground.
- **OLED ACKs but no image**: wrong controller, SSD1306 init sequence mismatch, incorrect control phase configuration, wrong address mode.
- **Random pixels / unstable**: bus too fast for wiring/pull-ups; try 100kHz and shorten wires.

### Why this playbook does not use UART console

UART pins are frequently repurposed and can corrupt peripheral traffic; this repo’s baseline is USB Serial/JTAG for `esp_console`.
