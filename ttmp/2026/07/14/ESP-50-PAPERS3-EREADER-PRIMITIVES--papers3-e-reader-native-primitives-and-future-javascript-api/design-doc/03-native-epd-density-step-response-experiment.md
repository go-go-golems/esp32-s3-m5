---
Title: Native EPD Density Step-Response Experiment
Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES
Status: active
Topics:
    - papers3
    - eink
    - hardware-qualification
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://0107-papers3-epd-painter-control/main/app_main.cpp
      Note: Reviewed direct-driver baseline
    - Path: repo://0110-papers3-epd-density-step-response/README.md
      Note: Build/safety contract
    - Path: repo://0110-papers3-epd-density-step-response/main/app_main.cpp
      Note: Automatic semantic marker and direct-driver sequence
ExternalSources: []
Summary: Design for a bounded direct-driver white-black-white experiment that joins semantic firmware markers to fixed-aperture Printalyzer density readings.
LastUpdated: 2026-07-15T02:45:00Z
WhatFor: Replace opaque FactoryTest behavior with a minimal reproducible native EPD step-response experiment.
WhenToUse: Use before building, flashing, or interpreting the P0.19 direct-driver density run.
---


# Native EPD Density Step-Response Experiment

## Executive Summary

FactoryTest established that the panel has a repeatable poor optical endpoint, while F0/F1 established that a fixed Printalyzer aperture can measure temporal density changes and F2 established safe reset-time USB observation. None of those runs directly states which *semantic firmware action* caused a measured density response.

P0.19 introduces a deliberately narrow, independent `EPD_Painter` firmware. After a ten-second boot grace period it performs exactly three HIGH-quality, two-stage operations: a HARD white cleanup, full black, and full white. Each operation emits a machine-readable marker before work, after the direct-driver `waitIdle()` result, and after a fixed settled dwell. A host collector records those markers through the PaperS3 read-only reconnecting descriptor and Printalyzer raw readings on one host clock.

The experiment makes a fixed-aperture temporal claim only: whether density changes in the expected direction and with what timing relative to three explicit operations. It does not certify whole-panel uniformity, ghosting, edges, VCOM, rails, absolute density across repositioning, or waveform correctness.

## Problem Statement

The prior FactoryTest sequence interleaves vendor initialization, app behavior, M5GFX scheduling, and unknown panel history. Its visual phases are not a dependable semantic clock. The F2 ring further showed that partial instrumentation can prove boot and frames without proving full power-off/idle coverage. A new test must make the requested image, operation boundaries, target bytes, and settled observation window explicit without using an unsafe PaperS3 console writer.

## Proposed Solution

Create project `0110-papers3-epd-density-step-response` from the pinned `0107` direct-driver control. The local EPD_Painter source, preset, HIGH tables, pure-IDF hardening, ESP-IDF 5.4.2, PSRAM requirement, and USB Serial/JTAG output configuration are copied unchanged. The application removes command-driven treatment selection in favor of a boot-time, fixed script:

```text
boot + 10 s capture grace
marker phase=cleanup before
HARD clear to white; waitIdle
marker phase=cleanup idle
wait 4 s
marker phase=black before
paint packed full-black target; waitIdle
marker phase=black idle
wait 4 s
marker phase=white before
paint packed full-white target; waitIdle
marker phase=white idle
wait 8 s
marker experiment_end
```

Each marker includes: schema version, monotonic `esp_timer` microseconds, semantic phase, operation index, target SHA-256, direct-driver pending-stage count, rail-active state, result, and free/minimum heap. `printf` occurs only before invoking an operation or after `waitIdle()` has returned; no logging occurs in the EPD worker or scan loop.

The experiment uses the established fixed Printalyzer aperture and 100 ms, HIGH-gain, 128-duty raw reflection stream. The host uses read-only reconnecting PaperS3 observation and never sends serial input. A physical Reset is used after capture is armed, because it preserves the single-owner policy and captures the boot-time automatic sequence.

## Decision Records

### DR-1: Use the independent EPD_Painter control, not M5GFX/F2

**Status:** accepted.

**Context:** F2 demonstrated that FactoryTest boots and exposes frame activity, but its trace is incomplete and FactoryTest behavior is not a controllable product contract.

**Decision:** use the existing pinned direct-driver control as the causal treatment engine.

**Consequences:** results compare driver families rather than reproduce vendor binary behavior. The copied waveform family remains upstream HIGH and every binary/source identity is recorded.

### DR-2: One white-black-white cycle

**Status:** accepted.

**Decision:** use cleanup → black → white once, with four-second interphase dwells and an eight-second terminal dwell.

**Rationale:** it measures both darkening and recovery while bounding panel stress. Repeated cycles, checkerboards, area matrices, and grayscale are deferred.

### DR-3: Firmware markers plus density; no camera required

**Status:** accepted.

**Decision:** the first run is density-only and claims a fixed location only.

**Consequences:** a camera is still required for spatial gradients, edge behavior, and full-panel optical disposition.

## Acceptance Criteria

Automatic success requires all of:

1. exact build identity and source manifest pass;
2. capture begins before physical Reset;
3. three operation begin/idle/settled marker groups arrive in order;
4. every `waitIdle()` result is successful and rails report idle after each operation;
5. raw-density stream completes cleanup with zero saturation and invalid estimates;
6. no reset loop, fault, heat, odor, sound, or power anomaly is reported.

Analysis reports onset and extrema relative to each firmware marker, but does not label density features as a panel phase without the marker relationship.

## Implementation Plan

1. Copy the reviewed direct-driver project to `0110`, preserving component contents.
2. Replace its command-driven main with the fixed automatic sequence and post-idle marker contract.
3. Build under exact ESP-IDF 5.4.2; record SHA-256 and source audit.
4. Preregister one immutable run directory and guarded flash/capture runner.
5. Arm combined density/read-only-reconnect capture, ask the operator for one Reset, then finalize and analyze evidence.
6. Update disposition, diary, tasks, changelog, and file relations.

## Risks and Limits

- The Printalyzer LED is an intentional optical probe and may not establish absolute density after repositioning.
- `waitIdle()` is a direct-driver software observation, not an analog rail measurement.
- The white-black-white cycle itself may retain ghosting; the result describes it rather than correcting it.
- Any timeout leaves the app in a terminal fault state and forbids automatic cleanup.

## References

- `0107-papers3-epd-painter-control/main/app_main.cpp` — vetted direct-driver operation/idle policy.
- `0107-papers3-epd-painter-control/README.md` — pin, waveform, toolchain, and safety baseline.
- `scripts/29-capture-synchronized-serial.py` — existing Printalyzer raw-density derivation and safe PaperS3 descriptor pattern.
- `scripts/45-capture-papers3-readonly-reconnect.py` — reset-surviving non-controlling PaperS3 observer.
- `scripts/experiments/EXP-20260715-015-factory-f2-readonly-reconnect-ring-retry/04-analysis.md` — F2 boot proof and trace-coverage limit.
