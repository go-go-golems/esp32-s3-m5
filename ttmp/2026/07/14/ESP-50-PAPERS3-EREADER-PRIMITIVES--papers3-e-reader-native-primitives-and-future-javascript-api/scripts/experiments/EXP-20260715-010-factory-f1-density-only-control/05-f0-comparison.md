---
Title: "F0/F1 Fixed-Point Density Comparison"
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
Summary: "Deterministic host-marker-normalized F0/F1 point-density shape comparison."
LastUpdated: 2026-07-15T01:45:00Z
WhatFor: "Decide whether F1 trace-off is an adequate fixed-point source proxy before F2."
WhenToUse: "Review after F1 density-only capture and before F2 physical authorization."
---

# F0/F1 fixed-point density comparison

## Result

The best baseline-subtracted shape correlation over `0.0`–`25.0` seconds after flash-runner completion is **0.943874** with F1 sampled at F0 time plus `-0.5` seconds. Equivalently, shift the F1 timeline by `+0.5` seconds to overlay it on F0. The normalized RMS difference is `0.019832 D` over `251` resampled points.

```text
F0 first-1.0s baseline: 0.608864264 D
F1 first-1.0s baseline: 0.618167157 D
best Pearson correlation: 0.943874
best F1 sample lag relative to F0: -0.5 s
normalized RMS difference: 0.019832 D
```

## Limits

This compares only baseline-subtracted shape at the unchanged Printalyzer aperture, normalized by each run's host `flash_runner_complete` marker. It cannot prove spatial equivalence, absolute optical density equivalence after a movement, or that each candidate maps to the same semantic display phase. It is nevertheless the required trace-off observer/source control for deciding whether F2 is meaningful.
