---
Title: "Experiment - EXP-20260715-015-factory-f2-readonly-reconnect-ring-retry"
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
Summary: "Preregistered long-window F2 reconnecting boot trace retry."
LastUpdated: 2026-07-15T02:15:00Z
WhatFor: "Retry the F2 reset-time read-only trace with an operator-ready longer window."
WhenToUse: "Use only after EXP-014 expired before Reset."
---

# EXP-20260715-015-factory-f2-readonly-reconnect-ring-retry

The observer runs for 180 seconds. Reset exactly once immediately after `capture_armed=yes`; report approximate time.
