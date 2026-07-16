---
Title: E-ink Canvas Intern Guide - Analysis, Design, and Implementation
Ticket: ESP-52-EINK-CANVAS
Status: active
Topics:
    - papers3
    - eink
    - esp32s3
    - architecture
    - microquickjs
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-07-16T16:24:09.381970518-04:00
WhatFor: ""
WhenToUse: ""
---

# E-ink Canvas Intern Guide — Analysis, Design, and Implementation

## 1. What you are building, in one page

You are adding **freehand drawing primitives** — lines at arbitrary angles, circles (filled and stroked), rectangles, and area fills — to the s3paper rendering stack, exposing them to JavaScript as a **Canvas widget** in the PULP OS v2 builder API, and shipping a **minimalist showcase application** ("Ink") that demonstrates what a 16-gray e-ink panel does best: crisp geometry, quiet partial updates, and deliberate full-refresh moments.

Today the draw-op vocabulary is rectilinear: `FillRect`, `StrokeRect`, `HLine`, `VLine`, `GlyphRun`, `Bitmap` (see `components/s3paper_core/include/s3paper/draw_ops.h`). Everything on screen — every app in PULP OS — is composed of axis-aligned rectangles and text. This ticket adds two op kinds (`Line`, `Circle`), one widget kind (`Canvas`) with a native command store, five JS prototype methods, and one app.

The work spans all four layers of the proven stack, so this guide walks each layer in order. The prior tickets' guides are prerequisite background: `ESP-50` (`ttmp/2026/07/14/ESP-50-*/design-doc/01-*.md`) for the rendering architecture, `ESP-51` (`ttmp/2026/07/16/ESP-51-*/design-doc/01-*.md`) for the component layout and the v2 JS class machinery. Their diaries record every prior failure; read them before debugging anything that smells familiar.

## 2. Architecture recap (the four layers you will touch)

```
JS app (tools/js/apps/pulp.js)          canvas().line(...).circle(...)
    |                                                  [Phase 4]
main/js_widgets.cpp  (0114)             Canvas factory + methods
    |                                                  [Phase 3]
components/s3paper_core                 WidgetKind::Canvas + CanvasStore
  widget.h / widget_render.cpp          EmitNode -> ops     [Phase 2]
  draw_ops.h / frame_builder.cpp        DrawOpKind::Line/Circle [Phase 1]
    |
components/s3paper_core/fake_backend    trace lines (host tests, goldens)
components/s3paper_m5/m5_backend        M5GFX drawLine/fillCircle [Phase 1]
```

Invariants you inherit and must not break:

- **DrawOps are POD.** No pointers into JS or SD buffers; geometry is by value; every op carries the clip in force when it was emitted, and backends that paint outside `bounds` (as glyph and now circle rasterizers do) must honor `clip`.
- **Fully-clipped ops are dropped and counted** by the FrameBuilder. Damage = union of op bounds. This is what makes diff updates exact.
- **The widget tree is POD.** `WidgetNode` is a fixed ~136-byte struct in a 128-slot arena; nodes never hold heap pointers or callbacks. A canvas's command list therefore cannot live in the node — it lives in a store owned by the arena (§4).
- **JS never runs inside layout or present.** Canvas commands are appended by JS calls *between* presents; the render path only reads them.
- **Mutators bump `content_version` and no-op on equality** where cheap. The diff engine turns version changes into damage rects. Canvas appends bump the version; an unchanged canvas costs zero panel work.

## 3. Phase 1 — core draw ops: Line and Circle

### 3.1 Data model (`draw_ops.h`)

Add two kinds and two payloads:

```cpp
enum class DrawOpKind : uint8_t {
    FillRect, StrokeRect, HLine, VLine, GlyphRun, Bitmap,
    Line,    // arbitrary-angle segment
    Circle,  // filled (thickness == 0) or ring (thickness > 0)
};

struct LinePayload {
    int32_t x0, y0, x1, y1;   // absolute endpoints, PRE-clip
    int32_t thickness;        // >= 1
};

struct CirclePayload {
    int32_t cx, cy, r;        // absolute center, radius in px
    int32_t thickness;        // 0 = filled disc, >0 = ring width
};
```

Why endpoints in the payload when `bounds` exists: `bounds` is the *clipped bounding box* (what the diff/damage machinery consumes); the rasterizer needs the true geometry and must clip per-pixel against `op.clip`. This is exactly the GlyphRun pattern (baseline + text live in the payload; bounds is the clipped box).

### 3.2 FrameBuilder API (`frame_builder.{h,cpp}`)

