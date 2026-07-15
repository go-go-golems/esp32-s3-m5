---
Title: "Preregistration - EXP-20260715-007-printalyzer-hand-wave-perturbation"
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
Summary: "Frozen nearby-hand-motion perturbation and stability threshold."
LastUpdated: 2026-07-15T00:42:00Z
WhatFor: "Test whether nearby hand motion perturbs an ostensibly sealed fixed-position Printalyzer reading."
WhenToUse: "Diagnose unexplained transient excursions before dynamic panel capture."
---

# Preregistration

Keep the Printalyzer and PaperS3 fixed. During the ten-second gain-2/100-ms/duty-128 stream, wave a hand around the instrument without touching any equipment. Exclude two settling samples. Require at least 80 post-settling samples, no saturation/invalid estimate, range and maximum median excursion each at most 0.01 D, and complete cleanup.
