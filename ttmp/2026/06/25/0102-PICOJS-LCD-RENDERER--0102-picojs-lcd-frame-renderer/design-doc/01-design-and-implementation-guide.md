---
Title: Design and Implementation Guide
Ticket: 0102-PICOJS-LCD-RENDERER
Status: active
Topics:
    - esp32-p4
    - quickjs
    - picocalc
    - visual-repl
    - javascript
    - firmware
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp
      Note: Firmware app orchestration and console command integration point
    - Path: components/qjs_service/include/qjs_service.h
      Note: QuickJS service API and JSContext job boundary
    - Path: components/visual_repl/include/visual_repl.h
      Note: Fixed-cell visual model and dump/render target
ExternalSources: []
Summary: Frame-oriented LCD rendering path for PicoJS screen buffers.
LastUpdated: 2026-06-25T15:30:00-07:00
WhatFor: Plan and implementation guide for display PicoJS frame buffers on the PicoCalc LCD without relying on scrollback semantics.
WhenToUse: Use before implementing or reviewing this phase of the PicoJS device integration.
---


# Design and Implementation Guide

## Executive Summary

This child ticket is Phase 7 of the umbrella `0102-PICOJS-DEVICE-INTEGRATION` effort. Its purpose is to display PicoJS frame buffers on the PicoCalc LCD without relying on scrollback semantics. The phase must end with a buildable firmware tree, explicit console-visible validation, diary entries, and focused commits.

The umbrella design remains the source of cross-phase architecture. This document narrows the work to the smallest independently reviewable slice. The expected pattern is: implement one capability, validate it from UART, record exact commands/results in the diary, relate changed files, update the changelog, then commit.

## Scope

- Decide whether to extend `visual_repl` with row-matrix rendering or render directly through `picocalc_lcd`.
- Implement full-row or dirty-row blitting for PicoJS screen rows.
- Keep UART `picojs dump` as the regression oracle.
- Validate LCD output only after console dump parity is stable.

## Non-goals

- Do not implement later-phase DSL features in this ticket just because nearby code makes it convenient.
- Do not rely on LCD-only validation; every phase must expose enough UART output for scripted checks.
- Do not stage unrelated dirty files from the large shared worktree.
- Do not create a second QuickJS owner outside `qjs_service` unless a later design explicitly supersedes that decision.

## Console Contract

The phase should expose or validate these console commands:

- `picojs render`
- `picojs dump`
- `screen dump`
- `status`

Command output should be line-oriented and stable enough for a probe script to assert substrings. Human-readable output is acceptable, but avoid formatting that requires interpreting ANSI cursor movement or visual LCD state.

## Design Decisions

- Frame apps should not be modeled as scrollback history long-term.
- Console dump remains required even after LCD rendering works.
- The renderer should preserve the 40-column contract unless a later ticket changes display geometry.

## Implementation Plan

1. Re-read the umbrella guide and this child guide.
2. Inspect the exact firmware/component files named in the references section.
3. Make the smallest code change that exposes the next console-observable behavior.
4. Build the 0102 firmware with ESP-IDF 5.4.2.
5. If hardware is available, run the UART probe or manual command sequence.
6. Update this ticket's diary with exact commands, failures, and results.
7. Relate changed files with `docmgr doc relate` and update the changelog.
8. Commit only intentional paths.

## Validation Strategy

- Rendered LCD frame matches UART dump content semantically.
- Repeated frames do not grow visual history unexpectedly.
- Render timing and heap use are visible in status output.

For every validation failure, copy the exact command and error text into the diary before fixing it. If a probe suggests the board needs a manual reset, stop and ask the operator rather than repeatedly opening the serial port.

## Risks and Review Notes

- QuickJS object lifetime bugs can appear only at reset or teardown; review every stored `JSValue` path.
- Console commands run in the firmware orchestration layer, but large runtime logic should move into components.
- The main worktree has unrelated dirty/untracked files; use explicit path staging.
- ESP32-P4 uses UART0 through the CH343 bridge, not ESP32-S3 USB Serial/JTAG.

## References

- Umbrella ticket: `0102-PICOJS-DEVICE-INTEGRATION`.
- Firmware app: `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp`.
- QuickJS service API: `components/qjs_service/include/qjs_service.h`.
- Visual model API: `components/visual_repl/include/visual_repl.h`.
- Keyboard API: `components/picocalc_keyboard/include/picocalc_keyboard.h`.
- Desktop DSL reference after merge: `0102-esp32-p4-visual-quickjs-repl/js/tools/native-host/src/pico_native_api.cpp`.
