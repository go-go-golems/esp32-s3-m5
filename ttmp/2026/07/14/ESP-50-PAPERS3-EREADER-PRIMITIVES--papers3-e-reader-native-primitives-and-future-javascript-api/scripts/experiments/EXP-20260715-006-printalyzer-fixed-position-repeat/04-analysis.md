---
Title: "Analysis - EXP-20260715-006-printalyzer-fixed-position-repeat"
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
Summary: "Immediate repeat contained a recoverable 0.0856 D transient and failed fixed-position stability thresholds."
LastUpdated: 2026-07-15T00:38:01Z
WhatFor: "Separate fixed-position repeatability from manual repositioning sensitivity."
WhenToUse: "Interpret EXP-004/005 placement differences and future fixed-position traces."
---

# Fixed-position repeat analysis

## Result

**FAIL under the preregistered whole-run rule.** Forty-six post-settling samples were valid and unsaturated, but the density range was `0.085603 D` rather than at most `0.01 D`; mean delta from EXP-005 was `0.009820 D` rather than at most `0.005 D`.

The trace was structured rather than noisy. Samples 2–15 were stable near `0.601123 D`; samples 16–25 formed a temporary excursion reaching `0.686352 D`; samples 26–47 recovered toward `0.605468 D`. Both sensor channels fell during the excursion. No PaperS3 software operation was issued, but later operator context noted that the visible app background may itself change, so the cause cannot be assigned uniquely to geometry, ambient intrusion, or target change.

```text
post-settling n: 46
mean: 0.609895 D
standard deviation: 0.015850 D
range: 0.085603 D
saturated / invalid: 0 / 0
capture SHA-256: 7b3fb713df1eee296da79b9bfacee32ffc1596d25b8e1aab63454e8e20a81d43
cleanup: complete
```
