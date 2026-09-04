---
Title: Official Arduino Four-Chip I2C Trace Comparison
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
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://0116-m5stackchan-nfc-debug-ui/overlay/firmware/main/apps/app_nfc_debug/st25r3916/st25r3916.c
      Note: ESP-IDF transport implementation being compared
    - Path: repo://ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/scripts/04-instrument-official-arduino-trace.py
      Note: Reproducible M5Unified in-memory transport instrumentation
    - Path: repo://ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/scripts/05-analyze-arduino-trace.py
      Note: Reproducible capture parser and summarizer
    - Path: repo://ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/sources/code/arduino-trace/Detect-continuous-traced.cpp
      Note: WUPA polling, bounded serial summaries, and 320x240 screen log
    - Path: repo://ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/sources/hardware/02-official-arduino-four-chip-full-i2c-trace.analysis.json
      Note: Machine-readable phase and latency summary
    - Path: repo://ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/sources/hardware/02-official-arduino-four-chip-full-i2c-trace.log.gz
      Note: Exact complete four-chip Arduino serial trace
    - Path: repo://ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/sources/hardware/03-arduino-continuous-screen-runtime.log
      Note: Twenty-second continuous four-chip hardware validation
    - Path: repo://ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/sources/hardware/04-arduino-persistent-four-device-registry.log
      Note: Uninterrupted proof that four UID rows survive empty scans
ExternalSources: []
Summary: Transaction-level comparison of the successful official Arduino/M5 I2C path with NFC LAB's intermittent ESP-IDF failures.
LastUpdated: 2026-08-21T16:42:48.949026988-04:00
WhatFor: Use this evidence when selecting and implementing the next ST25R3916 transport backend experiment.
WhenToUse: Read before attributing ESP_ERR_INVALID_STATE to the tag, RF coupling, or a specific ST25R register.
---




# Official Arduino Four-Chip I2C Trace Comparison

## Executive conclusion

The instrumented official Arduino firmware completed **10,188 reported ST25R3916 I2C transactions with zero M5Unified-level failures** across initialization, one successful multi-tag detection window, three identification attempts, and a subsequent no-tag detection window. It read one UID successfully and discovered three distinct PICCs while four physical chips were on the reader.

The trace traversed the exact registers that failed in NFC LAB:

- Arduino initialization read operation-control register `0x02` seven times and wrote it four times without failure.
- Arduino detection read auxiliary-definition register `0x0A` 153 times and wrote it seven times without failure.
- The first Arduino REQA sequence performed the `0x0A` read-modify-write successfully immediately before direct command `0xC6`.

This is the strongest evidence so far that the intermittent NFC LAB fault is associated with the ESP-IDF transport/controller path or its interaction with the shared bus, rather than an inherent inability of this ST25R3916 to accept accesses to registers `0x02` or `0x0A`. It does not by itself identify whether the decisive M5 behavior is transaction framing, per-transaction FSM reset, forced STOP recovery, locking, or timing.

## Test conditions

- Physical placement: four NFC chips on the literal narrow top edge of the StackChan head.
- Firmware basis: official StackChan-BSP `Detect.ino` behavior.
- Board: M5Stack CoreS3 / ESP32-S3.
- PlatformIO platform: PIOArduino `55.03.311`.
- Arduino-ESP32: `3.3.11`.
- Framework ESP-IDF libraries: `5.5.5`.
- M5Unified: `0.2.20`.
- M5GFX: `0.2.27`.
- M5UnitUnified: `0.5.5`.
- M5Unit-NFC: preserved official local source, version `0.1.0`.
- Bus address: `0x50`.
- Serial: `/dev/ttyACM0`, 115200 baud, exclusive owner.
- Instrumentation commit: `04c8a7c26ead2dcfcdd6f009c9b1012e846b4632` plus the 6,000-entry ring adjustment recorded with this analysis.

## Instrumentation method

The test avoids printing from inside M5's I2C methods. Patched `M5Unified::I2C_Class` maintains a per-port transaction context and writes fixed-size records into a static RAM ring. A transaction begins at `start()`, may include `write()`, `restart()`, and `read()`, and completes at `stop()`. Start/restart failures are completed immediately because the adapter does not issue STOP after those failures.

Each record contains:

- monotonic sequence;
- start timestamp and elapsed microseconds;
- transaction kind: write, read, or write/repeated-start/read;
- first transmitted byte (`key`);
- write and read lengths;
- a bitmask for start, restart, write, read, and STOP failures.

Only address `0x50` is retained. The sketch drains and prints the ring **after** initialization, detection, or identification finishes. Serial throughput therefore cannot add pauses between the measured I2C transactions. The complete one-second detection window required a 6,000-entry ring; the initial 512-entry test preserved aggregate counters but overwrote 4,245 early records.

## Trace-key interpretation

The `key` is the first byte written to the ST25R3916:

- Space-A register write: raw register, such as `0x02` or `0x0A`.
- Space-A register read: `0x40 | register`, such as `0x42` or `0x4A`.
- FIFO load: `0x80`.
- FIFO read: `0x9F`.
- REQA direct command: `0xC6`.
- WUPA direct command: `0xC7`.

