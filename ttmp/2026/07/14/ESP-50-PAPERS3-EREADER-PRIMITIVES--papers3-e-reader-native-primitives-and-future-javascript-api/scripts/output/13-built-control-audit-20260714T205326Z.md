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
Summary: "Static and binary audit of the bounded P0.16 independent EPD control."
LastUpdated: 2026-07-14T20:53:26.355679+00:00
WhatFor: "Prove that the independent-control build is pinned, waveform-identical, no-drive at boot, and exposes only bounded state-gated physical commands."
WhenToUse: "Run after every P0.15/P0.16 source or build change and before creating a flash command."
---

# Built independent EPD control audit

Generated: 2026-07-14T20:53:26.355679+00:00

Gate: **PASS**

| Check | Result | Evidence |
|---|---|---|
| Waveform/preset bytes unchanged | PASS | `98152d0a16bfe02d4c150617822ebd39dae940884aca7a9d5bcb5900b0169f47` |
| Explicit PaperS3 preset compile definition | PASS | `compile_commands.json` |
| Automatic boot controller excluded | PASS | `compile definition and ELF symbols` |
| No M5GFX/M5Unified/Adafruit/Arduino symbols | PASS | `ELF symbol scan` |
| Exact 1000 Hz waveform-delay tick | PASS | `sdkconfig.ticket` |
| USB Serial/JTAG console | PASS | `sdkconfig.ticket` |
| Octal PSRAM enabled | PASS | `sdkconfig.ticket` |
| No-drive boot path | PASS | `app_main() initializes and starts the console only` |
| Bounded command and evidence surface | PASS | `fixed command grammar and timeout terminal record` |
| Reader fixture identity | PASS | `129600 bytes; SHA-256 14dcffa9...` |
| Driver initializes packed buffers | PASS | `prepared EPD_Painter.cpp` |
| Bounded idle API linked | PASS | `ELF symbol scan` |
| Clean build has zero warnings | PASS | `12-epd-painter-build-20260714T205220Z.log` |
| Build did not flash hardware | PASS | `12-epd-painter-build-20260714T205220Z.log` |

## Binary identity

- Application SHA-256: `2791e8334e2dae02612cf57ef58437758420a8168487fde3994d4fc73f3c5135`
- ELF SHA-256: `451b4ffa026217a7fe10ff545174e0d6c62dd92b1ba2e9817577a7411f983358`
- Prepared preset SHA-256: `98152d0a16bfe02d4c150617822ebd39dae940884aca7a9d5bcb5900b0169f47`
- Latest build log: `ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/12-epd-painter-build-20260714T205220Z.log`
- Hardware modified: **no**

## Review item

IRAM utilization is nearly saturated: `used=16383 percent=99.99 remaining=1`. P0.16 must avoid adding IRAM-attributed code and must rerun the size gate.

This is a static/build gate. It does not prove runtime boot behavior or optical safety; those remain P0.17 hardware observations.
