---
Title: PicoCalc Keyboard Implementation Guide
Ticket: ESP32-P4-PICOCALC
Status: active
Topics:
    - esp32-p4
    - picocalc
    - firmware-port
    - keyboard
    - i2c
DocType: design-doc
Intent: implementation-guide
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../code/wesen/2026-05-05--ulisp-picocalc/pico-sdk-picocalc-wm/src/input/keyboard.cpp
      Note: Reference Pico SDK keyboard register-read behavior
    - Path: ../../../../../../../../../../code/wesen/2026-05-05--ulisp-picocalc/pico-sdk-picocalc-wm/src/input/keymap.cpp
      Note: Reference special key names and host-side mapping behavior
    - Path: 0098-esp32-p4-wifi6-webserver/main/app_main.c
      Note: |-
        Console command integration point for keyboard bring-up
        Console command integration for kbd status/poll/raw diagnostics
    - Path: 0098-esp32-p4-wifi6-webserver/main/picocalc_keyboard.c
      Note: |-
        ESP-IDF PicoCalc keyboard I2C driver planned/implemented from this guide
        ESP-IDF PicoCalc keyboard I2C driver implementation
    - Path: 0098-esp32-p4-wifi6-webserver/main/picocalc_keyboard.h
      Note: Public keyboard driver constants
ExternalSources:
    - ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/sources/picocalc-hardware-spec-pipapo.md
Summary: Detailed implementation guide for reading the PicoCalc STM32 keyboard controller from ESP32-P4 over I2C
LastUpdated: 2026-06-01T18:15:00-04:00
WhatFor: Implement and validate PicoCalc keyboard input on the Waveshare ESP32-P4-WIFI6 replacement board
WhenToUse: Before wiring PicoCalc keyboard pins to ESP32-P4 GPIO50/GPIO49 on the same-position physical adapter or debugging keyboard I2C/console behavior
---


# PicoCalc Keyboard Implementation Guide

## Executive summary

The PicoCalc keyboard is presented to the host MCU as an I2C peripheral, not as a directly scanned row/column matrix. A southbridge STM32 scans the physical keyboard and exposes a small register protocol at 7-bit I2C address `0x1F`. The host polls a status register to learn how many key events are queued, then reads two-byte `{state, key}` records from a FIFO register.

For the same-position Waveshare ESP32-P4-WIFI6 replacement adapter, the first implementation should use ESP32-P4 GPIO50/GPIO49 at 10 kHz. The verified physical mapping is Pico GP6/SDA/physical pin 9 to Waveshare GPIO50, and Pico GP7/SCL/physical pin 10 to Waveshare GPIO49. The first firmware goal is diagnostic, not a full UI stack: initialize the I2C bus, expose `kbd status`, `kbd poll`, and `kbd raw on/off` on the existing CH343 UART console, and print raw events so the physical wiring and STM32 protocol can be verified independently of display or shell integration.

## Scope

This guide covers:

1. Electrical and logical keyboard connection.
2. ESP-IDF driver shape for ESP32-P4.
3. Console commands for bring-up.
4. Validation steps and expected output.
5. Known risks when the keyboard bus is shared with onboard Waveshare I2C devices.

This guide does not cover:

1. LCD rendering of typed text.
2. Full terminal/editor keybinding integration.
3. Keyboard backlight, battery, power-off, or firmware update support beyond naming the relevant registers.

## Hardware connection

### Original PicoCalc host connection

The original PicoCalc design routes the keyboard/southbridge I2C bus to the Pico host pins:

| PicoCalc net | Original host pin | Function |
|---|---:|---|
| Keyboard SDA | GP6 | I2C SDA |
| Keyboard SCL | GP7 | I2C SCL |

The PicoCalc hardware reference notes external 4.7 kΩ pull-up resistors on these lines. The existing Pico SDK firmware initializes this as `i2c1` at 10 kHz.

### ESP32-P4 adapter mapping

For the ESP32-P4-WIFI6 adapter phase, use:

| PicoCalc net | ESP32-P4 GPIO | Function |
|---|---:|---|
| Keyboard SDA / Pico GP6 / physical pin 9 | GPIO50 | ESP32-P4 software I2C SDA on same-position adapter |
| Keyboard SCL / Pico GP7 / physical pin 10 | GPIO49 | ESP32-P4 software I2C SCL on same-position adapter |

Earlier notes considered using the Waveshare board's native labeled I2C0 pins GPIO7/GPIO8, but the supplied pinout images showed that a same-position RPico adapter routes the PicoCalc keyboard pins to GPIO50/GPIO49 instead. GPIO7/GPIO8 would only be correct for a custom cross-routed interposer, not the current physical adapter.

## Southbridge protocol

### Address

The STM32 keyboard/southbridge uses 7-bit I2C address:

```text
0x1F
```

### Register map subset

