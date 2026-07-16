---
Title: PULP OS v2 Intern Guide - Analysis, Design, and Implementation
Ticket: ESP-51-PULP-OS-V2
Status: active
Topics:
    - papers3
    - eink
    - esp32s3
    - microquickjs
    - architecture
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://0112-papers3-reader-primitives/components/s3paper_core/include/s3paper/widget.h
      Note: Widget arena contract the v2 builders wrap
    - Path: repo://0112-papers3-reader-primitives/main/app_js.cpp
      Note: v1 JS host/ABI - migration source for v2
    - Path: repo://0112-papers3-reader-primitives/main/app_storage.cpp
      Note: Persistence to extract as s3paper_storage
    - Path: repo://0112-papers3-reader-primitives/main/app_ui.cpp
      Note: Present pipeline to extract as s3paper_runtime
    - Path: repo://0112-papers3-reader-primitives/tools/js/apps/pulp.js
      Note: v1 PULP apps - rewrite source
ExternalSources: []
Summary: ""
LastUpdated: 2026-07-16T13:08:55.531923714-04:00
WhatFor: ""
WhenToUse: ""
---


# PULP OS v2 Intern Guide - Analysis, Design, and Implementation

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

## 1. What you are building, in one page

You are building **PULP OS v2**: a new, self-contained firmware (`0114-papers3-pulp-os`) for the M5Stack PaperS3 e-ink tablet. It boots into a JavaScript-driven launcher ("PULP — the paperback of computers") with a suite of small apps (e-reader, chess clock, 2048, dice, tea timer, journal), all written in ES5 against a **fluent builder API whose builder objects are native C++ objects** exposed to MicroQuickJS as opaque-handle classes.

You are NOT starting from scratch. A previous ticket (`ESP-50-PAPERS3-EREADER-PRIMITIVES`, in `ttmp/2026/07/14/`) built and hardware-validated every primitive you need inside the firmware `0112-papers3-reader-primitives`: a pure C++ rendering/layout/widget core with 37,989 host-test checks, an EPD backend, a refresh planner that produces minimal partial updates, storage with crash-safe persistence, a power lifecycle with verified wake sources, an embedded MicroQuickJS runtime, and a working (v1) JS API. `0112` stays alive as the stable native-reader firmware; your job is to:

1. **Extract** the proven pieces into shared components (`s3paper_core`, `s3paper_m5`, `s3paper_storage`, `s3paper_runtime`).
2. **Re-point** `0112` at them without regressions (its host suite and console evidence guard you).
3. **Build `0114`** on those components with the **v2 builder API** (native classes instead of the v1 flat function ABI + JS wrapper objects).
4. **Rewrite the PULP apps** declaratively on v2.

Everything in this guide has a file reference. When a claim sounds surprising, go read the referenced code — it is the source of truth. The second source of truth is the ESP-50 implementation diary (`ttmp/2026/07/14/ESP-50-*/reference/03-implementation-diary-reader-primitives-firmware.md`, Steps 1–21): every subsystem's failures and fixes are recorded there, and §14 of this guide indexes it by topic.

## 2. Hardware and workstation orientation

### 2.1 The device

