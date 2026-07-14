---
Title: M5GFX Runtime Waveform Instrumentation and Scientific Experiment Ledger
Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES
Status: active
Topics:
    - papers3
    - eink
    - esp-idf
    - hardware-qualification
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://0106-papers3-epd-qualification/.component-matrix/current/M5GFX/src/lgfx/v1/platforms/esp32/Bus_EPD.cpp
      Note: Physical scan and power GPIO ordering to trace
    - Path: repo://0106-papers3-epd-qualification/.component-matrix/current/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp
      Note: Runtime LUT state machine and frame loop to instrument
    - Path: repo://0108-papers3-m5gfx-runtime-trace/main/epd_trace_runtime.cpp
      Note: Fixed-ring publication and idle-time JSONL dump implementation
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/17-decode-m5gfx-epd-waveforms.py
      Note: Deterministic non-invasive waveform decoder
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/20-m5gfx-runtime-trace-audit-latest.md
      Note: 18-check observer-effect audit and machine-code identity evidence
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/patches/18-m5gfx-runtime-trace-hooks.patch
      Note: Compile-time-gated M5GFX queue power and frame hooks
ExternalSources: []
Summary: Design for minimally perturbing M5GFX runtime traces, external physical validation, and immutable PaperS3 experiment records.
LastUpdated: 2026-07-14T21:25:00Z
WhatFor: Turn visual EPD trials into source-backed, timing-aware, reproducible experiments.
WhenToUse: Use before modifying M5GFX, replaying FactoryTest, attaching measurement equipment, or comparing waveform endpoints.
---





# M5GFX runtime waveform instrumentation and scientific experiment ledger

## Executive summary

Runtime logging is essential. Static LUT decoding tells us what M5GFX can command, but not what it actually scheduled for a particular origin, target, queue history, or display mode. It also does not reveal frame duration, rail-enable duration, queue coalescing, task stalls, or whether an update passed through the eraser LUT before the target LUT.

The runtime trace must be designed as measurement instrumentation rather than debug printing. It must never print from the scan loop. The driver should append small fixed-size records to a preallocated memory ring, and the application should dump those records only after `waitDisplay()` and rail shutdown. External logic/analog capture remains the authority for the unmodified factory binary and for quantifying instrumentation observer effects.

## What “stock” means

There are three related but distinct controls:

1. **Official merged FactoryTest V0.5 binary** — exact vendor artifact; only external observation can instrument it without changing it.
2. **Source-equivalent M5GFX 0.2.15 build** — permits runtime hooks, but rebuilding means it is no longer the exact released binary. Exact ESP-IDF 5.3.3 is currently unavailable locally.
3. **Current M5GFX 0.2.25 qualification build** — buildable under IDF 5.3.4/5.4.2; its five Panel_EPD LUTs are byte-identical to 0.2.15, but platform and application context differ.

These controls must never be collapsed into one “stock firmware” label.

## Evidence layers

### Layer 1: static source decoding

A deterministic decoder should preserve:

- every `lut_eraser`, `lut_quality`, `lut_text`, `lut_fast`, and `lut_fastest` row;
- all sixteen target-tone schedules;
- code counts and ordering before the first terminator;
- source and canonical LUT hashes;
- PaperS3 bus clock, line padding, pin map, and power ordering.

This layer is non-invasive and can compare 0.2.15 with 0.2.25 exactly. It cannot prove runtime selection or physical voltage polarity.

### Layer 2: minimally perturbing software timing trace

Patch M5GFX behind `LGFX_ENABLE_EPD_TRACE`. When disabled, trace code must compile out. When enabled, driver sites call a fixed nonblocking hook implemented by the qualification application.

Required events:

```text
APP_OPERATION_BEGIN       operation id, fixture hash, requested mode/origin/target
DISPLAY_ENQUEUE           mode, rectangle, queue result
UPDATE_DEQUEUE            mode, rectangle, queue depth
ERASER_ARMED              count of pixels moved to eraser state
TARGET_ARMED              count of pixels assigned target LUT
POWER_ON_BEGIN/END        timestamps and commanded GPIO state
FRAME_BEGIN/END           frame ordinal, remain flag, duration
POWER_OFF_BEGIN/END       timestamps and commanded GPIO state
DISPLAY_IDLE              total frames and operation duration
APP_OPERATION_END         heap, result, trace overflow count
```

