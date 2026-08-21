---
Title: ESP-IDF ST25R3916 NFC reader console app — analysis, design, and implementation guide
Ticket: ESP-60-M5STACKCHAN-NFC
Status: active
Topics:
    - m5stackchan
    - nfc
    - st25r3916
    - esp32-s3
    - esp-idf
    - esp-console
    - intern-guide
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: sources/code/ST25R3916_definition.hpp
      Note: Register constants and direct-command opcodes for the ST25R3916 (extracted from M5Unit-NFC) — the coding reference
    - Path: sources/code/unit_ST25R3916.hpp
      Note: M5Unit-NFC register-level driver class — reference for the init + NFC-A poll sequence
    - Path: sources/code/PY32IOExpander_Class.hpp
      Note: Firmware's existing I2C driver pattern to mirror (new driver/i2c_master.h API)
    - Path: sources/code/stackchan-board-config.h
      Note: I2C pins (SDA=GPIO12, SCL=GPIO11) and board pin map
    - Path: sources/code/BSP-NFC-Detect-example.ino
      Note: Official Arduino NFC-A detect example (logic to port to ESP-IDF)
ExternalSources:
    - Path: sources/web/01-elechouse-ST25R3916-esp32-readme.md
      Note: ESP32-focused ST25R3916 driver fork docs (SPI + I2C validation notes)
    - Path: sources/datasheets/README-download-instructions.md
      Note: ST25R3916B official datasheet URL + community links
WhatFor: Build a standalone ESP-IDF NFC reader for the M5StackChan that reads tags over a USB Serial/JTAG console before any UI work.
WhenToUse: Onboarding a new intern to NFC + the StackChan firmware; reference for implementing Phase 1 (console reader) and planning Phase 2 (UI).
---

# ESP-IDF ST25R3916 NFC Reader Console App: Analysis, Design, and Implementation Guide

**Ticket:** ESP-60-M5STACKCHAN-NFC
**Audience:** New intern — you can read C and use a terminal, but you have never touched this codebase or written an NFC driver before.
**Goal:** Build a small ESP-IDF firmware that reads NFC tags from the M5StackChan's on-board ST25R3916 reader IC and prints the tag's UID, ATQA, and SAK over a USB Serial/JTAG console. No graphical UI in this phase — we first prove the reader works from a text console, then we build a nicer UI later.

---

## 1. Executive Summary

The M5StackChan (SKU K151) is a desktop robot whose **body module** contains a full NFC reader built around the **ST25R3916** NFC controller IC. The chip is wired to the ESP32-S3's I2C bus at address **0x50**. The official M5Stack firmware runs on ESP-IDF, but **none of its existing code touches NFC** — the only NFC examples ship in the separate *Arduino* BSP (`StackChan-BSP/examples/NFC/`) and use the Arduino-only `M5UnitUnifiedNFC` library.

This guide explains everything you need to understand the system and then build a **standalone ESP-IDF firmware** (a fresh project, not a Mooncake app) that:

1. Initializes the shared I2C bus exactly the way the StackChan firmware does.
2. Talks to the ST25R3916 over I2C using the ESP-IDF `driver/i2c_master.h` API (the same pattern the firmware already uses for its PY32IOExpander).
3. Brings the reader up, turns the RF field on, polls for ISO14443-A tags, and runs anticollision to get the UID.
4. Exposes the whole thing through `esp_console` commands typed over USB Serial/JTAG (`nfc scan`, `nfc probe`, `nfc read`, `nfc field on|off`).

We deliberately scope this to a **console reader first**. A polished on-device UI is Phase 2 and is only sketched here.

The guide is organized in four layers:

1. **System architecture** — the hardware and software you are targeting.
2. **NFC fundamentals** — how ISO14443-A tag polling actually works, so the code is not magic.
3. **Design and implementation** — the ESP-IDF project, a minimal ST25R3916 component, the console commands, with pseudocode and API references.
4. **Build, flash, and verify** — exact commands and expected output on this exact machine.

---

## 2. Problem Statement and Scope

### 2.1 What we want

A firmware you can flash to the connected StackChan that, over the USB serial console, lets you:

- `nfc scan` — I2C bus scan (confirm 0x50 is alive).
- `nfc probe` — read the ST25R3916 chip identity (confirm it is a real ST25R3916, type `0x05`).
- `nfc field on` / `nfc field off` — toggle the 13.56 MHz RF field.
- `nfc read` — poll once for an ISO14443-A tag and print `UID`, `ATQA`, `SAK`, and a provisional type guess.
- `nfc poll` — continuously poll (every ~500 ms) and print each newly seen tag.

### 2.2 What is explicitly out of scope (Phase 1)

- No LVGL / display UI. Output is text only, over `esp_console`.
- No tag writing, NDEF parsing, or MIFARE Classic authentication. We only **read** the UID layer.
- No NFC-B / NFC-F / NFC-V (ISO14443-B, FeliCa, ISO15693). Phase 1 is ISO14443-A only.
- No integration into the Mooncake app launcher. This is a standalone project.

### 2.3 Why a standalone project (not a Mooncake app)

The full StackChan firmware is large (dual-OTA partition scheme, XiaoZhi AI agent, audio, camera, LVGL). For a focused NFC reader we want a small, fast-to-build firmware that uses the **same I2C pins and the same driver style** as the real firmware, without pulling in the entire Mooncake + XiaoZhi + LVGL stack. Per the project's `AGENTS.md`, component dependencies go in `main/idf_component.yml`, and we may create our own ESP-IDF component for the ST25R3916 driver. This keeps the reader tiny and easy to debug.

---

## 3. System Architecture

### 3.1 Hardware overview

The M5StackChan is an M5Stack **CoreS3** host (an ESP32-S3 module) plugged into a **robot body module**. The two talk over a shared I2C bus and a UART bus.

