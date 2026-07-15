---
Title: "Incomplete Capture - EXP-20260715-014-factory-f2-readonly-reconnect-ring"
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
Summary: "The armed pre-reset read-only observer completed normally but received zero firmware lines after the operator-reported single Reset."
LastUpdated: 2026-07-15T02:05:00Z
WhatFor: "Preserve the silent reset-time trace result and block a blind repeat."
WhenToUse: "Review before changing F2 serial output or reset diagnostics."
---

# Silent reset-time F2 observation

The observer was armed successfully and used only `O_RDONLY | O_NOCTTY | O_NONBLOCK`, sent no bytes, and issued no modem-control ioctl. It completed its 75-second window normally.

**Operator correction:** the physical Reset was pressed only after this 75-second window had expired. The resulting capture contains exactly four observer lifecycle records—`capture_begin`, `source_open`, `source_close`, and `capture_end`—and **zero** `rx_line` records because no reset-time F2 output occurred within its window. It recorded no USB disconnect/reconnect event.

This experiment is an expired-window procedural miss, not evidence about F2 stdout, reset behavior, or trace availability. Do not infer a silent F2 boot from it. A separately preregistered retry should use a longer arm window and only proceed when the operator is ready to press Reset immediately.
