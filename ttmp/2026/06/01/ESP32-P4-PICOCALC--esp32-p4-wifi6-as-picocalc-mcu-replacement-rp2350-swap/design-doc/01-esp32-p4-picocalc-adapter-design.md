---
Title: ESP32-P4 PicoCalc Adapter Design
Ticket: ESP32-P4-PICOCALC
Status: active
Topics:
    - esp32-p4
    - picocalc
    - hardware
    - firmware-port
    - waveshare
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../code/wesen/2026-05-05--ulisp-picocalc/pico-sdk-picocalc-wm/CMakeLists.txt
      Note: Current PicoCalc WM firmware build config (Pico SDK)
    - Path: ../../../../../../../../../../code/wesen/2026-05-05--ulisp-picocalc/pico-sdk-picocalc-wm/src/display/ili9488.cpp
      Note: LCD driver to port to ESP-IDF SPI2
    - Path: ../../../../../../../../../../code/wesen/2026-05-05--ulisp-picocalc/pico-sdk-picocalc-wm/src/input/keyboard.cpp
      Note: I2C keyboard driver to port to ESP-IDF I2C master
ExternalSources:
    - sources/chatgpt-esp32-p4-picocalc-analysis.md
    - sources/esp32-p4-waveshare-board-pinout.md
    - sources/picocalc-hardware-spec-pipapo.md
    - sources/picocalc-lcd-spec.md
    - sources/picocalc-notes-README.md
    - sources/esp32-p4_datasheet_en.pdf
    - sources/waveshare-esp32-p4-module-wiki.md
    - sources/waveshare-esp32-p4-module-schematic.pdf
Summary: Design for replacing the RP2350 Pico in ClockworkPi PicoCalc with a Waveshare ESP32-P4-WIFI6 board, including adapter PCB design, pin mapping, firmware port strategy, and risk assessment.
LastUpdated: 2026-06-01T16:00:00-04:00
WhatFor: Guide the hardware adapter design and firmware port for ESP32-P4-WIFI6 on PicoCalc
WhenToUse: Reference when designing the interposer PCB or porting PicoCalc firmware to ESP-IDF
---


# ESP32-P4 PicoCalc Adapter Design

## Executive Summary

The Waveshare ESP32-P4-WIFI6 board can replace the Raspberry Pi Pico (RP2040/RP2350) in the ClockworkPi PicoCalc, but it is **not a drop-in replacement**. An adapter PCB (interposer) is required to remap the PicoCalc's hardwired peripheral connections to the ESP32-P4-WIFI6's available GPIOs. The ESP32-P4 offers significant advantages — a 360 MHz dual-core RISC-V CPU, 32 MB PSRAM, 32 MB flash, Wi-Fi 6 via the integrated ESP32-C6 co-processor, MIPI-DSI/CSI, and abundant GPIO — but requires a full firmware port from Pico SDK to ESP-IDF.

**Peripheral compatibility: yes. Drop-in compatibility: no.**

This document covers: the PicoCalc's current pin mapping, the ESP32-P4-WIFI6's available GPIOs, a proposed pin-to-pin adapter plan, power considerations, firmware migration strategy, and open risks.

## Problem Statement

The ClockworkPi PicoCalc uses a Raspberry Pi Pico (RP2040 or RP2350) as its main MCU. While functional, the Pico has limitations for more ambitious use cases:

- **No built-in Wi-Fi/BLE** on the base Pico (Pico 2W adds Wi-Fi but with limited throughput)
- **Limited RAM** — 264 KB SRAM (RP2040) / 520 KB SRAM (RP2350), plus 8 MB PSRAM via PIO bit-bang
- **No hardware video acceleration** — SPI LCD only, no MIPI-DSI
- **PIO-dependent PSRAM** — uses software bit-bang, not a native memory bus
- **Single SPI bus** shared or limited routing between LCD and SD

The Waveshare ESP32-P4-WIFI6 offers:
- 360 MHz dual-core RISC-V (HP) + 40 MHz single-core RISC-V (LP)
- 32 MB stacked PSRAM (hardware HEX-SPI, no PIO hack)
- 32 MB NOR flash
- Wi-Fi 6 + BLE 5 via integrated ESP32-C6 (SDIO interface)
- MIPI-DSI 2-lane (hardware display interface)
- MIPI-CSI 2-lane (camera interface)
- Multiple hardware SPI buses (SPI2 with IO-MUX direct pins at up to 80 MHz)
- SDIO 3.0 host for SD card (4-bit mode, much faster than SPI-mode SD)
- I²S audio codec support (hardware, not PWM bit-bang)
- USB 2.0 HS OTG
- 55 GPIOs total (27 on user headers)

