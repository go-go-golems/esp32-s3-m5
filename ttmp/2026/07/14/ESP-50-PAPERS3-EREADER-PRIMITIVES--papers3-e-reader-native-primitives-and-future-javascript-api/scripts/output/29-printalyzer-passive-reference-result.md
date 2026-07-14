---
Title: Printalyzer Passive Reference Measurement Result
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
Summary: "Passive timestamp capture of repeated Printalyzer CAL-LO and CAL-HI reflection reference measurements."
LastUpdated: 2026-07-14T23:56:41Z
WhatFor: "Validate normal calibrated Printalyzer event capture and display-level repeatability before panel measurements."
WhenToUse: "Review before using Printalyzer readings as an optical endpoint or enabling diagnostic raw streaming."
---

# Printalyzer passive reference measurement result

## Disposition

**PASS at the normal display resolution of 0.01 D.** Three operator-identified valid CAL-LO measurements were all `R+0.05D`; three valid CAL-HI measurements were all `R+1.49D`. The parenthesized operator report identified the CAL-HI reference as `1.49`. No serial input was sent, no remote mode was entered, and no diagnostic or calibration state was changed.

## Evidence identity

```text
capture: 29-printalyzer-passive-calibration-20260714T235306Z.jsonl
SHA-256: 5e8235e8f9ed179806d914c75b4857b65d895ad7f15a17158190f6423560b8f7
size: 6834 bytes
Printalyzer: v1.1.0 / g7101373 / UID 323147103439323344002900
capture_end: result=ok, interrupted=true, serial_input_sent=false
```

## Chronology

| Classification | UTC | Reading |
|---|---:|---:|
| operator-invalid | 23:54:46.288785725 | `T+2.19D` |
| operator-invalid | 23:54:49.796613223 | `T+2.19D` |
| operator-invalid | 23:54:52.356388044 | `T+2.19D` |
| operator-invalid | 23:55:05.193382146 | `R+1.98D` |
| operator-invalid | 23:55:07.248911331 | `R+1.98D` |
| operator-invalid | 23:55:09.304557165 | `R+1.98D` |
| valid CAL-LO | 23:55:13.718859708 | `R+0.05D` |
| valid CAL-LO | 23:55:15.624138583 | `R+0.05D` |
| valid CAL-LO | 23:55:17.328580350 | `R+0.05D` |
| valid CAL-HI | 23:56:15.143512835 | `R+1.49D` |
| valid CAL-HI | 23:56:18.954711305 | `R+1.49D` |
| valid CAL-HI | 23:56:22.765665995 | `R+1.49D` |

The first six records remain immutable evidence but are excluded from reference statistics because the operator immediately classified them as procedural mistakes before seeing this report.

## Repeatability

At two-decimal BASIC output precision:

| Target | n | Mean | Minimum | Maximum | Display-level spread |
|---|---:|---:|---:|---:|---:|
| CAL-LO | 3 | 0.05 D | 0.05 D | 0.05 D | 0.00 D |
| CAL-HI | 3 | 1.49 D | 1.49 D | 1.49 D | 0.00 D |

This proves event delivery and repeatability only to 0.01 D. It does not establish sub-centidensity repeatability because BASIC output rounds the value. A later normal-measurement run may explicitly request transient `SM FORMAT,EXT` output to preserve the full density, zero, raw-count, and corrected-count floats without entering remote mode.

## Timing interpretation

The logger timestamps complete CDC lines on receipt. It did not record Action-button press times, so these samples do not measure button-to-result latency. The spacing between valid readings reflects operator handling plus instrument measurement time and must not be interpreted as sensor integration duration.

## Safety and panel state

- PaperS3 serial was not opened.
- PaperS3 remains in ROM download mode from the separately documented observer-control failure.
- No PaperS3 panel operation occurred.
- The Printalyzer was used only through its normal local calibrated-measurement workflow.
- Actual diagnostic raw streaming remains unexecuted on the physical instrument.
