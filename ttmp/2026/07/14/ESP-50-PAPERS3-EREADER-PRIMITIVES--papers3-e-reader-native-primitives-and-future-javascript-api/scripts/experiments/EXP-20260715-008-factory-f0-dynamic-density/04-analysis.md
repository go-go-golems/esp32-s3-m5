---
Title: "Analysis - EXP-20260715-008-factory-f0-dynamic-density"
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
Summary: "Reproducible timeline and candidate change points for the exact-F0 fixed-point density trace."
LastUpdated: 2026-07-15T01:00:00Z
WhatFor: "Capture an objective fixed-point density trace of exact FactoryTest F0."
WhenToUse: "Establish F0 temporal black/white/grayscale behavior before source-derived F1/F2 runs."
---

# Exact F0 dynamic density analysis

## Automatic result

```text
samples: 442
duration: 44.773896 seconds
post-flash-runner coverage: 29.067766 seconds
saturated / invalid: 0 / 0
density min / max: 0.606205 / 0.835286 D
cleanup: ID S,STOP -> SD LR,0 -> IS REMOTE,0
capture result: ok
```

The fixed-point trace clearly detects multiple FactoryTest update blocks and the later periodic dashboard refresh. Exact title/black/white/grayscale endpoint labels remain provisional because F0 has no internal frame-boundary ring; F2 is required to join optical transitions to scheduler events without relying only on waveform shape.

## Host markers

| Marker | Relative to first sample |
|---|---:|
| `orchestrator_begin` | -1.182684 s |
| `raw_stream_confirmed` | -0.115985 s |
| `flash_begin` | 1.922857 s |
| `flash_runner_complete` | 15.706130 s |
| `orchestrator_complete` | 45.059253 s |

## Candidate 500 ms change bins

A candidate is a 500 ms bin whose mean differs from the previous bin by at least `0.010 D`. This is a deterministic activity detector, not a semantic phase classifier.

| Bin start | Mean density | Delta from previous bin |
|---:|---:|---:|
| 17.5 s | 0.624811 D | +0.016346 D |
| 18.5 s | 0.652729 D | +0.035456 D |
| 19.0 s | 0.770559 D | +0.117830 D |
| 19.5 s | 0.739529 D | -0.031030 D |
| 20.0 s | 0.630962 D | -0.108568 D |
| 21.0 s | 0.677797 D | +0.039735 D |
| 21.5 s | 0.696800 D | +0.019003 D |
| 22.0 s | 0.668644 D | -0.028156 D |
| 22.5 s | 0.643068 D | -0.025576 D |
| 23.0 s | 0.621520 D | -0.021548 D |
| 24.5 s | 0.636154 D | +0.020012 D |
| 25.0 s | 0.659710 D | +0.023556 D |
| 25.5 s | 0.627055 D | -0.032655 D |
| 26.0 s | 0.662458 D | +0.035403 D |
| 26.5 s | 0.767199 D | +0.104741 D |
| 27.0 s | 0.795327 D | +0.028128 D |
| 28.0 s | 0.780787 D | -0.013834 D |
| 29.0 s | 0.696854 D | -0.086984 D |
| 29.5 s | 0.627200 D | -0.069654 D |
| 44.5 s | 0.637266 D | +0.014963 D |

## Files

- `density-timeline.csv` — every raw sample and both host-relative time axes.
- `density-analysis.json` — complete statistics, markers, bins, and candidates.
- `density-timeline.svg` — dependency-free visualization; orange lines mark candidate bins.
