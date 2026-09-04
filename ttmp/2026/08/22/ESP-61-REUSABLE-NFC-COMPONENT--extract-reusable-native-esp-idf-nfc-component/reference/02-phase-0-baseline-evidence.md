---
Title: Phase 0 Baseline Evidence
Ticket: ESP-61-REUSABLE-NFC-COMPONENT
Status: active
Topics:
    - nfc
    - esp-idf
    - st25r3916
    - hardware-qualification
    - intern-guide
DocType: reference
Intent: short-term
Owners: []
RelatedFiles:
    - Path: repo://0115-m5stackchan-nfc-reader/test_host/test_st25r_trace.c
      Note: Host-tested observer-safe trace baseline
    - Path: repo://0117-m5stackchan-nfc-feature-explorer/dependencies.lock
      Note: Exact ESP-IDF and transitive M5 revision baseline
    - Path: repo://0117-m5stackchan-nfc-feature-explorer/main/idf_component.yml
      Note: Direct pinned M5Unit-NFC dependency used by Phase 0
    - Path: repo://ttmp/2026/08/22/ESP-61-REUSABLE-NFC-COMPONENT--extract-reusable-native-esp-idf-nfc-component/scripts/01-restore-reader-mode.py
      Note: Ticket-local prompt-aware reader-mode restore script
    - Path: repo://ttmp/2026/08/22/ESP-61-REUSABLE-NFC-COMPONENT--extract-reusable-native-esp-idf-nfc-component/sources/hardware/01-phase0-reader-mode-restored.txt
      Note: Reader mode restored after stale monitor release
    - Path: repo://ttmp/2026/08/22/ESP-61-REUSABLE-NFC-COMPONENT--extract-reusable-native-esp-idf-nfc-component/sources/hardware/02-phase0-read-only-probe.txt
      Note: 'Fresh Phase 0 probe: no tag present in the field'
    - Path: repo://ttmp/2026/08/22/ESP-61-REUSABLE-NFC-COMPONENT--extract-reusable-native-esp-idf-nfc-component/sources/software/01-0117-esp-idf-5.5.4-build.txt
      Note: Fresh Phase 0 ESP-IDF build output
    - Path: repo://ttmp/2026/08/22/ESP-61-REUSABLE-NFC-COMPONENT--extract-reusable-native-esp-idf-nfc-component/sources/software/02-st25r-trace-host-tests.txt
      Note: Fresh Phase 0 host-test output
    - Path: repo://ttmp/2026/08/22/ESP-61-REUSABLE-NFC-COMPONENT--extract-reusable-native-esp-idf-nfc-component/sources/software/04-serial-owner-blocker.txt
      Note: Exact process preventing the hardware probe
ExternalSources: []
Summary: Phase 0 software build, host-test, dependency, and serial-ownership evidence collected before reusable-component extraction.
LastUpdated: 2026-08-22T19:50:00-04:00
WhatFor: Establish the reproducible known-good software baseline and record why hardware acceptance is not yet complete.
WhenToUse: Check before starting extraction or deciding whether Phase 0 may be marked complete.
---



# Phase 0 Baseline Evidence

## Acceptance status

Phase 0 is **partially complete and must not be marked done yet**.

Completed:

- `0117-m5stackchan-nfc-feature-explorer` builds under ESP-IDF 5.5.4 without a local warning or error match.
- The resulting application is `0x67760` bytes and leaves `0x988a0` bytes, 60%, free in the 1 MiB application partition.
- Every `0115` observer-safe trace host test passes.
- Direct and transitive NFC dependency revisions are recorded.

Hardware status:

- The stale `esp_idf_monitor` was terminated after remaining idle for more than an hour and after two explicit requests to close it.
- A prompt-aware script restored and verified reader mode.
- The fresh read-only probe ran as the only serial owner but detected no tag. The physical NTAG215 is not currently in the RF field.
- The user must place the physical NTAG215 on the narrow top antenna edge before acceptance can complete.

## Build evidence

Command:

```bash
source ~/esp/esp-idf-5.5.4/export.sh
cd 0117-m5stackchan-nfc-feature-explorer
idf.py build
```

Result:

```text
m5stackchan_nfc_feature_explorer.bin binary size 0x67760 bytes.
Smallest app partition is 0x100000 bytes.
0x988a0 bytes (60%) free.
Project build complete.
```

Full incremental build output:

```text
sources/software/01-0117-esp-idf-5.5.4-build.txt
```

## Host trace test evidence

Command:

```bash
0115-m5stackchan-nfc-reader/test_host/build.sh
```

Coverage includes:

- trace disabled mode;
- success and failure recording;
- first-error freeze across ring overwrite;
- wraparound and overwrite accounting;
- inter-transaction gap computation;
- diagnostic NOT_FOUND classification;
- failure-only mode;
- clear behavior;
- driver-hint annotation;
- normalized dump format;
- tail dumping.

Result:

```text
ALL TESTS PASSED
```

Full output:

```text
sources/software/02-st25r-trace-host-tests.txt
```

## Locked revisions

```text
ESP-IDF=5.5.4
M5Unit-NFC=93745b547364f310cd64b5155a870103a7800a5d
M5UnitUnified=bf711f370047cf16355b00005450ef615fab36e2
M5HAL=0f06f9d3134706ce030fd5515601cce65a267233
M5Utility=301a6b5c6413875e1dd80b027e0639921972b433
```

Source:

```text
0117-m5stackchan-nfc-feature-explorer/main/idf_component.yml
0117-m5stackchan-nfc-feature-explorer/dependencies.lock
sources/software/03-locked-dependencies.txt
```

## Hardware blocker

`fuser` reported:

```text
/dev/ttyACM0: manuel 189173 F.... python
```

The process is:

```text
python -m esp_idf_monitor
  -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_44:1B:F6:E2:80:28-if00
  ...
  0117-m5stackchan-nfc-feature-explorer/build/m5stackchan_nfc_feature_explorer.elf
```

The monitor belongs to terminal `pts/23`. Opening another monitor or probe would violate the single-owner serial requirement and could generate false timeouts or incomplete output.

Full process evidence:

```text
sources/software/04-serial-owner-blocker.txt
```

## Reader restoration and no-tag probe

The ticket-local script:

```text
scripts/01-restore-reader-mode.py
```

sent the exact reader-mode reboot command and reopened the USB device. Its captured `nfc-mode` result confirmed:

```text
NFC_MODE current=reader ready=1
```

The existing read-only feature probe then ran as the only serial owner. It completed every command but found no card:

```text
NFC_RESULT op=scan ok=0 detected=0 identified=0 timeout_ms=1000
NFC_RESULT op=info ok=0
NFC_RESULT op=raw-read ok=0 address=0
NFC_RESULT op=ndef-read ok=0 valid=0
NFC_RESULT op=dump ok=0
```

Captures:

```text
sources/hardware/01-phase0-reader-mode-restored.txt
sources/hardware/02-phase0-read-only-probe.txt
```

This is a physical no-tag outcome, not a serial, initialization, or console failure.

## Required action to finish Phase 0

1. Place the known NTAG215 on the narrow top antenna edge.
2. Confirm `/dev/ttyACM0` has no owner.
3. Re-run `scripts/12-probe-nfc-feature-explorer.py` as the only serial owner.
4. Verify the known UID, exact NTAG215 identification, raw read, valid NDEF result, and full dump.
5. Preserve the successful fresh capture under this ticket.
6. Only then mark task `4igv`, print the Phase 0 completion slip, and start Phase 1.