The goal is to design an adapter board and firmware that lets the ESP32-P4-WIFI6 plug into the PicoCalc's Pico socket, providing all existing functionality (LCD, keyboard, SD, audio, serial) plus new capabilities (Wi-Fi, MIPI-DSI, camera, I²S audio).

## Current-State Architecture (PicoCalc Pin Map)

The PicoCalc hardwires the following peripherals to specific RP2040/RP2350 pins:

| Interface | Signal | Pico Pin | Notes |
|-----------|--------|----------|-------|
| **SPI1** | SCK | GP10 | LCD Clock |
| **SPI1** | MOSI | GP11 | LCD Data (no MISO needed) |
| **GPIO** | LCD CS | GP13 | LCD Chip Select |
| **GPIO** | LCD DC | GP14 | LCD Data/Command |
| **GPIO** | LCD RST | GP15 | LCD Reset |
| **I2C1** | SDA | GP6 | Keyboard southbridge SDA |
| **I2C1** | SCL | GP7 | Keyboard southbridge SCL |
| **SPI0** | MISO | GP16 | SD Card Data Out |
| **GPIO** | SD CS | GP17 | SD Card Chip Select |
| **SPI0** | SCK | GP18 | SD Card Clock |
| **SPI0** | MOSI | GP19 | SD Card Data In |
| **GPIO** | SD CD | GP22 | SD Card Detect (active low) |
| **PIO** | PSRAM SIO0 | GP2 | PSRAM quad data 0 |
| **PIO** | PSRAM SIO1 | GP3 | PSRAM quad data 1 |
| **PIO** | PSRAM SIO2 | GP4 | PSRAM quad data 2 |
| **PIO** | PSRAM SIO3 | GP5 | PSRAM quad data 3 |
| **PIO** | PSRAM CS | GP20 | PSRAM Chip Select |
| **PIO** | PSRAM SCK | GP21 | PSRAM Clock |
| **PWM** | Audio L | GP26 | Left speaker PWM |
| **PWM** | Audio R | GP27 | Right speaker PWM |
| **UART0** | TX | GP0 | Serial TX (to USB-C bridge) |
| **UART0** | RX | GP1 | Serial RX (from USB-C bridge) |
| **GPIO** | LED | GP25 | On-board LED |

### PicoCalc Southbridge (STM32) I²C Protocol

- **Slave address**: `0x1F`
- **Bus speed**: 10 kHz (critical — not a typo, the STM32 requires this)
- **Registers**: 0x01–0x0E for version, config, key FIFO, backlight, battery, power-off
- **Key polling**: Read REG_ID_KEY (0x04) for FIFO count, then REG_ID_FIF (0x09) for 2-byte key events (state + ASCII keycode)
- **No hardware reset pin** for the STM32 — allow ~100 ms after I2C init

### PicoCalc LCD Controller

- **Controller**: ST7365P (Sitronix, ILI9488-compatible marketing name)
- **Resolution**: 320×320, RGB565
- **SPI speed**: ~33 MHz on Pico
- **Vendor unlock required**: CMD `0xF0` with keys `0xC3`, `0x96` to enable RGB565 over SPI
- **Display inversion on** (CMD `0x21`) required for correct colour polarity
- **MADCTL**: `0x48` (MX | BGR)

## ESP32-P4-WIFI6 Board Available GPIOs

The Waveshare ESP32-P4-WIFI6 board has 55 GPIOs total on the ESP32-P4 chip. Many GPIOs are consumed by on-board peripherals (ESP32-C6 SDIO, I²S codec, SDMMC, CH343P UART, USB HS, speaker amp) — but those are all **internal traces, not on the user headers**. The board exposes **25 GPIOs** on the two 2×20 user headers, and **all 25 are available** for our PicoCalc project.

### All 25 header GPIOs

**Left header (top → bottom):**

