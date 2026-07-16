---
Title: PULP OS System Onboarding Guide - Every Part of the System, for a New Intern
Ticket: ESP-53-PULP-CONNECTIVITY
Status: active
Topics:
    - papers3
    - esp32s3
    - microquickjs
    - architecture
    - eink
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://0114-papers3-pulp-os/main/app_js.cpp
      Note: Binding-layer host core the guide documents (handles, __cbs, dispatch, tick)
    - Path: repo://0114-papers3-pulp-os/main/app_js_bindings.h
      Note: Authoritative list of JS API entry points referenced in section 9
    - Path: repo://0114-papers3-pulp-os/tools/js/apps/pulp.js
      Note: Product app whose patterns section 10 teaches
    - Path: repo://components/s3paper_core/include/s3paper/widget.h
      Note: POD widget tree contract documented in section 4
ExternalSources: []
Summary: 'Full-system onboarding for a new engineer: hardware, component stack, present pipeline, MicroQuickJS engine and bindings, bytecode toolchain, JS API reference, product apps, console tooling, and the gotcha catalog. Read this before the connectivity guide (design-doc/01).'
LastUpdated: 2026-07-16T18:57:07.182617458-04:00
WhatFor: Onboarding a new engineer onto PULP OS before they touch ESP-53 connectivity work.
WhenToUse: Read first, cover to cover. Then read design-doc/01 for the ESP-53-specific design.
---


# PULP OS System Onboarding Guide — Every Part of the System, for a New Intern

This document explains the whole system: the hardware, every layer of software between a JavaScript closure and a grayscale pixel, the toolchain that turns `.js` files into C headers, and the tooling you will use to validate your work. It assumes you are a competent C/C++ programmer who has never seen this codebase, this device, or MicroQuickJS. Read it in order; later sections assume earlier ones.

The companion document `design-doc/01-connectivity-intern-guide-analysis-design-and-implementation.md` is the design for the work you will actually do (wifi, http, serve, files, buzzer). This document is everything you need to understand *before* that one makes sense.

---

## 1. What PULP OS is

PULP OS is a small application operating system for the M5Stack PaperS3, an ESP32-S3 device with a 540×960 16-gray e-ink panel, capacitive touch, an SD card slot, a buzzer, and a battery. It boots into a launcher and runs ten applications (reader, library, dice, chess clock, 2048, tea timer, postcard, daily log, ink gallery, home) written in JavaScript.

The architecture is a strict layering:

```
┌─────────────────────────────────────────────────────────┐
│  pulp.js — the product (launcher + 10 apps, ~750 lines) │   JavaScript, compiled to bytecode
├─────────────────────────────────────────────────────────┤
│  v2 builder API — Widget/Page native classes, services  │   C bindings in 0114 main/js_*.cpp
├─────────────────────────────────────────────────────────┤
│  MicroQuickJS — tiny JS engine, compacting GC           │   components/mquickjs
├─────────────────────────────────────────────────────────┤
│  s3paper_runtime — present pipeline orchestration       │   shared component
│  s3paper_storage — SD mount, state file, seeding        │   shared component
│  s3paper_core — POD widget tree, layout, draw ops       │   shared component (pure C++, host-testable)
├─────────────────────────────────────────────────────────┤
│  s3paper_m5 — M5Unified/M5GFX display + touch backend   │   shared component
├─────────────────────────────────────────────────────────┤
│  ESP-IDF 5.3.4 — FreeRTOS, drivers, VFS, esp_timer      │   pinned toolchain
└─────────────────────────────────────────────────────────┘
```

Three design commitments explain almost every decision below:

- **POD everywhere at the core.** The widget tree, draw ops, and canvas commands are plain-old-data structs in fixed arenas. No `std::string`, no heap churn per frame, no virtual dispatch in the tree. This makes the core host-testable under ASan/UBSan and makes damage diffing a memcmp-class problem.
- **One owner task.** All UI state, the JS engine, and the widget arena are owned by a single FreeRTOS task ("the owner"). Everything else (console, touch ISR-adjacent polling, future wifi/http workers) communicates with it by posting events to a queue. There are no locks around UI state because there is no sharing.
- **E-ink discipline.** Every present is planned: partial updates when damage is small, full refreshes to clear ghosting on a budget. The JS layer cannot spam the panel; it describes trees and the native side decides what actually gets blitted.

---

## 2. The hardware, and the constraints it imposes

The M5Stack PaperS3:

- **MCU**: ESP32-S3 (dual-core Xtensa LX7, 240 MHz), 512 KB internal SRAM plus 8 MB PSRAM. Internal RAM is the scarce resource — the display driver, Wi-Fi stack (when you enable it), and FreeRTOS all want it. Our JS arena lives in PSRAM.
- **Panel**: 540×960, 16 gray levels, driven through M5GFX's EPD support. A full refresh (the blink-to-black cycle) takes on the order of a second; partial updates are a few hundred milliseconds but leave ghosting that accumulates.
- **Touch**: GT911 capacitive controller, polled (not interrupt-driven in our setup) by the input service.
- **SD card**: SPI-attached, mounted at `/sdcard` via FAT/VFS. Holds books (`/sdcard/books/*.txt`), the state file, and postcards.
- **Buzzer**: passive piezo on **GPIO 21**, driven by the LEDC peripheral (low-speed mode, 13-bit resolution, 50% duty ≈ 4096). Verified against the vendor demo at `M5PaperS3-UserDemo/main/hal/hal.cpp:385`.
- **RTC**: BM8563, used for deep-sleep wake. Battery level is read via M5Unified.
- **USB**: the console runs over the ESP32-S3's built-in USB-Serial-JTAG. This has a sharp edge covered in §12: if no host process is reading the port, device `printf` output is dropped silently.

Constraints you must internalize:

- Frame presents are *slow and visible*. UI code is structured around "build tree → present once", never around incremental redraws.
- Internal RAM headroom sits around 220–230 KB free at steady state (`js status` prints it). Wi-Fi will take a large bite; that is why ESP-53's design is so careful about buffer placement.
- Blocking the owner task blocks touch response, the tick timer, and the console — everything. Anything slower than ~50 ms must move to a worker.

---

## 3. Repository layout and the build system

Everything lives in one repo (`esp32-s3-m5`). The parts relevant to PULP OS:

```
components/                      ← shared across firmwares
  s3paper_core/                  ← pure C++ core (no ESP-IDF includes)
    include/s3paper/*.h
    src/*.cpp
    tests/host/                  ← host test suite: `make run` (ASan/UBSan, 38k+ checks)
  s3paper_m5/                    ← M5Unified/M5GFX backend
  s3paper_storage/               ← SD + state file
  s3paper_runtime/               ← present pipeline glue
  mquickjs/                      ← the JS engine (vendored MicroQuickJS)
0112-papers3-pulp/               ← v1 firmware (kept as regression reference)
0114-papers3-pulp-os/            ← v2 firmware — YOUR TARGET
  main/                          ← owner, console, input, power, JS bindings
  tools/js/                      ← stdlib generator, host compiler, apps/pulp.js
ttmp/                            ← docmgr ticket workspaces (docs, diaries, scripts)
```

### 3.1 Toolchain rules (non-negotiable)

- **ESP-IDF 5.3.4**, not 5.4.x. Every build shell starts with:
  ```bash
  unset IDF_PYTHON_ENV_PATH && source ~/esp/esp-idf-5.3.4/export.sh
  ```
  Shell state does not persist between your terminal invocations in automation contexts — re-source every time.
- Managed components pinned: `m5unified ==0.2.18`, `m5gfx ==0.2.25`.
- Build/flash from `0114-papers3-pulp-os/`:
  ```bash
  idf.py build
  idf.py -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00 flash
  ```
- The firmware's `CMakeLists.txt` points at the shared components with `EXTRA_COMPONENT_DIRS` and an explicit `set(COMPONENTS main esp_psram)` — `esp_psram` must be named or PSRAM silently vanishes.
- **Never run `idf.py build` inside a component directory.** A stray `build/` + `sdkconfig` inside `components/s3paper_core` once poisoned dependency resolution and had to be deleted.

### 3.2 Host tests

`components/s3paper_core/tests/host/` builds the entire core (widgets, layout, render, text, frame builder, paginator, canvas) as a native binary with ASan/UBSan:

```bash
cd components/s3paper_core/tests/host && make run
# expect: "OK (38174 checks)" or higher
```

This suite is the first gate for any core change. It includes a widget-tree fuzzer that has found real bugs (see §14). Run it before you ever flash.

---

## 4. s3paper_core — the POD widget tree

This is the heart of the system. All files under `components/s3paper_core/`.

### 4.1 The arena and the node (`include/s3paper/widget.h`, `src/widget.cpp`)

Widgets live in a `WidgetArena`: a fixed array of `WidgetNode` slots plus side stores (text buffer, canvas store). A node is identified by a `WidgetHandle` — an index + generation pair, so stale handles are detectable after slot reuse. `IsNull(h)` tests validity; there is deliberately no `operator==`.

```
WidgetNode (POD):
  kind        : WidgetKind (Text, Row, Col, Spacer, Divider, Progress,
                            List, Region, Book, Canvas)
  parent, first_child, next_sibling : WidgetHandle   ← intrusive tree links
  props       : union-ish per-kind props (TextProps, BoxProps, CanvasProps, ...)
  layout      : resolved rect after layout pass
  content_version : bumped when content changes → damage granularity
```

Key operations (all free functions taking the arena):

