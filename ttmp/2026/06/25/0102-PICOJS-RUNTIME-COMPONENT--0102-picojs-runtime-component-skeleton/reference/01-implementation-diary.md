---
Title: Implementation Diary
Ticket: 0102-PICOJS-RUNTIME-COMPONENT
Status: active
Topics:
    - esp32-p4
    - quickjs
    - picocalc
    - visual-repl
    - javascript
    - firmware
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Diary for ESP-IDF component boundary for the firmware-native PicoJS runtime."
LastUpdated: 2026-06-25T15:30:00-07:00
WhatFor: "Chronological implementation notes for 0102 PicoJS Runtime Component Skeleton."
WhenToUse: "Read before resuming this phase, before committing, and before hardware probe work."
---

# Diary

## Goal

This diary records the implementation of `0102-PICOJS-RUNTIME-COMPONENT`. The phase goal is to create a maintainable component before porting large native QuickJS bindings.

## Step 1: Create child ticket and phase guide

This step split the broad PicoJS device integration effort into a focused child ticket. The design guide defines the phase scope, console contract, validation strategy, and review risks so implementation can proceed in small commits.

The child ticket links back to the umbrella `0102-PICOJS-DEVICE-INTEGRATION` plan. Its work should remain narrow; later-phase features belong in their own child tickets unless the umbrella design is updated.

### Prompt Context

**User prompt (verbatim):** "alright, do it, and for each ticket write a design + implementation guide as usual (and upload to remarkable), then stsart working on tasks one by one, committing at appropriate intervals, and keeping a detailed diary."

**Assistant interpretation:** Create the phase tickets and their implementation guides, upload the documentation package to reMarkable, then begin work phase-by-phase with commits and diary updates.

**Inferred user intent:** Keep a complex firmware integration manageable, reviewable, and recoverable by splitting it into documented tickets with concrete validation gates.

**Commit (code):** N/A — ticket/documentation setup only.

### What I did

- Created child ticket `0102-PICOJS-RUNTIME-COMPONENT`.
- Added this implementation diary.
- Wrote `design-doc/01-design-and-implementation-guide.md`.
- Defined initial scope, non-goals, console contract, validation strategy, and risks.

### Why

- The umbrella task is too broad to implement safely as one monolithic ticket.
- This phase has its own validation target and can be reviewed independently.

### What worked

- The ticket workspace and document structure were created through `docmgr`.
- The phase guide now provides an implementation boundary and a stopping condition.

### What didn't work

- N/A for this documentation setup step.

### What I learned

- Splitting by console-observable vertical slices gives each child ticket a concrete pass/fail loop.

### What was tricky to build

- The main challenge is scope control: adjacent phases touch the same files, especially `app_main.cpp`, so commits must remain explicit and diaries must record which ticket owns each behavior.

### What warrants a second pair of eyes

- Whether the phase boundaries are still right after the first firmware build/probe results.

### What should be done in the future

- Implement the phase tasks and update this diary after each commit or failed validation attempt.

### Code review instructions

- Start with this ticket's design guide.
- Review only changes that are in scope for `0102-PICOJS-RUNTIME-COMPONENT`.
- Validate using the commands listed in the design guide.

### Technical details

Phase commands to keep in view:

- `picojs status`
- `picojs dump`


## Step 2: Add the firmware PicoJS runtime component skeleton

I added the first ESP-IDF component boundary for the firmware-native PicoJS runtime. This step deliberately does not install QuickJS bindings yet. It creates a C-callable runtime API, owns a fixed 40x20 text screen buffer, wires the component into the 0102 firmware build, and exposes `picojs status`, `picojs dump`, `picojs frame`, and `picojs key` console commands.

The purpose is to validate ownership and console plumbing before porting the native DSL builder API. The runtime can now be built, flashed, queried, advanced by one frame, given a synthetic key token, and dumped over UART without depending on LCD inspection or JavaScript binding code.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Continue implementing child tickets in order after the console feedback slice, beginning with the runtime component skeleton and validating it through the by-id UART console loop.

**Inferred user intent:** Establish a maintainable firmware component boundary before moving complex QuickJS builder bindings into the device firmware.

**Commit (code):** Pending — code and docs changed in this step.

### What I did

- Added `components/picojs_runtime/`:
  - `CMakeLists.txt`
  - `include/picojs_runtime.h`
  - `picojs_runtime.cpp`