```cpp
Status Line(int32_t x0, int32_t y0, int32_t x1, int32_t y1,
            Gray8 gray, int32_t thickness = 1);
Status Circle(int32_t cx, int32_t cy, int32_t r, Gray8 gray);          // disc
Status Ring(int32_t cx, int32_t cy, int32_t r, Gray8 gray,
            int32_t thickness);                                        // outline
```

Pseudocode for `Line` (Circle is analogous with bbox `{cx-r, cy-r, 2r+1, 2r+1}`):

```
bbox = {min(x0,x1)-t/2, min(y0,y1)-t/2, |dx|+t, |dy|+t}
clipped = Intersect(bbox, current_clip)
if !clipped.ok(): drop, count, return Ok          // fully clipped
emit DrawOp{kind=Line, gray, bounds=clipped, clip=current_clip,
            payload.line={x0,y0,x1,y1,t}}
```

Follow the existing `FillRect` implementation for the capacity/`Finish` bookkeeping. Reject `r < 0`, `thickness < 0`, and `thickness > r` (ring thicker than radius = disc; either clamp or reject — pick one and test it).

### 3.3 Backends

**Fake backend** (`fake_backend.cpp`) — one trace line per op, following the existing format exactly (the trace is golden-tested and the trace-equivalence harness depends on byte-stable output):

```
op kind=Line gray=0 bounds=... clip=... from=40,40 to=200,120 t=2
op kind=Circle gray=128 bounds=... clip=... c=270,480 r=60 t=0
```

**M5 backend** (`m5_backend.cpp`, the op switch at ~line 315) — map to M5GFX:

- `Line`: `M5.Display.drawLine(x0, y0, x1, y1, color)` for `t == 1`; for `t > 1`, draw `t` parallel lines offset along the minor axis (M5GFX has no thick-line primitive; this is the standard fallback). Set the display clip window from `op.clip` first (`setClipRect`), restore after — check how GlyphRun handles clipping and copy that discipline.
- `Circle`: `fillCircle(cx, cy, r, color)` when `t == 0`; for a ring, `fillCircle(r)` then `fillCircle(r - t)` in white is WRONG (it erases what is behind); use `drawCircle` at radii `r, r-1, ..., r-t+1`.
- Gray mapping: the backend already converts `Gray8` to the panel's 16-level space for fills; use the same helper.

### 3.4 Host tests (Phase 1 gate)

In `components/s3paper_core/tests/host/test_main.cpp`:

- Line/Circle bounds math: emitted `bounds` equals the expected clipped bbox for in-view, partially-clipped, and fully-clipped (dropped + counted) cases.
- Payload round-trip: endpoints/center/radius survive into the op untouched by clipping.
- Trace golden: a small fixture emitting one of each new op through the fake backend; pin the trace.
- Run: `cd components/s3paper_core/tests/host && make run` — currently 38,007 checks; every phase ends green.

## 4. Phase 2 — the Canvas widget

### 4.1 Command store (the design decision that matters)

`WidgetNode` is POD and fixed-size; a variable command list cannot live in it. The store lives in the `WidgetArena` alongside the nodes, sized at compile time:

```cpp
// widget.h
struct CanvasCmd {          // 16 bytes, POD
    uint8_t kind;           // 0=fill rect,1=box,2=line,3=disc,4=ring
    Gray8 gray;
    uint8_t thickness;
    uint8_t _pad;
    int16_t a, b, c, d;     // rect: x,y,w,h | line: x0,y0,x1,y1 | circle: cx,cy,r,-
};

struct CanvasProps { uint16_t store; uint16_t count; };   // node side

class WidgetArena {
    static constexpr uint32_t kCanvasSlots = 8;
    static constexpr uint32_t kCanvasCmds  = 96;   // per slot
    CanvasCmd canvas_[kCanvasSlots][kCanvasCmds];
    bool canvas_used_[kCanvasSlots];
    ...
};
```

8 slots × 96 commands × 16 B = 12 KiB, allocated once with the arena (which already lives in PSRAM via the runtime; total grows from ~17 KiB to ~29 KiB — verify the `runtime ready:` boot line).

API (all owner-task-only, all generation-checked like `SetText`):

```cpp
Result<WidgetHandle> NewCanvas(WidgetArena &arena);          // allocates a slot
Status CanvasAppend(WidgetHandle h, const CanvasCmd &cmd);   // bumps version
Status CanvasClear(WidgetHandle h);                          // count=0, bumps version
```

Rules with reasons:

- `NewCanvas` fails `CapacityExceeded` when all 8 slots are taken. `Reset()` releases slots (and bumps generations, so JS wrappers go stale exactly like every other widget).
- **Coordinates in commands are canvas-relative.** The emitter adds the frame origin at render time, so a canvas can move without its commands changing — and the diff sees a moved canvas as frame damage automatically.
- `CanvasAppend` past `kCanvasCmds` returns `CapacityExceeded`; the JS binding turns that into a TypeError. No silent truncation.
- Appends bump `content_version` once per call. The diff damages the whole canvas frame when the version changed — commands are not diffed individually (documented coarseness; a drawing app that appends one line per frame still gets one canvas-frame blit, which the planner clips to the union anyway).

### 4.2 Layout and render

- Layout: a Canvas measures like a Spacer — it has no intrinsic content size; give it `fixed_w`/`fixed_h` or flex from JS. `MeasureWidget` returns {0,0}; document that an unsized canvas collapses.
- `widget_render.cpp` `EmitNode`, new case:

```
case WidgetKind::Canvas:
    fb.PushClip(frame)                     // commands never escape the canvas
    for cmd in arena.CanvasCmds(node):
        switch cmd.kind:
            fill: fb.FillRect({frame.x+a, frame.y+b, c, d}, gray)
            box:  fb.StrokeRect(..., thickness)
            line: fb.Line(frame.x+a, frame.y+b, frame.x+c, frame.y+d, gray, thickness)
            disc: fb.Circle(frame.x+a, frame.y+b, c, gray)
            ring: fb.Ring(frame.x+a, frame.y+b, c, gray, thickness)
    fb.PopClip()
```

  Note `EmitNode` currently takes `(node, frame, fb)`; the canvas case needs the arena for the store — extend the signature (the call site is one place, `CompileTree`).

### 4.3 Host tests (Phase 2 gate)

- Store lifecycle: allocate 8 canvases, 9th fails; Reset frees; stale append after Reset fails on generation.
- Emission: canvas at frame (100,200) with a line (0,0)->(50,50) emits absolute (100,200)->(150,250) clipped to the frame.
- Diff: append -> version bump -> damage rect == canvas frame; no append -> zero damage.
- Fuzz: extend `TestFuzzWidgetOps` with random canvas appends/clears (the fuzzers found the AddChild cycle bug; give them the new surface).

## 5. Phase 3 — the JS binding (0114)

The stdlib gains one factory and six Widget prototype methods (Canvas-kind-checked, chaining `*this_val`, coordinates int16-clamped):

```
canvas()                          -> Widget (Canvas kind)
.line(x0, y0, x1, y1, gray, t?)   .disc(cx, cy, r, gray)
.ring(cx, cy, r, gray, t?)        .box(x, y, w, h, gray, t?)
.paint(x, y, w, h, gray)          .wipe()      // clear commands
```

(`fill`/`clear` are taken conceptually by other layers; `paint`/`wipe` avoid overload confusion. Naming is yours, but pick before generating atoms.)

Process discipline — this is the part that bites (ESP-51 diary Step 6, ESP-50 Step 19):

1. Edit `0114/tools/js/pulp_stdlib.c` (prototype table) and `mqjs_stdlib_pulp.c` (CONFIG_PULP block: `canvas` factory).
2. Add stubs to `tools/js/pulpjsc.c` (the host compiler links the same table by name).
3. `tools/js/gen_pulp_stdlib.sh` — regenerates `main/js_stdlib.h` AND `components/mquickjs/mquickjs_atom.h`. **Both, always, together.**
4. `tools/js/build_bytecode_apps.sh` — bytecode is atom-coupled; every stdlib change invalidates every image.
5. Implement the bindings in `main/js_widgets.cpp` (prototypes in `main/app_js_bindings.h`).

Binding skeleton (the established pattern from `js_w_set`):

```cpp
JSValue js_w_line(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    JSValue err; s3paper::WidgetHandle h;
    s3paper::WidgetNode *n = ThisNode(ctx, this_val, &h, &err);
    if (!n) return err;
    if (n->kind != WidgetKind::Canvas) return JS_ThrowTypeError(ctx, "line: not a Canvas");
    /* JS_ToInt32 x0,y0,x1,y1,gray[,t] ... */
    CanvasCmd cmd{ /* kind=line, clamp to int16 */ };
    Status st = Arena().CanvasAppend(h, cmd);
    if (!st.ok()) return JS_ThrowTypeError(ctx, "canvas full");
    return *this_val;
}
```

Console validation probe (`js probe 11`): build a canvas page with one of each primitive, present, verify op count and (probe 12, traced) the fake-backend op list — the same two-probe pattern that localized the MeasureText bug.

