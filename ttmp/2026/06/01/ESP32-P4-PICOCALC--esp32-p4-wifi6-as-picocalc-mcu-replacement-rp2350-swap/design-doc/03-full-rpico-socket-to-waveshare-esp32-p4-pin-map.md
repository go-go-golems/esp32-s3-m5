---
Title: Full Physical RPico Socket to Waveshare ESP32-P4-WIFI6 Pin Map
Ticket: ESP32-P4-PICOCALC
Status: active
Topics:
    - esp32-p4
    - picocalc
    - hardware
    - adapter-pcb
    - pin-mapping
DocType: design-doc
Intent: implementation-guide
Owners: []
RelatedFiles:
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0098-esp32-p4-wifi6-webserver/main/picocalc_keyboard.h
      Note: Keyboard GPIO constants must be changed if using the same-position physical adapter mapping
ExternalSources:
    - /tmp/pi-clipboard-820e03d4-338d-4e96-b13d-7ebe5904db0a.png
    - /tmp/pi-clipboard-06a9a70c-47c2-4dc7-9745-4ecd33ef9204.png
    - ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/sources/picocalc-hardware-spec-pipapo.md
    - ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/sources/esp32-p4-waveshare-board-pinout.md
Summary: Corrected pin-by-pin physical mapping when the Raspberry Pi Pico 2 socket positions are matched directly to the Waveshare ESP32-P4-WIFI6 2x20 header positions.
LastUpdated: 2026-06-01T19:05:00-04:00
WhatFor: Verify actual adapter PCB/header wiring by physical pin position, not by peripheral function
WhenToUse: Before schematic capture, cable harness wiring, or ESP-IDF GPIO constant changes
---

# Full Physical RPico Socket to Waveshare ESP32-P4-WIFI6 Pin Map

## Executive summary

This document verifies the **actual physical pin-position mapping** between the Raspberry Pi Pico 2 / RP2350 pinout image and the Waveshare ESP32-P4-WIFI6 pinout image supplied in the chat.

The key correction is that this is **not** the same as a function-optimized remap. If an adapter places the Waveshare board in the Pico footprint by matching the two 20-pin side rows position-for-position, then Pico physical pin 9 maps to Waveshare physical left-row position 9. In the supplied Waveshare image, that position is **GPIO50**. Therefore:

```text
Pico GP6 / physical pin 9 / SDA  →  ESP32-P4 GPIO50
Pico GP7 / physical pin 10 / SCL →  ESP32-P4 GPIO49
```

That directly explains why the earlier keyboard firmware that probed GPIO7/GPIO8 at address `0x1F` received a NACK: GPIO7/GPIO8 are the Waveshare board's own labeled SDA/SCL header positions, but they are **not** the positions occupied by Pico GP6/GP7 in a same-position physical adapter.

## Orientation assumption

The mapping below assumes both boards are viewed with their USB connector at the top, matching the supplied images:

- Raspberry Pi Pico 2 image: USB at top, left side physical pins 1–20 top-to-bottom, right side physical pins 40–21 top-to-bottom.
- Waveshare ESP32-P4-WIFI6 image: USB-C at top, left header top-to-bottom, right header top-to-bottom.

Under this orientation, the adapter maps:

```text
Pico left physical pin 1  → Waveshare left position 1
Pico left physical pin 2  → Waveshare left position 2
...
Pico left physical pin 20 → Waveshare left position 20

Pico right physical pin 40 → Waveshare right position 40/top
Pico right physical pin 39 → Waveshare right position 39
...
Pico right physical pin 21 → Waveshare right position 21/bottom
```

If the Waveshare board is flipped, rotated, placed on the underside, or connected through mirrored headers, this table must be mirrored accordingly. Do not route the PCB until connector-side/orientation is fixed mechanically.

## Complete physical pin-position map

### Left side: Pico physical pins 1–20