NFC LAB's `NFC_I2C_FAIL key` records the raw logical register for register operations, so compare its `READ_A key=0x02` with Arduino key `0x42`, and its `WRITE_A key=0x0A` directly with Arduino key `0x0A`.

## Phase results

| Phase | High-level result | Transactions | I2C failures | Median | p95 | Maximum |
|---|---:|---:|---:|---:|---:|---:|
| Initialization | success | 338 | 0 | 176 us | 187 us | 351 us |
| Detection window 1 | 3 PICCs | 4,816 | 0 | 178 us | 213 us | 478 us |
| Identify UID `047B...6180` | success | 92 | 0 | 178.5 us | 228 us | 391 us |
| Identify UID `0491...6180` | protocol false | 87 | 0 | 178 us | 242 us | 400 us |
| Identify UID `04DA...6180` | protocol false | 87 | 0 | 179 us | 243 us | 357 us |
| Detection window 2 | no PICC | 4,768 | 0 | 178 us | 213 us | 414 us |

The first five phases contain 5,420 transactions and are present completely in the serial capture. Capture stopped immediately after the second `M5_DETECT` summary; 4,767 of that phase's 4,768 records reached the file. The phase's firmware-side counters still reported zero failures.

## Tag results

The firmware discovered three distinct UIDs from four physical chips:

1. `047BD44D9E6180` — identified as NTAG 215, ATQA `0x0044`, SAK `0x00`, 504-byte user area.
2. `0491D44C9E6180` — provisionally MIFARE Ultralight, ATQA `0x0044`, SAK `0x00`; deeper identification returned false.
3. `04DAF74D9E6180` — provisionally MIFARE Ultralight, ATQA `0x0044`, SAK `0x00`; deeper identification returned false.

The fourth physical chip was not enumerated in this window. That is an RF/anticollision/protocol observation, not an I2C transport failure. Similarly, two identification calls returned false after 87 completely successful I2C transactions each. The trace therefore demonstrates why protocol outcomes and transport outcomes must remain separate.

## First REQA sequence

The beginning of the successful multi-tag detection phase was:

| Transaction | Kind | Key | Meaning | Elapsed | Result |
|---:|---|---:|---|---:|---|
| 1 | WR | `0x52` | Read Space-A register `0x12` | 175 us | success |
| 2 | W | `0x10` | Write NRT register `0x10` | 136 us | success |
| 3 | W | `0x05` | Write ISO14443-A settings | 105 us | success |
| 4 | WR | `0x4A` | Read auxiliary definition `0x0A` | 180 us | success |
| 5 | W | `0x0A` | Write auxiliary definition `0x0A` | 102 us | success |
| 6 | WR | `0x5A` | Read interrupt register `0x1A` | 209 us | success |
| 7 | W | `0xDB` | Clear FIFO direct command | 78 us | success |
| 8 | W | `0xC6` | Transmit REQA direct command | 79 us | success |
| 9+ | WR | IRQ/FIFO registers | Poll request completion | 174–187 us initially | success |
| 15 | WR | `0x9F` | Read FIFO response | 178 us | success |

This sequence reaches the same auxiliary-definition read-modify-write where the first NFC LAB physical READ ONCE aborted before transmitting RF. M5 completed both halves, transmitted REQA, and read response data.

## Direct comparison with NFC LAB

| Evidence | NFC LAB / ESP-IDF new driver | Official Arduino / M5 backend |
|---|---|---|
| Initialization result | Intermittently fails | 338/338 transactions succeeded |
| Operation-control register | `READ_A key=0x02` failed at transaction 65 | 7 encoded reads (`0x42`) and 4 writes (`0x02`) succeeded during init |
| Auxiliary-definition register | `WRITE_A key=0x0A` failed before REQA | 153 encoded reads (`0x4A`) and 7 writes (`0x0A`) succeeded during detection |
| Error form | `ESP_ERR_INVALID_STATE` | No M5Unified failure-stage bits |
| Typical logical read latency | failing example 195 us | median phase latency about 176–179 us; p95 187–243 us |
| Request policy | one REQA, then WUPA | retries requests for the one-second detect window |
| Controller handling | ESP-IDF new master driver | M5GFX direct controller handling with FSM reset and explicit START/restart/STOP |
| UID | none yet | `047BD44D9E6180` |

The 195 us ESP-IDF failure duration is within the broad range of successful M5 logical read transactions. Elapsed time alone cannot classify the controller outcome.

## What this proves

- The physical reader, antenna, and tags function.
- M5's address-`0x50` API path completed thousands of logical transactions without exposing an error during this run.
- Registers `0x02` and `0x0A` are not deterministically rejected by the ST25R3916.
- M5 can reach REQA, anticollision, select, and UID extraction with the current physical arrangement.
- A false identify result can occur with a completely clean transport, validating the UI's separate protocol state.

## What this does not prove

