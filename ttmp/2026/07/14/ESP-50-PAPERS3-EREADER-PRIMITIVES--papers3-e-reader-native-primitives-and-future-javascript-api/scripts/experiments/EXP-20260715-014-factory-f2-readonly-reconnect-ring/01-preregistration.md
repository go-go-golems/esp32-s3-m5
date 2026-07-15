---
Title: "Preregistration - EXP-20260715-014-factory-f2-readonly-reconnect-ring"
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
Summary: "Frozen reset-time read-only serial observation protocol."
LastUpdated: 2026-07-15T02:00:00Z
WhatFor: "Safely observe F2 one-shot output across a physical reset."
WhenToUse: "Use after F2 flash verification when automatic post-reset observation is silent."
---

# Preregistration

The current board image was hash-verified as F2 during EXP-013, but its visual FactoryTest sequence is intentionally indistinguishable from vendor FactoryTest and the automatic post-reset observer received zero bytes.

A reconnecting observer opens PaperS3 read-only before reset, records USB disconnect/reopen, and never sends data or DTR/RTS/termios control. After `capture_armed=yes`, the operator presses Reset exactly once. This is a serial/ring diagnostic only; Printalyzer is unused and its LED remains off.

Pass requires a complete F2 dump and valid ring records.
