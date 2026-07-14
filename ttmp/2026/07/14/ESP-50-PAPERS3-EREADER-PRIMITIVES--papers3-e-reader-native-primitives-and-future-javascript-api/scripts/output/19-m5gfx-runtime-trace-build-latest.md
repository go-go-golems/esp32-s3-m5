---
Title: M5GFX Runtime Trace Variant Build Evidence
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
Summary: "Warning-free trace-off and fixed-ring trace-timing builds from the same pinned M5GFX source and configuration."
LastUpdated: 2026-07-14T22:04:53Z
WhatFor: "Compare compile-time and memory observer effects before any instrumented firmware is flashed."
WhenToUse: "Use when reviewing runtime trace implementation, binary identity, or authorization for a physical timing experiment."
---

# M5GFX runtime trace variant build evidence

- Build UTC: `20260714T220453Z`
- ESP-IDF: `ESP-IDF v5.4.2`
- M5GFX: `ad9b814264d4e2000e9f30070002310bbccaffc9` (0.2.25)
- M5Unified: `b1ffcc677014ed8bd01e5a1f240736ae654bfe12` (0.2.18)
- Trace patch SHA-256: `6c21e45e0909accd2b5df5ae3178534192b10a51ec4a319ebc2309bfe983d89f`
- Canonical LUT SHA-256: `d24b2df188e4261d5891a0884e2510567ea45c38bcaebeb66ade1d4f4b979af3`
- FreeRTOS tick: `100 Hz` (preserves the qualification/factory-family M5GFX baseline)
- Hardware modified: **no**

| Artifact | Trace off | Trace timing | Delta |
|---|---:|---:|---:|
| application bytes | 546064 | 547648 | 1584 |
| ELF bytes | 11504936 | 11521576 | 16640 |
| M5GFX archive bytes | 55405056 | 55414720 | 9664 |
| linked trace symbols | 0 | 1 | 1 |

## Artifact identities

- Off application SHA-256: `609aba851db118ee26a3051d4f78ae96255229493f9783f60f43334355925e68`
- Off ELF SHA-256: `3298826f82158a6eb47023dfb9a379fc71b6ccb85afc0e20f75be1d6329fe1ce`
- Off sdkconfig SHA-256: `396f1c39263c0d2a66c66c8dbae5fad5a5cedca6c58044e90ea528553f08e295`
- Trace application SHA-256: `a081daabe5a77d7405cde68e43955279ed5e5c0f954c2aee027b62d03fd9f6ea`
- Trace ELF SHA-256: `da5274b85e478c3919ef16bfea7f01d111bdd3166b5a5ba34e1517d744f1fdcc`
- Trace sdkconfig SHA-256: `3797d48d537e2455376ee0b5fc3229b881ffa6c5237e5cb9f369892a76e43047`

## Build dispositions

- Both variants built from clean state with zero compiler warnings.
- Both variants use the same patched source tree; the Kconfig boolean is the only trace-mode selection.
- The off variant contains no linked `lgfx_epd_trace_emit` symbol.
- The timing variant links the fixed-ring hook and retains no per-row drive-code counting.
- Neither build command included flash or monitor actions.