- **CoreS3 host (ESP32-S3):** dual-core Xtensa @ 240 MHz, 16 MB SPI flash, 8 MB PSRAM, Wi-Fi + BLE 5, USB-C that is **both** power and a built-in USB JTAG/serial debug interface (no separate USB-UART chip).
- **Display:** 2" ILI9342C IPS LCD, 320×240, FT6336U capacitive touch.
- **Body module peripherals (all on the shared I2C bus unless noted):**
  - 2× SCS0009 feedback servos on **UART** (GPIO 6/7) at 1 Mbps — *not I2C*.
  - 12× RGB LEDs — driven **through** the PY32IOExpander's on-chip LED controller (not a WS2812 chain).
  - **ST25R3916 NFC controller — I2C address 0x50** ← this is the star of this guide.
  - Si12T capacitive touch sensor — I2C 0x68 (head petting).
  - INA226 battery monitor — I2C 0x41.
  - PY32L020 I/O expander — I2C 0x6F (controls servo power on pin 0, an RGB/enable output on pin 13, and the 12 LED channels).

```
┌────────────────────────────────────────────────────────┐
│                    CoreS3 (Host)                       │
│  ESP32-S3 @ 240 MHz  │  16 MB Flash  │  8 MB PSRAM     │
│  Wi-Fi + BLE 5        │  USB-C = CDC/JTAG (ttyACM0)     │
│                                                        │
│  Display: ILI9342C 320×240 (SPI)   Touch: FT6336 (0x38) │
│  Camera: GC0308   Audio: ES7210+AW88298 (I2S)           │
│  IMU: BMI270+BMM150   PMIC: AXP2101 (0x34)              │
│  RTC: BM8563 (0x51)   IO Expander(host): AW9523 (0x58)  │
└──────────────┬─────────────────────────────────────────┘
               │ shared I2C bus  SDA=GPIO12  SCL=GPIO11  (I2C port 1)
               │ + UART for servos (GPIO 6/7)
┌──────────────▼─────────────────────────────────────────┐
│                   Robot Body                           │
│  Servos: 2× SCS0009 on UART (NOT I2C)                 │
│  RGB LEDs: 12×  via PY32IOExpander LED controller      │
│  >>> NFC: ST25R3916  I2C 0x50  <<<   (this guide)       │
│  Touch: Si12T (0x68)   Battery: INA226 (0x41)          │
│  IO Expander(body): PY32L020 (0x6F)                    │
│  IR: IRM56384 receiver + transmitter                   │
└────────────────────────────────────────────────────────┘
```

**The single most important fact for this project:** the ST25R3916 is a slave on the **same I2C bus** as everything else (port 1, SDA=GPIO12, SCL=GPIO11). You do not need new wires. You only need to address 0x50.

### 3.2 Software stack

The *real* StackChan firmware is built on ESP-IDF and layers several frameworks:

```
┌───────────────────────────────────────────────────┐
│  Mooncake apps (Launcher, AI Agent, Avatar, ...) │
├───────────────────────────────────────────────────┤
│  Mooncake App Framework   │  XiaoZhi AI Framework │
├──────────────────┬────────────────────────────────┤
│  StackChan Subsys│  HAL (Hardware Abstraction)    │
├──────────────────┴────────────────────────────────┤
│  LVGL 9.4 + smooth_ui_toolkit                      │
├───────────────────────────────────────────────────┤
│  ESP-IDF v5.5.x + FreeRTOS                         │  ← we live here
├───────────────────────────────────────────────────┤
│  ESP32-S3 Hardware                                 │
└───────────────────────────────────────────────────┘
```

Our **standalone reader** does not use Mooncake, XiaoZhi, or LVGL. It lives one layer down — directly on **ESP-IDF v5.5.x + FreeRTOS**, talking to the I2C bus and the ST25R3916. This is intentional: it is the smallest firmware that exercises the NFC hardware, and it is the easiest to debug.

### 3.3 I2C bus on this board (the exact contract)

The firmware initializes the shared I2C bus in `main/hal/board/stackchan.cc` (`M5StackCoreS3Board::InitializeI2c`). This is the authoritative configuration you must match:

```c
// From StackChan/firmware/main/hal/board/stackchan.cc (InitializeI2c)
i2c_master_bus_config_t i2c_bus_cfg = {
    .i2c_port          = (i2c_port_t)1,          // I2C port 1
    .sda_io_num        = GPIO_NUM_12,            // AUDIO_CODEC_I2C_SDA_PIN
    .scl_io_num        = GPIO_NUM_11,            // AUDIO_CODEC_I2C_SCL_PIN
    .clk_source        = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .intr_priority     = 0,
    .trans_queue_depth = 0,
    .flags = { .enable_internal_pullup = 1 },
};
ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus_));
```

Key points:

- **Port 1**, **SDA=GPIO12**, **SCL=GPIO11**. Internal pull-ups are enabled. (The body board also has external pull-ups; internal ones are fine for bring-up.)
- This uses the **new** I2C master driver (`driver/i2c_master.h`), *not* the legacy `i2c.h` API. Every device on this bus is added with `i2c_master_bus_add_device()` + `i2c_master_transmit/transmit_receive()`.
- The firmware even has a built-in I2C scanner (`I2cDetect()`) that probes 0x00–0x7f with `i2c_master_probe()`. We will reuse that exact pattern for `nfc scan`.

### 3.4 Known I2C address map (so you do not get confused)

| Address | Device | Notes |
|--------:|--------|-------|
| 0x34 | AXP2101 PMIC | Power management; controls backlight DLDO1 |
| 0x38 | FT6336U touch | Display capacitive touch |
| 0x41 | INA226 | Battery voltage/current (body) |
| 0x50 | **ST25R3916 NFC** | **← our target** |
| 0x58 | AW9523B | Host-side IO expander (display/key backlight) |
| 0x68 | Si12T | Body capacitive touch (head petting) |
| 0x6F | PY32L020 | Body IO expander (servo power, RGB enable, LED controller) |

If `nfc scan` shows `0x50` present, the chip is on the bus. If it does not, see §11 troubleshooting (power, body not seated, wrong port).

---

## 4. NFC Fundamentals (read this before the code)

You do not need to be an NFC expert to build Phase 1, but you must understand the **ISO14443-A polling sequence**, because the ST25R3916 makes you drive it at the frame level. Here is the whole model in one page.

### 4.1 The actors

- **PCD** (Proximity Coupling Device) = the reader = our ST25R3916. It generates the 13.56 MHz RF field and modulates it.
- **PICC** (Proximity Integrated Circuit Card) = the tag (NTAG, MIFARE Ultralight, MIFARE Classic, etc.). It is passive: it harvests power from the field and talks back by load-modulating the field.

