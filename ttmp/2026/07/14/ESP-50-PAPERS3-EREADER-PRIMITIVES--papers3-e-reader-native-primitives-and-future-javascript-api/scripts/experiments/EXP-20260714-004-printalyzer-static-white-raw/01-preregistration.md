---
Title: "Preregistration - EXP-20260714-004-printalyzer-static-white-raw"
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
Summary: "Frozen settings and decision rule for a five-second static white-region raw capture."
LastUpdated: 2026-07-15T00:05:00Z
WhatFor: "Qualify static Printalyzer raw sampling on PaperS3 without driving the panel."
WhenToUse: "Review before interpreting dynamic Printalyzer traces or derived density."
---

# Preregistration

## Fixed procedure

1. Leave PaperS3 in ROM download mode; do not reset or flash.
2. Keep the already-positioned Printalyzer head flat and stationary over the operator-identified white region.
3. Snapshot read-only calibration; enter remote mode; set gain 2, integration 0, and reflection duty 128.
4. Stream for five seconds. Exclude exactly the first two samples as preregistered settling samples.
5. Stop sensor, turn reflection light off, and exit remote mode even on failure.

## Decision rule

Require at least 20 post-settling samples, zero saturation, zero invalid estimates, density range at most 0.05 D, and complete cleanup. Do not reinterpret this point measurement as whole-panel quality.