The trace hook records only integers into a preallocated ring. It must not allocate, lock a contended mutex, touch NVS/filesystems, or write USB serial in the display worker. A monotonic sequence number and `esp_timer_get_time()` timestamp make dropped or reordered events detectable.

### Layer 3: optional drive-content trace

A second, explicitly perturbing build may count generated two-bit drive codes per frame. It records `neutral`, code-1, code-2, and code-3 pixel totals after `blit_dmabuf()` creates each DMA row.

This mode is valuable for mixed grayscale and partial updates, but it adds work in the hot path and may stretch row/frame timing. It cannot serve as the primary timing baseline. For uniform black/white fixtures, static LUT schedules plus Layer-2 frame events should provide the same semantic answer with less perturbation.

### Layer 4: external physical capture

External capture is the only way to measure the exact official FactoryTest binary without rebuilding it.

Digital capture candidates:

- `EP_PWR` GPIO46;
- `OE` GPIO45;
- `SPV` GPIO17;
- `CKV` GPIO18;
- `LE` GPIO15;
- `SPH` GPIO13;
- `CL/CKH` GPIO16;
- selected data pins, if channel count and safe attachment permit.

Analog capture candidates:

- VPOS;
- VNEG;
- VGH;
- VGL/VEE;
- VCOM;
- supply current.

No analog probe should be attached until probe ground, voltage range, loading, exposed nodes, and power-off behavior have been reviewed. Digital logic thresholds and analyzer ground must also be checked first.

## Observer-effect controls

Instrumentation modifies the program it measures. The acceptance sequence is:

1. build trace-disabled and trace-timing variants from the same source/configuration;
2. prove LUT, pin, clock, padding, and power source are identical;
3. compare application size and IRAM placement;
4. run the same fixture once per variant under fixed history;
5. compare external frame/rail timing if a logic analyzer is available;
6. reject timing claims if trace-on materially changes frame duration or optical output.

`printf`, ESP logging, JSON formatting, hashing, and serial transmission are forbidden while rails are active. Trace dumping occurs after display idle and power-off.

## Experiment ledger

Each physical run receives an immutable directory:

```text
scripts/experiments/EXP-YYYYMMDD-NNN-short-name/
  manifest.json
  preregistration.md
  software-events.jsonl
  physical-capture/        # optional analyzer/scope files
  optical/                 # photos/video plus camera metadata
  operator-observation.md
  hashes.sha256
  disposition.md
```

`manifest.json` records:

```text
schema, experiment_id, hypothesis, decision_rule
board identity, panel identity if available
firmware app/ELF hashes, project version, source commits, patch hashes
ESP-IDF version, sdkconfig hash, FreeRTOS tick rate
fixture binary/hash, origin procedure, target, dwell
M5GFX mode, LUT hashes, trace mode
ambient/panel temperature and measurement method
serial port, flash log, software trace hash
physical capture equipment/settings/hashes
camera/exposure/white-balance/illumination/reference patches
operator observation verbatim
automatic disposition, optical disposition, stop reason
```

The hypothesis and decision rule are written before flashing. Existing run directories are append-only except for completing fields explicitly marked pending. Corrections create a signed amendment or a new experiment; they do not silently rewrite observations.

## First comparison protocol

Do not begin with every mode. The smallest useful comparison is:

1. static-decode and hash M5GFX 0.2.15/0.2.25;
2. implement trace-timing hooks and a trace dump command without flashing;
3. audit/build trace-off and trace-on variants;
4. restore FactoryTest V0.5 and video its black→white boot transition as the exact-binary optical baseline;
5. if safe equipment is available, externally capture the same factory sequence;
6. run one source-instrumented M5GFX QUALITY black→white sequence using the same dwell and optical setup;
7. compare frame count, rail-on duration, endpoint residue, and any externally observed timing;
8. only then decide whether another EPD_Painter operation has discriminating value.

