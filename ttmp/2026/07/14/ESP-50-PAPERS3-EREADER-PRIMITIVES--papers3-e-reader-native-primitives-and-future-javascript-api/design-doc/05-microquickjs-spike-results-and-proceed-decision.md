---
Title: MicroQuickJS Spike Results and Proceed Decision
Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES
Status: active
Topics:
    - papers3
    - eink
    - ereader
    - esp-idf
    - esp32s3
    - m5gfx
    - microquickjs
    - architecture
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-07-15T18:36:41.567078629-04:00
WhatFor: ""
WhenToUse: ""
---

# MicroQuickJS Spike Results and Proceed Decision

## Executive Summary

<!-- Provide a high-level overview of the design proposal -->

## Problem Statement

<!-- Describe the problem this design addresses -->

## Proposed Solution

<!-- Describe the proposed solution in detail -->

## Design Decisions

<!-- Document key design decisions and rationale -->

## Alternatives Considered

<!-- List alternative approaches that were considered and why they were rejected -->

## Implementation Plan

<!-- Outline the steps to implement this design -->

## Open Questions

<!-- List any unresolved questions or concerns -->

## References

<!-- Link to related documents, RFCs, or external resources -->

## Summary

The Phase 11 bounded spike ran a 38-probe suite on the actual PaperS3
(project `0113-papers3-mquickjs-spike`, never linked into the reader).
**All 38 probes pass. Decision: PROCEED to Phase 12** — both stop-rule
criteria (bounded execution/cancellation, C API safety) are demonstrated.
Evidence transcript: `scripts/output/0113-spike-final.log`.

## Engine pin (P11.1)

- MicroQuickJS, MIT (Bellard/Gordon). In-repo vendored copy at
  `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/`, copied
  byte-identical into `0113-papers3-mquickjs-spike/components/mquickjs/`
  (only `mquickjs_atom.h` regenerated for the spike stdlib).
- Reference upstream: `bellard/mquickjs@84d793e0` (local clone
  `~/code/others/mquickjs`); the vendored revision predates it
  (`mquickjs.h` identical, `mquickjs.c` differs in internal StringBuffer
  bookkeeping). Upgrades have a recorded baseline.
- Shared service layer: `components/mqjs_service` (MqjsVm + FreeRTOS
  worker) — used as-is; its deadline/interrupt hook is what the
  cancellation probe exercises.

## Measured results

| Question | Result |
|---|---|
| Context startup (868e) | 8 KB–256 KB internal and 1 MB–4 MB PSRAM arenas all work: `JS_NewContext` ≈ 0.6 ms, first eval ≈ 0.9 ms, size-independent |
| Memory exhaustion (868e) | 24 KB arena exhausts in 20 ms with `InternalError: out of memory`; the SAME context evaluates `1+1` correctly afterwards |
| Diagnostic binding (durp) | `millis()` C function binds, converts, and returns; exceptions from C throw cleanly |
| Opaque handles (dygk) | `S3Widget` packs (generation<<16\|index)+1 into the opaque; native `widgetDestroyAll()` bumps generations → stale wrapper access throws `TypeError: stale widget handle` (never touches freed state); finalizer runs on GC (created=1 finalized=1 live=0) |
| Compacting GC (vq48) | Rooted object survived 20 forced GC cycles with heap churn and **observably moved** (`moved=yes`) — `JS_AddGCRef`/`JS_PushGCRef` discipline is real and sufficient |
| Trusted bytecode (m1w2) | Full on-device round trip: `JS_NewContext2(prepare)` → `JS_Parse` → `JS_PrepareBytecode` → `JS_RelocateBytecode2(base 0)` → 244-byte image → fresh context → `JS_IsBytecode`/`JS_RelocateBytecode`/`JS_LoadBytecode`/`JS_Run` → `"bytecode:42"`. Only build-produced images may ever be loaded (format unverified by design) |
| Cancellation (dzfz) | `for(;;);` with a 100 ms MqjsVm deadline stops at exactly 100 ms with `InternalError: interrupted`; context reusable immediately after |

## Syntax matrix (0fdb)

Supported: `var`, closures, prototypes, fluent ES5 chains, object getter
literals, **`for-of`**, `JSON`, regexp, `Array.prototype.map/join`.
Rejected (SyntaxError): `let`, `const`, arrows, `class`, template
literals, spread, destructuring, modules. Stricter-mode behaviors
confirmed: undeclared global assignment → ReferenceError; array hole
write → TypeError.

**Consequence for Phase 12:** the fluent `s3paper` facade must be
authored in the ES5 subset (or host-transpiled); the design doc's §13.5
sample is already compatible.

## Constraints carried into Phase 12

1. One `MqjsVm` on the owner side; scripts run only via the service/VM
   with a deadline armed — never unbounded.
2. Every native JSValue that lives across an allocating call must be
   rooted (`JS_PushGCRef` stack or `JS_AddGCRef` list); the widget
   binding template in `spike_stdlib_runtime.c` shows the pattern.
3. Handles cross the boundary as generation-checked integers, mirroring
   the s3paper_core WidgetArena — the two generation schemes should be
   unified when binding real widgets.
4. Stdlib composition is a host-generation step
   (`tools/gen_spike_stdlib.sh`); the atom header is stdlib-specific and
   must stay project-local.
5. `set(COMPONENTS ...)` trimming silently drops Kconfig symbols —
   `esp_psram` must be named or PSRAM vanishes without error (cost one
   debug cycle).
