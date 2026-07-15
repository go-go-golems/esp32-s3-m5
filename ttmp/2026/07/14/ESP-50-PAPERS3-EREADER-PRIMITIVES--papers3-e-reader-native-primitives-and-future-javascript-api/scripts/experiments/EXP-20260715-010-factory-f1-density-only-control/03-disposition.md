---
Title: "Disposition - EXP-20260715-010-factory-f1-density-only-control"
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
Summary: "Pending F1 automatic and F0 point-trace comparison disposition."
LastUpdated: 2026-07-15T01:35:00Z
WhatFor: "Run the required F1 trace-off control using fixed-point density without a new camera recording."
WhenToUse: "Use after exact F0 density analysis and before F2 trace-on execution."
---

# Disposition

- Execution: complete — F1 trace-off replay with 45-second fixed-point density stream
- Automatic transaction disposition: **pass** — F1 hash/preflight, flash, 442-sample collector, zero saturation/invalid records, and complete cleanup passed
- F1/F0 point-trace comparison: **provisional pass** — baseline-subtracted 0–25 s shape Pearson correlation 0.943874 after a 0.5 s F1-to-F0 alignment; normalized RMS difference 0.019832 D
- Operator/no-anomaly disposition: pending
- F2 eligibility: conditional pending operator confirmation and review of F1/F0 comparison limits