## 6. Phase 4 — "Ink": the showcase app

One launcher entry, three scenes on one retained page, rotating on tap. Swipe-down goes home (default grammar). Design intent: *show the panel, not the framework* — generous whitespace, few elements, deliberate refresh choices.

- **Scene 1 — Clock.** An analog clock: ring face (r≈200), 12 tick marks (lines), hour/minute hands (thick lines), a small filled hub. A 1-second `page.every` tick redraws ONLY when the minute changes: `wipe()` + re-append + `p.update()` — one canvas-frame blit per minute, zero work between. This is the e-ink argument in one screen.
- **Scene 2 — Field.** A deterministic generative composition (seeded from `millis()` at scene entry): ~40 grays-spanning discs/rings on a thirds grid with 3–5 long thin lines. Full clean present on entry (`show(true)`) — the flash IS the reveal.
- **Scene 3 — Ladder.** The 16-gray vocabulary as art: concentric rings stepping through gray levels, labels in `xs`. Doubles as a panel-quality diagnostic.

Implementation notes: keep each scene a function returning a configured canvas; `enter('ink')` once, scenes swap via `wipe()` + append + present (partial for clock, full for field/ladder). Announce `pulp screen: ink/<scene>` for transcripts.

## 7. Where to look for X

| You need... | Go to |
|---|---|
| Op emission pattern to copy | `frame_builder.cpp` `FillRect`/`GlyphRun` |
| Backend clip discipline | `m5_backend.cpp` GlyphRun case |
| Widget mutator pattern (version bump, no-op) | `widget.cpp` `SetText` |
| JS method pattern | `0114/main/js_widgets.cpp` `js_w_set`, `js_w_every` |
| Stdlib/atoms/bytecode pipeline | ESP-51 guide §6.2–6.3; `0114/tools/js/*.sh` |
| Probe + trace validation technique | ESP-51 diary Step 6; `js probe 9/10`, `js hits` |
| Why ops carry clip; dropped-op counting | ESP-50 design-doc/01 §4; `frame_builder.cpp` |
| Serial discipline / console client | `0114/README.md`; ESP-50 `scripts/52-*.py` |

## 8. Gotchas inherited from the prior tickets (each one burned us)

1. Regenerate stdlib AND atom headers together; rebuild bytecode after; host tools use COPIED engine sources (quoted-include shadowing).
2. `JS_CLASS_COUNT` must cover user classes in BOTH the device TU and pulpjsc.
3. mquickjs array holes are TypeErrors; `__cbs` reset seeds slot 0.
4. Never store a JSValue natively; the GC compacts. Commands are POD — keep them that way.
5. TTF-only font ids need `IsTtfFont(id) ||` guards — if you add any text helper, check its font validation.
6. Owner stack is 8 KiB: no multi-KB locals (the CanvasCmd store belongs to the arena, never the stack).
7. Tap targets: give interactive things explicit width/height (fingers are not cursors).
8. Validate with `js hits` and traced probes BEFORE guessing coordinates from typography.
9. `sdkconfig.defaults` only seeds absent values; `rm sdkconfig` to re-seed.
10. One console owner per port; stop captures before `idf.py flash`.

## 9. Phases and acceptance

- **P0 Orientation**: build 0114, run host suite (38,007), flash, `js pulp`, read this guide + both prior guides.
- **P1 Core ops**: draw_ops + FrameBuilder + both backends + host tests green. Gate: `widget hello`-style fixture showing a line and circle on the panel via a temporary probe.
- **P2 Canvas widget**: store + emission + diff + fuzz. Gate: host suite green; canvas damage semantics proven in a test.
- **P3 JS binding**: stdlib regen + bindings + probes. Gate: `js probe 11` renders all primitives; traced probe op list matches; stale/full containment throws.
- **P4 Ink app**: three scenes; launcher entry. Gate: clock updates once per minute (transcript: one small damage rect; silence between); field/ladder present clean-full; swipe-down home.
- **P5 Hardening + docs**: goldens re-pinned with dated comments, ~30-minute clock soak (heap flat, no exceptions), diary complete, doctor clean.

## 10. Glossary

**Canvas** — a widget whose content is a POD command list rendered inside its frame. **Command store** — fixed arena-owned slabs holding those commands (8 slots × 96 cmds). **Disc/Ring** — filled/stroked circle (named to avoid `circle` meaning both). **Scene** — one composition in the Ink app; scenes share one page and one canvas. **Trace probe** — a console probe presenting through the fake backend and printing every op.