### 4.2 The ISO14443-A anti-collision dance

When a tag enters the field, the PCD must discover it and get a unique ID. The standard sequence is:

```
PCD (ST25R3916)                         PICC (tag)
   │  1. Turn RF field ON
   │  2. REQA  (0x26, 7-bit, no CRC)  ────────►
   │                                       │ tag powers up
   │  ◄────────  ATQA (2 bytes)            │  "I'm here"
   │  3. ANTICOLLISION CL1 (0x93, 0x20) ─► │
   │  ◄────────  UID + BCC (up to 5 bytes) │
   │  4. SELECT CL1 (0x93, 0x70, UID, BCC) ►│
   │  ◄────────  SAK (1 byte)              │  "selected; maybe more UID levels"
   │  (if SAK says cascade: repeat CL2/CL3 with 0x95 / 0x97)
```

- **REQA** (Request Type A) = `0x26`, sent as a short 7-bit frame with no CRC.
- **ATQA** (Answer To Request) = 2 bytes the tag sends back. It hints at the UID size and tag family.
- **UID** = the tag's unique identifier. Can be 4 bytes (single cascade level, CL1), 7 bytes (CL1+CL2), or 10 bytes (CL1+CL2+CL3). NTAG213/215/216 and MIFARE Ultralight use **7-byte UIDs** (two cascade levels).
- **SAK** (Select Acknowledge) = 1 byte returned after SELECT. It tells you whether more cascade levels remain and gives a *provisional* type guess (e.g. `0x00` = MIFARE Ultralight family, `0x08` = MIFARE 1K, `0x09` = MIFARE Mini, `0x18` = 4K). SAK is only a hint — definitive identification needs more commands.
- **BCC** = block check character = XOR of the UID bytes in that cascade level.

### 4.3 What Phase 1 prints

For each detected tag, Phase 1 prints:

- `UID` as hex (the full cascaded UID, e.g. `04:34:56:78:9A:BC:DE`).
- `ATQA` as 4-hex (e.g. `0044`).
- `SAK` as 2-hex (e.g. `00`).
- A provisional type string derived from SAK (e.g. `"MIFARE Ultralight / NTAG"`).

That is enough to "read" a tag for our purposes. Reading NDEF or user memory is Phase 2+.

### 4.4 The ST25R3916's role

The ST25R3916 is a "NFC front-end": it handles the analog RF, the framing, the CRC, and the FIFO. You give it **direct commands** and read/write its **registers** over I2C. For ISO14443-A it can:

- Transmit REQA/WUPA with one direct command (`CMD_TRANSMIT_REQA` / `CMD_TRANSMIT_WUPA`).
- Tell you how many bytes came back from the FIFO.
- Send arbitrary frames (for anticollision) by loading the FIFO and triggering a transmit.
- Generate and check CRCs automatically for standard frames.

So our driver is, in essence: *configure chip → field on → send REQA → read ATQA from FIFO → run anticollision frames → read UID + SAK from FIFO*.

---

## 5. The ST25R3916 over I2C

### 5.1 I2C register access protocol

The ST25R3916 is an I2C slave at **0x50** (7-bit address). Its register access uses a **command byte** before the register address:

- **Read register:** master writes `[cmd_byte]` where `cmd_byte = (reg & 0x3F) | 0x40`, then a **repeated-start** read of N bytes. (`0x40` is the read direction bit; `reg & 0x3F` is the register index in "Space A".)
- **Write register:** master writes `[cmd_byte] [data...]` where `cmd_byte = (reg & 0x3F) | 0x00` (write direction).
- **Direct command:** master writes `[cmd]` (e.g. `0xC1` = `CMD_SET_DEFAULT`), optionally followed by data bytes.
- **Space B registers:** prefix the command with `CMD_REGISTER_SPACEB_ACCESS` (`0xFB`) — not needed for Phase 1.

This is captured in the M5Unit-NFC driver (`sources/code/unit_ST25R3916.hpp`):

```cpp
// read:   command byte = (reg & 0x3F) | OP_READ_REGISTER   (OP_READ_REGISTER = 0x40)
// write:  command byte = (reg & 0x3F) | OP_WRITE_REGISTER  (OP_WRITE_REGISTER = 0x00)
```

Concretely, with the ESP-IDF I2C master API:

- **read_register8(reg):** `i2c_master_transmit_receive(dev, &cmd, 1, &val, 1, timeout)` where `cmd = (reg & 0x3F) | 0x40`.
- **write_register8(reg, val):** `i2c_master_transmit(dev, buf, 2, timeout)` where `buf = { (reg & 0x3F), val }`.
- **direct_command(cmd):** `i2c_master_transmit(dev, &cmd, 1, timeout)`.

All register constants are in `sources/code/ST25R3916_definition.hpp`. The key ones for Phase 1:

| Symbol | Value | Meaning |
|--------|------:|---------|
| `REG_IC_IDENTITY` | 0x00 | Chip identity register (read to confirm chip) |
| `VALID_IDENTIFY_TYPE` | 0x05 | Expected IC type for ST25R3916/7 |
| `CMD_SET_DEFAULT` | 0xC1 | Reset chip to power-up state |
| `CMD_STOP_ALL_ACTIVITIES` | 0x00 (cmd) | Stop RF, TX, RX, clear FIFO |
| `CMD_CLEAR_FIFO` | — | Clear the FIFO |
| `CMD_ADJUST_REGULATORS` | — | Tune internal voltage regulators |
| `CMD_TRANSMIT_REQA` | — | Send ISO14443-A REQA |
| `CMD_TRANSMIT_WUPA` | — | Send ISO14443-A WUPA (wakeup-all) |
| `CMD_NFC_INITIAL_FIELD_ON` | — | Turn the RF field on |
| `REG_MAIN_INTERRUPT` | — | IRQ status (RX done, collision, etc.) |
| `MAX_FIFO_DEPTH` | 512 | FIFO capacity |

### 5.2 The bring-up / init sequence

Derived from `M5Unit-NFC/src/unit/unit_ST25R3916.cpp` (`UnitST25R3916::begin()`). This is the order that reliably brings the chip from cold to ready:

```
1. read IC identity  -> REG_IC_IDENTITY; type must == 0x05 (ST25R3916)
2. writeDirectCommand(CMD_STOP_ALL_ACTIVITIES)   // stop any leftover RF/TX/RX
3. writeDirectCommand(CMD_SET_DEFAULT)            // reset to power-up defaults
4. (optional) writeDirectCommand(CMD_TEST_ACCESS, protection)  // unlock test regs
5. writeDirectCommand(CMD_CLEAR_FIFO)
6. configure voltage regulators, then writeDirectCommand(CMD_ADJUST_REGULATORS)
7. read regulator/display register to confirm regulators settled
8. chip is now READY; RF field is OFF until you turn it on
```

### 5.3 Field on + REQA + anticollision (the read sequence)

Derived from `unit_ST25R3916_nfca.cpp`:

```
A. writeModeDefinition(0xC8)  -> target = ST25R3916, tech = NFC-A, bitrate-detection mode
B. Turn field ON: writeDirectCommand(CMD_NFC_INITIAL_FIELD_ON); set guard timer
C. REQA:  writeDirectCommand(CMD_TRANSMIT_REQA)
          wait for IRQ (RX-done or error) with timeout
          read FIFO -> 2 bytes = ATQA  (atqa = rbuf[1]<<8 | rbuf[0])
D. ANTICOLLISION (per cascade level, start at CL1 = 0x93):
     anticoll_frame = { SEL, NVB=0x20, ... }   // 0x20 = "send UID, all bits"
     writeFIFO(anticoll_frame)
     set number-of-transmitted-bytes
     trigger transmit
     wait for IRQ (I_rxe | I_col)  // received or collision
     if collision: narrow NVB to the collision bit, re-transmit, loop
     else: rbuf = UID bytes (+BCC); go to SELECT
E. SELECT (SEL, NVB=0x70, UID, BCC):
     writeFIFO(select_frame); transmit; wait IRQ; read FIFO -> SAK = rbuf[0]
     if SAK indicates more cascade levels (cascade bit set): next CL (0x95, 0x97), goto D
     else: UID complete
F. deactivate: writeDirectCommand to stop, optionally turn field off
```

You do **not** have to implement the full collision-narrowing loop for Phase 1 if only one tag is in the field (the common case). Send `NVB=0x20` (full UID request); if a single tag is present there is no collision and you get the UID directly. Leave the collision-narrowing loop as a documented TODO.

---

## 6. Current-State Firmware Analysis (what already exists)

### 6.1 What the StackChan firmware has

- **I2C bus init** at `main/hal/board/stackchan.cc::InitializeI2c` — port 1, GPIO 12/11, internal pullup. This is your reference for bus setup.
- **I2C scanner** at `M5StackCoreS3Board::I2cDetect()` — uses `i2c_master_probe()`. This is your reference for `nfc scan`.
- **An I2C driver to copy the style of:** `main/hal/drivers/PY32IOExpander_Class/PY32IOExpander_Class.{hpp,cpp}` — a clean, small class that:
  - takes an `i2c_master_bus_handle_t` in its constructor,
  - calls `i2c_master_bus_add_device()` with a 7-bit address and 100 kHz,
  - does `writeRegister8` via `i2c_master_transmit`,
  - does `readRegister8` via `i2c_master_transmit_receive`.
  **Mirror this exact structure for the ST25R3916 driver.**
- **`esp_console` usage** is everywhere in this monorepo (e.g. `0096-m5dial-dithered-3d/main/console_commands.{c,h}`, `0094-tab5-wifi-bench/main/wifi_console.c`). The pattern is: a `*_console_register(void)` function that calls `esp_console_cmd_reg_t` for each command, plus `esp_console_new_repl_*` + a REPL task. On ESP32-S3 we use the **USB Serial/JTAG** console (see `AGENTS.md`).

### 6.2 What the StackChan firmware does NOT have

- **No NFC code anywhere in the ESP-IDF firmware.** Grep for `st25r`/`nfc`/`rfal` in `StackChan/firmware/main` returns nothing relevant. NFC is only in the Arduino BSP examples.
- The Arduino BSP NFC examples (`StackChan-BSP/examples/NFC/Detect/Detect.ino`) use `m5::unit::UnitNFC` + `M5UnitUnifiedNFC` over `M5.In_I2C`. That is an Arduino library stack (`M5Unified`, `M5GFX`, `M5UnitUnified`, `M5UnitUnifiedNFC`). It is not trivial to use from bare ESP-IDF because it pulls in M5Unified/M5GFX.

### 6.3 Decision: minimal from-scratch driver vs. porting M5UnitUnified

Two options were considered:

- **Option A — port `M5UnitUnified` + `M5UnitUnifiedNFC` to ESP-IDF.** Pros: battle-tested, full ISO14443-A/B/F/V + NDEF + emulation. Cons: drags in `M5Unified` + `M5GFX` (huge, display-oriented), the `Component` I2C abstraction layer, and a different threading model. High friction for a "just read a UID" firmware.
- **Option B — write a minimal ESP-IDF ST25R3916 driver from scratch**, using `ST25R3916_definition.hpp` as the register reference and the firmware's `PY32IOExpander_Class` as the I2C style template. Pros: tiny, matches the firmware's conventions, fully debuggable, no Arduino dependencies. Cons: we implement only the ISO14443-A poll path (fine for Phase 1); we must get the init sequence right (we copy it verbatim from `unit_ST25R3916.cpp`).

**Decision: Option B.** A minimal from-scratch driver in a self-contained ESP-IDF component. The M5Unit-NFC sources are kept in `sources/code/` as the authoritative reference to copy the sequence from. If Phase 2 needs NDEF/ISO15693, we can later bridge to Option A.

### 6.4 Decision record

- **Context:** We need a Phase-1 NFC reader that reads ISO14443-A UIDs over a console on the StackChan's ST25R3916.
- **Options:** (A) port M5UnitUnifiedNFC; (B) minimal from-scratch ESP-IDF driver.
- **Decision:** Option B.
- **Rationale:** Smallest blast radius, matches firmware I2C conventions, no Arduino/display dependencies, fastest to debug on a console.
- **Consequences:** Phase 1 is ISO14443-A read-only; NDEF/ISO15693/writing are deferred. If those are later required, revisit Option A.
- **Status:** accepted.

---

## 7. Design: The Standalone ESP-IDF NFC Reader App

