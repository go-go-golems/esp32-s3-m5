---
Title: "Disposition - EXP-20260715-008-factory-f0-dynamic-density"
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
Summary: "Pending automatic, temporal-density, and F0 interpretation disposition."
LastUpdated: 2026-07-15T00:50:00Z
WhatFor: "Capture an objective fixed-point density trace of exact FactoryTest F0."
WhenToUse: "Establish F0 temporal black/white/grayscale behavior before source-derived F1/F2 runs."
---

# Disposition

- Execution: complete — exact F0 replay with 45-second fixed-head density stream
- Automatic transaction disposition: **pass** — exact artifact, flash, collector, and cleanup passed
- Density-trace disposition: **pass** — 442 samples, zero saturation/invalid records, expected sequence observed; phase labels remain source/host-time approximate
- F0 optical disposition: remains separate — density is one point and existing video remains spatial evidence
- F1 eligibility: conditional yes — requires a new F1 dynamic ledger, unchanged head, fixed camera/video, and explicit operator authorization