- `NewText / NewRow / NewCol / NewSpacer / NewDivider / NewProgress / NewList / NewRegion / NewCanvas` — allocate a node.
- `AddChild(arena, parent, child)` / `RemoveChild` — tree surgery.
- `Destroy(arena, h)` — recursive free. **It unlinks from a live parent first**; this ordering fixed a fuzz-found infinite recursion (§14).
- `SetText`, `SetProgress`, etc. — mutate props, bump `content_version`.
- Canvas: `CanvasAppend(arena, h, CanvasCmd)`, `CanvasClear(arena, h)`, `CanvasCmds(arena, h)` — see §4.4.

Everything is bounded: `kCanvasSlots = 8` canvas widgets, `kCanvasCmds = 96` commands each, fixed text buffer. When you exceed a bound you get a `Status` error, not a heap allocation.

### 4.2 Layout (`widget_layout.h/.cpp`)

A single top-down/bottom-up pass, flexbox-flavored:

- Rows and columns measure children, distribute leftover space to `flex` children, apply `pad`, `gap`, main/cross alignment.
- Text measures via the text engine (§7): `MeasureText` for single lines, `BreakLines` for wrapped blocks.
- `Book` and `Canvas` measure `{0,0}` — they are containers whose content is painted, not laid out; they take whatever frame their parent gives them.

Output: every node's `layout` rect is absolute panel coordinates.

### 4.3 Render (`widget_render.h/.cpp`) and draw ops (`draw_ops.h`, `frame_builder.h/.cpp`)

`EmitNode(builder, arena, node, ...)` walks the laid-out tree and emits **draw ops** into a `FrameBuilder`. Ops are POD records in a `FrameArena`:

```
DrawOpKind: FillRect | GlyphRun | HLine | VLine | Line | Circle | PushClip | PopClip
LinePayload   { x0, y0, x1, y1, thickness }
CirclePayload { cx, cy, r, thickness }   // thickness 0 = filled disc
```

`FrameBuilder` methods you will see: `FillRect`, `Glyphs`, `HLine`, `VLine`, `Line`, `Circle`, `Ring`, `PushClip`, `PopClip`. Line/Circle compute conservative bounding boxes for the damage tracker and clamp thickness.

Two render behaviors worth knowing:

- **Inverted text** (`TextProps.invert`): emitted as a FillRect of ink plus a GlyphRun in the inverse gray — used for selected states.
- **Canvas** nodes emit `PushClip(frame)`, then replay their stored `CanvasCmd` list translated from canvas-relative to absolute coordinates, then `PopClip`. Freehand drawing can never scribble outside its widget.

### 4.4 Canvas store

A `CanvasCmd` is 12 bytes: `{kind, gray, thickness, _pad, a, b, c, d : int16}`. Kinds map onto line / disc / ring / box / fill / clear. Appending a command bumps the node's `content_version`, so damage granularity is the whole canvas frame — cheap and correct, if conservative. The full design rationale is in the ESP-52 guide (`ttmp/2026/07/16/ESP-52-EINK-CANVAS--*/design-doc/01-*.md`).

### 4.5 Diff, damage, refresh policy (`widget_diff.h`, `refresh_planner.h`)

After building a new tree (or mutating the current one), the present path diffs `content_version`/layout against the previous frame to compute damage rectangles. The `RefreshPlanner` then decides the EPD mode: partial update for small damage, full refresh when the ghosting budget (a count of partial updates) is exhausted or when a policy demands it. `set_policy` lets callers force behavior — the 2048 app uses this to re-blank after heavy play.

### 4.6 Text engine (`text.h/.cpp`)

Two font families coexist:

- Bitmap fonts (ids 0/1) with a glyph table in the core.
- TTF-rendered faces registered at runtime by `s3paper_runtime`: PT Serif 22/34/44 (44 = `kFontTitle`) and LibertinusSansBold 44/84. `kFontCount = 5`.

The guard pattern matters: `MeasureText`/`BreakLines` must treat a font as valid if **either** the bitmap table knows it **or** `IsTtfFont(id)`. Getting this wrong caused the nastiest bug of ESP-51 (§14). UTF-8 is handled throughout — the seeded library includes a Cyrillic book precisely to keep that path exercised.

### 4.7 Fake backend (`fake_backend.h/.cpp`)

A `DisplayBackend` implementation that renders ops into a text trace (`FillRect x=.. y=..`, `Line from=.. to=.. t=..`) instead of pixels. Host tests assert on traces; the device can also print traces (`SetTracePresent` + probes) so you can diff device behavior against host expectations byte-for-byte. Probe 14 does exactly this ("trace equivalence": device trace vs golden, `EQUAL 831 bytes`).

---

## 5. s3paper_m5, s3paper_storage, s3paper_runtime

### 5.1 s3paper_m5 — the real display/touch backend

`components/s3paper_m5/src/m5_backend.cpp` implements `DisplayBackend` over M5GFX. It is a switch over `DrawOpKind`:

- Every op is clipped via `setClipRect` from the active clip stack.
- `Line` with thickness > 1: parallel `drawLine` calls offset along the minor axis (never a filled polygon).
- `Circle`: `fillCircle` for discs; **concentric `drawCircle` calls** for rings. Never fill-then-erase — on e-ink, an erase is a visible flash.
- Glyph runs route to the font renderer; the backend must apply the same bitmap-vs-TTF guard as the core (an earlier ESP-50 bug class).

It also owns EPD mode selection when a present executes the planner's decision, and provides touch reads from the GT911.

### 5.2 s3paper_storage — SD and durable state

`components/s3paper_storage/` mounts the SD card and owns the binary state file (reader positions, key-value store used by `storeGet`/`storeSet`, postcards). API highlights:

- `StorageConfigure(StorageConfig{ pre_mount, seed_path, seed_text, seed_len })` — `pre_mount` is a hook (0114 uses it to ensure M5 init happens first); the seed fields let the firmware plant a book on first boot (`/sdcard/books/kobzar.txt`, Shevchenko, UTF-8/Cyrillic).
- `StorageMount / StorageUnmount / StorageWriteSeedBook / GetStats`.
- `StorageFlushIfDue()` — called from the owner tick; writes are debounced, never synchronous with UI actions.
- **Fault injection**: `DebugCorruptStateFile(kind 0-4, mode flip|trunc|del)` + `DebugReloadState`, surfaced on the console as `sd fault <kind> <flip|trunc|del>`. Probe 13 runs a battery over these. Any new record type you add (ESP-53 adds Wi-Fi credentials) must survive this battery.

### 5.3 s3paper_runtime — the present pipeline, assembled

`components/s3paper_runtime/` glues core + backend into two calls the firmware uses:

- `RuntimeInit(RuntimeConfig)` — brings up the backend, registers the five fonts, sizes arenas.
- `PresentPage(...)` / `PresentPageUpdate(...)` — full present vs. diff-based update of the current tree.
- `PresentCount()`, `SetTracePresent(bool)`, `FindRegion(...)` — introspection used by probes.

The pipeline, end to end:

```
JS builder calls          (js_widgets.cpp mutate the arena)
        │
        ▼
p.show() / p.update() ──► PresentPage / PresentPageUpdate
        │
        ▼
layout pass  (widget_layout)
        │
        ▼
EmitNode ──► FrameBuilder ──► DrawOp list in FrameArena
        │
        ▼
diff vs previous frame ──► damage rects
        │
        ▼
RefreshPlanner ──► partial | full refresh decision
        │
        ▼
m5_backend executes ops within damage clip ──► EPD blit
```

---

## 6. MicroQuickJS — the engine, and the five facts that rule everything

The engine is vendored in `components/mquickjs/` (`mquickjs.c`, ~13k lines). It is a *very* small JavaScript: stricter-than-ES5 dialect (array holes are a `TypeError`!), no modules, compacting GC. You do not need to read all of it, but you must internalize these facts, because every binding convention in §7 exists because of one of them:

1. **`JSValue` is a tagged 32-bit word on device** (`JSW=4`; 64-bit on host). `JS_TAG_INT` values are 31-bit immediates and are GC-safe forever. `JS_TAG_PTR` values point into the GC heap **and move when the compacting GC runs**. Any allocation can trigger GC. Therefore: never hold a pointer-tagged `JSValue` in a C local across an allocating call, and never store one in C memory at all.
2. **User classes and the opaque slot.** `JS_NewObjectClassUser(ctx, class_id)` creates an object with a `void*`-sized opaque slot (`p->u.user.opaque`) that the GC copies verbatim and never interprets. This is the *only* safe place to stash C-side identity on a JS object. It is nulled at creation.
3. **The finalizer table is sized by `JS_CLASS_COUNT`** (`js_c_finalizer_table[JS_CLASS_COUNT - JS_CLASS_USER]`), and `JS_CLASS_COUNT` must be `#define`d consistently in **both** the device build (`0114 main/app_js_bindings.h`) and the host compiler (`tools/js/pulpjsc.c`). We define `JS_CLASS_WIDGET = USER+0`, `JS_CLASS_PAGE = USER+1`, `JS_CLASS_COUNT = USER+2`. Forgetting one side produces "array index in initializer exceeds array bounds".
4. **Calling into JS**: check stack headroom with `JS_StackCheck(ctx, n+16)` (it may GC), push args in **reverse order** with `JS_PushArg`, then the function, then `this`, then `JS_Call(ctx, argc)`. Call depth is capped at `JS_MAX_CALL_RECURSE = 8`. Declared-arity padding: the engine pads missing args with `undefined`.
5. **Atoms and bytecode.** Identifiers live in atom tables. ROM atom tables are baked at compile time; `N_ROM_ATOM_TABLES_MAX = 2` means the stdlib table plus exactly **one** bytecode image can be loaded. `JS_LoadBytecode` enforces the zero-RAM-atom rule ("no atom must be defined in RAM", `mquickjs.c:12948`): all identifiers in your compiled apps must already exist in the stdlib atom table. This is why the stdlib generator and app compiler are one pipeline (§8), and why loading bytecode happens **before** any RAM-atom-creating eval.

