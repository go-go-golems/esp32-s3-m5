---
Title: M5GFX PaperS3 Waveform Static Decoding
Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES
Status: active
Topics:
    - papers3
    - eink
    - hardware-qualification
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Canonical static decoding of legacy and current M5GFX PaperS3 LUTs, schedules, bus timing configuration, and power ordering."
LastUpdated: 2026-07-14T21:20:35Z
WhatFor: "Separate source-level waveform identity from runtime timing and physical electrical evidence."
WhenToUse: "Use when comparing factory, qualification, and independent-driver EPD experiments."
---

# M5GFX PaperS3 waveform static decoding

This report is generated without opening a serial port. `B` means code 1 (toward black), `W` means code 2 (toward white), and `-` means code 3 (no operation), matching M5GFX's source comments. These names are software semantics, not independently probed source-driver voltages.

## Snapshot identity

- Legacy/factory-family M5GFX commit: `c6f92dc03226cdc04d67c705a2020f62ad21ad01`
- Current qualification M5GFX commit: `ad9b814264d4e2000e9f30070002310bbccaffc9`
- Legacy canonical LUT SHA-256: `d24b2df188e4261d5891a0884e2510567ea45c38bcaebeb66ade1d4f4b979af3`
- Current canonical LUT SHA-256: `d24b2df188e4261d5891a0884e2510567ea45c38bcaebeb66ade1d4f4b979af3`
- Canonical LUTs identical: **yes**
- PaperS3 bus speed: `16000000` Hz
- Encoded scan-line padding: `8` bytes

## Power ordering

The M5GFX `Bus_EPD` source commands the following fixed GPIO ordering; it does not program a rail voltage or VCOM value:

- on: OE high → 100 µs → PWR high → 100 µs → SPV high → 1 ms;
- off: 1 ms → PWR low → 10 µs → OE low → 100 µs → SPV low.

## Target schedules

The table shows the black and white endpoint schedules through the first terminator. No-op padding remains visible because it affects runtime frame count even though it does not command particle motion.

| LUT | rows | black target | white target | row hash |
|---|---:|---|---|---|
| `lut_eraser` | 4 | `WW-` | `BB-` | `7764d9ed514aa115003dd9c36d373c1a2b7453b6607c4a28e48c7baf546743ee` |
| `lut_fast` | 10 | `WWBBBBBB-` | `BBWWWWWW-` | `926265280e8c98e5632ef7706f11ce8dedc44ab22c38fdb60d72c3faa1fa4b4b` |
| `lut_fastest` | 7 | `WBBBB-` | `BWWWW-` | `ace3f55e34799a77b809a7a996523e39a5b3ba6d4e9b9362642b90a466d693ec` |
| `lut_quality` | 32 | `BWWBBBBBB--BB------------------` | `BWWBBB-WWWWWW------------------` | `aed47534778ca40a6cc3ffe07d04112c497f526ae54442530634c196963276bc` |
| `lut_text` | 32 | `WWWWWBBB-BBB-------------------` | `BBBBBWWW-WWW-------------------` | `46c54830fe96c654c24e7933246bd87b3eb3dc4fdc46f012c001c3cb4f9cccb5` |

## What static decoding proves

- The legacy/factory-family and current qualification LUT bytes can be compared exactly.
- Every nominal grayscale target's software drive-code sequence is explicit and hashable.
- Bus clock configuration, line padding, and power GPIO ordering are source-backed.

## What it does not prove

- Actual frame duration, GPIO edge timing, rail voltage, VCOM, current, or temperature.
- That software code 1 and code 2 produce the intended physical source polarity on this board.
- Which target/history the official merged binary was executing at a given optical instant.
- That adding runtime logging leaves timing unchanged.

The JSON companion preserves every raw LUT row and all sixteen derived target schedules for machine comparison.
