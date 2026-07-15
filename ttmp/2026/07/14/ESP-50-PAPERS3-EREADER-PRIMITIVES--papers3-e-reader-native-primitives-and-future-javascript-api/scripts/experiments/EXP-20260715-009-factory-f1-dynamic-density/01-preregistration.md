---
Title: "Preregistration - EXP-20260715-009-factory-f1-dynamic-density"
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
Summary: "Frozen F1 artifact, Printalyzer settings, camera gate, and F0 comparison rules."
LastUpdated: 2026-07-15T01:20:00Z
WhatFor: "Compare exact F0 with a source-derived trace-off F1 density and video record."
WhenToUse: "Only after F0 review and camera/head stabilization; before F2 authorization."
---

# Preregistration

## Preconditions

1. F0 experiment 008 has been reviewed.
2. Printalyzer remains fixed at the F0 aperture point.
3. Camera uses the same locked position, exposure, focus, white balance, and illumination as the F0 comparison.
4. Camera is recording before F1 flash/reset.

## Procedure

Start a 45-second gain-2/100-ms/duty-128 density stream; retain two seconds pre-flash baseline; flash source-derived trace-off F1 SHA-256 `3d9bf...`; record title, black, white, grayscale, and dashboard; then require complete cleanup.

## Decision rule

Require clean flash/capture/cleanup, no saturation or invalid estimates, and 25 seconds post-flash capture. Compare F1's fixed-head transition structure with F0 after time normalization. Review video separately for spatial endpoint similarity. Do not run F2 until both comparisons are reviewed.
