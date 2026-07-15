---
Title: "Disposition - EXP-20260715-016-native-epd-density-step-response"
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
Summary: "Automatic and fixed-aperture disposition for native density step run."
LastUpdated: 2026-07-15T03:00:00Z
WhatFor: "Record the native direct-driver density step experiment."
WhenToUse: "Review P0.19 results before further panel experiments."
---

# Disposition

- Exact build/flash: pass — SHA-256 `eb77a34c7073e0dce54725d2d10ae134168e843825013861f5dfaf8d37134bce`, esptool hash verification
- Firmware transaction: pass — cleanup, black, and white each returned `result=ok`, `pending=0`, `rails=idle`
- Density capture/cleanup: pass — 1,477 valid, 0 invalid, 0 saturated raw estimates; guarded cleanup completed
- Fixed-aperture directional criterion: fail — full black settled at `0.617748 D`, below preceding white settled `0.621181 D`
- Optical/operator disposition: pending
- Interpretation: a successful direct-driver full-black request is not sufficient to establish expected darkening at the fixed aperture
