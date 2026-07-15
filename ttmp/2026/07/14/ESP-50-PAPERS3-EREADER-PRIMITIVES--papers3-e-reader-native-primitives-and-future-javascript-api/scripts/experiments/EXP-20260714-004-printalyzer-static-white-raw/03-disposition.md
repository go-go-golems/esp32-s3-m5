---
Title: "Disposition - EXP-20260714-004-printalyzer-static-white-raw"
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
Summary: "Pending automatic and interpretation disposition for static raw capture."
LastUpdated: 2026-07-15T00:05:00Z
WhatFor: "Qualify static Printalyzer raw sampling on PaperS3 without driving the panel."
WhenToUse: "Review before interpreting dynamic Printalyzer traces or derived density."
---

# Disposition

- Execution: complete — five-second static capture; PaperS3 remained in ROM mode
- Automatic data disposition: **pass** — 46 post-settling samples, zero saturation, zero invalid estimates, 0.002243 D range, complete cleanup
- Geometry disposition: **provisional pass for within-run stability** — head remained stationary; cross-placement repeatability still pending
- Absolute density disposition: unqualified; 0.678142 D is a stable host estimate, not yet a normal calibrated panel measurement
- Cleanup disposition: **pass** — device acknowledged STOP, LED-off, and remote-exit; operator confirmed LED was off afterward but did not observe the exact shutdown instant
- Panel-observation note: no software path touched PaperS3 and no obvious visual change was reported; operator visual confirmation was uncertain
- Eligible for dynamic density run: no (pending repositioning and reference-target raw review)