| Pico physical pin | Pico label/function from image | Waveshare same-position label | ESP32-P4 GPIO/net | Consequence for PicoCalc |
|---:|---|---|---|---|
| 1 | GP0 / UART0 TX | GPIO52 | GPIO52 | PicoCalc UART0 TX position lands on GPIO52. |
| 2 | GP1 / UART0 RX | GPIO51 | GPIO51 | PicoCalc UART0 RX position lands on GPIO51. |
| 3 | GND | GND | GND | Ground matches. |
| 4 | GP2 | GPIO31 | GPIO31 | PicoCalc PSRAM SIO0 position lands on GPIO31. |
| 5 | GP3 | GPIO30 | GPIO30 | PicoCalc PSRAM SIO1 position lands on GPIO30. |
| 6 | GP4 | GPIO29 | GPIO29 | PicoCalc PSRAM SIO2 position lands on GPIO29. |
| 7 | GP5 | GPIO28 | GPIO28 | PicoCalc PSRAM SIO3 position lands on GPIO28. |
| 8 | GND | GND | GND | Ground matches. |
| 9 | GP6 / I2C1 SDA; keyboard bus | GPIO50 | GPIO50 | **Keyboard SDA at Pico pin 9 lands on GPIO50.** |
| 10 | GP7 / I2C1 SCL; keyboard bus | GPIO49 | GPIO49 | **Keyboard SCL at Pico pin 10 lands on GPIO49.** |
| 11 | GP8 / UART1 TX | GPIO5 | GPIO5 | PicoCalc UART1 TX position lands on GPIO5. |
| 12 | GP9 / UART1 RX | GPIO4 | GPIO4 | PicoCalc UART1 RX position lands on GPIO4. |
| 13 | GND | GND | GND | Ground matches. |
| 14 | GP10 / SPI1 SCK / LCD SCK | GPIO3 | GPIO3 | LCD SCK lands on GPIO3, not GPIO30. |
| 15 | GP11 / SPI1 MOSI / LCD MOSI | GPIO2 | GPIO2 | LCD MOSI lands on GPIO2, not GPIO29. |
| 16 | GP12 / SPI1 MISO | SCL / GPIO8 | GPIO8 | LCD MISO/readback position lands on GPIO8. Usually optional. |
| 17 | GP13 / SPI1 CS / LCD CS | SDA / GPIO7 | GPIO7 | LCD CS lands on GPIO7. |
| 18 | GND | GND | GND | Ground matches. |
| 19 | GP14 / LCD DC | DM / GPIO24 | GPIO24 | LCD DC lands on GPIO24. |
| 20 | GP15 / LCD RST | DP / GPIO25 | GPIO25 | LCD reset lands on GPIO25. |

### Right side: Pico physical pins 40–21

| Pico physical pin | Pico label/function from image | Waveshare same-position label | ESP32-P4 GPIO/net | Consequence for PicoCalc |
|---:|---|---|---|---|
| 40 | VBUS | VBUS | VBUS | Power position matches by label. Must still check backfeed behavior. |
| 39 | VSYS | VSYS | VSYS | Power position matches by label. Must still check supply direction/current. |
| 38 | GND | GND | GND | Ground matches. |
| 37 | 3V3_EN | EN | EN / CHIP_PU | **Not equivalent.** Pico 3V3_EN would land on ESP32-P4 chip enable/reset. Needs review; likely do not hardwire. |
| 36 | 3V3 | 3V3 | 3V3 | Power label matches. Must verify regulator/source direction. |
| 35 | ADC_VREF | GPIO20 | GPIO20 | ADC_VREF position lands on GPIO20. Probably leave isolated unless needed. |
| 34 | GP28 / ADC2 | GPIO21 | GPIO21 | Pico GP28 position lands on GPIO21. |
| 33 | GND / AGND in Pico image | GND | GND | Ground matches. |
| 32 | GP27 / audio PWM R | GPIO22 | GPIO22 | PicoCalc audio R lands on GPIO22. |
| 31 | GP26 / audio PWM L | GPIO23 | GPIO23 | PicoCalc audio L lands on GPIO23. |
| 30 | RUN/RESET | RUN | RUN | Reset position appears to match by label, but reset semantics must be verified. |
| 29 | GP22 / SD card detect | GPIO26 | GPIO26 | SD detect lands on GPIO26. |
| 28 | GND | GND | GND | Ground matches. |
| 27 | GP21 / PSRAM SCK | GPIO27 | GPIO27 | PicoCalc PSRAM SCK position lands on GPIO27. |
| 26 | GP20 / PSRAM CS | GPIO32 | GPIO32 | PicoCalc PSRAM CS position lands on GPIO32. |
| 25 | GP19 / SPI0 MOSI / SD MOSI | GPIO33 | GPIO33 | SD MOSI lands on GPIO33. |
| 24 | GP18 / SPI0 SCK / SD SCK | GPIO46 | GPIO46 | SD SCK lands on GPIO46. |
| 23 | GND | GND | GND | Ground matches. |
| 22 | GP17 / SPI0 CS / SD CS | GPIO47 | GPIO47 | SD CS lands on GPIO47. |
| 21 | GP16 / SPI0 MISO / SD MISO | GPIO48 | GPIO48 | SD MISO lands on GPIO48. |

