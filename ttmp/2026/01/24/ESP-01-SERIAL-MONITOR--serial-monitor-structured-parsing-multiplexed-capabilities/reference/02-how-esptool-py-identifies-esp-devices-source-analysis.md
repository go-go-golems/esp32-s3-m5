---
Title: How esptool.py identifies ESP devices (source analysis)
Ticket: ESP-01-SERIAL-MONITOR
Status: active
Topics:
    - serial
    - console
    - tooling
    - esp-idf
    - usb-serial-jtag
    - debugging
    - display
    - esp32s3
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esptool/cmds.py
      Note: detect_chip() chip autodetection algorithm
    - Path: ../../../../../../../../../../.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esptool/loader.py
      Note: connect/sync + get_security_info() + get_chip_id()
    - Path: ../../../../../../../../../../.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esptool/targets/esp32s3.py
      Note: ESP32-S3 description + eFuse-derived identity
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-24T19:32:44.31420999-05:00
WhatFor: ""
WhenToUse: ""
---


# How esptool.py identifies ESP devices (source analysis)

## Goal

Explain (from the **actual `esptool` source code**) how `esptool.py`:

1. selects and filters serial ports,
2. connects and synchronizes with the ROM bootloader (and detects stubs),
3. identifies the chip family/type (ESP32-S3 vs ESP32-C6, etc.),
4. derives “device facts” (revision, package, features, MAC, USB mode),
5. and what it cannot identify without firmware cooperation (board model, peripherals).

This is intended as a “reputable” internal reference for implementing `esper`’s own identification/probing and for explaining to users what is and isn’t knowable from the host.

## Context

This analysis is based on the locally installed `esptool` in the ESP-IDF Python environment on this machine:

- ESP-IDF: 5.4.1 (`IDF_PATH=/home/manuel/esp/esp-idf-5.4.1`)
- esptool: 4.10.0
- esptool package root:
  - `/home/manuel/.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esptool/`

All code references below point at those exact files.

## Quick Reference

### What esptool uses to identify “the device”

`esptool` identifies **chip family/type** primarily by one of two ROM-level signals:

1. **Chip ID from “get security info”** (preferred; ESP32-C3 and later; works even in Secure Download Mode)
   - ROM command: `ESP_GET_SECURITY_INFO` (`0x14`)
   - extracted field: `chip_id`
   - used in: `esptool/cmds.py` → `detect_chip()`

2. **Magic value read from a fixed ROM address** (fallback; ESP8266/ESP32/ESP32-S2 and some failure modes)
   - ROM read-reg command: `ESP_READ_REG` (`0x0A`)
   - register address: `ESPLoader.CHIP_DETECT_MAGIC_REG_ADDR = 0x40001000`
   - used in: `esptool/cmds.py` → `detect_chip()`

Once the chip type is known, esptool reads more descriptive identity information using chip-specific logic (mostly eFuse reads via `read_reg()`), implemented in `esptool/targets/*.py` (for example `targets/esp32s3.py`).

### What esptool does *not* identify reliably

Without cooperative firmware, `esptool` generally cannot identify:

- the *board model* (Cardputer vs AtomS3R vs custom),
- pin mappings and attached peripherals,
- “which app is running” in a user-friendly way (beyond reading flash and interpreting images).

It can identify chip family, revision, MAC, and some packaging/feature fuses, which is a different level of identity.

## Detailed Analysis

### 1) Port selection, filtering, and USB transport hints

At the CLI layer, `esptool` supports:

- `--port` (and `ESPTOOL_PORT`)
- `--port-filter` values (filters can be `vid=...`, `pid=...`, `name=...`, `serial=...`)

Even when a port is explicitly selected, some identification is derived from the host’s view of the USB device. For example, `ESPLoader._get_pid()` attempts to learn the USB PID by using pyserial’s port enumeration. This is used to choose reset strategies, including a dedicated USB Serial/JTAG reset behavior:

- `USB_JTAG_SERIAL_PID = 0x1001` in `esptool/loader.py`
- reset strategy selection in `_construct_reset_strategy_sequence()`:
  - `if mode == "usb_reset" or self._get_pid() == self.USB_JTAG_SERIAL_PID: return (USBJTAGSerialReset(...),)`

This is “transport identification” (how the host talks to the chip), not “chip family identification.”

### 2) Connection and sync: how esptool establishes a ROM command channel

Before chip identification works, esptool needs to talk to the ROM bootloader protocol. The logic is in `ESPLoader.connect()` in `esptool/loader.py`.

Key behaviors:

- It cycles through reset strategies based on OS and transport.
- After reset, `_connect_attempt()` reads boot log bytes and checks whether the chip is “waiting for download,” producing targeted “wrong boot mode” errors when appropriate.
- It then calls `sync()` repeatedly until it succeeds.

`sync()` sends:

