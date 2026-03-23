---
Title: Diary
Ticket: ESP-45-PAPERS3-ATOMS3R-BOARD-CONFIG-COMPARISON
Status: active
Topics:
    - papers3
    - atoms3r
    - wasm
    - firmware
    - esp-idf
    - debugging
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-23T15:05:00-04:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Keep the board-comparison track separate from the loader root-cause track so the evidence stays legible.

## 2026-03-23 15:05 EDT

Created this ticket because the board/config comparison is still useful, but it is now secondary. `ESP-44` is about the direct embedded-buffer mechanism. This ticket is about the environment in which that mechanism fails on PaperS3 but not on AtomS3R.

## 2026-03-23 15:48 EDT

Started the comparison with a reusable local script rather than another one-off shell diff:

- `scripts/compare_memory_configs.py`

The script compares the active AtomS3R `sdkconfig` against the active PaperS3 internal-pool `sdkconfig.variant` for flash, CPU, PSRAM, console, and WAMR-related settings.

## 2026-03-23 15:50 EDT

The first config result is more interesting for what it does **not** show than for what it does.

Shared settings:

- both are `esp32s3`
- both use octal PSRAM at `40 MHz`
- both use `80 MHz` DIO flash mode in config
- both use USB Serial/JTAG console
- both use the same narrow WAMR interpreter feature set

Important differences:

- AtomS3R config uses `8MB` flash, PaperS3 uses `16MB`
- AtomS3R config sets CPU frequency to `240 MHz`, while the active PaperS3 build is `160 MHz`
- PaperS3 has the current embedded-load mitigation and allocator-control flags; AtomS3R does not because it is a different probe project

The important interpretation is that the obvious PSRAM-mode settings are not diverging in a way that immediately explains the bug. The config-level evidence so far points more toward board topology and flash source handling than toward a simple PSRAM-mode mismatch.

## 2026-03-23 15:55 EDT

Pulled the official M5 docs for both boards.

The most relevant hardware distinction is:

- PaperS3: `ESP32-S3R8` with `8MB PSRAM` and **external `16MB` flash**
- AtomS3R-M12: `ESP32-S3-PICO-1-N8R8` with `8MB Flash` and `8MB PSRAM` integrated in the SiP

That does not prove causality on its own, but it gives the loader-root-cause theory a more concrete board backdrop. If WAMR is mutating a flash-mapped source buffer in place, the PaperS3 external-flash arrangement is a more plausible place for ugly side effects than a RAM copy or a different S3 packaging.
