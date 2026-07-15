---
Title: "Preregistration - EXP-20260715-006-printalyzer-fixed-position-repeat"
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
Summary: "No-movement repeat with a 0.005 D mean-delta gate."
LastUpdated: 2026-07-15T00:35:00Z
WhatFor: "Separate fixed-position repeatability from manual repositioning sensitivity."
WhenToUse: "Interpret EXP-004/005 placement differences and future fixed-position traces."
---

# Preregistration

Keep both devices unchanged. Repeat gain 2 / 100 ms / duty 128 for five seconds, exclude the first two samples, and require at least 20 valid unsaturated post-settling samples, range at most 0.01 D, absolute mean delta from EXP-005 at most 0.005 D, and complete cleanup.
