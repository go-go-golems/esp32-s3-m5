---
Title: Printalyzer Static Capture Calculations
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
Summary: "Deterministic statistics and pairwise deltas for static Printalyzer raw captures."
LastUpdated: 2026-07-15T01:15:00Z
WhatFor: "Reproduce placement and repeatability calculations from immutable raw JSONL captures."
WhenToUse: "Review static point-density stability, placement sensitivity, or capture validity."
---

# Printalyzer static capture calculations

Settling samples excluded from every capture: **2**.

| Capture | post n | CH0 mean | Density mean | Density SD | Density range | Saturated | Invalid |
|---|---:|---:|---:|---:|---:|---:|---:|
| EXP-004-initial | 46 | 1929.304348 | 0.678141960 D | 0.000624952 D | 0.002242800 D | 0 | 0 |
| EXP-005-repositioned | 46 | 2311.608696 | 0.600075053 D | 0.000100153 D | 0.000373236 D | 0 | 0 |
| EXP-006-fixed-repeat | 46 | 2261.043478 | 0.609895364 D | 0.015850000 D | 0.085603095 D | 0 | 0 |
| EXP-007-hand-wave | 96 | 2279.395833 | 0.606275250 D | 0.011423911 D | 0.078396275 D | 0 | 0 |

## Adjacent mean deltas

| Earlier | Later | Signed delta | Absolute delta |
|---|---|---:|---:|
| EXP-004-initial | EXP-005-repositioned | -0.078066907 D | 0.078066907 D |
| EXP-005-repositioned | EXP-006-fixed-repeat | 0.009820311 D | 0.009820311 D |
| EXP-006-fixed-repeat | EXP-007-hand-wave | -0.003620114 D | 0.003620114 D |

## Cleanup

- **EXP-004-initial:** result=ok; commands=`ID S,STOP -> SD LR,0 -> IS REMOTE,0`
- **EXP-005-repositioned:** result=ok; commands=`ID S,STOP -> SD LR,0 -> IS REMOTE,0`
- **EXP-006-fixed-repeat:** result=ok; commands=`ID S,STOP -> SD LR,0 -> IS REMOTE,0`
- **EXP-007-hand-wave:** result=ok; commands=`ID S,STOP -> SD LR,0 -> IS REMOTE,0`
