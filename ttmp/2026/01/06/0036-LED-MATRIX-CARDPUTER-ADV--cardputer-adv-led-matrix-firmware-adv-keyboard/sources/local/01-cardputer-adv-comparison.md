---
Title: "Cardputer vs Cardputer-ADV comparison (imported notes)"
Ticket: 0036-LED-MATRIX-CARDPUTER-ADV
Status: active
Topics:
  - cardputer
  - esp32s3
  - keyboard
  - sensors
DocType: reference
Intent: short-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Imported notes comparing Cardputer vs Cardputer-ADV hardware differences and key pin-level deltas."
LastUpdated: 2026-01-06T19:38:40-05:00
WhatFor: ""
WhenToUse: ""
---

[![M5Stack-Cardputer base shield — Zephyr Project Documentation](https://tse4.mm.bing.net/th/id/OIP.hIWSft3d11--UUXTHEuVpQHaHa?pid=Api)](https://docs.zephyrproject.org/latest/boards/shields/m5stack_cardputer/doc/index.html?utm_source=chatgpt.com)

### What actually changes between Cardputer and Cardputer-ADV (for firmware)

From M5Stack’s own comparison + pin maps, the big differences are: ([M5Stack Docs][1])

* **Core module**: Cardputer uses **Stamp-S3**, ADV uses **Stamp-S3A**. (Same ESP32-S3 class + 8MB flash, but different module) ([M5Stack Docs][1])
* **Keyboard interface (this is the #1 breaking change)**

  * Cardputer: keyboard is scanned via **GPIO + 74HC138** (you’re driving address lines and reading scan outputs). ([M5Stack Docs][2])
  * ADV: keyboard is a **TCA8418 I²C keypad controller** with an **INT** pin. ([M5Stack Docs][2])
* **Audio path (often the #2 breaking change)**

  * Cardputer: **NS4168** I2S amp + **SPM1423** digital mic. ([M5Stack Docs][2])
  * ADV: **ES8311 audio codec** (+ amp/speaker) and a headphone jack. ([M5Stack Docs][1])
* **New sensor**: ADV adds **BMI270 IMU** on I²C. ([M5Stack Docs][1])
* **More expansion**: ADV adds the **EXT 2.54-14P** header, and the pins that used to be tied up in keyboard scanning are now generally more usable externally. ([M5Stack Docs][1])
* **Battery**: ADV is a single **1750mAh** pack vs Cardputer’s smaller internal + base battery setup. ([M5Stack Docs][1])

### Pin-level deltas you must account for

The “unchanged” parts (nice surprise):

* **LCD**: still ST7789V2 on the same SPI-ish pins: **G38 BL, G33 RST, G34 DC/RS, G35 MOSI/DAT, G36 SCK, G37 CS**. ([M5Stack Docs][2])
* **microSD**: still **G12 CS, G14 MOSI, G40 SCK, G39 MISO**. ([M5Stack Docs][2])
* **IR TX**: still **G44**. ([M5Stack Docs][2])
* **Battery ADC**: still **G10**. ([M5Stack Docs][2])

The “breaking” parts:

#### Keyboard

* **Cardputer (old)**:

  * Uses **G7/G6/G5/G4/G3/G15/G13** and **G11/G9/G8** with a **74HC138** scan scheme. ([M5Stack Docs][2])
* **Cardputer-ADV**:

  * Uses **I²C on G8 (SDA) + G9 (SCL)** and **INT on G11** with **TCA8418**. ([M5Stack Docs][2])

So if your old firmware bit-banged/scanned the keyboard (or used those GPIOs for other stuff), you must replace that with an I²C keypad-controller driver (or use a library that already supports ADV).

> Small gotcha: you’ll see people refer to the TCA8418 address as **0x34** or **0x69** — that’s just **7-bit vs 8-bit** convention. Many libraries default to **0x34 (7-bit)**. ([CircuitPython Documentation][3])

#### Audio

* **Cardputer (old)**: direct amp + mic wiring (NS4168 + SPM1423). ([M5Stack Docs][2])
* **ADV**: ES8311 codec:

  * Control via **I²C on G8/G9**
  * I2S signals on **G41/G46/G43/G42** (per M5’s pinmap). ([M5Stack Docs][2])

If you were talking directly to NS4168 / SPM1423 (I2S + PDM/I2S mic), you’ll need to switch to a codec-driven setup on ADV.

### What you need to change to “recompile from Cardputer → Cardputer-ADV”

Depends on how abstracted your firmware is:

#### If you use M5Unified / M5Cardputer (Arduino or ESP-IDF)

* In many cases it’s **mostly a dependency bump**, because the M5Cardputer library is explicitly meant to support **both Cardputer and Cardputer-ADV**. 
* Make sure you’re on versions new enough for ADV keyboard support (M5’s keyboard doc calls out minimum versions). 
* Ensure you’re calling the “update/poll” function frequently (common source of “keyboard doesn’t work”). 

#### If you have “bare” ESP-IDF drivers with your own pin definitions

Make a “board port” layer and change these parts:

1. **Keyboard driver**: replace “GPIO scan” with **TCA8418 over I²C (G8/G9) + interrupt (G11)**. ([M5Stack Docs][2])
2. **Audio driver**: replace NS4168/SPM1423 handling with **ES8311 codec init + I2S routing**. ([M5Stack Docs][2])
3. **I²C buses**: ADV has a *real* internal I²C bus on **G8/G9** for keyboard/imu/audio, while the Grove port still exists separately. ([M5Stack Docs][2])
4. **Optional**: add BMI270 IMU support (same I²C bus). ([M5Stack Docs][1])
5. **Review any custom GPIO usage**: pins that were “quiet” or “busy” on Cardputer (because the keyboard scan used them) may be available/repurposed on ADV and also appear on the EXT header. ([M5Stack Docs][2])

A very practical reference point: M5’s own ESP-IDF “UserDemo” has a **CardputerADV branch** and explicitly references a **TCA8418** library and newer ESP-IDF toolchain. ([GitHub][4])

If you tell me what stack you’re using (Arduino/M5Unified vs pure ESP-IDF, and whether you’re doing custom keyboard/audio), I can turn this into a precise “diff checklist” for your repo (files + exact constants to change).

[1]: https://docs.m5stack.com/en/core/Cardputer-Adv "m5-docs"
[2]: https://docs.m5stack.com/en/core/Cardputer "m5-docs"
[3]: https://docs.circuitpython.org/projects/tca8418/en/stable/api.html?utm_source=chatgpt.com "API Reference — Adafruit CircuitPython TCA8418 Library ..."
[4]: https://github.com/m5stack/M5Cardputer-UserDemo/tree/CardputerADV "GitHub - m5stack/M5Cardputer-UserDemo at CardputerADV"


---
