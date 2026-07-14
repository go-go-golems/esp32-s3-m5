---
Title: Built Independent EPD Control Audit
Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES
Status: active
Topics:
    - papers3
    - eink
    - esp-idf
    - hardware-qualification
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Static and binary audit of the no-drive P0.15 independent EPD control."
LastUpdated: 2026-07-14T20:33:36.291702+00:00
WhatFor: "Prove that the first independent-control build is pinned, waveform-identical to upstream, hardened, and incapable of panel operations from its exposed console."
WhenToUse: "Run after every P0.15/P0.16 source or build change and before creating a flash command."
---

# Built independent EPD control audit

Generated: 2026-07-14T20:33:36.291702+00:00

Gate: **FAIL**

| Check | Result | Evidence |
|---|---|---|
| Waveform/preset bytes unchanged | PASS | `98152d0a16bfe02d4c150617822ebd39dae940884aca7a9d5bcb5900b0169f47` |
| Explicit PaperS3 preset compile definition | PASS | `compile_commands.json` |
| Automatic boot controller excluded | PASS | `compile definition and ELF symbols` |
| No M5GFX/M5Unified/Adafruit/Arduino symbols | PASS | `ELF symbol scan` |
| Exact 1000 Hz waveform-delay tick | PASS | `sdkconfig.ticket` |
| USB Serial/JTAG console | PASS | `sdkconfig.ticket` |
| Octal PSRAM enabled | PASS | `sdkconfig.ticket` |
| No-drive command surface | PASS | `app_main.cpp exposes only help/status` |
| Driver initializes packed buffers | PASS | `prepared EPD_Painter.cpp` |
| Bounded idle API linked | FAIL | `ELF symbol scan` |
| Clean build has zero warnings | PASS | `12-epd-painter-build-20260714T202950Z.log` |
| Build did not flash hardware | PASS | `12-epd-painter-build-20260714T202950Z.log` |

## Binary identity

- Application SHA-256: `cc5ffc05906278bd86449d9315b65c99e8113de3f1e0ec28b3d4c34fde9120c4`
- ELF SHA-256: `9bff789e9114c5e55a7d98f13089f72caa5e057b4c548d5eacbd5aea101a4ce9`
- Prepared preset SHA-256: `98152d0a16bfe02d4c150617822ebd39dae940884aca7a9d5bcb5900b0169f47`
- Latest build log: `ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/12-epd-painter-build-20260714T202950Z.log`
- Hardware modified: **no**

## Review item

IRAM utilization is nearly saturated: `used=63295 percent=18.52 remaining=278465`. P0.16 must avoid adding IRAM-attributed code and must rerun the size gate.

This is a static/build gate. It does not prove runtime boot behavior or optical safety; those remain P0.17 hardware observations.

## Failures

- Bounded idle API linked