## Expected scientific payoff

This design lets us answer questions that visual trials alone cannot:

- Did full white use eraser plus QUALITY, or only a target LUT?
- How many physical frames were emitted, and for how long were rails enabled?
- Was a queued update merged into an active transition?
- Did the runtime schedule match the statically decoded LUT?
- Does trace instrumentation change timing?
- Does official stock reach cleaner white with a known black origin?
- Are poor endpoints correlated with software schedule, analog rails, temperature, area, or history?

Until these records exist, “factory blank,” “quality white,” and “hard clear” are useful descriptions but not interchangeable experimental treatments.

## Implementation result: trace-off identity and bounded timing instrumentation

The design is now implemented in numbered project `0108-papers3-m5gfx-runtime-trace`. A reproducible preparation script copies the clean pinned M5GFX/M5Unified checkouts and applies patch `18-m5gfx-runtime-trace-hooks.patch`. The patch changes only `Panel_EPD`, `Bus_EPD`, and a compile-time trace header; the generated audit independently re-parses the patched source and confirms canonical LUT SHA-256 `d24b2df...` is unchanged.

The application supplies a 512-record fixed ring. Each 48-byte record contains a release-published commit marker, monotonic sequence number, microsecond timestamp, current application operation, event ID, and five integer arguments. Writers reserve records with an atomic sequence increment and never allocate, format text, access storage, or print. The operator may dump JSON Lines only through `epd trace dump`, which takes the display mutex and calls `waitDisplay()` first.

Driver events cover:

- display enqueue and update dequeue;
- target and eraser two-pixel-unit counts during update preparation;
- power-on and power-off begin/end;
- frame begin/end and continuation state;
- display idle.

Application events bracket every controlled draw transaction. There is deliberately no per-row hook and no drive-code histogram. Static LUT schedules can be joined to frame records for uniform fixtures without moving work into the scan loop.

Two clean ESP-IDF 5.4.2 variants built with zero warnings:

```text
trace off application:    546064 bytes
trace timing application: 547648 bytes
delta:                       1584 bytes
trace ring BSS:             24576 bytes
```

The most important observer-effect result is stronger than equal symbol sizes. The audit extracted the relocatable `.text` sections for `Panel_EPD::task_update` and `Bus_EPD::powerControl` from the trace-off build and the earlier clean Cell D build. Both pairs are byte-for-byte identical:

```text
Panel_EPD::task_update:
634d10897d6fc00f0f31c5fdeeb9468ac131a6113ae42244a397d8707b6277b5

Bus_EPD::powerControl:
0688a43e27ad3af9b410419477f0eda234f4e464f5007fc3ccc77a0833e884d4
```

This required using a compile-time macro that removes both hook calls and argument evaluation. An earlier empty inline function still evaluated `uxQueueMessagesWaiting()` in the off build and changed generated code. The final macro also compiles counters and frame ordinals out. The off ELF contains neither the trace hook nor ring.

The timing variant intentionally changes the two critical functions:

```text
Panel_EPD::task_update: 815 -> 1024 bytes (+209)
Bus_EPD::powerControl:  292 -> 388 bytes (+96)
linked hook call sites: 10
```

No call site occurs inside the 540-row loop. The application grows by 1,584 bytes, while the ring adds 24,576 BSS bytes. These are compile-time bounds, not a runtime duration measurement. A later physical trace-off/trace-on comparison is still required before interpreting microsecond differences as unperturbed panel timing.

Final identities:

```text
trace patch SHA-256: 6c21e45e0909accd2b5df5ae3178534192b10a51ec4a319ebc2309bfe983d89f
trace-off app SHA-256: 609aba851db118ee26a3051d4f78ae96255229493f9783f60f43334355925e68
trace-on app SHA-256: a081daabe5a77d7405cde68e43955279ed5e5c0f954c2aee027b62d03fd9f6ea
observer audit: 18 / 18 PASS
hardware modified: no
```
