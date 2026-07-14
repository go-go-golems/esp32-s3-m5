---
Title: "Preregistration - EXP-20260714-001-factory-v05-exact-f0"
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
Summary: "Frozen hypothesis, decision rule, and protocol for F0-exact-vendor."
LastUpdated: 2026-07-14T22:50:00Z
WhatFor: "Preserve a preregistered FactoryTest F0/F1/F2 experiment treatment and its evidence."
WhenToUse: "Use during and after the corresponding physical treatment; never substitute evidence from another treatment."
---

# Preregistration: EXP-20260714-001-factory-v05-exact-f0

## Hypothesis

The exact released FactoryTest V0.5 black-to-white sequence establishes whether vendor firmware erases its immediately preceding full-black field cleanly.

## Decision rule

Pass white only if the full-black field disappears without material retained field, gradient, or edge residue; dashboard quality is scored separately.

## Fixed sequence

1. Confirm artifact SHA, audit PASS, stable by-id port, and no serial owner.
2. Start fixed video before flash-triggered reset; do not change camera or lighting between F0/F1/F2.
3. Observe title, full black, full white, sixteen grayscale bars, and dashboard.
4. Judge the two-second white interval specifically against the immediately preceding black field.
5. Record automatic and optical dispositions separately and verbatim.
6. Stop on any preregistered safety condition; do not skip ahead to a later treatment.

## Forbidden post-hoc changes

Do not change exposure, white balance, focus, illumination, waveform, dwell, source, SDK, or treatment order after seeing an endpoint. A necessary correction creates a new experiment ID.
