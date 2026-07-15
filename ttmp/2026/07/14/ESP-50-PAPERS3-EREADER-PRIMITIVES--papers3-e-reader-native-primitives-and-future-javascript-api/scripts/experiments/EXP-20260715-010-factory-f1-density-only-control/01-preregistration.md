---
Title: "Preregistration - EXP-20260715-010-factory-f1-density-only-control"
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
Summary: "Frozen F1 trace-off point-density control and F2 decision gate."
LastUpdated: 2026-07-15T01:35:00Z
WhatFor: "Run the required F1 trace-off control using fixed-point density without a new camera recording."
WhenToUse: "Use after exact F0 density analysis and before F2 trace-on execution."
---

# Preregistration

## Why F1 is not skipped

F1 uses the stock FactoryTest source, exact IDF 5.3.3 lineage, and trace compiled completely out. It is the control that separates a source/runtime difference from F2 tracing effects. F2 alone cannot prove that its observed point-density behavior belongs to uninstrumented FactoryTest source.

## Fixed procedure

1. Keep Printalyzer/PaperS3/table/cables unmoved at the F0 aperture point.
2. No camera is required for this density-only control.
3. Capture 45 seconds at gain 2, 100 ms, and duty 128 with two seconds pre-flash baseline.
4. Flash F1 SHA-256 `3d9bf...`; capture the built-in sequence and dashboard.
5. Require sensor stop, LED off, and remote exit.

## Decision rule

Require clean flash/capture/cleanup, no saturation/invalid estimates, and at least 25 seconds after flash-runner completion. Compare F1/F0 only at the unchanged point after host-marker time normalization. A material unexplained order/shape divergence blocks F2. This experiment does not make a spatial quality claim.
