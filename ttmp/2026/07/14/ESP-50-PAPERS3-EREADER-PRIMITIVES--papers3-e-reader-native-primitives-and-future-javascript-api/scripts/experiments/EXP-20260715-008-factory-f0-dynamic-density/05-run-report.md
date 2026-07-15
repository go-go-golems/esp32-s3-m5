---
Title: "Run Report - EXP-20260715-008-factory-f0-dynamic-density"
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
Summary: "Exact FactoryTest F0 replay produced a complete 45-second valid fixed-point density trace and expected operator-visible sequence."
LastUpdated: 2026-07-15T01:10:00Z
WhatFor: "Record automatic and temporal-density F0 results before deciding whether F1 may be run."
WhenToUse: "Review exact-vendor F0 objective evidence and its limits before source-derived F1/F2 comparison."
---

# Exact F0 dynamic density run report

## Disposition

**Automatic transaction: PASS. Temporal fixed-point density: PASS. Spatial/optical disposition: still separate.**

The guarded orchestrator verified the exact FactoryTest V0.5 merged image, started the Printalyzer stream, retained a two-second pre-flash baseline, flashed F0, and captured 442 raw samples over 44.773896 seconds. The collector ended normally with zero saturation and zero invalid host density estimates. The Printalyzer acknowledged the required `ID S,STOP → SD LR,0 → IS REMOTE,0` cleanup sequence.

The operator answered the post-run numbered checklist as `yes`, `yes`, `yes`, `no`: the head remained fixed, the LED was off afterward, the expected title → black → white → grayscale → dashboard sequence was observed, and no heat/odor/unusual sound/reset-loop/power problem was observed.

## Objective evidence

```text
firmware: exact FactoryTest V0.5 merged release
firmware SHA-256: d6733a0ca378f95335fa5fba4d4d992fb1dd97c17557b20e9aebfca08ba6d624
samples: 442
capture duration: 44.773896 s
coverage after flash-runner completion: 29.353123 s
saturated / invalid: 0 / 0
density minimum / maximum: 0.608100 / 0.835291 D
full density span: 0.227191 D
capture result: ok
```

The deterministic analyzer found 20 half-second activity candidates using a threshold of 0.010 D change in consecutive bin means. This is evidence that the point under the aperture changed during F0, including later dashboard activity. It is not a semantic classifier by itself.

## What can and cannot be correlated

The density samples, host raw-stream marker, host flash-begin marker, and host flash-runner-complete marker share one host monotonic timeline. Physical density timing is bounded mainly by the 100 ms sensor integration period plus USB delivery latency.

F0 has no runtime ring, and PaperS3 serial was deliberately not opened during the exact-binary run. Therefore no internal device event can be assigned to an individual density transition with stronger precision than source-order plus host flash/reset timing. The dashboard intentionally renders candidate activity but does not label every spike “black,” “white,” or “grayscale.”

The original locked-camera F0 recording remains the spatial evidence. This density trace is a fixed single-aperture temporal record: it cannot detect edge residue, gradients, or a different region of the panel.

## Reproducible artifacts

| File | Purpose |
|---|---|
| `raw-dynamic-f0.jsonl` | Immutable raw Printalyzer samples, calibration snapshot, commands, and cleanup events |
| `host-events.jsonl` | Host flash/stream timing anchors |
| `flash-full.log` | Full exact-image flashing transcript |
| `density-timeline.csv` | Flat per-sample table with time axes and derived density |
| `density-analysis.json` | Deterministic bins, markers, candidate detector, and statistics |
| `density-timeline.svg` | Dependency-free static graph |
| `dashboard.html` | Self-contained interactive HTML/JS dashboard |
| `evidence.sha256` | Hash manifest for all evidence above |

Run from the ticket root:

```text
scripts/32-analyze-printalyzer-static-captures.py \
  --output-json scripts/output/32-printalyzer-static-calculations.json \
  --output-markdown scripts/output/32-printalyzer-static-calculations.md
scripts/33-analyze-f0-dynamic-density.py
scripts/34-build-f0-density-dashboard.py
```

## Next decision

F1 trace-off is eligible only as a **new, separately preregistered fixed-head and fixed-camera source-proxy comparison**. It must not be treated as a replacement for F0. Camera/video remains required to compare spatial behavior; the density trace compares only the unchanged aperture point.