| Pin | GPIO | Notes |
|-----|------|-------|
| 1 | GPIO52 | |
| 2 | GPIO51 | |
| 3 | GND | |
| 4 | GPIO31 | SPI2 IO_MUX direct (Q_PAD) |
| 5 | GPIO30 | SPI2 IO_MUX direct (CK_PAD) |
| 6 | GPIO29 | SPI2 IO_MUX direct (D_PAD) |
| 7 | GPIO28 | SPI2 IO_MUX direct (CS_PAD) |
| 8 | GND | |
| 9 | GPIO50 | |
| 10 | GPIO49 | |
| 11 | GPIO5 | LP_IO (deep-sleep wake capable) |
| 12 | GPIO4 | ⚠ JTAG MTMS default |
| 13 | GND | |
| 14 | GPIO3 | ⚠ JTAG MTDI default |
| 15 | GPIO2 | ⚠ JTAG MTCK default |
| 16 | GPIO8 | ⚠ I²C0 SCL (shares bus with on-board ES8311 codec at 0x18) |
| 17 | GPIO7 | ⚠ I²C0 SDA (shares bus with on-board ES8311 codec at 0x18) |
| 18 | GND | |
| 19 | GPIO24 | ⚠ USB Serial/JTAG D− default |
| 20 | GPIO25 | ⚠ USB Serial/JTAG D+ default |

**Right header (top → bottom):**

| Pin | GPIO/Signal | Notes |
|-----|------------|-------|
| 1 | VBUS | +5 V input |
| 2 | VSYS | +5 V battery/external |
| 3 | GND | |
| 4 | EN | CHIP_PU (hold low = reset) |
| 5 | 3V3 | 3.3 V output from LDO (≤500 mA) |
| 6 | GPIO20 | |
| 7 | GPIO21 | |
| 8 | GND | |
| 9 | GPIO22 | |
| 10 | GPIO23 | |
| 11 | RUN | System reset button net |
| 12 | GPIO26 | |
| 13 | GND | |
| 14 | GPIO27 | |
| 15 | GPIO32 | |
| 16 | GPIO33 | |
| 17 | GPIO46 | ⚠ GND pad between GPIO46 and GPIO47 |
| 18 | GND | ⚠ Multi-pin housings will short |
| 19 | GPIO47 | ⚠ GND pad between GPIO46 and GPIO47 |
| 20 | GPIO48 | |

### GPIO categories

**Truly free (no caveats) — 18 GPIOs:**

```
GPIO2   GPIO3   GPIO4   GPIO5   GPIO20  GPIO21
GPIO22  GPIO23  GPIO26  GPIO27  GPIO32  GPIO33
GPIO46  GPIO47  GPIO48  GPIO49  GPIO51  GPIO52
```

**With I²C bus sharing caveat — 2 GPIOs:**

| GPIO | Header | Note |
|------|--------|------|
| GPIO7 | left (SDA) | On-board ES8311 codec at 0x18 is always on this I²C0 bus. PicoCalc keyboard at 0x1F can share (no address collision), but keyboard's 10 kHz speed may conflict with codec. |
| GPIO8 | left (SCL) | Same — I²C0 SCL shared with codec. |

**With JTAG/USB default caveat — 5 GPIOs (GPIO2–4, GPIO24–25):**

These default to JTAG or USB Serial/JTAG at boot. Using them as general GPIO disables JTAG-via-USB-Serial. In practice JTAG-via-USB-Serial is rarely needed (we have CH343P UART console), so these are de-facto available.

**Not on user headers (board-internal, cannot reassign):**

| Peripheral | GPIOs (all internal traces) |
|-----------|-------|
| ESP32-C6 SDIO (Wi-Fi 6) | GPIO6, GPIO14–GPIO19, GPIO54 |
| I²S0 codec (ES8311 audio) | GPIO9–GPIO13 |
| SDMMC (TF card, 4-bit) | GPIO39–GPIO44 |
| CH343P UART console | GPIO35, GPIO37, GPIO38 |
| USB 2.0 HS PHY | chip pins 49/50 (dedicated) |
| Speaker amp (NS4150B) | GPIO53 |
| MIPI-DSI | dedicated chip pins (no GPIO cost) |
| MIPI-CSI | dedicated chip pins (no GPIO cost) |
| BOOT strapping | GPIO0 (not on header) |

