---
Title: Implementation Diary
Ticket: ESP-52-EINK-CANVAS
Status: active
Topics:
    - papers3
    - eink
    - esp32s3
    - architecture
    - microquickjs
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-07-16T16:24:13.172828435-04:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Chronological record of ESP-52: freehand drawing primitives (line, circle, rect, fill) through the s3paper stack, the Canvas widget, the JS binding, and the Ink showcase app.

## Step 1: Phases 1-2 — Line/Circle ops and the Canvas widget (host-proven)

The core now speaks freehand geometry. Two new DrawOp kinds (`Line`, `Circle`) follow the GlyphRun pattern: true geometry in the payload, clipped bbox in `bounds`, rasterizers honor `clip`. FrameBuilder gained `Line/Circle/Ring` (conservative bboxes, drop-and-count when fully clipped, thickness clamps); the fake backend traces them; the M5 backend maps them to drawLine (thickness via parallel offsets along the minor axis) and fillCircle/drawCircle (rings as concentric outlines — never fill the interior, it would erase what is behind). The Canvas widget stores POD commands in arena-owned slabs (8 slots x 96 cmds, 12 bytes each) because a fixed-size node cannot hold a list; commands are canvas-relative and the emitter clips them to the widget frame.

### Prompt Context

**User prompt (verbatim):** "Then, after writing the report about all this new v3 work and the JS implementation work, add drawing primitives (circle, rectangles, lines, fill) to the OS and make a refined minimalist demo application showcasing all the beauty of eink. ... Then implement task by task, keeping a detailed diary as you work, and committing at appropriate intervals."

**Assistant interpretation:** Extend the rendering vocabulary with freehand primitives at every layer and prove them with tests before the JS surface.

**Inferred user intent:** A canvas the OS apps can draw on, done with the same rigor as the rest of the stack.

**Commits (code):** e1f9231 (P1), 2deb364 (P2)

### What I did
- P1: draw_ops.h kinds+payloads, FrameBuilder emitters, fake trace, M5 rendering, `TestLineCircleOps` (bounds math, payload round-trip, dropped-clipped counting, trace format).
- P2: `WidgetKind::Canvas`, `CanvasProps`/`CanvasCmd`, `NewCanvas`/`CanvasAppend`/`CanvasClear`/`CanvasCmds`, slot release on Destroy/Reset, `EmitNode` canvas case (frame-relative + PushClip(frame)), Canvas measures like Book (no intrinsic size), `TestCanvasWidget` (lifecycle, capacity, version semantics, offset emission, escape clipping, diff = exactly the canvas frame), fuzz extension with random canvas ops.
- Suite: 38,174 checks green.

### What didn't work (and became the best find of the phase)
- The canvas fuzz extension immediately hit an ASan stack-overflow in `DestroyIndex` — NOT a canvas bug: destroying a still-linked child always left the parent's child index dangling; once the slot was reused, the parent pointed at an unrelated node and link cycles became possible, making tree walks recurse forever. Latent since ESP-50 Phase 9 (the old fuzzer's op mix never surfaced it). Fix: `Destroy` now unlinks from a live parent before cascading. This is the second time a fuzz-mix change has found a real lifetime bug within minutes.

### What was tricky to build
- Ring rendering semantics: a ring must never paint its interior. The obvious fillCircle(r) + fillCircle(r-t, white) erases underlying content — concentric drawCircle outlines are correct if slower.
- `-Werror=switch` is a friend: adding a WidgetKind flagged the one measurement switch that needed a decision (canvas = no intrinsic size).

### What warrants a second pair of eyes
- Thick-line emulation offsets along the minor axis only; very thick nearly-45-degree lines will show slight ropiness. Fine for t <= 4.
- Canvas diff damage is the whole frame per change burst (documented coarseness).

## Step 2: Phase 3 — the JS canvas surface, proven on hardware

`canvas()` factory plus six ROM prototype methods (`line/disc/ring/box/paint/wipe`), all Canvas-kind-checked, int16-clamped, chaining. One shared native helper (`CanvasMethod`) parses coordinates+gray+optional thickness and appends; capacity and kind errors surface as TypeErrors. Probe 11 renders every primitive on the panel and proves containment; probe 12 is its traced twin.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Commit (code):** 40ff4ff — "ESP-52 P3: canvas() factory + line/disc/ring/box/paint/wipe JS methods, probes 11/12"

### What I did
- stdlib: prototype entries + `canvas` global; pulpjsc stubs; atoms + bytecode regenerated (the full four-step pipeline, no shortcuts).
- Bindings in js_widgets.cpp; prototypes in app_js_bindings.h.
- Hardware (p3-canvas-probe.log): probe11 present = 10 ops CleanFull; kind check throws "not a Canvas"; 200-line loop throws "canvas append failed: CapacityExceeded"; probe12 trace shows exact frame-relative geometry (e.g. `from=60,111 to=520,771` for the (20,20)->(480,680) line inside the canvas at (40,91)) and clip confinement to `40,91,460,700`.

### What worked
- Entire phase on the first flash. The Phase 5/6 (ESP-51) plumbing — ThisNode, MakeWidget, error discipline — made the binding almost mechanical.

### What should be done in the future
- Phase 4: the Ink app (clock / field / ladder scenes).

### Code review instructions
- Core: `components/s3paper_core/src/{frame_builder,widget,widget_render}.cpp` diffs; tests `TestLineCircleOps`, `TestCanvasWidget`.
- Binding: `0114/main/js_widgets.cpp` CanvasMethod.
- Validate: host suite; `js probe 11` and `js probe 12` on hardware.