## Derived PicoCalc peripheral mapping under same-position physical adapter

This is the mapping firmware should use **if the adapter is purely physical-position preserving**.

### UART0 / PicoCalc USB-C serial path

| PicoCalc net | Pico physical pin | ESP32-P4 GPIO/net |
|---|---:|---|
| UART0 TX | 1 | GPIO52 |
| UART0 RX | 2 | GPIO51 |

Note: the already-proven Waveshare CH343 console remains GPIO37/GPIO38 internally. That is a separate USB-C connector/path on the Waveshare board and is not part of the Pico socket physical mapping.

### Keyboard / STM32 southbridge I2C

| PicoCalc net | Pico physical pin | ESP32-P4 GPIO/net |
|---|---:|---|
| Keyboard SDA on Pico GP6 | 9 | GPIO50 |
| Keyboard SCL on Pico GP7 | 10 | GPIO49 |

The PicoCalc sources and the supplied Pico 2 pinout agree on the keyboard bus roles: GP6 is SDA1 and GP7 is SCL1. Therefore for a same-position physical adapter, the keyboard constants should be:

```c
#define PICOCALC_KBD_I2C_SDA_GPIO 50
#define PICOCALC_KBD_I2C_SCL_GPIO 49
```

This is the opposite of the earlier firmware assumption that used GPIO7/GPIO8 because those are the Waveshare board's native I2C0 silk labels. GPIO7/GPIO8 are physically at Pico pins 17/16 in a same-position adapter, not at Pico pins 9/10.

### LCD SPI/control

| PicoCalc LCD net | Pico physical pin | ESP32-P4 GPIO/net |
|---|---:|---|
| LCD SCK / Pico GP10 | 14 | GPIO3 |
| LCD MOSI / Pico GP11 | 15 | GPIO2 |
| LCD MISO / Pico GP12 | 16 | GPIO8 |
| LCD CS / Pico GP13 | 17 | GPIO7 |
| LCD DC / Pico GP14 | 19 | GPIO24 |
| LCD RST / Pico GP15 | 20 | GPIO25 |

Caveat: GPIO2/3/24/25 have JTAG/USB-Serial-JTAG caveats on ESP32-P4. This mapping is physically convenient but not the same as the earlier function-optimized SPI2 IO_MUX mapping on GPIO28/29/30/31.

### PicoCalc SD slot

| PicoCalc SD net | Pico physical pin | ESP32-P4 GPIO/net |
|---|---:|---|
| SD MISO / Pico GP16 | 21 | GPIO48 |
| SD CS / Pico GP17 | 22 | GPIO47 |
| SD SCK / Pico GP18 | 24 | GPIO46 |
| SD MOSI / Pico GP19 | 25 | GPIO33 |
| SD CD / Pico GP22 | 29 | GPIO26 |

