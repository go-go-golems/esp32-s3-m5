---
Title: "Preregistration - EXP-20260715-015-factory-f2-readonly-reconnect-ring-retry"
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
Summary: "Frozen operator-ready longer-window reset-time F2 serial protocol."
LastUpdated: 2026-07-15T02:15:00Z
WhatFor: "Retry the F2 reset-time read-only trace with an operator-ready longer window."
WhenToUse: "Use only after EXP-014 expired before Reset."
---

# Preregistration

EXP-014 expired before Reset and produced no F2 diagnostic evidence. This retry changes only execution timing: the capture window is 180 seconds, the operator is ready before arming, presses Reset immediately after the arm signal, and reports an approximate press time.

PaperS3 access remains read-only, nonblocking, and non-controlling. No Printalyzer activity occurs. A valid F2 dump/ring is required; otherwise this retry makes no firmware-output claim.