```text
op = ESP_SYNC (0x08)
data = 0x07 0x07 0x12 0x20 + 32 bytes of 0x55
timeout = SYNC_TIMEOUT
```

and reads additional responses. It sets `sync_stub_detected` if the response pattern looks like a flasher stub (ROM responds with a non-zero “val,” stub responds with 0).

This matters for identification because `detect_chip()` can wrap a ROM instance in a `STUB_CLASS` if a stub is detected, and because some commands differ between ROM and stub loaders.

### 3) Chip type autodetection: the primary algorithm

Chip type autodetection is implemented in `esptool/cmds.py` as `detect_chip()`. It does two-stage detection:

#### Stage A: chip ID via security info (preferred)

It first tries:

- `detect_port.get_chip_id()` → implemented in `ESPLoader.get_chip_id()` which reads `get_security_info()["chip_id"]`.

`get_security_info()`:

- sends the ROM command `ESP_GET_SECURITY_INFO` (`0x14`),
- unpacks either a 20-byte or 12-byte response (ESP32-S2 is the notable “no chip_id in security info” case),
- parses flags including `SECURE_DOWNLOAD_ENABLE`.

Then `detect_chip()` matches `chip_id` against each target class’ `IMAGE_CHIP_ID` (for classes that *don’t* use magic-value detection).

#### Stage B: magic value read (fallback)

If Stage A fails (`UnsupportedCommandError` or `FatalError`), it falls back to:

- `detect_port.read_reg(ESPLoader.CHIP_DETECT_MAGIC_REG_ADDR)` with `CHIP_DETECT_MAGIC_REG_ADDR = 0x40001000`
- then matches the returned value to per-class `MAGIC_VALUE` for classes that use magic-value detection.

Special case: ESP32-S2

- `detect_chip()` has an ESP32-S2 branch because ESP32-S2 does not provide `chip_id` in the `get_security_info()` response, but still supports secure download mode and flags.

This two-stage approach is the primary “source of truth” for how esptool identifies chip family/type.

### 4) Verifying `--chip` arguments after connect

Even when `--chip` is provided, `ESPLoader.connect()` will try to verify whether the connected chip matches the class:

- if `chip_id` exists, it compares it (or maps it) to expected `IMAGE_CHIP_ID`,
- otherwise it compares magic values when possible,
- and it may either warn (“doesn’t appear to be ... will attempt to continue”) or error (“Wrong --chip argument?”).

This is not cosmetic: it helps prevent flashing the wrong images to the wrong chip family.

### 5) From “chip type” to “chip description” and features

Once esptool selects a target class, it can compute a richer description by reading chip-specific registers/eFuses.

Example: `ESP32S3ROM.get_chip_description()` in `esptool/targets/esp32s3.py`:

- reads revision from eFuse fields (`get_major_chip_version()`, `get_minor_chip_version()`),
- reads package type (`get_pkg_version()`), and maps it to “QFN56” vs “PICO-1 (LGA56)” variants,
- returns a human-friendly string including revision.

Similarly, some target classes can infer whether USB Serial/JTAG or USB-OTG is being used by reading a ROM variable (`UARTDEV_BUF_NO`) and comparing against per-chip constants (e.g., `UARTDEV_BUF_NO_USB_JTAG_SERIAL = 4`).

This is “chip/package identity,” not board identity.

### 6) Implications for `esper` device recognition

If `esper` wants “identification,” there are three levels:

1. **Host-side scan (non-invasive)**: enumerate ports and score likely ESP devices using sysfs/USB info and stable `/dev/serial/by-id` names. This is low risk but probabilistic.
2. **ROM bootloader probe (authoritative chip family)**: enter download mode and run the same ROM commands:
   - try security-info chip_id first,
   - else read the magic register value.
   This is authoritative but invasive (requires reset/download mode).
3. **Firmware handshake (best board/app identity)**: keep app running and have firmware print an identity/capabilities message (JSON or a structured line). This is what enables “Cardputer vs AtomS3R” and “which app is running.”

## Usage Examples

### Inspect the exact source used in this analysis

Key files:

- chip detection entrypoint: `/home/manuel/.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esptool/cmds.py`
- bootloader protocol + security info: `/home/manuel/.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esptool/loader.py`
- ESP32-S3 target specifics: `/home/manuel/.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esptool/targets/esp32s3.py`

### Reproduce the identification logic (conceptually)

1. Connect + sync.
2. Try security info:

```text
ESP_GET_SECURITY_INFO -> parse flags + chip_id
```

3. If not supported:

```text
ESP_READ_REG at 0x40001000 -> compare to per-chip MAGIC_VALUE
```

4. Use chip-specific eFuse parsing to compute `get_chip_description()`.

## Related

- `ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/design-doc/02-go-serial-monitor-idf-py-compatible-design-implementation.md` (what `esper` wants to be compatible with and how we plan to implement it)