This is a much healthier GPIO budget than initially estimated. The 25 available header GPIOs give us plenty of room for all PicoCalc peripherals plus extras.

### On-board SD card slot

The Waveshare board has its own MicroSD slot on SDMMC 4-bit (GPIO39–GPIO44). This means **the PicoCalc's SPI-mode SD card slot is redundant** — you can use the board's own SD slot at much higher speed (SDIO 3.0 4-bit vs SPI 1-bit). However, this means the SD card is on the Waveshare board, not accessible through the PicoCalc's external SD slot.

**Decision point**: Whether to wire the PicoCalc's external SD slot to free ESP32-P4 GPIOs (consuming 5 more pins in SPI mode) or rely solely on the Waveshare's onboard SD slot. Using the onboard slot is simpler and faster, but loses the PicoCalc's user-accessible SD card.

## Proposed Pin Mapping (PicoCalc → ESP32-P4-WIFI6)

### Primary peripherals

| PicoCalc Net | Pico Pin | ESP32-P4 Pin | Header | Notes |
|-------------|----------|-------------|--------|-------|
| LCD SCK | GP10 | **GPIO30** | left | SPI2_CK_PAD — IO-MUX direct |
| LCD MOSI | GP11 | **GPIO29** | left | SPI2_D_PAD — IO-MUX direct |
| LCD CS | GP13 | **GPIO28** | left | SPI2_CS_PAD — IO-MUX direct |
| LCD DC | GP14 | **GPIO31** | left | plain GPIO output |
| LCD RST | GP15 | **GPIO49** | left | plain GPIO output |
| LCD BL | — | **GPIO50** | left | LEDC PWM backlight |
| I2C SDA | GP6 | **GPIO7** | left | I²C0 SDA (shared with on-board codec/IMU) |
| I2C SCL | GP7 | **GPIO8** | left | I²C0 SCL (shared with on-board codec/IMU) |
| Audio L | GP26 | **GPIO27** | right | LEDC PWM output |
| Audio R | GP27 | **GPIO32** | right | LEDC PWM output |
| UART TX | GP0 | **GPIO37** | on-board | CH343P console TX (P4→PC) |
| UART RX | GP1 | **GPIO38** | on-board | CH343P console RX (PC→P4) |

### SD card options

**Option A — Use Waveshare onboard SD only (recommended for first bring-up)**

No additional GPIOs consumed. Use SDMMC 4-bit mode at full speed.
Trade-off: PicoCalc's external SD slot is unusable.

**Option B — Wire PicoCalc SD slot via SPI**

| PicoCalc Net | Pico Pin | ESP32-P4 Pin | Header | Notes |
|-------------|----------|-------------|--------|-------|
| SD MISO | GP16 | **GPIO46** | right | SPI3 via GPIO matrix |
| SD CS | GP17 | **GPIO47** | right | plain GPIO output |
| SD SCK | GP18 | **GPIO48** | right | SPI3 via GPIO matrix |
| SD MOSI | GP19 | **GPIO33** | right | SPI3 via GPIO matrix |
| SD CD | GP22 | **GPIO52** | left | GPIO input, active low |

This consumes 5 header GPIOs. SPI3 through the GPIO matrix will work at ~40 MHz (not IO-MUX direct, so capped lower than SPI2). The right-header GND between GPIO46 and GPIO47 is a known trap — use individual jumpers, not multi-pin housings.

With 25 total header GPIOs available, consuming 5 for the PicoCalc SD slot is feasible but reduces the pool for future expansion (Wi-Fi accessories, camera, extra buttons, etc.).

### PSRAM — Skip for first revision

The PicoCalc's 8 MB PSRAM (GP2–GP5, GP20–GP21 via PIO) should be **left unconnected**. The ESP32-P4-WIFI6 already has 32 MB PSRAM stacked in the package on a hardware HEX-SPI bus — there is no need to use PicoCalc's external PSRAM, and the RP2040 PIO driver is not portable to ESP32-P4.

### Other pins

| PicoCalc Net | Pico Pin | ESP32-P4 Treatment |
|-------------|----------|-------------------|
| PSRAM SIO0–3 | GP2–GP5 | Leave unconnected |
| PSRAM CS | GP20 | Leave unconnected |
| PSRAM SCK | GP21 | Leave unconnected |
| LED | GP25 | Map to any free GPIO or skip |
| SD CD | GP22 | See Option B above |

