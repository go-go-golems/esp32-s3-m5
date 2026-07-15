---
Title: "Analysis - Source F1 trace-off dynamic density"
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
Summary: "Reproducible timeline and candidate change points for the Source F1 trace-off fixed-point density trace."
LastUpdated: 2026-07-15T01:00:00Z
WhatFor: "Capture an objective fixed-point density trace for Source F1 trace-off."
WhenToUse: "Review temporal black/white/grayscale activity before a later comparison treatment."
---

# Source F1 trace-off dynamic density analysis

## Automatic result

```text
samples: 442
duration: 44.722003 seconds
post-flash-runner coverage: 28.952269 seconds
saturated / invalid: 0 / 0
density min / max: 0.614619 / 0.851753 D
cleanup: ID S,STOP -> SD LR,0 -> IS REMOTE,0
capture result: ok
```

The fixed-point trace clearly detects multiple FactoryTest update blocks and the later periodic dashboard refresh. Exact title/black/white/grayscale endpoint labels remain provisional because this density stream is only a single physical aperture; internal frame-boundary evidence or video alignment is required to assign semantic phase names without relying only on waveform shape.

## Host markers

| Marker | Relative to first sample |
|---|---:|
| `orchestrator_begin` | -1.189455 s |
| `raw_stream_confirmed` | -0.134896 s |
| `flash_begin` | 1.903342 s |
| `flash_runner_complete` | 15.769734 s |
| `orchestrator_complete` | 45.055438 s |

## Candidate 500 ms change bins

A candidate is a 500 ms bin whose mean differs from the previous bin by at least `0.010 D`. This is a deterministic activity detector, not a semantic phase classifier.

| Bin start | Mean density | Delta from previous bin |
|---:|---:|---:|
| 2.0 s | 0.663393 D | +0.039760 D |
| 2.5 s | 0.641841 D | -0.021552 D |
| 3.0 s | 0.622100 D | -0.019741 D |
| 18.5 s | 0.729571 D | +0.097641 D |
| 19.0 s | 0.744929 D | +0.015358 D |
| 19.5 s | 0.640915 D | -0.104014 D |
| 20.5 s | 0.674002 D | +0.030278 D |
| 21.0 s | 0.690944 D | +0.016942 D |
| 21.5 s | 0.673870 D | -0.017074 D |
| 22.0 s | 0.643490 D | -0.030380 D |
| 22.5 s | 0.631390 D | -0.012099 D |
| 23.0 s | 0.620296 D | -0.011095 D |
| 24.0 s | 0.643454 D | +0.024095 D |
| 24.5 s | 0.688299 D | +0.044845 D |
| 25.0 s | 0.639646 D | -0.048654 D |
| 25.5 s | 0.667330 D | +0.027684 D |
| 26.0 s | 0.795301 D | +0.127971 D |
| 26.5 s | 0.810611 D | +0.015310 D |
| 27.5 s | 0.794675 D | -0.015265 D |
| 28.5 s | 0.692333 D | -0.098732 D |
| 29.0 s | 0.631809 D | -0.060524 D |
| 44.0 s | 0.657609 D | +0.027905 D |

## Files

- `density-timeline.csv` — every raw sample and both host-relative time axes.
- `density-analysis.json` — complete statistics, markers, bins, and candidates.
- `density-timeline.svg` — dependency-free visualization; orange lines mark candidate bins.
