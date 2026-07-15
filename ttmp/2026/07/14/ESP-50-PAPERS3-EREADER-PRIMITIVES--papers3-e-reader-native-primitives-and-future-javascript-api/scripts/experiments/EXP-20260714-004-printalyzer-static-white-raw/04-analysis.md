---
Title: "Analysis - EXP-20260714-004-printalyzer-static-white-raw"
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
Summary: "Static middle-ish white-region raw Printalyzer capture passed stability, saturation, validity, and cleanup gates."
LastUpdated: 2026-07-15T00:22:13Z
WhatFor: "Qualify static Printalyzer raw sampling on PaperS3 without driving the panel."
WhenToUse: "Review before interpreting dynamic Printalyzer traces or derived density."
---

# Static white-region raw analysis

## Result

**Automatic data gate: PASS. Absolute optical interpretation: NOT YET QUALIFIED.**

The Printalyzer sampled an operator-identified white region approximately middle-ish on the PaperS3. The PaperS3 remained in ROM download mode and did not update. The sensor produced 48 samples over five seconds. Per preregistration, the first two were excluded; all 46 post-settling samples were valid and unsaturated.

## Fixed settings

```text
Printalyzer: v1.1.0 / g7101373
mode: continuous remote diagnostic reflection
TSL2591 gain: 2 (HIGH)
integration: 0 (100 ms)
reflection LED duty: 128 (matches GC LIGHT calibration)
duration: 5 seconds
host formula: installed v1.1.0 single-sample density reproduction
```

## Post-settling statistics

| Metric | n | Mean | Standard deviation | Minimum | Maximum | Range |
|---|---:|---:|---:|---:|---:|---:|
| Channel 0 raw count | 46 | 1929.304 | 2.788 | 1922 | 1932 | 10 |
| Channel 1 raw count | 46 | 199.761 | 0.431 | 199 | 200 | 1 |
| Host density estimate | 46 | 0.678142 D | 0.000625 D | 0.677538 D | 0.679781 D | 0.002243 D |

Additional gates:

```text
saturated samples: 0
invalid density estimates: 0
post-settling samples: 46 (required >=20)
density range: 0.002243 D (required <=0.05 D)
cleanup: ID S,STOP -> SD LR,0 -> IS REMOTE,0
capture result: ok
```

Capture SHA-256:

```text
8ef75e37a070d7fa319afda30b942c7a4ce7e3334d18d627a4142bf79f70cfac
```

## Interpretation

The low variance proves that the fixed geometry can produce a very stable point signal and that gain 2 / 100 ms / duty 128 has substantial headroom. This setting is suitable for measuring temporal transitions at approximately the sensor integration cadence.

The estimated `0.678 D` must not yet be called the panel's calibrated white density. The host formula reproduces installed source behavior, but this diagnostic path differs from a normal measurement: it uses one 100 ms sample at fixed gain instead of auto-gain, 200 ms integration, and a two-reading average. More importantly, the PaperS3 cover glass and current head/target plane have not been validated against a normal measurement at the same position.

At face value, `0.678 D` corresponds to a reflectance factor of roughly `10^-0.678 = 0.21` relative to the densitometer calibration basis. That number is retained as a hypothesis-generating estimate only, not an absolute panel specification or F0 white disposition.

## Next gate

Before dynamic F0/F1/F2 density interpretation:

1. confirm the operator observed no movement and that the reflection LED turned off after capture;
2. repeat after deliberate lift/reposition to quantify placement sensitivity;
3. compare diagnostic host estimate with a normal local `SM FORMAT,EXT` measurement at the same target plane if mechanically possible;
4. characterize CAL-LO and CAL-HI using the same raw settings to verify host estimates against known targets;
5. preserve video for spatial gradients even if point density becomes the primary temporal endpoint.
