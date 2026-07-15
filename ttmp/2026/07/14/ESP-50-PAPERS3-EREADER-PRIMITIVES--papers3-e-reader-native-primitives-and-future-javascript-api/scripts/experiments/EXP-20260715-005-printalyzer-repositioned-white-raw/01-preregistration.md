---
Title: "Preregistration - EXP-20260715-005-printalyzer-repositioned-white-raw"
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
Summary: "Frozen settings and comparison threshold for PaperS3 placement sensitivity."
LastUpdated: 2026-07-15T00:30:00Z
WhatFor: "Measure lift-and-reseat sensitivity of static Printalyzer PaperS3 sampling."
WhenToUse: "Review before treating point-density changes as panel transitions."
---

# Preregistration

## Fixed procedure

1. PaperS3 remains in ROM download mode.
2. Use the operator-completed lift-and-reseat in the same orientation over approximately the same white region.
3. Repeat experiment 004 settings for five seconds; exclude exactly the first two samples.
4. Require complete sensor-stop, LED-off, and remote-exit cleanup.

## Decision rule

Require at least 20 post-settling samples, zero saturation/invalid estimates, within-run range at most 0.05 D, and absolute mean delta from experiment 004 at most 0.03 D. The delta includes both placement sensitivity and local panel nonuniformity.