This mapping is mostly compatible with the previous optional SD mapping except MISO/SCK are swapped relative to the function-optimized table. Use the physical table above if routing straight pin-position to pin-position.

### PicoCalc PSRAM positions

| PicoCalc PSRAM net | Pico physical pin | ESP32-P4 GPIO/net |
|---|---:|---|
| PSRAM SIO0 / GP2 | 4 | GPIO31 |
| PSRAM SIO1 / GP3 | 5 | GPIO30 |
| PSRAM SIO2 / GP4 | 6 | GPIO29 |
| PSRAM SIO3 / GP5 | 7 | GPIO28 |
| PSRAM SCK / GP21 | 27 | GPIO27 |
| PSRAM CS / GP20 | 26 | GPIO32 |

Recommendation remains: do not use PicoCalc PSRAM. ESP32-P4 has verified 32 MB stacked PSRAM.

### PicoCalc PWM audio

| PicoCalc audio net | Pico physical pin | ESP32-P4 GPIO/net |
|---|---:|---|
| Audio L / Pico GP26 | 31 | GPIO23 |
| Audio R / Pico GP27 | 32 | GPIO22 |

This corrects the earlier function-optimized table, which used GPIO27/GPIO32. Under physical-position mapping, audio lands on GPIO23/GPIO22.

## Mismatches from the earlier function-optimized mapping

The previous adapter design mixed two concepts: assigning the best ESP32-P4 pins by function, and physically replacing the Pico socket position-for-position. The images confirm that a position-for-position adapter produces these corrections:

| PicoCalc function | Earlier function-optimized ESP32 GPIOs | Correct same-position physical ESP32 GPIOs |
|---|---|---|
| Keyboard I2C | GPIO7/GPIO8 | GPIO49/GPIO50 |
| LCD SCK/MOSI/CS/DC/RST | GPIO30/29/28/31/49 | GPIO3/2/7/24/25 |
| SD MISO/CS/SCK/MOSI/CD | GPIO46/47/48/33/52 or 26 | GPIO48/47/46/33/26 |
| Audio L/R | GPIO27/GPIO32 | GPIO23/GPIO22 |
| Pico UART0 TX/RX | GPIO51/GPIO52 | GPIO52/GPIO51 by physical position |

The earlier mapping is still valid as a **custom interposer with cross-routing**, but it is not the pinout of a straight same-position adapter.

## Immediate firmware implication

For the current keyboard diagnostic driver in `0098`, if the physical adapter really maps position-for-position, change:

```c
#define PICOCALC_KBD_I2C_SDA_GPIO      7
#define PICOCALC_KBD_I2C_SCL_GPIO      8
```

to:

```c
#define PICOCALC_KBD_I2C_SDA_GPIO      50
#define PICOCALC_KBD_I2C_SCL_GPIO      49
```

assuming the Pico 2 image's GP7=SDA1 and GP6=SCL1 labels match the PicoCalc keyboard wiring. The earlier `kbd` NACK on GPIO7/GPIO8 is expected under the same-position adapter because those pins would be connected to Pico physical pins 17 and 16, not the keyboard pins 9 and 10.

## Verification checklist before changing hardware or firmware

1. Confirm mechanical orientation: both USB connectors at the same end, same side of PCB, no mirror inversion.
2. Use continuity mode on the adapter:
   - Pico socket pin 9 / GP6 / SDA should beep to Waveshare GPIO50 pad/header.
   - Pico socket pin 10 / GP7 / SCL should beep to Waveshare GPIO49 pad/header.
   - Pico socket pin 14 should beep to Waveshare GPIO3.
   - Pico socket pin 15 should beep to Waveshare GPIO2.
3. Change the keyboard firmware constants to GPIO49/GPIO50.
4. Build and flash.
5. Run:

```text
kbd status
```

6. If still NACKing, run a minimal scanner over GPIO49/GPIO50 and check power/ground/SDA/SCL idle levels.
