---
Title: "Disposition - EXP-20260715-013-factory-f2-esptool-reset-density"
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
Summary: "Pending F2 automatic flash/reset, density, ring, and observer disposition."
LastUpdated: 2026-07-15T02:45:00Z
WhatFor: "Run F2 without concurrent USB ownership while retaining the delayed scheduler ring."
WhenToUse: "Use after F2 auto ownership failure and instead of a human reset."
---

# Disposition

- Exact F2 flash: pass — all flash writes hash-verified
- Esptool hard reset: command completed — `Hard resetting via RTS pin...`; resulting boot state is not yet proven
- Density/cleanup: collector pass; 60-second raw stream retained
- Ring validation: fail/incomplete — zero firmware payload lines, no dump markers
- F2/F1 observer comparison: not valid; do not compare without ring
- Next branch decision: collect operator observation, then distinguish ROM-download reset outcome from unavailable USB console before any retry