## Power Considerations

This is the **single biggest risk** in the project. The PicoCalc's power system was designed for the RP2040/RP2350 power envelope, not for the ESP32-P4.

### PicoCalc power architecture

- USB-C VBUS feeds AXP2101 power management IC
- 18650 Li-ion battery feeds AXP2101 VBAT
- AXP2101 regulates to VSYS (~5 V) for the Pico and peripherals
- Pico onboard LDO: VSYS → 3V3 (SMPS on Pico 2, linear on Pico 1)
- All peripherals are 3.3 V I/O

### ESP32-P4-WIFI6 power requirements

- **5 V input** (VBUS or VSYS) specified by Waveshare
- On-board LDO generates 3.3 V (≤500 mA)
- ESP32-P4 peak current: ~200–350 mA (HP dual-core active + Wi-Fi + PSRAM)
- ESP32-C6 adds ~70–100 mA during Wi-Fi TX bursts
- Total peak: could reach 400–500 mA

### Recommended power approach

| Item | Recommendation |
|------|---------------|
| ESP32-P4-WIFI6 input | Feed VIN from PicoCalc VSYS (5 V rail) |
| Ground | Common ground between PicoCalc and ESP32-P4 |
| 3.3 V rails | **Do not tie PicoCalc 3V3 and ESP32-P4 SOC_3V3 together** — verify regulator topology and backfeed behavior first |
| I/O level | Both sides are 3.3 V — compatible |
| Reset/boot | Bring out CHIP_EN (EN pin) and BOOT button access on the adapter |
| Current budget | Measure PicoCalc VSYS current with Pico installed, verify VSYS can supply an additional 500 mA for the ESP32-P4-WIFI6 |

### Critical measurement before PCB

**Measure the PicoCalc VSYS voltage** under these conditions:
1. USB-C powered, battery full, Pico running
2. USB-C powered, battery full, Pico in deep sleep
3. Battery only, battery full
4. Battery only, battery low
5. Power-switch toggled states

If VSYS is not a clean 5 V, add a buck-boost regulator on the adapter board.

## Adapter Board Design

The adapter board must:

1. **Accept the Waveshare ESP32-P4-WIFI6** (via 2×20 header sockets or direct solder)
2. **Present a Pico-compatible pinout** (2×20 male headers at 0.1" spacing matching Pico footprint)
3. **Route signals** per the pin mapping above
4. **Handle power** — 5 V from PicoCalc VSYS to Waveshare VIN, isolated 3.3 V rails
5. **Expose CHIP_EN and BOOT** — test points or small buttons for reset and download mode
6. **Leave PicoCalc PSRAM pins unconnected** — no need, ESP32-P4 has its own
7. **Optional: route SD card** if PicoCalc external SD slot support is needed

### Physical constraints

- The ESP32-P4-WIFI6 board is larger than a Pico. Verify it fits inside the PicoCalc case.
- If it doesn't fit, the adapter board could be designed as a flex PCB or a small rigid board with a ribbon cable to the Waveshare board mounted elsewhere (e.g., in the battery compartment).
- Alternative: use the **ESP32-P4-Module** (the bare module) instead of the full WIFI6 dev board, which is much smaller — but loses the on-board SD slot, USB-C, speaker, etc.

## Firmware Migration Strategy

### Current firmware stack (Pico SDK)

The existing PicoCalc firmware (`pico-sdk-picocalc-wm`) uses:
- Pico SDK C/C++ with `hardware_spi`, `hardware_i2c`, `hardware_gpio`, `hardware_pwm`
- SPI1 for LCD (ILI9488/ST7365P driver)
- I2C1 for keyboard southbridge (10 kHz, address 0x1F)
- SPI0 for SD card (SPI mode, FatFS)
- PWM for dual-channel audio
- UART0 for serial console
- PIO for PSRAM (not needed on ESP32-P4)

### Target firmware stack (ESP-IDF)

The ESP32-P4-WIFI6 firmware must use **ESP-IDF** (Arduino support is preliminary/unstable for ESP32-P4 as of mid-2026). Key mappings:

| Pico SDK API | ESP-IDF Equivalent |
|-------------|-------------------|
| `hardware_spi` | `esp_lcd_panel_io_spi_config_t` or `spi_device_interface_config_t` |
| `hardware_i2c` | `i2c_master_driver` (ESP-IDF v6.x new API) or legacy `i2c_driver` |
| `hardware_gpio` | `gpio_config()` / `gpio_set_level()` |
| `hardware_pwm` | `ledc_channel_config()` (LEDC PWM controller) |
| `pico_stdlib` UART | `uart_driver_config` or USB Serial/JTAG console |
| `hardware_flash` | `esp_partition` / `nvs_flash` |
| FatFS (SPI SD) | `esp_vfs_fat_sdmmc_format` or `esp_vfs_fat_sdspi_format` |
| PIO PSRAM | Not needed — ESP32-P4 PSRAM is managed by MMU/heap |

### Firmware port phases

**Phase 1 — Blink and console (bring-up)**
- ESP-IDF project skeleton for ESP32-P4
- USB Serial/JTAG or CH343P UART console
- GPIO blink on an accessible LED
- Verify flash, PSRAM, boot

**Phase 2 — LCD driver**
- SPI2 LCD driver using `esp_lcd_panel_io_spi` + `esp_lcd_panel_dev`
- ST7365P/ILI9488 init sequence (vendor unlock, RGB565, inversion)
- Framebuffer in PSRAM, flush via DMA
- Test: fill screen with color bars

**Phase 3 — Keyboard southbridge**
- I²C0 master driver at 10 kHz
- Polling loop for REG_ID_KEY → REG_ID_FIF
- Map key events to the existing key code table
- Test: type on keyboard, see keycodes on LCD

**Phase 4 — SD card**
- Option A: Use onboard SDMMC slot (SDIO 4-bit, fastest)
- Option B: Use PicoCalc SPI SD (SPI3 via GPIO matrix)
- Mount FatFS, verify read/write

**Phase 5 — Audio**
- LEDC PWM on two channels → PicoCalc speaker path
- Test: play tones through PicoCalc speakers
- Future: I²S audio through Waveshare's ES8311 codec (separate path, doesn't go through PicoCalc speakers)