### 7.1 Project layout

Create a new project directory in the monorepo, e.g. `0115-m5stackchan-nfc-reader/` (the next free slot after `0114-papers3-pulp-os`):

```
0115-m5stackchan-nfc-reader/
├── CMakeLists.txt
├── sdkconfig.defaults
├── partitions.csv                 # small app; can use default factory, or a custom table
├── main/
│   ├── CMakeLists.txt
│   ├── idf_component.yml          # component deps go HERE (per AGENTS.md), not project root
│   ├── nfc_reader_main.c          # app_main: init I2C, init console, register cmds, REPL task
│   ├── st25r3916/
│   │   ├── st25r3916.h            # public API
│   │   ├── st25r3916.c            # driver: I2C read/write, init, field, poll
│   │   └── st25r3916_regs.h       # register + command constants (from ST25R3916_definition.hpp)
│   └── nfc_console.c/.h           # esp_console command registration
└── components/                    # (only if we later split st25r3916 into a reusable component)
```

Per `AGENTS.md`:

- Put dependencies in **`main/idf_component.yml`**, not a project-root one (a root manifest is ignored → "unknown component" errors).
- `sdkconfig.defaults` seeds only absent options. To force a change, `rm -f sdkconfig && idf.py build`.
- Target: `esp32s3`.
- Console on ESP32-S3: prefer **USB Serial/JTAG** (`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`).

### 7.2 `sdkconfig.defaults` (baseline)

```
CONFIG_IDF_TARGET="esp32s3"
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
# CONFIG_ESP_CONSOLE_UART is not set
CONFIG_ESP_MAIN_TASK_STACK_SIZE=8192
CONFIG_COMPILER_OPTIMIZATION_SIZE=y
# PSRAM not required for the reader; leave default unless you add a UI later
```

### 7.3 I2C bus init (mirrors the firmware)

```c
// Pseudocode — nfc_reader_main.c
#include "driver/i2c_master.h"

#define NFC_I2C_PORT   I2C_NUM_1
#define NFC_I2C_SDA    GPIO_NUM_12
#define NFC_I2C_SCL    GPIO_NUM_11

static i2c_master_bus_handle_t s_i2c_bus;

static esp_err_t i2c_init(void) {
    i2c_master_bus_config_t cfg = {
        .i2c_port          = NFC_I2C_PORT,
        .sda_io_num        = NFC_I2C_SDA,
        .scl_io_num        = NFC_I2C_SCL,
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority     = 0,
        .trans_queue_depth = 0,
        .flags             = { .enable_internal_pullup = 1 },
    };
    return i2c_new_master_bus(&cfg, &s_i2c_bus);
}
```

### 7.4 The ST25R3916 driver (component sketch)

Public API (`st25r3916.h`):

```c
typedef struct {
    uint8_t  type;        // IC type (should be 0x05)
    uint8_t  revision;    // IC revision
} st25r3916_id_t;

typedef struct {
    uint8_t  uid[10];
    uint8_t  uid_len;      // 4, 7, or 10
    uint16_t atqa;
    uint8_t  sak;
} nfc_picc_t;

esp_err_t st25r3916_init(i2c_master_bus_handle_t bus);   // add device + bring up chip
esp_err_t st25r3916_read_id(st25r3916_id_t *out);
esp_err_t st25r3916_field_on(void);
esp_err_t st25r3916_field_off(void);
esp_err_t st25r3916_poll_nfca(nfc_picc_t *out);         // REQA + anticollision -> UID/ATQA/SAK
```

Implementation sketch (`st25r3916.c`):

```c
// Pseudocode — st25r3916.c
#include "driver/i2c_master.h"
#include "st25r3916_regs.h"

#define ST25R_ADDR      0x50
#define I2C_FREQ_HZ     400000   // chip supports up to 1 MHz; 400 kHz is safe
#define I2C_TIMEOUT_MS  100

static i2c_master_dev_handle_t s_dev;

// --- low-level register access (mirrors PY32IOExpander_Class) ---
static esp_err_t rd8(uint8_t reg, uint8_t *out) {
    uint8_t cmd = (reg & 0x3F) | 0x40;                 // read direction
    return i2c_master_transmit_receive(s_dev, &cmd, 1, out, 1, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}
static esp_err_t wr8(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = { (reg & 0x3F), val };            // write direction
    return i2c_master_transmit(s_dev, buf, 2, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}
static esp_err_t cmd(uint8_t c) {
    return i2c_master_transmit(s_dev, &c, 1, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}
static esp_err_t fifo_write(const uint8_t *d, size_t n) { /* write to FIFO reg */ }
static esp_err_t fifo_read(uint8_t *d, size_t n)        { /* read from FIFO reg */ }

// --- init (copy sequence from unit_ST25R3916.cpp::begin) ---
esp_err_t st25r3916_init(i2c_master_bus_handle_t bus) {
    i2c_device_config_t dev = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = ST25R_ADDR,
        .scl_speed_hz    = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev, &s_dev));

    st25r3916_id_t id;
    st25r3916_read_id(&id);
    if (id.type != 0x05) return ESP_ERR_NOT_FOUND;     // not a ST25R3916

    cmd(CMD_STOP_ALL_ACTIVITIES);
    cmd(CMD_SET_DEFAULT);
    cmd(CMD_CLEAR_FIFO);
    // configure regulators (REG_REGULATOR_VOLTAGE_CONTROL 0x2C) then:
    cmd(CMD_ADJUST_REGULATORS);
    // optionally read regulator/display to confirm settled
    return ESP_OK;
}

// --- poll (copy sequence from unit_ST25R3916_nfca.cpp) ---
esp_err_t st25r3916_poll_nfca(nfc_picc_t *out) {
    // A. mode definition: target=ST25R3916, tech=NFC-A
    // B. field on (if not already)
    // C. REQA -> ATQA
    cmd(CMD_TRANSMIT_REQA);
    if (!wait_irq_rx_or_timeout(TIMEOUT_REQA)) return ESP_ERR_NOT_FOUND;
    uint8_t rbuf[2];
    fifo_read(rbuf, 2);
    out->atqa = (uint16_t)rbuf[1] << 8 | rbuf[0];

    // D+E. anticollision + select, cascade levels CL1(0x93)->CL2(0x95)->CL3(0x97)
    uint8_t sel = 0x93, level = 0;
    out->uid_len = 0;
    while (1) {
        uint8_t frame[7] = { sel, 0x20 };              // ANTICOLLISION, NVB=0x20 (all bits)
        fifo_write(frame, 2);
        set_tx_bytes(2);
        trigger_transmit();
        if (!wait_irq_rx_or_timeout(TIMEOUT_ANTICOLL)) return ESP_FAIL;
        uint8_t got[5];
        size_t n = fifo_read(got, 5);                  // UID + BCC
        // SELECT: { sel, 0x70, <got[0..n-1]> }
        uint8_t sel_frame[9] = { sel, 0x70 };
        memcpy(sel_frame + 2, got, n);
        fifo_write(sel_frame, 2 + n);
        set_tx_bytes(2 + n);
        trigger_transmit();
        wait_irq_rx_or_timeout(TIMEOUT_ANTICOLL);
        uint8_t sak;
        fifo_read(&sak, 1);
        // copy UID bytes (drop cascade tag 0x88 if present)
        // ...
        if ((sak & 0x04) == 0) {                        // no more cascade
            out->sak = sak;
            break;
        }
        sel = (sel == 0x93) ? 0x95 : 0x97;             // next cascade level
    }
    return ESP_OK;
}
```