A sixth, smaller fact: the `ATOM_ALIGN=64` hash-clamp warning during stdlib generation is benign; ignore it.

---

## 7. The binding layer — 0114 `main/app_js*.{h,cpp}`

This is where the engine meets the widget tree. The code is split into focused translation units:

| File | Role |
|---|---|
| `app_js.h` / `app_js.cpp` | host core (~520 lines): engine lifetime, eval, dispatch, tick, handle packing |
| `app_js_internal.h` | the `pulp::jsi` contract shared by the `js_*.cpp` TUs |
| `app_js_bindings.h` | `PULP_JS_FN` prototypes + the `JS_CLASS_*` defines |
| `js_widgets.cpp` | widget factories + 26 Widget methods (incl. canvas verbs) |
| `js_pages.cpp` | Page methods + the `paper` singleton |
| `js_services.cpp` | book/library/store/postcard/battery services |
| `js_probes.cpp` | probes 1–14 (validation harness) |
| `js_stdlib.h`, `js_pulp.h`, `js_stdlib_table.c` | **generated** — never hand-edit |

### 7.1 Handles: how a JS object names a C node without a pointer

Fact 1 of §6 forbids storing `JSValue`s in C and makes JS-side C pointers fragile. The solution: the opaque slot stores a packed integer, and the arena is the source of truth.

```
opaque = ((generation << 16) | index) + 1        // +1 so 0 still means "empty slot"

ThisNode(ctx):                                    // start of every Widget method
  v   = opaque of `this`
  if v == 0                        → throw "stale widget handle"
  h   = {index: (v-1) & 0xFFFF, gen: (v-1) >> 16}
  if arena.generation(h.index) != h.gen → throw "stale widget handle"
  return h
```

`PackWidget`/`UnpackWidget`/`PackPage`/`UnpackPage` implement this; `ThisPage` is the Page twin. Widget/Page finalizers are **no-ops** — the tree owns node lifetime, JS objects are just names. A JS object outliving its node fails loudly at next use instead of dangling.

### 7.2 Callbacks: the `__cbs` array kernel

C cannot root a JS closure (fact 1). So closures are rooted *in JS*, in a global array, and C stores only integer indices. The entire RAM-evaluated "kernel" is two lines:

```js
var __cbs = [null];
var G = {TAP:0, LONG:1, LEFT:2, RIGHT:3, UP:4, DOWN:5, TICK:100};
```

