---
Title: "Preregistration - EXP-20260715-013-factory-f2-esptool-reset-density"
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
Summary: "Frozen esptool reset ownership sequence and F2 capture evidence rules."
LastUpdated: 2026-07-15T02:45:00Z
WhatFor: "Run F2 without concurrent USB ownership while retaining the delayed scheduler ring."
WhenToUse: "Use after F2 auto ownership failure and instead of a human reset."
---

# Preregistration

## Rationale

Experiment 011 correctly refused concurrent esptool and capture ownership. This experiment separates ownership sequentially without human intervention: esptool stages F2 without reset, uses its own ESP32-S3 USB-JTAG hard-reset handling, closes the port, then the collector opens PaperS3 read-only. The F2 ring is dumped post-idle after the FactoryTest sequence, so the short post-reset handoff does not omit the stored ring.

## Procedure

1. Confirm exact F2 SHA-256 and audited trace build under ESP-IDF 5.3.3.
2. Flash using esptool `--after no_reset`.
3. Query bootloader `chip_id` using `--before no_reset --after hard_reset --no-stub`; retain its log.
4. After esptool closes ACM0, collect 60 seconds of fixed-head Printalyzer raw readings and read-only PaperS3 output.
5. Require collector cleanup, dump delimiters, contiguous records, and required power/frame/idle events.

## Limits

The host opens the observer after esptool hard reset. This experiment is not an early-boot transcript claim. It relies on the post-idle trace ring for events that occurred before the observer opened; its host alignment remains approximate.