> The `wait_irq_rx_or_timeout` helper polls `REG_MAIN_INTERRUPT` for the RX-done bit (and optionally the collision bit) with a bounded loop using `vTaskDelay(pdMS_TO_TICKS(1))`. For Phase 1, polling the IRQ register is simpler than wiring a GPIO IRQ line; the chip's IRQ pin is available but routing it is extra work for no Phase-1 benefit.

### 7.5 `esp_console` command set

On ESP32-S3 we use the **USB Serial/JTAG** REPL. Pattern (from `0096-m5dial-dithered-3d/main/console_commands.c`):

```c
// Pseudocode — nfc_console.c
#include "esp_console.h"
#include "linenoise/linenoise.h"
#include "esp_vfs_dev.h"
#include "driver/usb_serial_jtag.h"

static int cmd_scan(int argc, char **argv) {           // i2c bus scan, print 0x00..0x7f grid
    for (int a = 0; a < 128; a++) {
        esp_err_t r = i2c_master_probe(s_i2c_bus, a, pdMS_TO_TICKS(200));
        if (r == ESP_OK) printf("%02x ", a);
    }
    return 0;
}
static int cmd_probe(int argc, char **argv) {          // read IC identity
    st25r3916_id_t id; st25r3916_read_id(&id);
    printf("ST25R3916 type=0x%02x rev=0x%02x\n", id.type, id.revision);
    return 0;
}
static int cmd_field(int argc, char **argv) {          // "nfc field on|off"
    if (strcmp(argv[2], "on") == 0) st25r3916_field_on();
    else                            st25r3916_field_off();
    return 0;
}
static int cmd_read(int argc, char **argv) {           // single poll
    nfc_picc_t p;
    if (st25r3916_poll_nfca(&p) == ESP_OK) print_picc(&p);
    else printf("no tag\n");
    return 0;
}
static int cmd_poll(int argc, char **argv) {           // continuous poll
    while (loop) { cmd_read(...); vTaskDelay(pdMS_TO_TICKS(500)); }
}

void nfc_console_register(void) {
    esp_console_cmd_reg_t c = {};
    c.func = cmd_scan;  c.command = "nfc-scan";  c.help = "I2C bus scan";     esp_console_cmd_register(&c);
    c.func = cmd_probe; c.command = "nfc-probe"; c.help = "Read ST25R3916 ID"; esp_console_cmd_register(&c);
    c.func = cmd_field; c.command = "nfc-field"; c.help = "RF field on|off"; esp_console_cmd_register(&c);
    c.func = cmd_read;  c.command = "nfc-read";  c.help = "Poll one ISO14443-A tag"; esp_console_cmd_register(&c);
    c.func = cmd_poll;  c.command = "nfc-poll";   c.help = "Continuously poll"; esp_console_cmd_register(&c);
}
```

`app_main`:

```c
void app_main(void) {
    i2c_init();                       // port 1, GPIO 12/11
    st25r3916_init(s_i2c_bus);        // probe 0x50, bring up chip
    // console on USB Serial/JTAG
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    esp_console_dev_usb_serial_jtag_config_t dev_cfg = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    esp_console_new_repl_usb_serial_jtag(&dev_cfg, &repl_cfg, &repl);
    nfc_console_register();
    esp_console_start_repl(repl);     // runs REPL on the console task
}
```

### 7.6 Console output example (what success looks like)

```
I (312) nfc: I2C bus ready (port1 SDA12 SCL11)
I (340) st25r3916: chip id type=0x05 rev=0x02
esp> nfc-scan
0x34 0x38 0x41 0x50 0x58 0x68 0x6f
esp> nfc-field on
field ON
esp> nfc-read
PICC: UID=04:34:56:78:9A:BC:DE ATQA=0044 SAK=00 type=MIFARE Ultralight/NTAG
esp> nfc-poll
... press a tag ...
PICC: UID=04:34:56:78:9A:BC:DE ATQA=0044 SAK=00 type=MIFARE Ultralight/NTAG
```

---

## 8. Build, Flash, and Verify (exact commands on this machine)

### 8.1 Environment (from `AGENTS.md`)

- The StackChan firmware requires **ESP-IDF >= 5.5.2**. This machine has **`~/esp/esp-idf-5.5.4`**. Source *that* version:
  ```bash
  source ~/esp/esp-idf-5.5.4/export.sh
  ```
- The connected StackChan shows up as the Espressif USB JTAG/serial debug unit on **`/dev/ttyACM0`** (`usb-Espressif_USB_JTAG_serial_debug_unit_...-if00`). On ESP32-S3 the console is USB Serial/JTAG — *not* UART.
- **Serial ownership:** do not run two monitors/flashers on `/dev/ttyACM0` at once (per `AGENTS.md`). Close any `idf.py monitor` before flashing.

### 8.2 Build + flash + monitor

