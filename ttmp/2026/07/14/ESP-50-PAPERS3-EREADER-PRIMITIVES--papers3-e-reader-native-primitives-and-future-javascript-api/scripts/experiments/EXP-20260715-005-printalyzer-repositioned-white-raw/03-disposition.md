---
Title: "Disposition - EXP-20260715-005-printalyzer-repositioned-white-raw"
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
Summary: "Pending automatic and placement-sensitivity disposition."
LastUpdated: 2026-07-15T00:30:00Z
WhatFor: "Measure lift-and-reseat sensitivity of static Printalyzer PaperS3 sampling."
WhenToUse: "Review before treating point-density changes as panel transitions."
---

# Disposition

- Execution: complete — five-second repositioned static capture
- Automatic data disposition: **pass** — 46 valid post-settling samples, no saturation, complete cleanup, 0.000373 D within-run range
- Placement-sensitivity disposition: **fail** — absolute mean delta from EXP-004 was 0.078067 D, exceeding the preregistered 0.03 D maximum
- Dynamic fixed-placement timing eligibility: provisional; requires reference-target validation and no movement throughout the run
- Cross-run absolute-density eligibility: no; requires a mechanical fixture or explicit repositioning uncertainty
