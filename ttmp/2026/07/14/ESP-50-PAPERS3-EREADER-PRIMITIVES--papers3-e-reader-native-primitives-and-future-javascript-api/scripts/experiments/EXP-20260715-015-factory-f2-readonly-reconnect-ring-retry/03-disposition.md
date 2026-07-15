---
Title: "Disposition - EXP-20260715-015-factory-f2-readonly-reconnect-ring-retry"
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
Summary: "Pending F2 reset-time trace/ring retry outcome."
LastUpdated: 2026-07-15T02:15:00Z
WhatFor: "Retry the F2 reset-time read-only trace with an operator-ready longer window."
WhenToUse: "Use only after EXP-014 expired before Reset."
---

# Disposition

- Observer/capture: pass — 180-second read-only capture completed; boot output received
- Operator timing confirmation: pass — reported approximately 2026-07-14 22:23:20 EDT
- F2 dump markers: pass — `begin=0 end=287 overwritten=0`, followed by end marker
- Ring integrity: pass — 287 contiguous records with monotonic device timestamps
- Full scheduler-ring validation: fail — no POWER_OFF_BEGIN, POWER_OFF_END, or DISPLAY_IDLE records
- F2 boot provenance: pass — hash-verified F2 boot output and trace dump captured
- F2/F1 scheduler comparison: blocked — full preregistered power-off/idle event criterion not met