```bash
cd 0115-m5stackchan-nfc-reader
source ~/esp/esp-idf-5.5.4/export.sh
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash
idf.py -p /dev/ttyACM0 monitor
# in the monitor, type: nfc-scan, nfc-probe, nfc-field on, nfc-read
# Ctrl-] to exit monitor
```

### 8.3 Forced-config change trap (from `AGENTS.md`)

`sdkconfig.defaults` only seeds *absent* options. If you change partition/console/PSRAM defaults, force a re-seed:

```bash
rm -f sdkconfig && idf.py build
```

`idf.py fullclean` removes `build/` and `managed_components/` but **not** `sdkconfig` — this is a known trap that costs rebuild cycles.

### 8.4 Validation checklist

- [ ] `nfc-scan` shows `0x50` (and the other known addresses). If `0x50` is missing → power/body seating (§11).
- [ ] `nfc-probe` prints `type=0x05`. If type is wrong or read fails → I2C wiring/address.
- [ ] `nfc-field on` then `nfc-read` with an NTAG on the reader prints a 7-byte UID, `ATQA=0044`, `SAK=00`.
- [ ] `nfc-field off` and `nfc-read` returns "no tag".
- [ ] Remove the tag, `nfc-read` returns "no tag"; re-place it, `nfc-read` reads it again.

---

## 9. Phased Plan

### Phase 1 — Console reader (this guide, in scope)
- Standalone ESP-IDF project, minimal ST25R3916 component, ISO14443-A read only, `esp_console` over USB Serial/JTAG.
- Deliverable: flash, type `nfc-read`, see a UID.

### Phase 2 — On-device UI (out of scope here, sketched)
- Reuse the ST25R3916 component inside the real StackChan firmware as a **Mooncake app** (`app_nfc_reader`), following the `app_template` + `AppAbility` pattern (see the BLINKY intern guide, ticket `M5STACKCHAN-BLINKY`).
- LVGL screen: "Tap a tag" → show UID + type + a QR of the UID. Use `LvglLockGuard` for all LVGL calls (the firmware renders LVGL on a separate core/task — never touch LVGL without the lock).
- Optional: beep the speaker on detect (`M5.Speaker.tone(...)` as the Arduino example does).

### Phase 3+ (future)
- NDEF read/parse (URI, text records), ISO15693, tag writing, MIFARE Classic auth, integration with the XiaoZhi AI agent (e.g. "what's on this tag?"). At that point, re-evaluate porting `M5UnitUnifiedNFC` (Option A).

---

## 10. Risks, Open Questions, and Alternatives

