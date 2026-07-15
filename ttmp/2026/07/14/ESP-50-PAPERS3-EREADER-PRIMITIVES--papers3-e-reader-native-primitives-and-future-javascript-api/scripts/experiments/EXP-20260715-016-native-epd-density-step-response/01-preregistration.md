---
Title: "Preregistration - EXP-20260715-016-native-epd-density-step-response"
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
Summary: "Frozen native direct-driver density step protocol."
LastUpdated: 2026-07-15T03:00:00Z
WhatFor: "Record the native direct-driver density step experiment."
WhenToUse: "Review P0.19 results before further panel experiments."
---

# Preregistration

Fixed target: hash `eb77a34c7073e0dce54725d2d10ae134168e843825013861f5dfaf8d37134bce`, direct-driver HIGH two-stage HARD-white → black → white automatic sequence. Preserve fixed Printalyzer geometry. Arm both streams before exactly one physical Reset. Reject any absent semantic marker, fault, saturation/invalid density, or incomplete cleanup.
