# Provenance: captures 21–24 — native ESP-IDF NFC feature explorer

Date: 2026-08-22. Hardware: M5StackChan CoreS3, one physical tag on the narrow top-edge antenna. Firmware: `0117-m5stackchan-nfc-feature-explorer`, ESP-IDF 5.5.4, USB Serial/JTAG. Serial device: `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_44:1B:F6:E2:80:28-if00`. Each capture used one prompt-aware Python process with no concurrent monitor.

The firmware uses M5Unit-NFC commit `93745b547364f310cd64b5155a870103a7800a5d` through its native ESP-IDF component and gives M5UnitUnified an application-created `i2c_master_bus_handle_t` for I2C port 1, GPIO12/11.

## Capture 21 — first read-only run and HALT lifecycle defect

File: `21-native-feature-explorer-read-only.txt`.

The first native feature-explorer build initialized successfully and `nfc-scan` identified the tag as:

```text
uid=0491D44C9E6180
type="NTAG 215"
atqa=0044
sak=00
blocks=135
unit=4
user=504
total=540
first_user=4
last_user=129
ndef=1
```

The next `nfc-info`, `nfc-raw-read`, `nfc-ndef-read`, and `nfc-dump` commands failed at detection. Cause: multi-PICC enumeration deliberately HALTs discovered tags. A stationary tag therefore does not answer the next REQA. The initial wrapper assumed every new command started with an IDLE PICC.

## Capture 22 — WUPA activation fallback and complete read-only success

File: `22-native-feature-explorer-read-only-after-wupa-fix.txt`.

`activate_one()` was changed to:

1. send REQA;
2. if REQA fails, send WUPA;
3. SELECT;
4. identify;
5. reactivate the identified PICC.

The same prompt sequence then succeeded without moving the tag:

- `nfc-scan 1000`: one NTAG215 identified;
- `nfc-info`: successful through `source=WUPA`;
- `nfc-raw-read 0`: successful 16-byte read;
- `nfc-ndef-read`: valid NDEF Type 2 format, zero records;
- `nfc-dump`: all 135 pages read successfully.

Important card data:

```text
page 0: 04 91 D4 C9
page 1: 4C 9E 61 80
page 2: 33 48 00 00
page 3: E1 10 3E 00   # Type 2 capability container, 0x3E * 8 = 496 NDEF bytes
page 4: 03 00 FE 00   # zero-length NDEF Message TLV + Terminator
```

The tag is formatted for NDEF but currently contains no records. No writes occurred.

## Capture 23 — persisted emulation mode cycle

File: `23-native-feature-explorer-emulation-mode-cycle.txt`.

The script switched modes with the exact `REBOOT` confirmation and reopened the same USB device after each software reset.

Ultralight profile:

```text
uid=043456789ABCDE
type="MIFARE Ultralight"
atqa=0044
sak=00
memory=64
state=off
```

NTAG213 profile:

```text
uid=99887766554433
type="NTAG 213"
atqa=0044
sak=00
memory=180
state=off
```

Both profiles initialized their official memory templates and reported local status. No external NFC reader was present, so this capture does not prove over-the-air emulation. Reader mode was persisted again, the board rebooted, and `nfc-info` successfully read the physical NTAG215.

Opening USB Serial/JTAG produced the expected `rst:0x15 (USB_UART_CHIP_RESET)` resets. Software mode changes produced `rst:0xc (RTC_SW_CPU_RST)`.

## Capture 24 — mutation guards

File: `24-native-feature-explorer-mutation-guards.txt`.

The prompt-aware script intentionally omitted or supplied incorrect confirmation strings. Every state-changing command returned usage and a non-zero command result:

- `nfc-write-test` requires `RESTORE-AFTER-TEST`;
- `nfc-ndef-write-demo` requires `REPLACE-NDEF`;
- `nfc-wallet-demo` requires `MUTATE-CLASSIC`.

`nfc-value-inspect` activated the physical tag read-only and rejected it as non-Classic. No mutation command body executed.

## Build evidence

A clean ESP-IDF 5.5.4 build completed without local source warnings after command-structure cleanup:

```text
m5stackchan_nfc_feature_explorer.bin size 0x67760
factory partition 0x100000
free 0x988a0 (60%)
```

The dependency build printed:

```text
ESP-IDF I2C backend: i2c_master (new driver)
```

The generated `dependencies.lock` records the direct and transitive Git commits. `build/`, `managed_components/`, and `sdkconfig` are ignored.

## Scope conclusion

The native feature explorer now provides compiled command equivalents for all six official StackChan NFC Arduino sketch families. Hardware validation is complete for quick scan, precise identification, raw read, complete dump, NDEF inspection, emulator initialization, reader/emulator mode persistence, and mutation guards.

Not yet validated on hardware:

- over-the-air emulation with a phone or second reader;
- reversible write on a sacrificial Type 2 tag;
- NDEF replacement on a sacrificial NDEF tag;
- MIFARE Classic authentication/value-block behavior with a sacrificial Classic card.
