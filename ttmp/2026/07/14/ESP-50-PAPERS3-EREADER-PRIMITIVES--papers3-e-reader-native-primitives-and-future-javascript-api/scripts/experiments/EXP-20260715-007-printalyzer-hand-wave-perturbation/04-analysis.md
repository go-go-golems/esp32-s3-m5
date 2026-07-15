---
Title: "Analysis - EXP-20260715-007-printalyzer-hand-wave-perturbation"
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
Summary: "Hand-wave run detected a large optical excursion but is confounded by operator-reported app-background changes."
LastUpdated: 2026-07-15T00:42:11Z
WhatFor: "Test whether nearby hand motion perturbs an ostensibly sealed fixed-position Printalyzer reading."
WhenToUse: "Diagnose unexplained transient excursions before dynamic panel capture."
---

# Hand-wave perturbation analysis

## Result

**Automatic threshold FAIL; causal interpretation INCONCLUSIVE.** Ninety-six post-settling samples were valid and unsaturated. Density range was `0.078396 D` and maximum excursion from the median was `0.077458 D`, both far above the preregistered `0.01 D` threshold.

The strongest excursion occurred in approximately the first second and then decayed toward a stable `~0.602 D` plateau. The operator subsequently noted that “the background of the app also changes,” meaning the measured target may have changed during the deliberate hand perturbation. Hand timing was not independently marked. Therefore this run proves that the optical signal changed, but cannot distinguish hand/ambient influence from a real display-target change.

```text
post-settling n: 96
mean / median: 0.606275 / 0.602998 D
standard deviation: 0.011424 D
range: 0.078396 D
maximum median excursion: 0.077458 D
saturated / invalid: 0 / 0
capture SHA-256: 166f937f810be5957a99f6bb34a71be22840d40276c48c074287348542b16f98
cleanup: complete
```

This result must not be cited as proof of ambient-light leakage. A future perturbation test needs a verified static target and host markers for hand-wave start/stop.
