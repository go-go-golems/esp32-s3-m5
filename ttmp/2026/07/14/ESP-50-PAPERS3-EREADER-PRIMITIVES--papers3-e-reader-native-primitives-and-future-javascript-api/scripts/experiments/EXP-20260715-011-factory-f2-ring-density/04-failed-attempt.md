---
Title: "Failed Attempt - EXP-20260715-011-factory-f2-ring-density"
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
Summary: "F2 automatic flash-plus-firmware-capture attempt stopped before flashing because exclusive serial ownership was correctly enforced."
LastUpdated: 2026-07-15T02:10:00Z
WhatFor: "Preserve the failed automatic sequencing evidence and motivate the manual-reset F2 protocol."
WhenToUse: "Review before staging F2 without auto-reset and arming the safe read-only capture."
---

# F2 automatic sequencing failure

## Result

**No F2 firmware flash occurred. No PaperS3 panel operation occurred.**

The combined collector successfully ran the Printalyzer raw stream and opened PaperS3 USB Serial/JTAG with the safe read-only non-modem-control descriptor. The guarded flash runner then correctly refused to flash because it requires exclusive ownership of the same serial port:

```text
error: serial owned by PID(s): 172169
```

This is expected under the single-owner policy: the automatic approach cannot both monitor F2 from reset and use the same USB Serial/JTAG port for esptool auto-reset/flash.

## Evidence

- `raw-dynamic-f2.jsonl` — 60-second density-only stream; not an F2 execution record.
- `collector-console.log` — collector completed normally.
- `flash-runner-console.log` — exclusive-owner refusal.
- `host-events.jsonl` — attempted orchestration markers; no `flash_runner_complete` marker.

## Consequence

Do not retry experiment 011. A new manual-reset experiment must:

1. flash F2 with `--after no_reset` while no collector owns the port;
2. release the flashing port;
3. arm safe read-only firmware+density capture;
4. explicitly ask the operator to press Reset while capture is open;
5. extract the post-idle ring after capture.

This preserves serial ownership and captures the F2 dump from boot without any automatic reset race.