- **NFC power path (OPEN).** The ST25R3916 is on the body board. Confirm whether it is always powered when the body is seated, or whether it must be enabled via the PY32IOExpander (0x6F) like servo power (pin 0). The firmware's `io_expander_init()` sets pin 0 (servo) and pin 13 (RGB enable); it does **not** obviously toggle an NFC power pin. Verify against the body schematic (`sources/datasheets` in the `M5STACKCHAN` research ticket has the schematic PDFs). If a power pin exists, add an `nfc_power_on()` step before `st25r3916_init`.
- **I2C bus contention.** The reader shares the bus with touch, PMIC, battery monitor, and the IO expander. In the *standalone* firmware nothing else is polling, so there is no contention. When later integrated into the full firmware, the ST25R3916 must share the bus politely (it is a normal I2C slave; just don't talk to it while another device transaction is mid-flight). The `i2c_master` driver serializes transactions per device on the same bus, so this is generally safe.
- **IRQ handling.** Phase 1 polls the IRQ register. This is fine for a console tool. If we want fast, low-CPU detection later, wire the ST25R3916 IRQ pin to a GPIO and use an interrupt + task notification. Identify the IRQ GPIO from the schematic.
- **RF safety / EMC.** Do not leave the field on indefinitely. Default `nfc-poll` to field-on only during the poll window, or add a max-on-time. Continuous field-on drains battery and can heat the coil.
- **Single-tag assumption.** The anticollision sketch sends `NVB=0x20` (full UID) and assumes one tag. With two tags you get a collision. Implement the bit-narrowing loop (§5.3 step D) before claiming multi-tag support.

---

## 11. Troubleshooting (diagnosis-first)

| Symptom | First check | Then |
|--------|-------------|------|
| `nfc-scan` does not list `0x50` | Body module seated? USB-C snug? | Power path (schematic); try `nfc-scan` a few times (cold probe) |
| `nfc-probe` reads `type=0x00` or I2C error | Address/pins correct? (`0x50`, port1, 12/11) | Drop I2C speed to 100 kHz; check pull-ups |
| `nfc-probe` ok but `nfc-read` always "no tag" | Field on? (`nfc-field on` first) | Antenna/coil connected? Tag is ISO14443-A? |
| `nfc-read` returns garbage UID | FIFO not cleared between polls | Add `CMD_CLEAR_FIFO` at start of each poll |
| `nfc-read` hangs | IRQ never set (poll loop) | Bound `wait_irq` with a timeout, return `ESP_ERR_TIMEOUT` |
| Build: `Failed to resolve component ... unknown name` | Deps in project-root `idf_component.yml`? | Move deps to `main/idf_component.yml` (AGENTS.md) |
| Build: Kconfig/link errors after IDF bump | Wrong IDF sourced? | `source ~/esp/esp-idf-5.5.4/export.sh`; `rm -f sdkconfig` |

---

## 12. API Reference

### 12.1 ESP-IDF I2C master driver (the only transport we need)

Header: `driver/i2c_master.h` (ESP-IDF 5.x new API).

- `i2c_new_master_bus(const i2c_master_bus_config_t *cfg, i2c_master_bus_handle_t *ret)` — create the bus.
- `i2c_master_bus_add_device(bus, const i2c_device_config_t *cfg, i2c_master_dev_handle_t *ret)` — add a 7-bit-address slave at a given speed.
- `i2c_master_transmit(dev, uint8_t *data, size_t len, TickType_t timeout)` — write (used for write-register + direct-command).
- `i2c_master_transmit_receive(dev, uint8_t *wr, size_t wr_len, uint8_t *rd, size_t rd_len, TickType_t timeout)` — write-then-read with repeated start (used for read-register).
- `i2c_master_probe(bus, uint8_t addr, TickType_t timeout)` — quick address probe (used by `nfc-scan`).
- `i2c_master_bus_rm_device(dev)` — remove a device.

Docs: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/i2c.html

### 12.2 `esp_console` (USB Serial/JTAG REPL)

Headers: `esp_console.h`, `esp_console_repl.h`, `driver/usb_serial_jtag.h`, `linenoise/linenoise.h`.

- `esp_console_new_repl_usb_serial_jtag(const esp_console_dev_usb_serial_jtag_config_t *dev, const esp_console_repl_config_t *repl, esp_console_repl_t **ret)` — create the REPL on USB Serial/JTAG.
- `esp_console_start_repl(esp_console_repl_t *repl)` — start it (it runs on its own task).
- `esp_console_cmd_register(const esp_console_cmd_t *cmd)` — register one command.
- `esp_console_cmd_t` fields: `.command`, `.help`, `.hint`, `.func`, `.argtable` (optional argtable for arg parsing).

### 12.3 ST25R3916 registers / commands (subset, see `sources/code/ST25R3916_definition.hpp`)

- `REG_IC_IDENTITY` (0x00) — read to confirm chip (`type==0x05`).
- `CMD_SET_DEFAULT` (0xC1) — reset to power-up state.
- `CMD_STOP_ALL_ACTIVITIES` — stop all RF/TX/RX/FIFO activity.
- `CMD_CLEAR_FIFO` — flush the FIFO.
- `CMD_ADJUST_REGULATORS` — tune internal regulators (call after `SET_DEFAULT`).
- `CMD_NFC_INITIAL_FIELD_ON` — turn the 13.56 MHz field on.
- `CMD_TRANSMIT_REQA` / `CMD_TRANSMIT_WUPA` — send ISO14443-A request / wakeup-all.
- `REG_MAIN_INTERRUPT` — read to check RX-done / collision / error IRQ bits.
- `MAX_FIFO_DEPTH` = 512 bytes.

Canonical reference: ST25R3916B datasheet (`sources/datasheets/README-download-instructions.md` has the URL).

---

## 13. Key File Reference

Within this ticket's `sources/code/` (authoritative copies, since the live repos are gitignored/cloned-on-demand):

| File | Why it matters |
|------|----------------|
| `sources/code/ST25R3916_definition.hpp` | All register + command constants. Copy these into `st25r3916_regs.h`. |
| `sources/code/unit_ST25R3916.hpp` | Reference driver class: init sequence, field on/off, REQA, anticollision. Read `begin()` and the `nfca_*` methods. |
| `sources/code/PY32IOExpander_Class.hpp` | The firmware's I2C driver style to mirror (`i2c_master_bus_add_device` + `transmit`/`transmit_receive`). |
| `sources/code/stackchan-board-config.h` | I2C pins (SDA=GPIO12, SCL=GPIO11) and the board pin map. |
| `sources/code/BSP-NFC-Detect-example.ino` | Official Arduino NFC-A detect logic (`nfc_a.detect(piccs)`, `identify(u)`, `uidAsString()`, `atqa`, `sak`). Port this behavior. |
| `sources/code/BSP-NFC-Emulation-example.ino` | Shows the 7-byte UID embedding + emulation; useful context for Phase 3. |

In the live StackChan firmware (clone to `StackChan/firmware/`):

- `main/hal/board/stackchan.cc` — `InitializeI2c` (bus config) and `I2cDetect` (scanner pattern).
- `main/hal/drivers/PY32IOExpander_Class/PY32IOExpander_Class.cpp` — I2C driver style to copy.
- `main/main.cpp` — the real `app_main` (Mooncake boot; our standalone project replaces this).

In this monorepo (esp32-s3-m5):

- `0096-m5dial-dithered-3d/main/console_commands.{c,h}` and `app_main.cpp` — a clean `esp_console` registration + REPL example.
- `AGENTS.md` — build/environment rules (IDF version, console, serial ownership, partition traps).
- Prior intern guide: ticket `M5STACKCHAN-BLINKY` — how to build/flash a custom app on this exact device, and the LVGL lock rules you will need in Phase 2.

---

## 14. Summary Diagrams

### 14.1 Boot + read flow

```
app_main
  │
  ├── i2c_init()                 port1 SDA=12 SCL=11  (i2c_new_master_bus)
  ├── st25r3916_init(bus)        add 0x50 device; SET_DEFAULT; ADJUST_REGULATORS
  ├── esp_console_new_repl_usb_serial_jtag()
  ├── nfc_console_register()     register nfc-scan/probe/field/read/poll
  └── esp_console_start_repl()   REPL runs on console task; user types commands

nfc-read command:
  st25r3916_field_on()
  └─ st25r3916_poll_nfca()
       ├── CMD_TRANSMIT_REQA  ─► wait IRQ ─► read FIFO ─► ATQA
       └── anticollision loop (CL1 0x93 / CL2 0x95 / CL3 0x97)
            ├── ANTICOLL frame (NVB=0x20) ─► wait IRQ ─► read FIFO ─► UID+BCC
            └── SELECT  frame (NVB=0x70)  ─► wait IRQ ─► read FIFO ─► SAK
                └── if SAK cascade bit set → next CL, else done
  print UID / ATQA / SAK / type
```

### 14.2 I2C transaction shapes

```
read_register8(reg):
  START | addr+W | (reg&0x3F)|0x40 | RESTART | addr+R | val | STOP
  -> i2c_master_transmit_receive(&cmd,1, &val,1, t)

write_register8(reg,val):
  START | addr+W | (reg&0x3F) | val | STOP
  -> i2c_master_transmit({(reg&0x3F),val}, 2, t)

direct_command(cmd):
  START | addr+W | cmd | STOP
  -> i2c_master_transmit(&cmd, 1, t)
```

---

## 15. Appendix: ESP32-S3 console note (from AGENTS.md)

On the ESP32-S3, prefer the **USB Serial/JTAG** console for interactive REPL work, because UART pins are frequently repurposed for peripherals. Set:

```
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
# CONFIG_ESP_CONSOLE_UART is not set
```

This is why we flash to `/dev/ttyACM0` (the Espressif USB JTAG/serial debug unit) and why `nfc-read` output appears in `idf.py monitor` directly — no separate USB-UART chip, no baud rate to guess. (The ESP32-P4 is the exception to this rule and uses a CH343 UART bridge; it does not apply to the S3-based StackChan.)
