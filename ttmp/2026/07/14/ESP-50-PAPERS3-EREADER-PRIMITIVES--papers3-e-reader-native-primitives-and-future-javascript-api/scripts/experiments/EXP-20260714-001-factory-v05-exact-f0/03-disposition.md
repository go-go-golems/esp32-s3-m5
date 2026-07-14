---
Title: "Disposition - EXP-20260714-001-factory-v05-exact-f0"
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
Summary: "Separate automatic and optical dispositions for F0-exact-vendor."
LastUpdated: 2026-07-14T23:45:00Z
WhatFor: "Preserve a preregistered FactoryTest F0/F1/F2 experiment treatment and its evidence."
WhenToUse: "Use during and after the corresponding physical treatment; never substitute evidence from another treatment."
---

# Disposition: EXP-20260714-001-factory-v05-exact-f0

- Execution status: executed once; video ingested; optical review pending
- Automatic transaction disposition: **pass** — exact merged release hash verified, flash completed, and esptool verified flash data
- Optical disposition: pending frame extraction and operator review
- Stop reason: normal F0 observation completion; later observer-control smoke test reset the board into ROM download mode without replaying the panel sequence
- Eligible to proceed to next treatment: **no** (pending optical review)
- Reviewer notes: Preserve the later serial-attachment reset as a tooling observer failure, not as an F0 optical outcome. Explicit authorization is required before resetting out of ROM mode because F0 boot replays the display sequence.
