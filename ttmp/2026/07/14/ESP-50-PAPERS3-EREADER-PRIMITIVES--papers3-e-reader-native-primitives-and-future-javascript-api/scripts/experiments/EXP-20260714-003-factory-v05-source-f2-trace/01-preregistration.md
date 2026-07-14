---
Title: "Preregistration - EXP-20260714-003-factory-v05-source-f2-trace"
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
Summary: "Frozen hypothesis, decision rule, and protocol for F2-stock-source-trace-timing."
LastUpdated: 2026-07-14T22:50:00Z
WhatFor: "Preserve a preregistered FactoryTest F0/F1/F2 experiment treatment and its evidence."
WhenToUse: "Use during and after the corresponding physical treatment; never substitute evidence from another treatment."
---

# Preregistration: EXP-20260714-003-factory-v05-source-f2-trace

## Hypothesis

Fixed-ring boundary tracing does not materially change F1 optical behavior and reveals the actual source-derived eraser/target/frame/power schedule.

## Decision rule

Reject timing interpretation if F2 optical endpoint differs materially from F1, ring overwrites/invalid records occur, or trace dump begins before DISPLAY_IDLE/POWER_OFF_END.

## Fixed sequence

1. Confirm artifact SHA, audit PASS, stable by-id port, and no serial owner.
2. Start fixed video before flash-triggered reset; do not change camera or lighting between F0/F1/F2.
3. Observe title, full black, full white, sixteen grayscale bars, and dashboard.
4. Judge the two-second white interval specifically against the immediately preceding black field.
5. Record automatic and optical dispositions separately and verbatim.
6. Stop on any preregistered safety condition; do not skip ahead to a later treatment.

## Forbidden post-hoc changes

Do not change exposure, white balance, focus, illumination, waveform, dwell, source, SDK, or treatment order after seeing an endpoint. A necessary correction creates a new experiment ID.