- **M5Stack PaperS3**: ESP32-S3 (dual-core Xtensa, 16 MB flash, 8 MB octal PSRAM), 540×960 e-ink panel (4.7"), GT911 capacitive touch, microSD (SPI: MISO 40 / MOSI 38 / SCLK 39 / CS 47), BM8563 RTC, battery on ADC1/GPIO3 (ratio 2.0), charge status GPIO4.
- E-ink fundamentals you must internalize: pixels persist without power; every update is a *waveform* trade-off between speed and ghosting; **partial refreshes** update a rectangle without flashing; **full refreshes** flash the panel and erase ghosting. The refresh planner (§4.6) owns this trade-off — app code never chooses waveforms directly.
- Verified power facts (diary Step 15): touch INT is GPIO48 which is NOT an RTC IO, so **deep sleep wakes by timer only**; true power-off wakes via BM8563 RTC alarm or the physical side button.

### 2.2 Serial discipline — read this twice

The PaperS3 console is native USB Serial/JTAG at
`/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00`.

- **NEVER** use `idf.py monitor`, `screen`, `minicom`, or raw pyserial: they toggle DTR/RTS and reset the device into ROM download mode mid-session.
- Use the console client: `ttmp/2026/07/14/ESP-50-*/scripts/52-papers3-console-client.py` — it opens the tty with plain `os.open` + `flock` + raw termios, no modem control. Usage:
  `python3 52-papers3-console-client.py --settle 8 --cmd "status" --cmd "js pulp" --output out.log`
- One owner per port. Stop any capture before `idf.py flash` (esptool does not flock; a flash once silently failed against a running monitor — diary Step 6).
- USB output is DROPPED while no host reads. Two consequences: (a) one-shot boot output is lost unless something loops it; (b) transcripts show gaps/garbled lines under heavy logging. Design validation output to repeat (PULP prints `pulp screen: <name>` on every present for exactly this reason).

### 2.3 Toolchain

- **ESP-IDF 5.3.4, pinned.** `unset IDF_PYTHON_ENV_PATH && source ~/esp/esp-idf-5.3.4/export.sh` then `idf.py build`. (5.4.x belongs to unrelated experiments; do not "upgrade".)
- Registry pins: `m5stack/m5unified ==0.2.18`, `m5stack/m5gfx ==0.2.25` (m5unified does not declare m5gfx — both must be pinned). `dependencies.lock` is committed.
- Host tests need only g++: `cd components/s3paper_core/tests/host && make run` (ASan/UBSan, ~38k checks, <1 min).
- JS tooling needs host gcc + the vendored engine; fonts tooling needs `pyftsubset`/`ttx` (fonttools).

## 3. The big picture

### 3.1 Layer diagram

```
                    JS apps (tools/js/apps/pulp.js, ES5-stricter)
                                     |
             fluent builder API   <- YOU BUILD THIS (v2: native classes)
   text("hi").size("xl").onTap(fn)   |   closures stay in JS (__cbs array)
   ----------------------- JS / C++ boundary -----------------------------
                                     |
        MicroQuickJS engine (vendored, per-firmware atoms)  [§6]
                                     |
   js_host.cpp: VM host, deadlines, bytecode load, dispatch  [main/]
                                     |
   s3paper_runtime: present pipeline (full + diff-update)    [§5.3]
        |                |                    |
   s3paper_core     s3paper_storage      s3paper_m5          [§4, §5]
   widgets/layout/  SD, catalog,         EPD transaction
   diff/planner/    positions, settings  shell, glyphs,
   text/paginator   (versioned+CRC)      touch, power
        |                                     |
     (pure C++, host-testable)          M5GFX / M5Unified
                                              |
                                        PaperS3 hardware
```

### 3.2 The ownership rule (the architecture's spine)

**One task — the UI owner (core 1, priority 5) — owns ALL application and display state.** Everything else is a producer posting bounded POD events into one queue. `AssertOwner()` aborts on violation. Producers never block: a full queue is an explicit, counted rejection. Reply channels are bounded queues with timeouts. JS executes ONLY on the owner task, only under a deadline, and never inside a display transaction.

The owner loop shape (see `0112/main/app_owner.cpp:OwnerTask`):

```
loop:
  event = queue.receive(timeout=500ms)
  if event: handle(event)              // console ops, touch ticks, timers
  StorageFlushIfDue(now)               // coalesced persistence
  UiRegionTick(now)                    // native fixture regions
  JsTimerTick(now)                     // JS interval -> s3Dispatch(100,...)
  PowerAutoTick(now)                   // inactivity + low-battery policies
```

### 3.3 What a screen update actually is

```
JS mutates widgets (.set)  ->  re-layout retained tree
  -> RenderStateDiff vs last capture  ->  damage rects (or none!)
  -> clipped re-render into flat DrawOps  ->  RefreshPlanner
  -> plan: partial(damage) or forced clean-full (budget)
  -> M5 backend: wait busy, set EPD mode, batched write, present
```

The money property (diary Step 21): a ticking chess clock blits **one 460×86 rect** per second; a stopped one costs **zero EPD work**, because `SetText` no-ops on equal strings and an empty diff skips the panel entirely.

## 4. Component tour: s3paper_core (pure C++, host-testable)

Location today: `0112-papers3-reader-primitives/components/s3paper_core/` (you will move it to top-level `components/` in Phase 1). Every module is header + src + host tests; NOTHING here includes ESP or M5 headers. Idioms: `Status`/`Result<T>` (never exceptions, never silent bools), fixed capacities with explicit `CapacityExceeded`, half-open rects, int64 arithmetic at overflow boundaries.

### 4.1 Vocabulary — `status.h`, `geometry.h`
- `StatusCode { Ok, InvalidArgument, CapacityExceeded, Busy, Timeout, CorruptData, OutOfMemory, Unimplemented }` — stable numbering, shared with JS as plain ints.
- `Rect{x,y,w,h}` half-open; `Intersect/Union/Shrink/ClampTo/RotateInBounds/AlignDamageForEpd` all return `Result<Rect>`.

### 4.2 Draw ops and frames — `draw_ops.h`, `frame_arena.h`, `frame_builder.h`
- `DrawOp` is POD: `{FillRect|StrokeRect|HLine|VLine|GlyphRun|Bitmap}`, gray 0..255, bounds + the clip in force at emit time. Text bytes live in a per-frame arena, referenced by offset — **a DrawOp never stores a pointer** into SD buffers or JS values.
- `FrameBuilder(ops[], cap, arena, viewport)`: `Begin() / PushClip / FillRect / GlyphRun(bounds, baseline_y, font_id, ...) / Finish(id)` → frozen `RenderFrame{ops, damage, viewport}`. Fully-clipped ops are dropped and counted. Damage = union of op bounds.

### 4.3 Widgets — `widget.h` (the tree your API manipulates)
- `WidgetArena`: 128 fixed slots; `WidgetHandle{index:u16, generation:u16}` — generation bumps on destroy/reset, so **stale handles fail loudly, never dangle**. This exact packed 32-bit form is what crosses into JS.
- `WidgetKind { Text, Row, Col, Spacer, Divider, Progress, List, Book, Region }`. Nodes are POD: copied text (`TextProps::kCapacity = 64` bytes!), numeric hit/dependency ids — **never callbacks or borrowed pointers** (design rule from ESP-50 §6.1).
- Mutators (`SetText/SetProgress`) bump a `content_version` and **no-op when the value is unchanged** — this is what makes "update everything, only changes blit" work.
- `AddChild` rejects double-parenting and ancestor cycles via parent links (a fuzz-found bug — diary Step 19; do not "simplify" this away).

### 4.4 Layout & compile — `widget_layout.h`, `widget_render.h`, `page.h`
- Flexbox-lite: fixed size > flex share > intrinsic; cross-axis stretch/align; `List` paginates (never scrolls) and reports `list_shown`.
- `LayoutPage(arena, PageSlots{header,content,footer,overlay}, bounds, entries[], cap)`: header/footer at intrinsic height, content fills, overlay on top.
- `CompileTree(...)` → FrameBuilder ops + immutable `HitRegion[]` + `RegionSpec[]`. Effective clip per node = intersection of ancestor frames. **Gotcha: a node with `hit_id != 0` and a null hits array is a hard `CapacityExceeded`** (diary Step 21).
- `HitTest(regions, count, point)` — topmost z, paint order breaks ties.

### 4.5 Diffing — `widget_diff.h` (the minimal-blit engine)
- `RenderStateDiff::Capture(arena, entries, n)` snapshots (generation, kind, content_version, frame) per slot after a successful present.
- `Diff(...)` → exact damage rects for changed/moved/appeared/disappeared nodes. `DependencyTracker` maps dirty `DependencyId`s to bound widgets.
- Contract: content-only mutations between Capture and Diff. Geometry changes produce correct damage but callers keeping cached hit regions must know (see §5.3).

### 4.6 Refresh planner — `refresh_planner.h`
- Owns e-ink policy: merges damage (merge distance 16, x-align 8), maps `PresentIntent {InteractiveInk, TextRegion, TextPage, ImageQuality, CleanFull}` to waveforms, and FORCES clean fulls on triggers: first render, wake, screen change, explicit, 64 turns, partial-area budget, elapsed-time budget. Hardware-soaked 10k updates (diary Step 5).

### 4.7 Text — `text.h` + `fonts/`
- `Utf8Next` (malformed → U+FFFD, one-byte advance, progress guaranteed), kerned `MeasureText`, `BreakLines` (width == re-measurement invariant).
- TTF via vendored `stb_truetype` v1.26: **trusted firmware-embedded fonts ONLY** (not hardened against hostile files — ESP-50 design-doc/04). Registered faces: `kFontUi`=PT Serif 22px, `kFontBody`=PT Serif 34px, `kFontDisplay`=Liberation Sans Bold 44px, `kFontXL`=Liberation Bold 84px. Subsets pinned to Latin+Ukrainian via `scripts/53-…/54-…` in the ESP-50 ticket.
- **Gotcha (diary Step 20): the m5 backend's GlyphRun guard must accept `IsTtfFont(id) || GetFont(id)` — `GetFont` only knows bitmap-fallback ids 0/1.**

### 4.8 Content & pagination — `content.h`, `paginator.h`
- `ContentSource{Size, ReadAt, Hash}`; identity = FNV over first 4 KiB + size. `MemoryContentSource` + (in storage) `SdContentSource`.
- `Paginator(source, LayoutKey)`: locator-based streaming (8 KiB window, checkpoints, bounded backward reconstruction). **Locators, never page numbers**; `TextLocator{byte_offset, context_hash}` validates against content.
- **CRITICAL bug class (diary Step 8): pass locators BY VALUE through compose paths** — `ComposePage(at, &page)` overwrites the storage `page.next` aliases.
- `LayoutKey{content, font, viewport, margins(40/72/56), engine_version}` — keep it IDENTICAL across readers so persisted positions interoperate (the v1 JS reader and native reader hand books back and forth at the same page because of this).

### 4.9 Host tests — `tests/host/`
`make run`: 37,989 checks — unit, golden line-breaks (PT Serif pinned), golden widget draw-op trace, deterministic fuzz (malformed UTF-8, paginator round-trips, random widget ops). **Run before and after every core-touching change.** Goldens that shift deliberately get re-pinned with a dated comment.

## 5. The device-side components

### 5.1 s3paper_m5 (move as-is)
`m5_backend.cpp`: the ONLY code calling `M5.Display`. Transaction shell: bounded 5 s busy-wait → `setEpdMode` by intent → `startWrite`/batched ops/`endWrite`. Glyph blits: PSRAM cache (512 slots), 16-level AA for quality intents, 1-bit threshold for fast ones. `ReadTouch` polls GT911. `m5_power.cpp`: battery read, `PowerDeepSleep(us)` (timer wake), `PowerRtcOff(s)` (BM8563 alarm), `PowerOff()` — all hardware-verified.

### 5.2 s3paper_storage (extract in Phase 2 from `0112/main/app_storage.cpp`)
All state files share one pattern: `magic + version + count + fixed records + FNV crc`, written `tmp → fflush → fsync → rename(bak) → rename` — power loss yields old-or-new, never torn.

| File | Magic | Records | Notes |
|---|---|---|---|
| positions.bin | S3RP | 32 × {hash,offset,ctx} | reading positions, coalesced (2 s / screen-change / shutdown) |
| bookmarks.bin | S3MB | 64 × same | toggle-removes |
| catalog.bin  | S3CT | 32 × {path,title,size,mtime,hash} | scan cache; path+size+mtime validates; disposable |
| settings.bin | S3ST | 16 × {key[16], i32} | app store (2048 best, tea preset) |
| lastbook.bin | S3LB | 1 × path | boot restore |

Extraction notes: replace `app_events.h` types with `s3paper::StatusCode`; inject the "M5 display must init before SD mount" constraint as a callback (the SPI bus is shared — mounting first once crashed the device, diary Step 11); parameterize the demo-book seeder. **Never auto-format user media. ~5 KiB structs must NOT be stack locals on the 8 KiB owner task (this crash-looped once — diary Step 13); use static scratch.**

### 5.3 s3paper_runtime (extract in Phase 3 from `0112/main/app_ui.cpp` + `app_display.cpp`)
Owns: PSRAM frame storage (512 ops / 32 KiB arena / 16 KiB trace), fake + M5 backends, the planner, font registration, and the two present entry points:

```
PresentPage(slots, intent, screen_change, hits[], cap, extra_ops)
  Begin; FillRect(white); LayoutPage; CompileTree(hits, regions);
  extra_ops(fb, entries)        // e.g. book body lines
  Finish; [NoteScreenChange]; PresentPlanned(intent); Capture diff state

PresentPageUpdate(slots, hits, cap, extra_ops)   // diff mode
  LayoutPage; Diff vs capture
  0 rects -> return Ok, NO panel work
  >16 rects or no capture -> fall back to full PresentPage(TextPage)
  else: Begin; PushClip(union); FillRect; compile into SCRATCH hits
        (callers keep previous hit regions!); TextRegion present; Capture
```

Two invariants with scars behind them: (1) update mode must NOT propagate hits (clip-shrunken regions killed taps once); (2) compile always needs a hits array (see §4.4). Also exposes `PresentCount()` — transient screens (JS apps) treat "someone else presented" as losing the panel, which is how gestures route without coupling.

## 6. MicroQuickJS: everything you must know

### 6.1 Engine facts (validated by the ESP-50 Phase 11 spike, 38/38 hardware probes)
- Vendored copy (MIT, Bellard/Gordon) — `0113-papers3-mquickjs-spike/README.md` records provenance. Each firmware carries its OWN engine copy because `mquickjs_atom.h` is generated from that firmware's stdlib.
- Fixed caller-provided arena (`JS_NewContext(mem, size, &js_stdlib)`): 8 KB–4 MB all work; creation ~0.6 ms. We use 160 KB in PSRAM (PSRAM speed == internal, measured). OOM throws `InternalError: out of memory` and the context SURVIVES.
- **Compacting GC: objects move on any allocating call.** Native code must root every JSValue held across allocations: `JSGCRef ref; JSValue *v = JS_PushGCRef(ctx,&ref); … JS_PopGCRef`. Storing a JSValue into a JS-reachable container (array via `JS_SetPropertyUint32`) is the sanctioned way to keep long-lived references — the container roots it.
- **Deadlines**: `JS_SetInterruptHandler` + a monotonic deadline stops `for(;;);` in exactly the budget with the context reusable. EVERY eval/call goes through a deadline wrapper.
- **ES5-stricter subset**: var/closures/prototypes/getters/for-of/JSON/regexp/`Math.random`/array+string methods YES; let/const/arrow/class/template/spread/destructuring/modules NO; undeclared-global assignment → ReferenceError; array holes → TypeError; duplicate catch variable names in one scope → SyntaxError.

### 6.2 The stdlib/atom pipeline (memorize this diagram)

```
tools/js/mqjs_stdlib_s3.c   (upstream stdlib + CONFIG_S3 block: your API)
        + s3_stdlib.c       (class/prototype tables; names are STRINGIFIED,
                             the generator links NO implementations)
   |  gcc + mquickjs_build.c  (host)
   v
 s3_stdlib_gen  --m32-->  main/js_stdlib.h        (32-bit table for device)
                --m32 -a-> components/mquickjs/mquickjs_atom.h
   (no -m32)  -->  tools/js/host/*  (64-bit pair for the host compiler)

Device pairing: main/js_stdlib_table.c = prototypes header + generated table;
implementations are extern "C" functions in main/js_builders.cpp.
```

Rules with scars: regenerate BOTH headers after ANY stdlib change; the atom header is stdlib-specific (never share the engine component between different stdlibs); when building host tools, COPY `mquickjs.c` next to the host atom header — quoted includes search the source's own directory first and will silently pick the device atoms (symptom: `var x = 1;` becomes a parse error — diary Step 19).

### 6.3 Bytecode pipeline
`tools/js/s3jsc.c` (host): parse ES5 against the SAME stdlib → `JS_PrepareBytecode64to32` → C header. Device: **load at context setup BEFORE anything evaluates** (`JS_LoadBytecode` requires zero RAM atoms) → `JS_RelocateBytecode` in a malloc'd buffer that must outlive the context → `JS_Run(main)` any number of times. Only ONE extra atom table fits (`N_ROM_ATOM_TABLES_MAX=2`), so the whole OS ships as ONE image. Always `JS_IsException`-check the load result — an exception value fed to JS_Run masquerades as "bytecode function expected".

### 6.4 C API cookbook (patterns you will copy)

```c
/* user class with opaque handle (from the spike + v1) */
static const JSPropDef js_widget_proto[] = {
    JS_CFUNC_DEF("pad", 4, js_widget_pad),         /* returns *this_val */
    JS_CGETSET_DEF("value", js_widget_get_v, NULL),
    JS_PROP_END };
static const JSClassDef js_widget_class =
    JS_CLASS_DEF("Widget", 1, js_widget_ctor, JS_CLASS_WIDGET,
                 NULL, js_widget_proto, NULL, js_widget_finalizer);

/* storing a JS closure natively-safely: put it in a JS array */
JSValue cbs = JS_GetPropertyStr(ctx, JS_GetGlobalObject(ctx), "__cbs");
JS_SetPropertyUint32(ctx, cbs, id, fn_arg);        /* array roots fn */

/* calling JS from C (gesture dispatch, dynamic values) */
if (JS_StackCheck(ctx, 4)) return JS_EXCEPTION;
JS_PushArg(ctx, JS_NewInt32(ctx, y));
JS_PushArg(ctx, JS_NewInt32(ctx, x));
JS_PushArg(ctx, fn);
JS_PushArg(ctx, JS_NULL);                          /* this */
JSValue out = JS_Call(ctx, 2);                     /* 2 args */
```

## 7. The v2 builder API (what you implement in Phase 5–6)

### 7.1 Design summary
v1 (in 0112) exposes ~30 flat CFUNCs (`s3Text`, `s3Config(handle, prop, a..d)`, `s3Present`) and builds fluent wrapper objects in an evaluated-at-boot JS facade. v2 replaces that: **builder objects ARE native class instances** — opaque = the packed `WidgetHandle`; the fluent methods are C functions in the ROM prototype; the JS facade shrinks to a ~30-line kernel. Wins: prototypes in ROM not arena RAM, no facade eval, direct `JS_Call` dispatch (no eval-string parse per gesture), single native validation point, and the same builder functions become callable from future native apps.

### 7.2 Class inventory

| Class | opaque | Factories (globals) | Prototype methods |
|---|---|---|---|
| `Widget` | packed WidgetHandle | `text(v_or_fn)`, `row()`, `col()`, `spacer(px,flex)`, `divider(t,g)`, `progressBar(p,h)`, `list()`, `region(id,ms,quiet)`, `book(ref)` | `pad,gap,mainAlign,crossAlign,width,height,flex,size(token),gray,center,invert,ellipsis?,add(...),set(v),progress(p),onTap(fn),every(ms),quiet` |
| `Page` | PageId | `page(name)` | `header(w),content(w),footer(w),overlay(w),on(gesture,fn),show(full?)` |
| `paper` | singleton | — | `refresh({turns,area,ms}), rotation, home(pageOrFn), sleepImage(fn)` |

Ownership rule: **wrappers do not own tree nodes** — the finalizer is a no-op; staleness is generation-checked on every method (`stale widget handle` TypeError). Chaining = `return *this_val;`.

### 7.3 Closures and dynamism (the load-bearing design)

```
.onTap(fn)  ->  C: id = next_cb++; node.hit_id = id; __cbs[id] = fn
gesture     ->  native hit-test -> cb id -> JS_Call(__cbs[id], kind, x, y)

text(fn)    ->  C: id = next_cb++; __cbs[id] = fn;
                dyn_table[native] += {WidgetHandle, cb_id};
                node.text = ToString(JS_Call(fn))        /* initial value */
owner tick  ->  for each dyn entry: v = JS_Call(fn); arena.SetText(w, v)
                (no-op if unchanged) -> ONE PresentPageUpdate at the end
page.on(g,fn) -> native map {PageId, GestureKind} -> cb id
```

Native state stays POD (ids, handles). The `__cbs` JS array is created by the kernel at boot and roots every closure. Pseudocode for the tick (native):

```
JsTimerTick(now):
  if not js_screen_active or now < due: return
  changed = false
  for e in dyn_table where e.page == current:
      v = CallCb(e.cb_id)               # bounded, exceptions counted
      if SetText(e.widget, str(v)) changed anything: changed = true
  if changed: PresentPageUpdate(current_slots)   # 0-damage guard inside
```

### 7.4 What v2 deliberately rejects from the original studio pitch
(The pitch: `ttmp/2026/07/14/ESP-50-*/sources/local/s3paper-api-design.md`.)
- `refreshPolicy(ctx => …)` / `list.sort(cmp)` lambdas: they would execute inside layout/present — the one place JS must never run. Expose parameter setters instead.
- `icon()`: no asset pipeline yet; dynamic `text()` covers the demos.
- Size tokens map to the four registered faces (xs/sm→ui, md→body, lg→display, xl→XL) — fonts are fixed-size registrations, documented mapping.
- `.invert()` IS adopted (one new native text prop: filled background) and `sleep({screensaver: fn})` IS adopted (power path asks JS for a tree before sleeping).

### 7.5 Worked example — the chess clock on v2 (~15 lines)

```js
var z = { w: 300000, b: 300000, run: 0, last: 0 };
function hit(s){ settle(); /* inc+switch */ refreshLabels(); }
page('blitz')
  .content(col(
    col(text(function(){ return fmtClock(z.b); }).size('xl').center(),
        text(function(){ return (z.run===2?'> ':'')+'BLACK'; }).center())
      .pad(20,40,20,40).onTap(function(){ hit(2); }),
    divider(6),
    col(text(function(){ return fmtClock(z.w); }).size('xl').center(),
        text(function(){ return (z.run===1?'> ':'')+'WHITE'; }).center())
      .pad(20,40,20,40).onTap(function(){ hit(1); })))
  .on('longPress', pause).on('swipeDown', goHome)
  .every(1000)
  .show(true);
```

## 8. PULP OS: the product

Apps (all exist on v1 in `0112/tools/js/apps/pulp.js` — rewrite on v2): **Home launcher** (bold rows, live sub-labels), **Reader** (JS chrome over the native book service; positions interop with 0112), **Dice Tray**, **Blitz Ink** (chess clock), **2048 INK** (swipes; swipe-down trapped as a move), **Tea Timer**, **Postcard** (30-key tap keyboard; SEAL appends to `/sdcard/books/postcard.txt`, which the library scan then lists as a book), **Daily Pulp** (random page; "keep reading" hands off at the same locator).

Navigation grammar: tap = act; swipe down = home (unless trapped); long-press = app-specific. Typography: Swiss — Liberation Bold display faces for chrome, XL numerals, 6–8 px rules, flush-left; serif reserved for book text.

Boot flow for 0114: mount SD → load persistence → JS context + bytecode → launcher (`full` clean render) → touch on. Power: inactivity auto-sleep policy, sleep image from the JS `sleepImage(fn)` lambda, deep-sleep timer wake / RTC-off / side button (see §5.1 power facts).

## 9. Persistence formats quick reference

See §5.2 table for the five state files. Additional facts: all loaders fall back to `.bak`; invalid files are ignored with a log line, never repaired in place; the catalog is DISPOSABLE derived state (deleting it costs one slow rescan, never a position); positions/bookmarks/settings share one coalesced flush (dirty flags, 2 s age, forced on screen change and shutdown; worst-case loss = one page turn). Content identity is `FNV(first 4 KiB) ⊕ size` — identical text on SD and embedded intentionally collide so positions transfer.

## 10. Power lifecycle (carry the sequence verbatim)

```
sleep(mode):  touch tick OFF -> StorageFlushNow -> render sleep image
              (CleanFull; M5's waitDisplay guarantees EPD idle)
              -> SD unmount -> flush console -> wake source -> transition
modes:  deep N  (esp deep sleep, timer wake; USB survives)
        rtc-off N (true power off, BM8563 alarm re-latches power)
        off      (side button only)
wake = reboot; boot restore is the resume contract.
```

Boot causes for evidence: `reset_reason` 1=POWERON, 8=DEEPSLEEP, 11=USB; `wakeup_cause` 4=TIMER. Auto-policies in the owner tick: inactivity power-off (input-recency, default off) and low-battery (≤5% and not charging, sampled ≤ every 30 s, same quiesce path).

## 11. Development workflow

### 11.1 The loop
```
edit -> [host tests if core touched: make run]
     -> [tools/js/gen_s3_stdlib.sh if stdlib touched]
     -> [tools/js/build_bytecode_apps.sh if apps touched]
     -> idf.py build && idf.py -p /dev/serial/by-id/usb-Espressif_… flash
     -> console client: validate with transcripts saved to
        ttmp/<ticket>/scripts/output/  (evidence is part of the work)
     -> focused git commit  ->  diary step + changelog + task check
```
Keep the ESP-50 diary format (skill `diary`): every step records what failed verbatim. Future-you will thank present-you.

### 11.2 Validation techniques that took us weeks to converge on
- **Synthetic input from the console** — `js tap X Y`, `js swipe K` inject through the SAME dispatch path as real touch (JS first, then native). Row coordinates drift with typography; don't guess more than once.
- **Screens announce themselves** — `print('pulp screen: ' + name)` on every present makes transcripts deterministic.
- **Hit-count fingerprints** — each screen has a distinctive number of tap regions (home=7, blitz=2, postcard=30); `js present: hits=N` identifies what rendered.
- **Never trust an Ok log alone** — the per-second `js present` line fires even for zero-work updates; the `update present: N rect(s), damage WxH` line is the proof of actual panel work. Failed presents log a warning (they were silent once; it cost a blind debugging round).
- **Watch `skipped=` in backend lines** — nonzero means ops were dropped (the invisible-Swiss-fonts bug announced itself as `ops=15 skipped=27`).

### 11.3 The gotcha list (every one of these burned us — diary step in parens)
1. `idf.py monitor`/pyserial resets the device into ROM download mode (S1).
2. Flashing while a monitor holds the port silently fails (S6).
3. `sdkconfig.defaults` only seeds ABSENT values — `rm sdkconfig` to re-seed (S11).
4. `set(COMPONENTS main)` silently drops Kconfig symbols — name `esp_psram` or PSRAM vanishes with only a reconfigure-time warning (S16).
5. Owner task stack is 8 KiB — no multi-KB stack locals (5 KiB catalog struct crash-looped boot) (S13).
6. Locators/aliasing: pass `TextLocator` by value through compose (S8).
7. Regenerate atoms + stdlib together; host tools need copied engine sources (quoted-include shadowing) (S16, S19).
8. `JS_LoadBytecode` before ANY eval (zero-RAM-atoms rule); check its result for exceptions (S19).
9. xtensa printf: `int32_t` is `long int` — cast to int in logs (S3).
10. USB output drops with no reader attached; loop your evidence output (S16).
11. TTF-only font ids must not be skipped by the backend's bitmap-fallback guard (S20).
12. Widget text caps at 63 bytes; truncate at UTF-8 boundaries (S19).
13. `mainAlign`/`SpaceBetween` offsets only apply when no flex children exist; fixed cross-size beats Stretch (host-test proven).
14. FATFS long filenames need `CONFIG_FATFS_LFN_HEAP=y` (S11).

## 12. Where to look for X

| You need… | Go to |
|---|---|
| Any API contract | the header file — headers are the documentation |
| Why a design decision was made | ESP-50 design-doc/01 §14 decision records; diary step index below |
| How a bug was diagnosed | ESP-50 diary (`reference/03-…`), Steps 1–21 |
| MicroQuickJS integration answers | `0113-papers3-mquickjs-spike/` + ESP-50 design-doc/05 |
| The original API vision | ESP-50 `sources/local/s3paper-api-design.md` + `s3paper-studio.jsx` |
| v1 JS ABI + facade (your migration source) | `0112/main/app_js.cpp` |
| v1 PULP apps (your rewrite source) | `0112/tools/js/apps/pulp.js` |
| Console client / subset scripts | ESP-50 `scripts/52-…`, `53-…`, `54-…` |
| Hardware evidence transcripts | ESP-50 `scripts/output/*.log` |
| Font decision (stb vs FreeType, trust model) | ESP-50 design-doc/04 |

Diary step index: S1 scaffold/serial · S2 events/flood · S3 fixtures/printf · S4 planner · S5 soak · S6 input+flash-race · S7 goldens · S8 pagination/aliasing · S9 SD/SPI-bus · S10 typography/stb · S11 library+8.3 names · S12 bookmarks/boot-restore · S13 catalog/stack-crash · S14 widgets · S15 power · S16 mquickjs spike · S17 JS facade v1 · S18 trace equivalence + fault fallback · S19 JS reader + bytecode pipeline + AddChild cycle fuzz fix · S20 PULP OS + Swiss fonts + skip-guard bug · S21 diff updates + hit preservation.

## 13. Testing & acceptance

- **Host suite green at every commit** (37,989 checks; grow it with every core change — the fuzz suites found a real cycle bug within minutes of existing).
- **Trace equivalence**: native fixture and its JS mirror must produce byte-identical normalized draw-op traces (harness: `js trace` in 0112 — port it).
- **Hardware evidence per phase**: saved transcripts proving the phase gate (see tasks). "It looked right" is not evidence; a damage-rect log line is.
- **0112 must keep working** after every extraction phase: host tests + boot-restore smoke + one page-turn transcript.
- Final acceptance (Phase 9): docmgr doctor clean, both firmwares build, PULP boots standalone, all apps validated, sleep/wake cycle proven, license inventory complete (MIT engine, OFL fonts, MIT M5 stack).

## 14. Glossary

**Owner task** — the single task allowed to touch app/display state. **Locator** — `{byte_offset, context_hash}` position that survives font changes via validation. **Intent** — what the app means by a present (TextPage, TextRegion, CleanFull…); the planner picks the waveform. **Damage** — union of changed rects actually pushed to the panel. **Generation handle** — index+generation pair that makes stale references fail loudly. **Atoms** — mquickjs's interned names, baked at stdlib generation; bytecode is atom-coupled. **Kernel** — the ~30 lines of JS that ship with v2 (callback array, gesture-name constants, list-item sugar). **PULP** — the product: the paperback of computers.