- `RegisterCb(ctx, fnValue)` → `JS_SetPropertyUint32(__cbs, next_index, fn)` → returns the integer index. Slot 0 is pre-seeded with `null` because this dialect forbids array holes — writing index 1 into an empty array is a `TypeError` (a real bug we hit). After `JS_SetPropertyStr` the array reference is **re-fetched** because GC may have moved it.
- `CallCb(ctx, cb_id, a, b, c, argc)` → looks up `__cbs[cb_id]`, pushes up to three **int32** args, calls it. Int-only args at this boundary is deliberate: 31-bit immediates cannot be corrupted by GC movement mid-push. Strings cross the boundary via accessor functions the callback calls afterwards (ESP-53's mailbox design leans on this).

### 7.3 Event dispatch and the tick

`JsHandleGesture(kind, x, y)` (called from the owner when the input service posts a gesture):

```
if kind == TAP:
    hit = lookup (x, y) in g_hits          // hit-region table built at present time
    if hit → CallCb(hit.cb_id, kind, x, y); return
if page.gesture_cb[kind] → CallCb(...);  return
if kind == SWIPE_DOWN → paper.home fallback   // the OS-level "go home"
```

`JsTimerTick()` (called from the owner's periodic tick hook):

1. Fire the page's `TICK` callback if registered (`p.every(...)`).
2. `RefreshDynValues()` — walk the dyn-values table `{WidgetHandle, cb_id}` built from `text(function(){...})` nodes; call each closure, write the returned string into the node if changed.
3. If anything changed, **one** `PresentPageUpdate`. One tick, at most one blit — this is the invariant that made the 36-minute clock soak flat.

### 7.4 Engine lifetime and evaluation

`JsRunPulp()` (boot): create context with the PSRAM arena (160 KB, `js status` reports `arena=163840`) → `LoadBytecodeApps()` **before** the kernel eval (zero-RAM-atom rule, §6.5) → eval the 2-line kernel → call the bytecode's entry (`boot()` in pulp.js). If anything fails, the owner falls back to `HomeShowNative()` so the device is never a brick.

`EvalBounded` wraps evals with a deadline; `RecordException` snapshots any JS exception into `last_error` (visible via `js status`). Counters: `evals`, `exceptions`, `dispatches`.

---

## 8. The stdlib/bytecode toolchain — `0114/tools/js/`

JavaScript gets to the device as C headers containing bytecode. Three pieces:

```
pulp_stdlib.c            ← declares Widget/Page classes, methods, paper object,
                           service functions (device-truth for the API surface)
mqjs_stdlib_pulp.c       ← CONFIG_PULP global wiring
pulpjsc.c                ← HOST compiler: stub natives + finalizers + class ids

gen_pulp_stdlib.sh  ──►  main/js_stdlib.h  +  components/mquickjs/mquickjs_atom.h
                          (the ROM atom table now contains every API identifier)
build_bytecode_apps.sh ─► builds host pulpjsc (copies mquickjs.c locally to avoid
                          quoted-include atom shadowing), compiles apps/*.js
                          ──►  main/js_<name>.h   (bytecode images)
```

**The regeneration protocol** — after *any* change to the API surface (new method, new global, new service):

```bash
cd 0114-papers3-pulp-os
./tools/js/gen_pulp_stdlib.sh        # 1. regenerate stdlib + atom table
./tools/js/build_bytecode_apps.sh    # 2. recompile all apps against it
idf.py build                          # 3. rebuild firmware
```

Skipping step 1 after adding an identifier → the app compiles but `JS_LoadBytecode` rejects it on device (RAM atom). Skipping step 2 → stale bytecode with old atom indices. The scripts are idempotent; run them liberally. Remember `JS_CLASS_COUNT` must match between `app_js_bindings.h` and `pulpjsc.c` (§6.3).

Also remember `N_ROM_ATOM_TABLES_MAX = 2`: all apps are concatenated into **one** bytecode image. You cannot add a second.

---

## 9. The v2 JS API — reference

This is the surface pulp.js (and your future connectivity code) programs against. Grouped, with the C entry points from `app_js_bindings.h` in parentheses where non-obvious.

### 9.1 Globals

| JS | Meaning |
|---|---|
| `print(s)`, `gc()`, `load()` | engine basics |
| `setTimeout(fn, ms)` / `clearTimeout(id)` | one-shot timers (owner-tick driven) |
| `Date.now()`, `performance.now()`, `millis()` | clocks |
| `abiVersion()` | v2 ABI check (`js_pulp_abi_version`) |
| `resetTree()` | destroy all widgets/pages, reseed `__cbs` — every app entry calls this |
| `text(s\|fn)`, `row(...)`, `col(...)`, `spacer(n)`, `divider(t, len)`, `progressBar()`, `list()`, `region()`, `canvas()` | widget factories; `text(fn)` registers a dyn value (§7.3) |
| `page()` | page factory |
| `G` | gesture constants: `TAP LONG LEFT RIGHT UP DOWN TICK` |

### 9.2 Widget methods (chainable; all return `this`)

Layout/box: `.pad(n)` `.gap(n)` `.mainAlign(a)` `.crossAlign(a)` `.width(n)` `.height(n)` `.flex(n)` `.center()` `.align(a)`
Text: `.font(name)` `.size('sm'|'md'|'lg'|'xl')` `.gray(g)` `.invert()` `.set(s)` — `.set` also works on progress via `.progress(v)`
Tree: `.add(child...)` `.dep(...)`
Interaction: `.onTap(fn)` `.hit(w, h)` (explicit hit rect) `.every(fn)` `.quiet()`
Canvas verbs (on a `canvas()` node, coords canvas-relative): `.line(x0,y0,x1,y1,t,gray)` `.disc(cx,cy,r,gray)` `.ring(cx,cy,r,t,gray)` `.box(x,y,w,h,t,gray)` `.paint(x,y,w,h,gray)` `.wipe()` — all parsed by a shared `CanvasMethod` helper in `js_widgets.cpp`.

### 9.3 Page methods and `paper`

| JS | Meaning |
|---|---|
| `p.header(w)` `p.content(w)` `p.footer(w)` `p.overlay(w)` | slot assignment |
| `p.on(G.X, fn)` | gesture callback for this page |
| `p.every(fn)` | per-tick callback |
| `p.show()` | full present (mode 0) |
| `p.update()` / `p.update(true)` | diff present (modes 1/2; 2 = also refresh dyn values) |
| `paper.home(fn)` | register the OS-level swipe-down-to-home handler |
| `paper.sleepImage(fn)` | register the sleep-screen builder (power path calls it) |
| `paper.refreshTurns(n)` | full-refresh cadence hint (reader uses it) |
| `paper.version()` | firmware version string |

### 9.4 Services (`js_services.cpp`)

- **book**: `bookOpen(idx)`, `bookTitle()`, `bookLineCount()`, `bookLine(i)`, `bookNext()`, `bookPrev()`, `bookProgress()` — pagination shares the 0112 `LayoutKey` so reading positions interoperate between firmwares.
- **library**: `libraryCount()`, `libraryLine(i)`, `libraryRescan()`.
- **store**: `storeGet(key, def)`, `storeSet(key, val)` — durable int KV in the state file (the margin toggle uses `storeSet('margin')`).
- **postcard**: `appendPostcard(s)`.
- **battery**: `batteryLevel()`.

ESP-53 adds `wifi`, `http`, `serve`, `files`, `buzzer` as further singletons in this style — see design-doc/01 §4–8.

---

## 10. pulp.js — the product

`0114-papers3-pulp-os/tools/js/apps/pulp.js` (~750 lines). Structure:

- **Globals**: `P = {app: 'home'}` (current app), `var M = 40` — the global margin, applied at ~17 padding sites. Boot reads `M = storeGet('margin', 40)`; long-press on the launcher toggles 40↔0, persists, and re-renders.
- **`enter(name)`** — the app-switch ritual: `resetTree()`, re-register `paper.home` and `paper.sleepImage` (resetTree wiped them), announce on console (`print('pulp screen: ...')` — probes grep for this).
- **`chrome(title)` / `hintFooter(hint)`** — shared header (title + bold rule) and footer builders; the 40 px rule margin is the visual anchor the separators align to.
- **Apps**: `home` (launcher), `library` (shelf; serif book titles at app-row size), `reader` (LEFT/RIGHT/tap-half page turns, `paper.refreshTurns`), `dice` (fat `.hit()` targets — bare text rects were untappable), `blitz` (chess clock; `p.every` + dyn-value `text(fn)` faces), `g2048` (board re-blank via refresh policy to kill ghosting), `tea`, `postcard` (on-screen keyboard, intentionally keeps its own 24 px padding), `daily`, `ink` (the ESP-52 canvas gallery; geometry derives from `var W = 540 - 2*M; var CX = Math.floor(W/2)` so the margin toggle keeps it centered).

Patterns to copy when you write app code:

- State lives in a per-app global (`DZ`, `BZ`, `GG`, `RD`); the UI is a pure function of it.
- Rebuild-and-`p.update()` for coarse changes; dyn-value `text(fn)` + tick for per-second faces.
- Anything tappable gets an explicit `.hit(w, h)` at finger scale (~72 px min).

---

## 11. Firmware architecture — owner, events, services

`0114-papers3-pulp-os/main/`, one file per concern:

- **`app_main.cpp`** — trampoline into the owner.
- **`app_owner.cpp`** — the owner task. Boot sequence:
  ```
  RuntimeInit → StorageConfigure{pre_mount=EnsureM5Init, seed=kobzar} → mount → seed
  → InputServiceInit → JsRunPulp (fallback: HomeShowNative) → TouchEnable
  ```
  Event loop: a queue of `pulp::Event` (`app_events.h` defines the contracts). Console requests arrive as `ConsoleOp::Js` with an integer arg: `0` status, `10` pulp, `11` tap (x,y packed), `12` swipe, `13` hits, `20+N` probe N (probes were moved to 20+ after arg 10 collided with pulp). Tick hooks run in the loop: `StorageFlushIfDue + JsTimerTick + PowerAutoTick`.
- **`app_console.cpp`** — registers console commands: `touch [on|off|status]`, `sd [mount|unmount|seed|status|reload|fault k m]`, `sleep [status|deep N|rtc-off N|off|auto N]`, `home`, `js [status|probe N|pulp|tap X Y|swipe K|hits]`. Console handlers **post events** and return; they never touch UI state.
- **`app_input.cpp`** — GT911 polling service; `InputSetGestureHandler` routes gestures to the owner → `JsHandleGesture`.
- **`app_power.cpp`** — sleep/wake; `PowerSetSleepImageBuilder` hook lets JS draw the sleep screen; quiesce sequence (stop touch, flush storage, build sleep image, deep sleep with RTC wake).
- **`app_home.cpp`** — native fallback home page (the JS-less brick guard).
- **`app_book_seed.h`** — the embedded seed book.

The one-owner rule in practice: workers and ISQ-adjacent code *never* call `PresentPage`, never touch the arena, never call into JS. They post events. ESP-53's completion-mailbox pattern (design-doc/01 §3) is this rule extended to async peripherals.

---

## 12. Console, serial discipline, and the validation harness

### 12.1 Serial discipline (learn this before you flash anything)

- Port (stable by-id path):
  `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00`
- **One reader owns the port.** Never `idf.py monitor`, never ad-hoc pyserial. Use the console client:
  ```bash
  python3 ttmp/2026/07/14/ESP-50-*/scripts/52-papers3-console-client.py \
      --settle 3 --cmd "js status" --output /tmp/out.log
  ```
- Boot takes ~7 s; commands sent immediately after flash are swallowed — retry with settle. "Unrecognized command" right after wake is the same effect.
- USB-Serial-JTAG **drops output when nobody reads**: if you want boot logs, the reader must already be attached.

### 12.2 The `js` command family

- `js status` — `init/screen_active/arena/evals/exceptions/dispatches/last_error` plus heap lines. Your first command after any flash.
- `js pulp` — (re)launch the JS product.
- `js tap X Y` / `js swipe K` — synthetic input, exact coordinates.
- `js hits` — dump the live hit-region table. Built because blind tap coordinates kept missing; now you look up the rect first.
- `js probe N` — the harness, probes 1–14: builder construction, containment, tap routing, tick behavior, services, regression variants (the trio that bisected the TTF bug), canvas ops, storage fault battery, and probe 14 = trace equivalence against the fake backend.

### 12.3 The validation ladder (use it in this order)

1. Host tests (`make run`, ASan/UBSan) — core logic.
2. `idf.py build` — compile gate.
3. Flash + `js status` — engine up, no exceptions.
4. `js probe N` for the touched area — behavior on device.
5. `js hits` + `js tap` — interaction, without touching the panel.
6. Soak (heap flat over minutes; `js status` heap lines before/after) — for anything long-running.

Long captures: the Bash tool kills at 10 min and blocks bare `sleep` chains — use `until ...; do sleep N; done` loops or nohup-detached background processes.

---

## 13. Where ESP-53 fits

Read `design-doc/01` next. In one paragraph: the ticket adds `wifi` (scan/join/remember/forget, credentials as a new s3paper_storage record type), `http` (bounded fetch, builder DSL with a terminal verb), `serve` (a tiny httpd whose handlers are JS routes, using a semaphore handoff into the owner with dual timeouts), `files` (general SD access), and `buzzer` (GPIO 21 LEDC, §2). Everything async follows the **completion-mailbox pattern**: a worker task fills a POD mailbox, posts a `ModuleDone` event, the owner calls `CallCb(kind, value, err)` with ints only, and the JS callback pulls strings through accessor functions. That is §7.2's int32-only boundary and §11's one-owner rule, applied to networking. Implementation order (phases): buzzer → files → wifi → http → serve → settings app → hardening; 19 tasks in `tasks.md`.

## 14. The gotcha catalog (bugs we actually hit — do not repeat them)

1. **TTF-only fonts measured as invalid** — `MeasureText`/`BreakLines` guarded only by the bitmap table (`GetFont(id) == nullptr`), so `kFontDisplay` returned `InvalidArgument`, text measured zero-width, rows clipped it away entirely. Symptom: a styled text vanishing (`ops=2` instead of 3) only when nested in a row. Found by bisecting probe variants, then host font repro. Fix: `(!IsTtfFont(font_id) && GetFont(font_id) == nullptr)`. Same defect class had already bitten the ESP-50 backend glyph path. *Lesson: when two font systems coexist, audit every guard.*
2. **Destroy of a linked child** — `Destroy` didn't unlink from a live parent; slot reuse created child-list cycles → infinite recursion → ASan stack overflow. Found by the ESP-52 fuzzer; latent since ESP-50. Fix: unlink via `RemoveChild` first. *Lesson: fuzz tree invariants, not just leaf ops.*
3. **Array holes are a TypeError** — resetting `__cbs = []` then writing index 1 blew up. Seed slot 0, and re-fetch the array after any allocating engine call.
4. **`JS_CLASS_COUNT` defined once, needed twice** — device *and* host compiler.
5. **Probe/pulp console-arg collision** — probe 10 and the pulp op both used arg 10; probes moved to `20+N`. *Lesson: enum your op encodings, don't ad-hoc them.*
6. **Boot race** — console commands < 7 s after reset are swallowed.
7. **`esp_psram` must be named** in `set(COMPONENTS ...)` or PSRAM is silently absent.
8. **Fill-then-erase on e-ink flashes** — draw only the pixels you want (concentric rings, not filled-minus-filled).
9. **Bare text is untappable** — always `.hit()` at finger scale.
10. **cwd drift** — automation shells lose `cd`; use absolute paths from the repo root.

## 15. Glossary

- **arena** — fixed-capacity POD store (widgets, frame ops, JS heap each have one).
- **atom** — interned identifier in MicroQuickJS; ROM atoms are baked at compile time.
- **damage** — the set of rects that changed between presents; drives partial updates.
- **dyn value** — a `text(fn)` node whose string is recomputed each tick.
- **ghosting** — residual image after partial EPD updates; cleared by full refresh.
- **kernel** — the 2-line RAM-evaluated JS (`__cbs`, `G`).
- **mailbox** — POD result struct filled by a worker, drained by the owner (ESP-53).
- **owner** — the single FreeRTOS task allowed to touch UI/JS state.
- **present** — one planned trip through layout → emit → diff → blit.
- **probe** — numbered on-device validation routine (`js probe N`).
- **seed book** — `kobzar.txt`, planted on first mount so the library is never empty.
- **stale handle** — a Widget/Page whose generation no longer matches the arena slot.

---

*Companion documents: `design-doc/01` (ESP-53 connectivity design), the ESP-51 and ESP-52 intern guides in their tickets, and three deep-dive project reports in the go-go-parc vault (`Projects/2026/07/16/`).*
