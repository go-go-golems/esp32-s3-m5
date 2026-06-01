# ESP32-P4-WIFI6 PicoCalc Bring-Up

Phase 1 firmware for the Waveshare ESP32-P4-WIFI6 board as a PicoCalc MCU replacement.

## What this does

- Blinks an LED (if available on a header GPIO)
- Prints boot info and PSRAM status to USB Serial/JTAG console
- Verifies: flash, PSRAM, CPU frequency, Wi-Fi co-processor presence

## Hardware

- **Board**: Waveshare ESP32-P4-WIFI6 (SKU 32020)
- **Chip**: ESP32-P4NRW32 (dual-core 360 MHz RISC-V, 32MB stacked PSRAM)
- **Flash**: 32MB NOR (octal SPI)
- **Console**: USB Serial/JTAG (GPIO24/GPIO25) or CH343P UART (GPIO37/GPIO38)

## Pin assignments (Phase 1 — bring-up only)

| Function | GPIO | Header | Notes |
|----------|------|--------|-------|
| LED blink | GPIO49 | left | Active-high output, no LED on board — wire one |
| Console | USB Serial/JTAG | — | Default, no wiring needed |

## Build and flash

```bash
# Set up ESP-IDF environment
source ~/esp/esp-idf-5.4.2/export.sh

# Set target
idf.py set-target esp32p4

# Build
idf.py build

# Flash and monitor (USB Serial/JTAG)
idf.py -p /dev/ttyACM0 flash monitor
```

## Wiring

Just USB-C for power + console. Optionally wire an LED + resistor to GPIO49 and GND on the left header.

## Next phases

- Phase 2: SPI2 LCD driver (ST7365P/ILI9488, GPIO28–31)
- Phase 3: I²C keyboard southbridge (GPIO7/8, addr 0x1F, 10 kHz)
- Phase 4: SD card (onboard SDMMC or PicoCalc SPI)
- Phase 5: Audio (LEDC PWM on GPIO27/GPIO32)
