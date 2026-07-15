---
Title: "Analysis - EXP-20260715-005-printalyzer-repositioned-white-raw"
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
Summary: "Repositioned white-region signal remained stable but failed the preregistered between-placement density threshold."
LastUpdated: 2026-07-15T00:30:00Z
WhatFor: "Measure lift-and-reseat sensitivity of static Printalyzer PaperS3 sampling."
WhenToUse: "Review before treating point-density changes as panel transitions."
---

# Repositioned white-region analysis

## Result

**Within-run automatic gate: PASS. Placement-sensitivity gate: FAIL.**

After deliberate lift and reseating, the point signal was even more stable than experiment 004, with zero saturation and complete cleanup. However, the mean host density estimate changed from `0.678142 D` to `0.600075 D`, an absolute difference of `0.078067 D`. This exceeds the preregistered `0.03 D` maximum.

## Comparison

| Metric | EXP-004 initial placement | EXP-005 repositioned | Difference |
|---|---:|---:|---:|
| Post-settling samples | 46 | 46 | 0 |
| Channel 0 mean | 1929.304 | 2311.609 | +382.304 |
| Density mean | 0.678142 D | 0.600075 D | −0.078067 D |
| Density standard deviation | 0.000625 D | 0.000100 D | — |
| Density range | 0.002243 D | 0.000373 D | — |
| Saturated / invalid | 0 / 0 | 0 / 0 | — |

EXP-005 capture SHA-256:

```text
a40a4367ad712b9dd075982fe81ab3fc89a812c0aa63aa8ec9b00615e63f9de1
```

Cleanup was acknowledged in the required order:

```text
ID S,STOP
SD LR,0
IS REMOTE,0
```

## Interpretation

The sensor itself is quiet at either fixed placement. The much larger between-placement shift is therefore not electronic noise. It may combine:

- slightly different aperture coordinates over a nonuniform/ghosted “white” field;
- head angle, pressure, or target-plane changes;
- cover-glass reflections;
- local dashboard residue beneath the visually white area.

The experiment cannot separate those causes because exact coordinates, pressure, and angle were not instrumented. Consequently:

1. absolute comparisons across independently positioned runs are not trustworthy at a `0.03 D` scale;
2. a dynamic black→white trace can still be useful if the head remains fixed throughout all compared transitions;
3. F0/F1/F2 cross-run comparisons require a mechanical positioning fixture or a reseating uncertainty term at least as large as observed here;
4. this does not invalidate within-run transition timing or relative density change.