- Added the component to `0102-esp32-p4-visual-quickjs-repl/CMakeLists.txt`.
- Added `picojs_runtime` to the 0102 `main` component requirements.
- Added a global `g_picojs` runtime pointer in `app_main.cpp`.
- Created the runtime during `app_main()` startup with `VISUAL_REPL_COLS` and `VISUAL_REPL_ROWS`.
- Added `picojs` console subcommands:
  - `picojs status`
  - `picojs dump`
  - `picojs frame [dt_ms]`
  - `picojs key <token>`
- Built and flashed the firmware using ESP-IDF 5.4.2 and the ESP32-P4 by-id serial path.
- Ran a UART probe sequence against `picojs status`, `dump`, `frame`, `key`, and a second `dump`.

### Why

- The native DSL should not grow inside `app_main.cpp`; it needs a component boundary before the QuickJS builder API is ported.
- A screen buffer and dump command provide a stable target for later DSL rendering.
- `picojs frame` and `picojs key` are early placeholders for later app frame/input behavior, and they prove the command shape now.

### What worked

- `idf.py build` succeeded with the new component. The binary size was:

```text
0102-esp32-p4-visual-quickjs-repl.bin binary size 0xdca70 bytes. Smallest app partition is 0x400000 bytes. 0x323590 bytes (78%) free.
```

- Flash succeeded on:

```text
/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00
```

- Runtime console probe passed:

```text
picojs: initialized=1 cols=40 rows=20 apps=0 mounted=0 frames=0 last_frame_ms=0 errors=0
picojs dump: ESP_OK
[00] PicoJS runtime ready
[01] grid=40x20 frame=0
picojs frame: ESP_OK dt_ms=250
picojs key: ESP_OK token=left
[02] last_key=left
PICOJS_PROBE PASS [True, True, True, True, True]
```

### What didn't work

- The first implementation attempt defined `struct picojs_runtime` inside an anonymous namespace while the public header forward-declared `struct picojs_runtime` globally. I corrected this by defining the struct in the global namespace and keeping helper functions/constants internal where appropriate.
- Keyboard recovery warnings appeared during the UART probe:

```text
W (...) 0102: keyboard poll failed: ESP_ERR_INVALID_STATE consecutive_errors=30
W (...) picocalc_kbd: recovering PicoCalc keyboard I2C bus/device (attempt=2)
```

They did not affect the PicoJS runtime commands, but they remain background noise from the existing keyboard task.

### What I learned

- The component boundary builds cleanly as a C++ ESP-IDF component with a C-callable public header.
- The console command shape is sufficient for later phases: `picojs dump` is the text oracle, `picojs frame` advances deterministic state, and `picojs key` injects semantic input.
- Keeping QuickJS out of this skeleton made the phase small and easy to validate.

### What was tricky to build

- The C/C++ opaque type boundary matters. The public header exposes `typedef struct picojs_runtime picojs_runtime_t;`, so the implementation must define that same global tag, not an unrelated internal type.
- The screen dump sizing uses the same fixed-row convention as `visual_repl_dump_text`: `"[NN] " + cells + "\n"` for each row. This should remain stable for future probe assertions.

### What warrants a second pair of eyes

- Whether `picojs_runtime_key()` belongs in this skeleton or should have waited for the input ticket. I included it as a no-JS placeholder because it validates the console command shape and writes `last_key` into the dump.
- Whether the runtime should allocate the screen buffer dynamically in PSRAM once rows/cols become configurable beyond 40x20. The current fixed buffer is intentional for the first component skeleton.

### What should be done in the future

- Commit this runtime skeleton slice.
- Upload the updated runtime component guide/diary to reMarkable.
- Begin `0102-PICOJS-MINIMAL-DSL` by installing a minimal native `OS` object into `qjs_service`.

### Code review instructions

- Start with `components/picojs_runtime/include/picojs_runtime.h` to review the C API.
- Then review `components/picojs_runtime/picojs_runtime.cpp` for screen ownership and dump formatting.
- Finally review `app_main.cpp` for command registration and startup creation.
- Validate with:

```bash
cd 0102-esp32-p4-visual-quickjs-repl
source ~/esp/esp-idf-5.4.2/export.sh
idf.py build
idf.py -p /dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00 flash
```

Then issue:

```text
picojs status
picojs dump
picojs frame 250
picojs key left
picojs dump
```

### Technical details

The runtime skeleton intentionally reports zero apps and zero mounted apps. Those fields are reserved for the minimal DSL slice.
