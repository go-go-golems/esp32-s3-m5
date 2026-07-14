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
LastUpdated: 2026-07-14T21:03:05.577065+00:00
WhatFor: "Prove that the independent-control build is pinned, waveform-identical, no-drive at boot, and exposes only bounded state-gated physical commands."
WhenToUse: "Run after every P0.15/P0.16 source or build change and before creating a flash command."
---

# Built independent EPD control audit

Generated: 2026-07-14T21:03:05.577065+00:00

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
| Clean build has zero warnings | PASS | `12-epd-painter-build-20260714T210141Z.log` |
| Build did not flash hardware | PASS | `12-epd-painter-build-20260714T210141Z.log` |

## Binary identity

- Application SHA-256: `f24705a69ac0355006d82ea1873191c6084f96bc7a79fcd1008ef433208437f9`
- ELF SHA-256: `1f0134ada20285026c0c9df12b89a7c5cf9bba26d9bb9b030e97bb9172d1ffc2`
- Prepared preset SHA-256: `98152d0a16bfe02d4c150617822ebd39dae940884aca7a9d5bcb5900b0169f47`
- Latest build log: `ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/12-epd-painter-build-20260714T210141Z.log`
- Hardware modified: **no**

## Review item

IRAM utilization is nearly saturated: `used=16383 percent=99.99 remaining=1`. P0.16 must avoid adding IRAM-attributed code and must rerun the size gate.

This is a static/build gate. It does not prove runtime boot behavior or optical safety; those remain P0.17 hardware observations.
