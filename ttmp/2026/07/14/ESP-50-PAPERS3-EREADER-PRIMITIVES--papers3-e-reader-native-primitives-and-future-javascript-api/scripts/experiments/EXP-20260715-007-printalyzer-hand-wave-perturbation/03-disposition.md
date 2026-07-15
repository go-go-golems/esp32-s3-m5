---
Title: "Disposition - EXP-20260715-007-printalyzer-hand-wave-perturbation"
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
Summary: "Pending hand-wave perturbation disposition."
LastUpdated: 2026-07-15T00:42:00Z
WhatFor: "Test whether nearby hand motion perturbs an ostensibly sealed fixed-position Printalyzer reading."
WhenToUse: "Diagnose unexplained transient excursions before dynamic panel capture."
---

# Disposition

- Execution: complete
- Automatic data disposition: **fail** — 0.078396 D range and 0.077458 D median excursion exceeded 0.01 D limits
- Ambient/nearby-motion sensitivity: **inconclusive** — app background also changed and perturbation timing was not marked
- Evidence use: detects a real optical excursion only; do not assign cause
- Cleanup: pass
