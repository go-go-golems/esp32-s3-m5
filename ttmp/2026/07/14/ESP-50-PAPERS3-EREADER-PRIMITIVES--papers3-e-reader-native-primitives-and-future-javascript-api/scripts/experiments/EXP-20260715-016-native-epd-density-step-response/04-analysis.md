---
Title: "Analysis - EXP-20260715-016-native-epd-density-step-response"
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
Summary: "Fixed-aperture causal density analysis for direct-driver white-black-white sequence."
LastUpdated: 2026-07-15T03:00:00Z
WhatFor: "Record the native direct-driver density step experiment."
WhenToUse: "Review P0.19 results before further panel experiments."
---

# Analysis

## Automatic result

The direct-driver firmware SHA-256 `eb77a34c7073e0dce54725d2d10ae134168e843825013861f5dfaf8d37134bce` was written with all esptool segments hash-verified. The operator-reported Reset time was approximately `2026-07-14 22:50:34 EDT`. The selected automatic run is the second boot, whose capture grace ended at `2026-07-15T02:50:54.614455Z`; the first boot reset before its ten-second grace elapsed, so it performed no panel operation.

The selected run completed all three direct-driver operations with `result=ok`, `pending=0`, and `rails=idle`: HARD cleanup white (397 ms), full black (382 ms), and full white (382 ms). The Printalyzer stream provided 1,477 valid raw-density estimates with zero invalid and saturated samples; guarded cleanup completed normally.

## Fixed-aperture density result

| settled window | samples | mean D | standard deviation D |
| --- | ---: | ---: | ---: |
| cleanup-white, 4.4–8.75 s after begin | 43 | 0.621181 | 0.000291 |
| full-black, 8.45–12.80 s after cleanup begin | 43 | 0.617748 | 0.002182 |
| final-white, 12.45–20.45 s after cleanup begin | 78 | 0.614748 | 0.000464 |

The full-black operation did **not** produce a sustained density increase at this aperture. Its settled mean was `-0.003433 D` relative to the preceding white-settled mean, while final white reached a further `-0.003000 D` relative to black. This is a fixed-position observation, not a whole-panel or cross-placement claim.

## Firmware-to-density correlation

The firmware supplied explicit semantic boundaries on the same host monotonic clock as the densitometer. Relevant host marker times were:

```text
cleanup begin   02:50:54.631217Z   cleanup idle   02:50:55.044694Z
black begin     02:50:59.073733Z   black idle     02:50:59.471889Z
white begin     02:51:03.494421Z   white idle     02:51:03.892704Z
experiment end  02:51:11.900821Z
```

Therefore this run supports a direct causal statement: the pinned direct-driver full-black command completed successfully, yet the fixed Printalyzer aperture did not show the expected sustained darkening before the semantic full-white command. It does not identify whether the cause is waveform/rail/panel physics, spatial nonuniformity outside the aperture, or the optical probe geometry.

## Disposition

Automatic/software and fixed-point density collection pass. The expected blackening directional criterion fails at the measured aperture. Optical/operator disposition remains pending.
