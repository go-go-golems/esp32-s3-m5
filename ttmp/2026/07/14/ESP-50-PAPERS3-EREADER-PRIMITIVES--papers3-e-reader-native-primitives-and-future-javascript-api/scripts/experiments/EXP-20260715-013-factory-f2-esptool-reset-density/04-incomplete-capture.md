---
Title: "Incomplete Capture - EXP-20260715-013-factory-f2-esptool-reset-density"
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
Summary: "Exact F2 flash and esptool hard reset completed, but the subsequent safe read-only capture received zero firmware payload bytes and no trace dump markers."
LastUpdated: 2026-07-15T01:53:00Z
WhatFor: "Preserve the incomplete F2 attempt and prevent an unexamined retry."
WhenToUse: "Review before any additional F2 boot or serial-capture action."
---

# Incomplete F2 capture

## What completed

- Exact F2 application SHA-256 `95334c261762205ab95d3f578a5d3d0a0eac4fe7fffdfd1ada0e836ba8a2d755` was flashed and every esptool write reported `Hash of data verified.`
- The staged bootloader session identified the target as `ESP32-S3 (QFN56)` in USB-Serial/JTAG mode.
- esptool reported `Hard resetting via RTS pin...` and released the port.
- The 60-second combined collector completed `result=ok` and performed its required Printalyzer cleanup.

## What did not complete

The safe read-only PaperS3 source recorded only its own open and close events: zero firmware payload lines arrived during the 60.8-second capture. Consequently the extractor stopped with:

```text
F2 ring dump markers absent
```

No ring, alignment, or evidence checksum was generated. The raw density stream is retained but cannot support an F2 scheduler claim without the required ring dump.

## Immediate interpretation limit

The evidence proves a verified flash, an esptool hard-reset command, and healthy Printalyzer collection. It **does not prove that F2 booted into the FactoryTest application** or that the USB Serial/JTAG output path was observable after that reset. Do not infer an F2/F1 comparison or a panel disposition from this attempt.

## Required next evidence

Obtain the operator's visual/no-anomaly observation for this exact run. Then investigate the two unresolved alternatives before rerunning: (1) the reset may have left the board in ROM download mode rather than booting F2; (2) F2 may have booted while its USB console output was unavailable to the safe observer.