| Register | Name | Access | Purpose |
|---:|---|---|---|
| `0x01` | `REG_ID_VER` | R | Firmware version |
| `0x02` | `REG_ID_CFG` | R/W | Configuration |
| `0x03` | `REG_ID_INT` | R/W | Interrupt status |
| `0x04` | `REG_ID_KEY` | R | Key FIFO status |
| `0x05` | `REG_ID_BKL` | R/W | LCD backlight, 0–255 |
| `0x06` | `REG_ID_DEB` | R/W | Debounce configuration |
| `0x07` | `REG_ID_FRQ` | R/W | Polling frequency |
| `0x08` | `REG_ID_RST` | W | Southbridge reset command |
| `0x09` | `REG_ID_FIF` | R | Keyboard FIFO event record |
| `0x0A` | `REG_ID_BK2` | R/W | Keyboard backlight, 0–255 |
| `0x0B` | `REG_ID_BAT` | R | Battery voltage/percentage |
| `0x0E` | `REG_ID_OFF` | W | Power-off command |

### Status register

Read one byte from register `0x04`.

| Bits | Meaning |
|---|---|
| 0–4 | FIFO event count |
| 5 | Caps Lock indicator |
| 6 | Num Lock indicator |
| 7 | Reserved/unused for first bring-up |

Masks:

```c
#define PICOCALC_KBD_COUNT_MASK 0x1F
#define PICOCALC_KBD_CAPS_LOCK  0x20
#define PICOCALC_KBD_NUM_LOCK   0x40
```

### FIFO register

Read two bytes from register `0x09`.

| Byte | Meaning |
|---:|---|
| 0 | Event state |
| 1 | Key code |

Event states:

| Value | Meaning |
|---:|---|
| `1` | Pressed |
| `2` | Hold/repeat generated by STM32 firmware |
| `3` | Released |

A record `{0, 0}` should be treated as no valid event.

## Polling algorithm

The host should use a conservative transaction sequence compatible with the existing Pico SDK driver:

1. Transmit one byte: register ID `0x04`.
2. Read one byte: status.
3. Compute `fifo_count = status & 0x1F`.
4. If `fifo_count == 0`, stop.
5. Transmit one byte: register ID `0x09`.
6. Delay 2 ms.
7. Read two bytes: state and key code.
8. If the event is not `{0, 0}`, report it.
9. Repeat until the FIFO is empty or a caller-provided limit is reached.

The 2 ms delay after selecting the FIFO register exists because prior firmware found that the keyboard STM32 needs a short register-dispatch delay. Keep it for bring-up; optimize only after logic-analyzer or long-run evidence shows it is unnecessary.

## ESP-IDF implementation plan

### Driver module

Add a small module under the existing `0098-esp32-p4-wifi6-webserver/main/` directory:

```text
picocalc_keyboard.h
picocalc_keyboard.c
```

The driver should own:

1. I2C bus initialization for GPIO50/GPIO49 at 10 kHz.
2. Device attachment for address `0x1F`.
3. Register reads with explicit write-delay-read sequencing.
4. Error counters and last-status tracking for console diagnostics.
5. A minimal key-name helper for common special keys.

Suggested public API:

```c
typedef struct {
    uint8_t state;
    uint8_t key;
    bool valid;
} picocalc_key_event_t;

typedef struct {
    bool initialized;
    uint8_t last_status;
    uint32_t error_count;
} picocalc_keyboard_diag_t;

esp_err_t picocalc_keyboard_init(void);
esp_err_t picocalc_keyboard_read_status(uint8_t *status);
uint8_t picocalc_keyboard_fifo_count(uint8_t status);
esp_err_t picocalc_keyboard_poll_event(picocalc_key_event_t *event);
void picocalc_keyboard_get_diag(picocalc_keyboard_diag_t *diag);
const char *picocalc_keyboard_key_name(uint8_t key);
```

### Console commands

Add one top-level console command:

```text
kbd status
kbd poll [limit]
kbd raw on
kbd raw off
```

Command behavior:

| Command | Behavior |
|---|---|
| `kbd status` | Read status register and print raw byte, FIFO count, caps, num, init state, and error count |
| `kbd poll` | Drain a bounded number of pending events and print each `{state, key}` |
| `kbd poll 20` | Poll up to 20 events |
| `kbd raw on` | Start a background task that polls and prints events continuously |
| `kbd raw off` | Stop the background raw-print task |

The background raw task should poll gently, e.g. every 20 ms. It should not block the Wi-Fi/HTTP tasks for long periods. It should only print when an event is read.

### Output format

Use fixed, grep-friendly output:

```text
kbd init: sda=7 scl=8 speed=10000 addr=0x1f
kbd status ok=1 raw=0x03 fifo=3 caps=0 num=0 errors=0
kbd event state=1 key=0x68 ascii='h' name=
kbd event state=3 key=0x68 ascii='h' name=
kbd event state=1 key=0xb4 ascii=. name=left
```

This format preserves raw protocol evidence while still making printable keys readable.

## Key-code interpretation

Printable keys usually arrive as ASCII in the `0x20`–`0x7E` range. The STM32 firmware handles keyboard scanning and emits key events; host firmware should initially preserve raw events rather than forcing an early UI interpretation.

Common special key codes:

| Code | Key |
|---:|---|
| `0x08` | Backspace |
| `0x09` | Tab |
| `0x0A` | Enter |
| `0x81`–`0x90` | F1–F10 |
| `0x91` | Power |
| `0xA1` | Left Alt |
| `0xA2` | Left Shift |
| `0xA3` | Right Shift |
| `0xA4` | Sym |
| `0xA5` | Left Ctrl |
| `0xB1` | Escape/Break |
| `0xB4` | Left Arrow |
| `0xB5` | Up Arrow |
| `0xB6` | Down Arrow |
| `0xB7` | Right Arrow |
| `0xC1` | Caps Lock |
| `0xD0` | Break |
| `0xD1` | Insert |
| `0xD2` | Home |
| `0xD4` | Delete |
| `0xD5` | End |
| `0xD6` | Page Up |
| `0xD7` | Page Down |

## Validation procedure

### Before flashing or monitoring

Check serial ownership. The Waveshare board console is the CH343 UART bridge:

```bash
PORT=/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00
lsof "$PORT" || true
```

Do not run multiple monitors or probe scripts against the same serial device.

### Build

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0098-esp32-p4-wifi6-webserver
. $HOME/esp/esp-idf-5.4.2/export.sh
idf.py build
```

### Flash

```bash
idf.py -p "$PORT" flash
```

### Console validation

Use a real terminal/tmux pane for interactive console work, or use pyserial for scripted capture. `idf.py monitor` needs standard input attached to a TTY.

Commands to run after boot:

```text
help
kbd status
kbd poll 10
kbd raw on
# press normal keys, arrows, modifiers, function keys
kbd raw off
```

Expected no-key status when the keyboard is connected and idle:

```text
kbd status ok=1 raw=0x00 fifo=0 caps=0 num=0 errors=0
```

Expected disconnected or miswired behavior:

```text
kbd status ok=0 err=ESP_ERR_TIMEOUT initialized=1 errors=1
```

The exact error may be `ESP_ERR_TIMEOUT` or another I2C error depending on line state.

## Failure modes and debugging

### Timeout on every status read

Likely causes:

1. Keyboard SDA/SCL not connected.
2. SDA/SCL swapped.
3. PicoCalc southbridge unpowered.
4. Missing shared ground.
5. I2C bus held low by wiring fault.

Debug steps:

1. Confirm PicoCalc keyboard/southbridge has power.
2. Measure idle SDA/SCL high level.
3. Verify ESP32-P4 GPIO50 goes to PicoCalc GP6/SDA and GPIO49 goes to GP7/SCL.
4. Run a slow I2C scanner if needed, but keep the default keyboard path at 10 kHz.

### Status works but FIFO reads return no events

Likely causes:

1. No key was pressed.
2. Host is polling too slowly and only seeing released/empty state.
3. Register dispatch delay is too short.

Debug steps:

1. Use `kbd raw on` and press keys repeatedly.
2. Keep the 2 ms FIFO delay.
3. Increase raw polling cadence only after confirming no Wi-Fi/console starvation.

### Printable characters have unexpected case/symbols

The STM32 may emit printable ASCII and separate modifier events. Preserve raw events first. Do not assume the ESP32-P4 host must rescan or remap the physical key matrix. Later UI code can maintain modifier state if required, as the RP2350 firmware did.

### Interference with codec/IMU

If a future custom interposer moves the keyboard onto the Waveshare native GPIO7/GPIO8 I2C bus, the slow 10 kHz keyboard setting may affect the ES8311 codec or BNO085 sharing that bus. Options:

1. Keep a single 10 kHz bus if all devices tolerate it.
2. Dynamically use per-device speeds if the ESP-IDF bus/device configuration supports this cleanly.
3. Move the PicoCalc keyboard bus to a separate ESP32-P4 I2C controller/pin pair if board routing allows.

## Implementation checklist

1. Add `picocalc_keyboard.h` and `picocalc_keyboard.c`.
2. Add the new source to `main/CMakeLists.txt`.
3. Add `esp_driver_i2c` to component requirements.
4. Initialize the keyboard driver in `app_main()` before starting the console.
5. Register the `kbd` console command.
6. Build with ESP-IDF v5.4.2.
7. Flash only after checking serial ownership.
8. Validate `kbd status` with keyboard disconnected or connected.
9. Validate `kbd raw on/off` once the PicoCalc keyboard bus is physically wired.
10. Record results in the ticket diary.

## References

- `sources/picocalc-hardware-spec-pipapo.md`: PicoCalc southbridge I2C protocol, registers, key states, key codes, and hardware notes.
- `/home/manuel/code/wesen/2026-05-05--ulisp-picocalc/pico-sdk-picocalc-wm/src/input/keyboard.cpp`: Existing Pico SDK register-read sequence, including 2 ms FIFO delay and 50 ms timeout.
- `/home/manuel/code/wesen/2026-05-05--ulisp-picocalc/pico-sdk-picocalc-wm/src/input/keymap.cpp`: Existing host-side special key names and UI mapping behavior.
