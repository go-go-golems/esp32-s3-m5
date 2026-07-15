---
Title: "Preregistration - EXP-20260715-011-factory-f2-ring-density"
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
Summary: "Frozen F2 artifact, ring validation, density settings, and alignment limits."
LastUpdated: 2026-07-15T02:00:00Z
WhatFor: "Capture F2 trace-on scheduler evidence together with fixed-point density."
WhenToUse: "Use after F1 trace-off review and before any waveform/rail conclusion."
---

# Preregistration

## Preconditions

1. F1 density-only control has passed automatic, point-trace, and operator/no-anomaly gates.
2. Printalyzer/PaperS3/table/cables remain unmoved at the F0/F1 aperture point.
3. No PaperS3 serial input may be sent; firmware serial capture is read-only and must not issue modem control.

## Procedure

Start combined Printalyzer raw and read-only PaperS3 serial capture for 60 seconds. Retain two seconds pre-flash baseline. Flash audited F2 SHA-256 `95334...`. Capture the F2 post-idle dump, then extract, validate, and approximately host-align the ring by mapping final `DISPLAY_IDLE` to host receipt of `FACTORY_TRACE_DUMP_BEGIN`.

## Decision rule

Require clean flash/capture/cleanup; no density saturation/invalid estimates; dump begin/end markers; contiguous valid ring sequence; and `POWER_ON_BEGIN`, `FRAME_BEGIN`, `FRAME_END`, `POWER_OFF_END`, and `DISPLAY_IDLE`. Treat host/device mapping as relative sequence alignment only: printf/USB delay from idle to host dump-begin receipt is not independently bounded.