**Phase 6 — Window manager port**
- Port `pico-sdk-picocalc-wm` UI framework (text grid, line editor, terminal pane, app registry)
- Replace Pico SDK primitives with ESP-IDF equivalents
- Leverage PSRAM for large framebuffers and display lists

**Phase 7 — New capabilities**
- Wi-Fi 6 via ESP-Hosted (ESP32-C6 SDIO slave)
- MIPI-DSI display (if a DSI panel is fitted to the PicoCalc case)
- Camera input via MIPI-CSI
- BLE peripherals

## Design Decisions

### Decision 1: Use ESP-IDF, not Arduino

**Rationale**: ESP32-P4 Arduino support is still preliminary (arduino-esp32 issue #10278, merged in release/v3.1.x with basic functions only). Waveshare explicitly recommends ESP-IDF for ESP32-P4 development. ESP-IDF provides full peripheral support, proper SPI DMA, LCD panel drivers, and SDMMC host drivers.

### Decision 2: SPI2 LCD on IO-MUX direct pins (GPIO28–31)

**Rationale**: ESP32-P4 SPI2 has IO-MUX direct pins on GPIO28–31 that bypass the GPIO matrix, enabling up to 80 MHz SPI clock. The LCD is the highest-bandwidth peripheral — giving it the fastest path is critical for smooth UI.

### Decision 3: Share I²C0 bus with on-board peripherals

**Rationale**: The Waveshare board already wires GPIO7/GPIO8 to I²C0 (ES8311 codec + BNO085 IMU). The PicoCalc keyboard southbridge is also I²C at address 0x1F. No address collision with 0x18 (codec) or 0x4A (IMU). The 10 kHz PicoCalc keyboard bus speed is slow but works on the same physical bus — the codec and IMU must tolerate this during keyboard polls, or the bus speed can be dynamically switched.

**Risk**: The 10 kHz I²C speed is unusually slow. If the ES8311 or BNO085 has minimum clock requirements, the shared bus may need speed switching or the keyboard southbridge needs its own I²C bus on separate GPIOs.

### Decision 4: Onboard SD slot first, PicoCalc SD later

**Rationale**: The Waveshare's onboard SDMMC slot is 4-bit SDIO 3.0 — much faster than PicoCalc's SPI-mode SD. For first bring-up, use the onboard slot. Adding PicoCalc SD support is a Phase 4 refinement.

### Decision 5: PWM audio, not I²S

**Rationale**: The PicoCalc speaker path expects dual PWM (GP26/GP27). The Waveshare's I²S codec drives its own onboard speaker — it cannot drive the PicoCalc speakers through the Pico socket. So we use LEDC PWM to emulate the Pico's PWM audio path.

## Alternatives Considered

### M5Stack Stamp-P4

The ChatGPT analysis focused on the M5Stack Stamp-P4. While pin-compatible in spirit, the Waveshare ESP32-P4-WIFI6 was chosen because:
- More GPIOs exposed on standard 2×20 headers
- Onboard SDMMC slot (4-bit SDIO)
- Integrated ESP32-C6 for Wi-Fi 6
- Better documentation (schematic PDF available)
- Active community (adsb-p4 project provides proven pinout)

### Luckfox Lyra

The PicoCalc community has ported Luckfox Lyra (another RP2350 alternative). The Luckfox Lyra runs Linux, which is a different philosophy. The ESP32-P4 approach keeps bare-metal/RTOS control while offering much more compute.

### Raspberry Pi Pico 2W

The simplest upgrade path. Adds Wi-Fi/BLE but keeps the same RP2350 limitations (520 KB SRAM, PIO PSRAM hack, no MIPI). Good for incremental improvement, but doesn't unlock the ESP32-P4's advantages.

## Open Questions

1. **Physical fit**: Does the Waveshare ESP32-P4-WIFI6 board fit inside the PicoCalc case? If not, what mounting strategy? (External board with ribbon cable? Battery compartment? New case?)
2. **Power budget**: Can the PicoCalc's VSYS rail supply an additional ~500 mA for the ESP32-P4-WIFI6? Need real measurements.
3. **I²C bus sharing**: Will the 10 kHz keyboard polling interfere with the ES8311 codec or BNO085 IMU on the shared I²C0 bus? Need testing.
4. **PicoCalc SD slot**: Is access to the PicoCalc's external SD card slot important enough to consume 5 free GPIOs?
5. **Chip revision**: ESP-IDF now requires ESP32-P4 revision ≥ 3.1. Which revision does the Waveshare board ship? Early boards may have v1.3 silicon that's incompatible with current ESP-IDF.
6. **USB console path**: Use CH343P UART console (GPIO37/38) or USB Serial/JTAG (GPIO24/25)? CH343P is more convenient but consumes GPIO35/37/38.
7. **PicoCalc case USB-C**: The PicoCalc's USB-C routes to the STM32 southbridge UART bridge. Can the ESP32-P4-WIFI6's USB-C be used instead, or does it need a different connection?

## References

- [ChatGPT Analysis: ESP32-P4 as Pico Replacement](../sources/chatgpt-esp32-p4-picocalc-analysis.md) — Initial feasibility analysis with M5Stack Stamp-P4 focus
- [Waveshare ESP32-P4-WIFI6 Board Pinout](../sources/esp32-p4-waveshare-board-pinout.md) — Detailed GPIO map from adsb-p4 project (best reference)
- [PicoCalc Hardware Spec (PiPAPo)](../sources/picocalc-hardware-spec-pipapo.md) — Complete PicoCalc pin mapping and southbridge protocol
- [PicoCalc LCD Spec](../sources/picocalc-lcd-spec.md) — ST7365P initialization sequence and SPI details
- [PicoCalc Notes (LennartHennigs)](../sources/picocalc-notes-README.md) — Community pin reference
- [ESP32-P4 Datasheet](../sources/esp32-p4_datasheet_en.pdf) — Official Espressif datasheet
- [Waveshare ESP32-P4-Module Wiki](../sources/waveshare-esp32-p4-module-wiki.md) — Module pinout and specifications
- [Waveshare ESP32-P4-Module Schematic](../sources/waveshare-esp32-p4-module-schematic.pdf) — Board schematic PDF
- [ClockworkPi PicoCalc Official Repo](https://github.com/clockworkpi/PicoCalc) — Official firmware and schematics
- [Arduino ESP32-P4 Support Issue](https://github.com/espressif/arduino-esp32/issues/10278) — Arduino compatibility status