- It does not prove that no electrical NACK occurred below the M5Unified boundary if M5GFX internally recovered without returning an error.
- It does not identify which M5 behavior prevents or recovers the failure.
- It is not a simultaneous A/B waveform capture.
- Four physical chips did not guarantee four enumerated PICCs; multi-tag coupling and collision behavior remain separate variables.
- The 6,000-entry ring increases static RAM usage to about 44% for this diagnostic build. It is not intended for production firmware.

## Recommended next experiment

Implement the guide's standalone backend matrix in project `0115`:

1. Keep the current ESP-IDF new-driver backend as control.
2. Add explicit defined operations matching M5's START/write/restart/read-final-NACK/STOP sequence.
3. Add an isolated legacy/direct backend that resets the FSM at transaction start and performs explicit recovery.
4. Run the same initialization and request traces with one tag.
5. Require zero transaction failures, not merely eventual UID success.
6. Capture SDA/SCL for the ESP-IDF `0x02` or `0x0A` failure and the matching M5 sequence.

The strongest implementation candidate is the smallest backend change that reproduces M5's zero-error trace while remaining safe for StackChan's shared I2C bus.

## Continuous screen-monitor follow-up

The follow-up `.ino` monitor replaced the one-second multi-PICC enumeration loop with a 250 ms WUPA/select/identify cycle. WUPA wakes tags placed in HALT by the prior cycle, making the same physical chips visible repeatedly without removing or power-cycling them. The firmware retains the in-memory transaction recorder but prints only phase summaries and failures; this removes the long serial-drain pause between polls.

A 20-second run with the same four chips produced:

- 49 poll cycles;
- 47 successful WUPA responses;
- 31 successful selections;
- 30 successful identifications;
- 8,126 cumulative ST25R3916 transactions;
- zero M5Unified-level transport failures;
- three repeatedly selected UIDs: `04C9C54C9E6180`, `0491D44C9E6180`, and `047BD44D9E6180`.

The 320×240 display continuously renders poll number, state, elapsed time, UID, type, ATQA/SAK, cumulative I2C totals, last transaction key/timing/failure stage, and a 13-entry scrolling event log. Transport failures are red, protocol failures orange, tags green, and no-tag state yellow.

Selection occasionally failed after a valid WUPA response, and one selected tag failed identification. Some multi-tag cycles returned transient ATQA values `0x0188`, `0x0210`, or `0x2800` instead of `0x0044`. All underlying I2C transactions still succeeded. These are multi-tag RF/anticollision/protocol observations and reinforce that the live display must not classify every poll miss as transport failure.

## Persistent multi-device registry follow-up

The first multi-tag screen rendered only the current 120 ms collection window. An empty scan therefore erased the UID rows even though the scrolling log still contained prior discoveries. The final Arduino monitor separates **current presence** from **historical discovery** with a four-entry UID-keyed registry.

Every newly enumerated PICC is matched by UID. A matching UID updates its existing row's observation count, last-seen poll, type, ATQA, SAK, identification result, and current-presence marker. It does not allocate a duplicate row. At the beginning of each scan, all rows become absent; tags found in that scan become present again. Empty scans retain the UID and metadata. The screen displays `*` for current presence and `-` for a retained but currently absent tag.

An uninterrupted 197-cycle session proved the behavior. Four unique UIDs entered the registry:

- `04DAF74D9E6180`, observed 8 times;
- `04ACE84D9E6180`, observed 10 times;
- `0491D44C9E6180`, observed 10 times;
- `04C9C54C9E6180`, observed 3 times.

At cycle 197, the current detect window returned zero PICCs while the serial summary still reported `seen=4`. No I2C failures occurred. The registry therefore preserved all four rows across empty scans without assigning duplicate rows to repeated observations.

The four-entry capacity matches the physical test and screen layout. If a fifth unique UID appears, the least-recently-seen entry is evicted and the event is recorded on screen. Two physical tags that intentionally share an identical UID cannot be distinguished by this registry; they update the same logical device row.

## Artifacts

- Raw exact serial capture: `sources/hardware/02-official-arduino-four-chip-full-i2c-trace.log.gz`.
- Machine-readable summary: `sources/hardware/02-official-arduino-four-chip-full-i2c-trace.analysis.json`.
- Analyzer: `scripts/05-analyze-arduino-trace.py`.
- Instrumentation patcher: `scripts/04-instrument-official-arduino-trace.py`.
- Traced sketch: `sources/code/arduino-trace/Detect-traced.cpp`.
- Trace ABI: `sources/code/arduino-trace/esp60_m5_i2c_trace.h`.
- ESP-IDF comparison capture: `sources/hardware/01-nfc-lab-structured-serial-runtime.log`.
- Continuous monitor source: `sources/code/arduino-trace/Detect-continuous-traced.cpp`.
- Continuous monitor capture: `sources/hardware/03-arduino-continuous-screen-runtime.log`.
- Persistent registry capture: `sources/hardware/04-arduino-persistent-four-device-registry.log`.
