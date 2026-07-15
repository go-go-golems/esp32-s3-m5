---
Title: "Analysis - EXP-20260715-015-factory-f2-readonly-reconnect-ring-retry"
Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES
Status: active
Topics:
    - papers3
    - eink
    - hardware-qualification
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "The operator-timed reset captured F2 boot output and a complete 287-record trace dump, proving F2 boot provenance while exposing missing power-off/idle ring coverage."
LastUpdated: 2026-07-15T02:26:00Z
WhatFor: "Separate successful F2 boot/dump proof from the preregistered full-scheduler-ring requirement that the artifact did not meet."
WhenToUse: "Review before drawing an F2/F1 timing comparison or changing trace instrumentation."
---

# F2 boot and trace analysis

## Result

The operator reported Reset at approximately `2026-07-14 22:23:20 EDT` (`2026-07-15T02:23:20Z`). The observer captured ESP32-S3 boot output, then:

```text
FACTORY_TRACE_DUMP_BEGIN schema=esp50.factory-v05-runtime-trace.v1 begin=0 end=287 overwritten=0
FACTORY_TRACE_DUMP_END total=287
```

The begin marker arrived at `2026-07-15T02:23:28.566451Z`; the end marker arrived at `2026-07-15T02:23:28.706947Z`. This is direct evidence that the hash-verified F2 artifact booted and executed `FactoryTraceDumpAfterDisplayIdle()` after the FactoryTest sequence.

## Ring integrity

The dump has 287 valid records, sequences `0..286`, no overwrite, contiguous sequence numbers, and monotonically nondecreasing device timestamps. It includes 8 enqueue/dequeue/prepared groups, 1 `POWER_ON_BEGIN`, 1 `POWER_ON_END`, 131 `FRAME_BEGIN`, and 130 `FRAME_END` records.

## Preregistered limitation

The extractor correctly rejected the full scheduler-ring criterion because the trace contains **no** `POWER_OFF_BEGIN`, `POWER_OFF_END`, or `DISPLAY_IDLE` records; its final record is `FRAME_BEGIN` at device timestamp `10024540 us`. Therefore no qualified ring-host alignment or F2/F1 scheduler comparison is produced for this experiment.

The trace-dump call itself occurs after `M5.Display.waitDisplay()` in `main.cpp`, but that source-level wait cannot retroactively make the absent ring events present. The evidence supports F2 boot provenance and partial enqueue/power-on/frame structure, not a complete power-off/idle timing claim.
