# 0117 — M5StackChan Native ESP-IDF NFC Feature Explorer

This pure ESP-IDF 5.5.4 firmware maps the six official [StackChan NFC Arduino examples](https://docs.m5stack.com/en/arduino/stackchan/nfc) to an interactive USB Serial/JTAG console:

1. quick scan and detailed identification;
2. complete card dump;
3. MIFARE Ultralight and NTAG213 emulation;
4. direct page/block read and reversible write testing;
5. NDEF validation, read, parse, and replacement;
6. MIFARE Classic value-block inspection and wallet demonstrations.

The application creates the CoreS3 internal ESP-IDF I2C bus and attaches the official `M5Unit-NFC` protocol layer through `UnitUnified::add(i2c_master_bus_handle_t)`. It does not use Arduino or `Wire`.

`0115-m5stackchan-nfc-reader` remains the minimal C transport/RF/anticollision regression harness. This project uses the broader official C++ protocol layer for card-family operations, Crypto1, NDEF, ISO-DEP, value blocks, and ST25R3916 target emulation.

## Hardware

- target: ESP32-S3 / M5Stack CoreS3
- internal body bus: I2C port 1
- SDA: GPIO12
- SCL: GPIO11
- ST25R3916 address: `0x50`
- tag placement: the literal narrow top edge of the StackChan head
- console: USB Serial/JTAG at 115200 baud

Only one process may own the serial device. Prefer `/dev/serial/by-id/...`; `/dev/ttyACM0` is acceptable when stable.

## Build

The project is pinned to ESP-IDF 5.5.4. Dependencies belong in `main/idf_component.yml`; `dependencies.lock` records the exact transitive Git revisions.

```bash
cd 0117-m5stackchan-nfc-feature-explorer
source ~/esp/esp-idf-5.5.4/export.sh
idf.py set-target esp32s3   # first build only
idf.py build
```

If `sdkconfig.defaults` changes in a way that must replace an existing value:

```bash
rm -f sdkconfig
idf.py build
```

Flash a complete image when switching from Arduino or another project with a different partition layout:

```bash
idf.py -p /dev/ttyACM0 flash
```

## Reader commands

Reader mode is the default.

| Command | Official sketch equivalent | Mutation |
|---|---|---|
| `nfc-capabilities` | list the complete explorer surface | none |
| `nfc-scan [timeout_ms]` | Quick Scan Identification | none |
| `nfc-info` | Quick Scan Identification with full activation details | none |
| `nfc-dump` | Complete Data Reading | none |
| `nfc-raw-read <address>` | Direct Card Reading | none |
| `nfc-write-test <address> --confirm RESTORE-AFTER-TEST` | Direct Card Writing | writes, verifies, restores |
| `nfc-ndef-read` | NDEF Format Card Reading | none |
| `nfc-ndef-write-demo --confirm REPLACE-NDEF` | NDEF Format Card Writing | replaces current NDEF message |
| `nfc-value-inspect` | E-Wallet value-block discovery | none |
| `nfc-wallet-demo <block> <non-rechargeable\|rechargeable> --confirm MUTATE-CLASSIC` | E-Wallet | changes Classic access/data, then attempts restoration |

The default MIFARE Classic key is `FFFFFFFFFFFF`, matching the vendor examples. A failed authentication may put the card in HALT; commands deactivate or reactivate before returning.

## Tag emulation

Reader and target modes use different ST25R3916 initialization. `nfc-mode` stores the selected mode in NVS and reboots into it:

```text
nfc-mode
nfc-mode emulation-ultralight --confirm REBOOT
nfc-mode emulation-ntag213 --confirm REBOOT
nfc-emulation-status
nfc-mode reader --confirm REBOOT
```

The emulated tags use the official example UIDs and NDEF memory templates:

- Ultralight UID `04:34:56:78:9A:BC:DE`;
- NTAG213 UID `99:88:77:66:55:44:33`.

The firmware prints target state transitions: `off`, `idle`, `ready`, `active`, and `halt`.

## Safety

Read-only commands may be used on the current tag. Do **not** run `nfc-write-test`, `nfc-ndef-write-demo`, or `nfc-wallet-demo` until the physical tag is explicitly designated as sacrificial.

Confirmation tokens prevent accidental invocation; they do not verify that the intended tag is physically present. Communication loss during restoration can leave changed data or access conditions.

Version one intentionally refuses:

- converting a non-NDEF Ultralight tag to NDEF;
- formatting a DESFire card;
- writing manufacturer, lock, configuration, or sector-trailer areas through the reversible write command;
- automatic security-level upgrades.

## Upstream revisions

The direct dependency is pinned in `main/idf_component.yml`:

```text
M5Unit-NFC 93745b547364f310cd64b5155a870103a7800a5d
```

The committed lockfile records the resolved M5UnitUnified, M5Utility, and M5HAL revisions.
