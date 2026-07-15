---
Title: "Preregistration - EXP-20260715-008-factory-f0-dynamic-density"
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
Summary: "Frozen exact-F0 artifact, sensor settings, timing, and interpretation limits."
LastUpdated: 2026-07-15T00:50:00Z
WhatFor: "Capture an objective fixed-point density trace of exact FactoryTest F0."
WhenToUse: "Establish F0 temporal black/white/grayscale behavior before source-derived F1/F2 runs."
---

# Preregistration

## Procedure

1. Keep the Printalyzer fixed at its current middle-ish point.
2. Start gain-2, 100-ms, duty-128 raw capture for 45 seconds.
3. Record two seconds of pre-flash baseline.
4. Flash exact F0 SHA-256 `d6733a...` and hard reset.
5. Capture title, black, white, grayscale, and dashboard transitions.
6. Stop sensor, turn LED off, and exit remote mode.

## Decision rule

Require clean collector/flash completion, zero saturation/invalid estimates, complete cleanup, and at least 25 seconds after flash completion. Segment change points before assigning expected phase labels. This point trace cannot score spatial gradients or justify cross-run absolute comparisons after repositioning.
