---
Title: PaperS3 E-Reader Primitives Analysis Design and Implementation Guide
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
RelatedFiles:
    - Path: abs:///home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp
      Note: Actual EPD framebuffer waveform queue and allocation implementation
    - Path: abs:///home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/main/hal/hal.cpp
      Note: Factory HAL patterns for SD power RTC input and board services
    - Path: repo://0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp
      Note: Bounded guest intent queue and replay precedent
    - Path: repo://0080-papers3-ereader/main/ereader_app.cpp
      Note: Existing native reader vertical slice and lifecycle evidence
    - Path: repo://0080-papers3-ereader/main/paginator.cpp
      Note: Existing page-offset and word-wrap baseline
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/local/s3paper-api-design.md
      Note: Original fluent API semantics and e-ink defaults
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/local/s3paper-studio.jsx
      Note: Executable builder-layout-draw-op prototype and gap evidence
ExternalSources:
    - https://docs.m5stack.com/en/core/PaperS3
    - https://github.com/m5stack/M5PaperS3-UserDemo
    - https://github.com/m5stack/M5GFX/issues/181
    - https://github.com/m5stack/M5GFX/issues/152
    - https://github.com/m5stack/M5GFX/releases/tag/0.2.25
    - https://github.com/bellard/mquickjs
    - https://github.com/atomic14/diy-esp32-epub-reader
Summary: Native-first architecture and phased implementation guide for a reliable PaperS3 e-reader whose measured display, input, text, storage, pagination, layout, refresh, and power primitives can later support the fluent s3paper API through MicroQuickJS.
LastUpdated: 2026-07-14T16:30:00-04:00
WhatFor: Intern onboarding and implementation sequencing for the next PaperS3 reader firmware, including toolchain qualification and future JavaScript bindings.
WhenToUse: Before creating or reviewing the native PaperS3 reader-primitives firmware, changing its EPD policy, or exposing its APIs to MicroQuickJS.
---


# PaperS3 E-Reader Primitives: Analysis, Design, and Implementation Guide

## 1. Executive summary

The long-term product idea is a fluent JavaScript API in which an author writes `paper().page(...).render()`, composes `row`, `col`, `text`, `list`, `book`, and `region` objects, and overrides e-ink policy with lambdas. The prototype communicates that idea well. It captures the right product-level opinions: paginate instead of smooth-scroll, isolate dynamic regions, defer distracting updates while the user is reading, and periodically clean ghosting.

The next implementation should **not** begin by embedding MicroQuickJS or by translating every fluent builder directly into C functions. It should first build and qualify the native primitives that the JavaScript layer will eventually call. Those primitives are the difficult and reusable part of the system: safe EPD transactions, refresh planning, geometry and clipping, touch normalization, timers, text measurement, line breaking, pagination, storage, locators, persistence, power transitions, layout, draw operations, and observability.

The recommended sequence is therefore:

1. qualify the exact PaperS3 toolchain and M5 library combination on hardware;
2. establish one native owner for UI state and display calls;
3. build small, deterministic, host-testable C++ services;
4. ship a complete native reader vertical slice;
5. generalize only proven behavior into retained widgets and regions;
6. run a bounded MicroQuickJS feasibility spike;
7. place the fluent JavaScript facade on top of a versioned primitive ABI.

This is not a rewrite from zero. The repository already contains useful experiments:

- `0075` proves low-latency touch drawing with clipped `epd_fast` updates.
- `0078` proves a retained node tree, layout pass, dirty rectangles, and waveform hints.
- `0080` proves a basic library, text pagination, bookmarks, touch page turns, and an e-reader console.
- `0079` proves a bounded guest-to-host command queue and a display-intent replay boundary.
- `M5PaperS3-UserDemo` exposes board HAL patterns and the actual M5GFX PaperS3 driver stack.

The new work should harvest these lessons without copying their accidental constraints. In particular, it must correct `0080`'s character-count pagination, uppercase-only 5x7 reading font, synchronous whole-book pre-pagination, page-number-only persistence, and unsynchronized console/UI mutations.

### Recommended deliverable

Create a new standalone firmware, provisionally named:

```text
0106-papers3-ereader-primitives/
```

The number is provisional; use the next free tutorial number at implementation time. Its core must remain native and testable without MicroQuickJS. The JavaScript runtime enters only after the native reader passes its own acceptance suite.

---

## 2. Problem statement and scope

### 2.1 Product goal

Build a useful, battery-aware e-ink reader for the M5Stack PaperS3 and simultaneously create the primitive substrate for the future `s3paper` JavaScript API.

The first shippable reader should support:

- a library of plain UTF-8 text books stored on microSD;
- deterministic text measurement, wrapping, and pagination;
- next/previous page touch zones;
- reading position, progress, bookmarks, and resume;
- a library screen and a reading screen;
- partial page turns plus measured ghosting cleanup;
- a USB Serial/JTAG diagnostic console;
- safe shutdown and resume behavior.

The architecture should later support:

- retained widget trees (`text`, `row`, `col`, `divider`, `progress`, `list`);
- routable pages and overlays;
- reactive/deferred regions;
- EPUB ingestion and richer typography;
- MicroQuickJS builders, callbacks, and policy functions.

### 2.2 Explicit non-goals for the first native vertical slice

Do not make Phase 1 through Phase 8 depend on:

- JavaScript execution;
- EPUB CSS conformance;
- arbitrary HTML layout;
- PDF rendering;
- smooth scrolling;
- network book stores;
- user-installed untrusted bytecode;
- a general animation framework;
- a complete Unicode shaping engine.

Those are future features. The primitive contracts should leave room for them, but the first reader should be small enough to test exhaustively.

### 2.3 Why native-first matters

A scripting layer amplifies whatever contracts exist underneath it. If display ownership, page identity, text metrics, update regions, and power transitions are ambiguous in C++, exposing them to JavaScript makes failures harder to localize. Conversely, a stable native API makes the JavaScript binding mostly a conversion and lifetime-management problem.

The user request is therefore architecturally sound: prove the substrate first, then add MicroQuickJS.

---

## 3. Hardware and software orientation for a new intern

### 3.1 PaperS3 hardware

The official PaperS3 documentation reports:

- ESP32-S3R8, dual-core Xtensa LX7 at up to 240 MHz;
- 8 MB PSRAM and 16 MB flash;
- 960 by 540, 4.7-inch direct-driven EPD with 16 grayscale levels;
- GT911 two-point capacitive touch;
- microSD on GPIO 47/39/38/40 (CS/SCK/MOSI/MISO);
- BM8563 RTC, BMI270 IMU, buzzer, battery ADC, USB detection;
- 1800 mAh battery;
- USB OTG/CDC/MSC/flashing.

See `sources/web/01-m5stack-papers3-hardware.md` for the captured official page. The factory HAL independently confirms SD on GPIO 47/39/38/40, battery ADC on GPIO 3, USB detection on GPIO 5, buzzer on GPIO 21, and the board-level power-off sequence in `../M5PaperS3-UserDemo/main/hal/hal.cpp:67-159` and `:166-292`.

The direct-driven display matters. The ESP32 does not send a high-level “draw page” command to a self-contained e-paper controller. M5GFX owns a PSRAM framebuffer and a background waveform task that repeatedly scans lines through an I80-style bus. Display corruption can therefore arise from geometry, allocation, cache synchronization, queueing, rotation, or waveform errors.

### 3.2 Display stack

The active stack is:

```text
application primitives
    |
    v
M5.Display (M5Unified)
    |
    v
M5GFX / LGFXBase
    |
    v
Panel_EPD                  Bus_EPD
4-bpp target framebuffer   scanline DMA and panel control pins
PSRAM step-state buffer
LUT-driven waveform task
    |
    v
ED047TC1 panel
```

The older EPDiy documentation is historical for current M5GFX. `sources/web/03-m5gfx-papers3-driver.md` states that EPDiy is no longer used from M5GFX 0.2.7 onward. The local EPD deep dive at `ttmp/2025/12/30/M5PS3-EPD-DRAWING--m5papers3-how-epd-drawing-works-library-pipeline/analysis/01-epd-drawing-pipeline-library-deep-dive.md` explains the in-tree `Panel_EPD` and `Bus_EPD` implementation.

### 3.3 What an EPD transaction means

The safe application pattern found throughout the working demos is:

```cpp
M5.Display.waitDisplay();
M5.Display.setEpdMode(epd_mode_t::epd_text);
M5.Display.startWrite();
// issue one coherent batch of draw calls
M5.Display.endWrite();
M5.Display.waitDisplay();
```

`startWrite()` and `endWrite()` are not cosmetic. They bound a coherent framebuffer update and let the panel's automatic display behavior enqueue the changed range. `waitDisplay()` is required at ownership transitions, before shutdown, and before starting operations that must not overlap a previous waveform.

### 3.4 EPD modes are policy inputs, not semantic promises

M5GFX exposes at least:

- `epd_quality`: slow, cleanup-oriented, grayscale-capable;
- `epd_text`: balanced text update;
- `epd_fast`: interactive black/white update with more residue;
- `epd_fastest`: highest speed and greatest afterimage risk.

The exact visual result depends on panel state, temperature, update geometry, driver version, and update history. Application code should request an **intent** such as `InteractiveInk`, `TextPage`, or `CleanFull`, while the refresh planner maps that intent to the qualified M5GFX mode.

### 3.5 Console transport

Use USB Serial/JTAG for the PaperS3 console, consistent with this repository's ESP32-S3 guidance and `0080-papers3-ereader/main/ereader_console.cpp`. Do not consume peripheral UART pins for normal console output. Keep the serial device single-owner during flash and probe work.

---

## 4. Toolchain and M5GFX investigation

### 4.1 What is actually pinned locally

The upstream factory demo currently says ESP-IDF 5.3.3 in `../M5PaperS3-UserDemo/README.md:13-20`. Its `repos.json` pins:

- M5GFX 0.2.15;
- M5Unified 0.2.10.

The checked-out local demo has deliberate drift:

- `.envrc` sources ESP-IDF 5.3.4;
- `dependencies.lock` is locally modified from 5.3.3 to 5.3.4;
- the local M5GFX branch has instrumentation and PaperS3 fixes not present in tag 0.2.15.

That means “the local factory demo works” is not evidence for a clean upstream pin unless the exact Git status and component commits are recorded.

### 4.2 What changed upstream

Current web/API research on 2026-07-14 found:

1. M5GFX 0.2.17 (released 2025-11-06) explicitly says it fixes PaperS3 operation under ESP-IDF 5.4. The underlying change disables open-drain data pins for ESP-IDF 5.4 and later.
2. M5GFX Issue 181 documents two `Panel_EPD` PSRAM heap-corruption bugs:
   - an odd-byte-width overrun in `task_update()`;
   - unrotated logical update rectangles reaching the physical buffer.
3. Commit `33f8ce25e96903bc8d11122de81147d8a5cca39b` fixes both Issue 181 defects.
4. M5GFX 0.2.25 (released 2026-07-09) contains that commit.
5. M5GFX Issue 152 documents erratic/inverted/smeared PaperS3 output around 0.2.12 and the later waveform corrections. The maintainer specifically warned that 0.2.11 applied excessive panel control and refreshed unchanged pixels unnecessarily.

The internet answer is therefore nuanced: **ESP-IDF 5.4 PaperS3 support has been fixed upstream, and newer M5GFX also fixes two independent heap-corruption defects.** This does not eliminate the need for a board qualification matrix, but it means ESP-IDF 5.3.3 should not be treated as an eternal architectural requirement.

### 4.3 Current remaining local concern

The local `0080` crash investigation found an additional allocation problem in M5GFX 0.2.15: a 43,520-byte `_lut_2pixel` table was forced into DMA-capable internal RAM even though CPU code, not DMA, reads it. The local fix changed this allocation to generic 8-bit-capable memory. Current upstream 0.2.25 still uses `MALLOC_CAP_DMA` for this table.

Do not silently carry that patch forever, and do not silently drop it. Phase 0 must answer whether a clean current M5GFX initializes reliably with the planned firmware's early allocation order. If not, retain the narrow patch with a local test and pursue it upstream.

### 4.4 Required qualification matrix

Treat combinations as experiments, not beliefs:

| Cell | ESP-IDF | M5GFX | M5Unified | Purpose |
|---|---|---|---|---|
| A | 5.3.3 | 0.2.15 | 0.2.10 | Exact upstream factory-demo control |
| B | 5.3.3 | 0.2.25 | 0.2.18 | New-driver conservative-toolchain candidate |
| C | 5.3.4 | 0.2.25 | 0.2.18 | Match this repository's recent S3 work |
| D | 5.4.2 | 0.2.25 | 0.2.18 | Verify upstream's documented 5.4 repair |

Versions B-D are candidates until built and tested on the real device. Do not modify the established 0079/0082 pin while qualifying a new firmware.

Each cell must run the same test corpus:

```text
1. boot and verify display_count == 1
2. full white -> full black -> full white
3. 16 grayscale bars
4. portrait and landscape full-range update
5. 2-pixel and 6-pixel-wide partial updates (Issue 181 boundary cases)
6. all four corners and edge-touching rectangles
7. 1,000 alternating text-region updates
8. mixed full/partial updates with heap integrity checks
9. waitDisplay -> sleep/power transition -> wake/reinitialize
10. capture photos, logs, component SHAs, heap metrics, and timings
```

**Exit rule:** choose the newest cell that passes the complete corpus repeatedly. Keep 5.3.3 as the fallback control until another cell passes; do not call a toolchain “fixed” based only on a release note.

---

## 5. What the JavaScript prototype gets right

The imported design in `sources/local/s3paper-api-design.md` establishes these valuable semantics:

- `paper` owns device/display policy;
- `page` is a routable full-screen unit;
- `region` is the unit of partial refresh and scheduling;
- `book` owns layout and pagination;
- widgets compose into trees;
- values may be static or dynamic;
- e-ink expertise is encoded as defaults, with policy escape hatches.

The browser studio makes the intended pipeline explicit at `sources/local/s3paper-studio.jsx:6-16`:

```text
builders -> plain widget tree -> layout pass -> flat draw-op list -> backend
```

That separation is the strongest reusable idea in the prototype. Native code should preserve it.

The design also makes good product choices:

- first render is full quality;
- reading lists paginate instead of scroll;
- page turns are normally partial;
- periodic full refresh is automatic;
- scheduled overlays can defer while the user is active;
- global and page-level gestures route through the app.

---

## 6. Prototype gaps that native work must resolve

The studio is a communication prototype, not a direct embedded implementation. An intern should understand the following gaps before porting it.

### 6.1 “Plain tree” is not yet plain

The file header says closures are resolved below the builder layer, but the actual nodes retain JavaScript functions:

- text values (`p.value`);
- icon bindings (`p.bind`);
- list item builders and comparators (`p.item`, `p.sort`);
- selection handlers (`p.onSelect`);
- page gesture handlers;
- refresh policy.

A native descriptor cannot safely contain raw JS closures. The future binding must keep callback references in the JS runtime and pass native data, IDs, and patches across the boundary.

### 6.2 Partial refresh is simulated, not region-correct

`layoutPage()` generates a full-page operation list (`s3paper-studio.jsx:350-380`). `doPartial()` then paints all supplied operations and adds a visual ghost overlay (`:437-451`). `layoutRegion()` handles only a text widget. The prototype does not yet compute exact changed pixels, retain previous native frames, or constrain a partial hardware update to a proven damage rectangle.

### 6.3 Ghosting is a placeholder metric

The studio computes:

```js
ghostingScore = min(1, turns * 0.06)
```

and defaults to a full refresh after six turns (`:406`). This is useful for demonstrating policy, but it is not a panel measurement. Native policy needs observed history: update mode, area, contrast transition, elapsed time, and optionally temperature.

### 6.4 Layout coverage is intentionally narrow

- rows only understand text, icon, and spacer;
- regions only understand text;
- text measurement uses browser Canvas fonts;
- book pagination approximates `1pt ~= 3px` and wraps by browser word widths;
- lists stop at the first non-fitting item but do not expose continuation state;
- the hit model stores closures in rectangles;
- full refresh is a timed black/white animation rather than a panel operation.

### 6.5 Syntax does not directly target MicroQuickJS

The prototype uses ES module imports, `const`/`let`, arrow functions, template strings, spread syntax, and React. MicroQuickJS describes itself as a stricter mostly-ES5 subset. Its current tests do not demonstrate arrow functions, classes, modules, or object spread. The fluent API shape is portable; the exact prototype source is not.

### 6.6 API typo to resolve before freezing a contract

The Markdown sample uses `ctx.turnsSincefull` while the studio uses `ctx.turnsSinceFull`. The native ABI and JavaScript facade must choose one spelling. Use `turnsSinceFull`.

---

## 7. Existing firmware: reusable evidence and limitations

### 7.1 `0075-papers3-touch-draw-demo`

Reusable lessons:

- initialize M5Unified, set landscape rotation, then derive geometry from `width()`/`height()`;
- `epd_fast` plus a clip rectangle gives responsive live ink;
- press/move/release state is more reliable than acting on every raw sample;
- interpolate between touch points so sparse sampling does not leave holes.

Do not reuse its direct-display-from-input-handler pattern as the general architecture. It is acceptable in a single-task demo, not in a multi-source reader runtime.

### 7.2 `0078-papers3-gnosis-layout`

Reusable lessons:

- bounded node pool;
- retained tree;
- recursive layout;
- dirty rectangle collection and merging;
- waveform hints;
- full refresh on screen switches.

Limitations:

- a node is a large tagged bag of fields rather than type-safe widget data;
- overflow in node/dirty arrays is mostly silent;
- fixed 5x7 uppercase-like font is unsuitable for books;
- direct console calls mutate the same app object used by the UI task without a queue or lock;
- the fixed “60 partial rectangles” cleanup policy ignores area and mode.

### 7.3 `0080-papers3-ereader`

This is the most valuable vertical-slice reference. It already contains:

- SPIFFS book index (`book_store.cpp:13-110`);
- chunk reads (`book_store.cpp:113-145`);
- forward page offset table (`paginator.cpp:100-122`);
- word-wrap formatter (`paginator.cpp:125-192`);
- bookmarks (`bookmark_store.cpp:55-108`);
- library and reading screens (`ereader_app.cpp:67-170`);
- touch page zones (`:305-362`);
- partial/full refresh (`:277-418`).

It should not become the new foundation unchanged:

1. **Pagination uses character counts, not glyph measurements.** `kCharsPerLine=100` and `kLinesPerPage=31` are disconnected from the actual font metrics (`ereader_app.cpp:16-18`).
2. **Reading typography is a 5x7 font.** Lowercase maps to uppercase and unsupported characters become spaces in `bitmap_font.cpp`.
3. **Book open can paginate the whole file synchronously.** `ComputeTotalPages()` loops toward `kMaxPageOffsets`, blocking interaction.
4. **Bookmark byte offsets are always stored as zero.** `NextPage()` and `PreviousPage()` call `UpdatePosition(..., 0, current_page_)`.
5. **A page number is not stable.** Changing font, margins, viewport, or line spacing invalidates it.
6. **Storage is only 512 KB SPIFFS.** The PaperS3 already has microSD and real books are larger.
7. **Mount may format on failure.** `format_if_mount_failed=true` is dangerous for user content.
8. **Console and UI mutate one singleton concurrently.** Console commands call `OpenBook()`, `GotoPage()`, and `ForceRefresh()` directly while the UI task loops.
9. **Dirty overflow is silent.** More than 32 leaf updates can be dropped.
10. **Refresh count is area-blind.** One tiny label and one full-page partial each count as one.
11. **No error-rich contracts.** Many file failures collapse to zero bytes or `false` without enough context.
12. **No atomic persistence.** Index/bookmark files are rewritten in place.

### 7.4 `0079` queued guest host API

`0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp:24-105` uses a fixed-capacity command queue. Guest calls record drawing intent, and `FlushWasmHostFrame()` replays it later (`:264-344`). This is the right boundary idea for future scripting: guest code describes work; the display owner performs it.

Do not copy its exact command set as the final reader ABI. Use it as evidence that bounded intent capture and replay are practical.

### 7.5 Factory demo

`M5PaperS3-UserDemo/main/main.cpp` provides hardware acceptance patterns:

- boot identity;
- full black/white transitions;
- 16 grayscale bars;
- periodic quality redraw;
- app-level refresh requests.

Its HAL is also the best local reference for SD, battery, power-off, RTC, Wi-Fi, buzzer, and touch.

---

## 8. Proposed native architecture

### 8.1 Layer diagram

```text
+------------------------------------------------------------------+
| Application                                                       |
| LibraryController | ReaderController | Router | Settings          |
+-------------------------------+----------------------------------+
                                |
                                v
+------------------------------------------------------------------+
| Reader domain                                                     |
| BookCatalog | ContentSource | Paginator | PositionStore | Cache   |
+-------------------------------+----------------------------------+
                                |
                                v
+------------------------------------------------------------------+
| UI model                                                          |
| Widget tree | Layout | Hit regions | Region dependencies          |
+-------------------------------+----------------------------------+
                                |
                                v
+------------------------------------------------------------------+
| Rendering                                                         |
| DrawOp list | DamageTracker | RefreshPlanner | DisplayService     |
+-------------------------------+----------------------------------+
                                |
                                v
+------------------------------------------------------------------+
| Platform adapter                                                  |
| M5Unified/M5GFX | GT911 | SD | RTC | battery | power | console    |
+------------------------------------------------------------------+

Future sidecar, after native acceptance:

+----------------------+      descriptors/events/patches
| MicroQuickJS runtime | <----------------------------------------->
| fluent s3paper facade|      versioned native primitive ABI
+----------------------+
```

### 8.2 Central ownership rule

Exactly one task owns:

- application state;
- widget trees and callback registries;
- page buffers and pagination cursors;
- all `M5.Display` calls;
- refresh planner state;
- sleep/power transitions.

Other producers send messages:

```text
USB console ----+
touch sampler ---+--> bounded AppEvent queue --> UI owner task
RTC/timers ------+
future JS VM ----+
storage worker --+
```

This removes the race currently present in `0078` and `0080` and gives deterministic ordering for rendering and persistence.

### 8.3 Suggested component layout

```text
0106-papers3-ereader-primitives/
├── CMakeLists.txt
├── sdkconfig.defaults
├── partitions.csv
├── main/
│   ├── app_main.cpp
│   ├── app_controller.cpp/.h
│   ├── app_events.cpp/.h
│   └── idf_component.yml
├── components/
│   ├── s3paper_core/             # no M5/ESP includes in public domain headers
│   │   ├── geometry.*
│   │   ├── status.*
│   │   ├── draw_ops.*
│   │   ├── layout.*
│   │   ├── damage_tracker.*
│   │   ├── refresh_policy.*
│   │   ├── input_events.*
│   │   ├── text_layout.*
│   │   ├── paginator.*
│   │   └── reader_model.*
│   ├── s3paper_storage/
│   │   ├── content_source.*
│   │   ├── sd_catalog.*
│   │   ├── position_store.*
│   │   └── page_cache.*
│   ├── s3paper_m5/
│   │   ├── m5_display_backend.*
│   │   ├── m5_input_backend.*
│   │   ├── m5_power_backend.*
│   │   └── board_diagnostics.*
│   └── s3paper_js/               # absent until Phase 11/12
├── host-tests/
│   ├── fake_display.*
│   ├── fixtures/
│   └── *_test.cpp
└── test-assets/
    ├── utf8/
    ├── books/
    └── refresh-patterns/
```

The `main/idf_component.yml` rule in this repository must be followed; a project-root manifest is ignored by ESP-IDF.

---

## 9. Primitive contracts

### 9.1 Errors and results

Avoid silent booleans for operations that can fail for different reasons.

```cpp
enum class StatusCode : uint8_t {
    Ok,
    InvalidArgument,
    OutOfRange,
    CapacityExceeded,
    NotFound,
    IoError,
    CorruptData,
    Unsupported,
    Busy,
    Timeout,
    OutOfMemory,
    InternalError,
};

struct Status {
    StatusCode code;
    const char* subsystem;   // static string
    int32_t detail;          // errno/esp_err_t/domain value
};

template <typename T>
struct Result {
    T value;
    Status status;
    explicit operator bool() const { return status.code == StatusCode::Ok; }
};
```

Do not expose `std::string` or exceptions in the eventual C ABI. Native internals may use carefully bounded C++ containers where appropriate.

### 9.2 Geometry

Use half-open rectangles: `[x, x+w) x [y, y+h)`. Perform addition in 32 or 64 bits before narrowing.

```cpp
struct Size { int32_t w, h; };
struct Point { int32_t x, y; };
struct Rect { int32_t x, y, w, h; };

Result<Rect> Intersect(Rect a, Rect b);
Result<Rect> Union(Rect a, Rect b);
Result<Rect> ClampTo(Rect r, Size bounds);
bool Contains(Rect r, Point p);
Rect AlignDamageForEpd(Rect r, Size bounds);
```

`AlignDamageForEpd()` must encode the measured driver alignment contract. Issue 181 proves that tiny/odd update widths are not an academic edge case. Tests must cover negative input, overflow, zero area, every edge, rotation, and widths 1 through 16.

### 9.3 Flat draw operations

The render core emits plain operations. They contain no closures, M5 objects, or borrowed transient strings.

```cpp
enum class DrawOpKind : uint8_t {
    FillRect,
    StrokeRect,
    HLine,
    VLine,
    GlyphRun,
    Bitmap,
};

struct GlyphRunRef {
    FontId font;
    TextRunId run;
    Point baseline;
    Gray4 color;
};

struct DrawOp {
    DrawOpKind kind;
    Rect bounds;
    union Payload { /* POD payloads or stable arena offsets */ } payload;
};

struct RenderFrame {
    Span<const DrawOp> ops;
    Span<const Rect> damage_hints;
    FrameId id;
};
```

Text bytes and glyph arrays should live in a frame arena whose lifetime lasts through presentation. Never leave `const char*` fields pointing into a temporary JavaScript string or an SD read buffer.

### 9.4 Display backend

```cpp
enum class PresentIntent : uint8_t {
    InteractiveInk,
    TextRegion,
    TextPage,
    ImageQuality,
    CleanFull,
};

struct PresentRequest {
    PresentIntent intent;
    Span<const Rect> damage;
    FrameId frame;
    const char* reason;
};

struct PresentResult {
    Status status;
    uint32_t queue_wait_ms;
    uint32_t render_ms;
    uint32_t panel_busy_ms;
    Rect physical_damage;
    uint32_t partials_since_full;
    uint32_t ghost_budget;
};

class DisplayBackend {
public:
    virtual Result<Size> Begin() = 0;
    virtual Status Draw(Span<const DrawOp> ops, Rect clip) = 0;
    virtual Result<PresentResult> Present(const PresentRequest&) = 0;
    virtual Status WaitIdle(uint32_t timeout_ms) = 0;
    virtual Status Sleep() = 0;
};
```

Only `s3paper_m5` translates `PresentIntent` to `epd_mode_t` and calls M5GFX.

### 9.5 Refresh planner

The planner owns display history. Widgets may provide intent and damage, but they do not directly select `epd_quality`.

```cpp
struct RefreshContext {
    PresentIntent requested;
    uint32_t turns_since_full;
    uint32_t partial_area_since_full;
    uint32_t high_contrast_area_since_full;
    uint32_t elapsed_since_full_ms;
    int32_t temperature_milli_c;  // optional/unknown sentinel
    bool screen_changed;
    bool wake_resume;
};

struct RefreshPlan {
    EpdWaveform waveform;
    SmallVector<Rect, 8> regions;
    bool clear_before_draw;
    bool wait_after;
    RefreshReason reason;
};

RefreshPlan PlanRefresh(const RefreshContext&, Span<const Rect> damage);
```

An initial policy may use a page-turn count, but log enough data to replace it with evidence. A “ghosting score” is an estimate, not a sensor value.

### 9.6 Damage tracking and region dependencies

The future `region(text(() => clock...))` feature requires an explicit dependency model:

```cpp
using DependencyId = uint32_t;
using RegionId = uint32_t;

struct RegionSpec {
    RegionId id;
    Rect bounds;
    Span<const DependencyId> dependencies;
    uint32_t interval_ms;      // zero means event-only
    bool quiet_while_active;
};

void Invalidate(DependencyId changed);
Span<const RegionId> TakeInvalidRegions();
```

Dynamic JS values will map to dependencies and callback IDs. Native code remains callback-agnostic.

### 9.7 Input events and gestures

```cpp
enum class PointerPhase : uint8_t { Down, Move, Up, Cancel };

struct PointerEvent {
    PointerPhase phase;
    uint8_t pointer_id;
    Point logical;
    Point physical;
    int64_t time_us;
};

enum class GestureKind : uint8_t {
    Tap, LongPress, SwipeUp, SwipeDown, SwipeLeft, SwipeRight,
};

struct GestureEvent {
    GestureKind kind;
    Point start;
    Point end;
    int64_t duration_us;
};
```

The input backend transforms physical GT911 coordinates into the current logical orientation. A gesture recognizer consumes normalized events. Hit testing occurs against immutable layout output for the current frame.

### 9.8 Timers and quiet scheduling

Use one monotonic scheduler. Do not create one FreeRTOS timer per widget.

```text
on timer due(region):
    if region.quiet and now - last_input_time < quiet_window:
        reschedule at last_input_time + quiet_window
    else:
        emit AppEvent::RegionDue(region.id)
```

Timers only request model evaluation. They never draw directly.

---

## 10. Text and pagination architecture

### 10.1 Why text is a separate subsystem

A reader page is not a string copied into a rectangle. It is the result of decoding, fallback, measurement, line breaking, paragraph policy, and viewport constraints. Pagination and rendering must share the same measurements or page breaks will drift.

### 10.2 Text pipeline

```text
ContentSource bytes
    |
    v
incremental UTF-8 decoder
    |
    v
paragraph/block stream
    |
    v
style resolution + font fallback
    |
    v
glyph measurement / shaping-lite
    |
    v
line breaker
    |
    v
page composer
    |
    +--> PageLayout (glyph runs, bounds, locator range)
    |
    +--> renderer draw operations
```

### 10.3 First typography milestone

Phase 5 should provide:

- valid UTF-8 decoding with replacement for malformed sequences;
- code-point-safe truncation;
- proportional glyph advances from the actual rendered font;
- regular, bold, italic, and bold-italic style slots if storage permits;
- ASCII/Latin baseline plus explicit fallback behavior;
- paragraph spacing, first-line indent, margins, line height, and alignment;
- no split inside a UTF-8 sequence;
- deterministic host fixtures.

Do not claim full Unicode shaping. Arabic joining, Indic shaping, bidirectional text, grapheme-aware selection, and CJK line-breaking require a later explicit design.

### 10.4 Font choice

The `0080` 5x7 font is useful for diagnostics but unsuitable for sustained reading. Evaluate M5GFX font APIs and a packed bitmap font format with:

- glyph metrics accessible without rendering;
- PSRAM/flash/SD storage options;
- deterministic rasterization;
- an efficient black/white mode for text updates;
- optional grayscale antialiasing for quality mode.

The renderer and paginator must call the same `FontMetrics` interface:

```cpp
class FontMetrics {
public:
    Result<GlyphMetrics> Measure(FontId, uint32_t codepoint) const;
    Result<GlyphBitmap> LoadGlyph(FontId, GlyphId) const;
    int32_t LineHeight(FontId) const;
};
```

### 10.5 Stable locators

Persist a content locator, not merely a page number:

```cpp
struct TextLocator {
    BookId book;
    uint32_t spine_index;       // zero for plain TXT
    uint64_t byte_offset;
    uint32_t codepoint_offset;  // optional disambiguation
    uint64_t context_hash;      // nearby text for recovery after edits
};
```

A page is:

```cpp
struct PageLayout {
    TextLocator start;
    TextLocator end;
    SmallVector<LineLayout, 48> lines;
    uint32_t page_index_hint;
};
```

Page indices are derived cache data. Locators survive font and margin changes.

### 10.6 Pagination cache key

```cpp
struct LayoutKey {
    ContentHash content;
    FontId font;
    int32_t font_px;
    int32_t line_height_px;
    Insets margins;
    Size viewport;
    HyphenationMode hyphenation;
    TextAlign alignment;
    uint32_t engine_version;
};
```

Any key change invalidates page boundaries. Persist the key beside cached offsets.

### 10.7 Streaming paginator API

```cpp
class Paginator {
public:
    Result<PageLayout> PageFrom(TextLocator start, const LayoutKey&);
    Result<PageLayout> Next(const PageLayout&, const LayoutKey&);
    Result<PageLayout> Previous(const PageLayout&, const LayoutKey&);
    Result<TextLocator> SeekProgress(float fraction, const LayoutKey&);
    Result<PageCountEstimate> EstimateTotal(const LayoutKey&);
};
```

`Previous()` may use a sparse checkpoint cache plus bounded backward scanning. Do not require full-book pagination before opening page one.

### 10.8 Plain text before EPUB

Plain UTF-8 text is the first content adapter. It validates typography, locators, and caching without ZIP/XML/CSS complexity.

A later EPUB adapter should follow the proven embedded pattern captured in `sources/web/07-diy-esp32-epub-reader.md`:

```text
EPUB ZIP
 -> META-INF/container.xml
 -> OPF metadata + manifest + spine
 -> XHTML blocks
 -> styled text/image blocks
 -> common page composer
```

The modern CrossInk architecture in `sources/web/09-crossink-architecture.md` reinforces the importance of SD-backed metadata, CSS, and per-section layout caches keyed by typography settings.

---

## 11. Storage, catalog, and persistence

### 11.1 SD-first content

Use microSD for books and derived caches. Reserve internal flash/NVS or a small wear-aware filesystem for settings and critical resume metadata.

```cpp
class ContentSource {
public:
    virtual Result<uint64_t> Size() const = 0;
    virtual Result<size_t> ReadAt(uint64_t offset, Span<uint8_t> dst) = 0;
    virtual Result<ContentHash> Hash() = 0;
};
```

Adapters:

- `SdTextSource` first;
- `EmbeddedTextSource` for test fixtures;
- `EpubSource` later.

### 11.2 Library catalog

Do not use delimiter-separated metadata as the long-term on-device database. Use a versioned binary or carefully validated JSON record with explicit lengths and checksums.

```cpp
struct BookRecord {
    BookId id;                 // derived from path + size/hash, not list index
    ContentKind kind;
    FixedString<160> path;
    FixedString<96> title;
    FixedString<64> author;
    uint64_t byte_size;
    ContentHash hash;
    TextLocator last_position;
    int64_t last_opened_unix;
};
```

The UI uses `BookId`, never a transient array index.

### 11.3 Atomic state updates

Use write-new, flush, rename, and backup where the filesystem semantics permit:

```text
serialize state -> state.tmp
fsync/close state.tmp
rename state.bin -> state.bak
rename state.tmp -> state.bin
validate state.bin
remove state.bak later
```

A page turn updates in-memory position immediately. Persistence is coalesced by time/turn count and forced on explicit sleep/power-off.

### 11.4 Failure behavior

- Never format a card or user filesystem automatically after a mount failure.
- Preserve the current page if persistence fails; show a diagnostic state.
- Validate versions, lengths, checksums, and UTF-8 before trusting files.
- Put derived page caches under a disposable cache directory.
- Treat catalog and position data as reconstructable or backed up.

---

## 12. Native application flow

### 12.1 Boot

```text
app_main
  -> initialize diagnostics and event queue
  -> initialize M5 board/display before large competing allocations
  -> verify display_count and report heap blocks
  -> mount settings and SD without formatting
  -> load catalog and last session
  -> construct native reader controller
  -> build initial screen model
  -> layout -> draw ops -> CleanFull present
  -> enter UI owner event loop
```

### 12.2 Page turn

```text
GestureEvent(TapRight)
  -> ReaderController::NextPage()
  -> Paginator::Next(current_page, layout_key)
  -> update ReaderState.current_page/current_locator
  -> invalidate body, folio, progress dependencies
  -> rebuild only invalid widget values
  -> layout affected tree or reuse stable geometry
  -> diff previous/new render output
  -> RefreshPlanner(TextPage, damage, history)
  -> DisplayService presents one coherent batch
  -> update metrics and schedule position persistence
```

### 12.3 Quiet clock region

```text
minute timer expires
  -> scheduler checks last_input_time
  -> if active: defer
  -> else AppEvent::RegionDue(clock)
  -> resolve clock value
  -> invalidate clock dependency
  -> render clock region
  -> planner decides whether tiny update is worth presenting now
```

### 12.4 Console command

```text
console task parses: reader goto 120
  -> enqueue AppCommand::GotoPageHint(120)
  -> UI task validates and applies command
  -> response is posted to a reply queue
  -> console prints result
```

No console callback calls `M5.Display` or mutates the reader singleton directly.

---

## 13. Future MicroQuickJS integration

### 13.1 Why MicroQuickJS is promising

The captured upstream README reports:

- execution with as little as 10 KB RAM;
- roughly 100 KB ROM including C library on ARM Thumb-2;
- a caller-provided fixed memory arena;
- persistent bytecode that can execute from ROM after relocation;
- a C API similar to QuickJS;
- strings stored internally as UTF-8/WTF-8;
- a generated standard library that resides mostly in ROM.

These properties fit an embedded UI runtime better than a large general-purpose JS engine.

### 13.2 Important constraints

1. **Mostly ES5 stricter syntax.** The React studio source cannot be copied unchanged.
2. **Compacting GC.** JS objects can move on any allocating API call. Native bindings must use `JSGCRef`/`JS_PushGCRef()` and must not retain raw `JSValue` addresses incorrectly.
3. **Fixed memory arena.** Choose an explicit script budget and test exhaustion.
4. **Bytecode is unstable and unverified.** Upstream guarantees no backward compatibility; only trusted build-produced bytecode may run.
5. **C functions and user objects require careful lifetime design.** Finalizers must release native handles without touching destroyed services.
6. **Execution limits are not yet an established product contract.** The feasibility spike must prove timeout/cancellation or constrain scripts to short event handlers.

### 13.3 Binding boundary

Do not bind every M5GFX call. Bind the stable native model:

```c
// Conceptual C ABI, not final syntax.
s3_status s3_text_create(s3_runtime*, s3_string_view, s3_widget_handle* out);
s3_status s3_row_create(s3_runtime*, const s3_widget_handle*, size_t, s3_widget_handle* out);
s3_status s3_widget_set_i32(s3_runtime*, s3_widget_handle, s3_prop_id, int32_t);
s3_status s3_page_create(s3_runtime*, s3_string_view name, s3_page_handle* out);
s3_status s3_page_set_slot(s3_runtime*, s3_page_handle, s3_slot_id, s3_widget_handle);
s3_status s3_app_dispatch(s3_runtime*, const s3_event*);
```

Opaque handles carry generation counters so stale JavaScript wrappers fail cleanly after a page/tree is destroyed.

### 13.4 Callback model

JavaScript owns callbacks. Native code stores only a callback ID and emits an event:

```text
native GestureEvent + callback_id
    -> JS event dispatcher
    -> call rooted JS function with plain event object
    -> JS mutates builder/model facade
    -> facade emits bounded native patches
    -> native validates patches and renders
```

A dynamic value is represented by:

```cpp
struct DynamicBinding {
    CallbackId evaluator;
    DependencyId output;
    RegionId owner;
};
```

The JS facade invokes evaluators when dependencies invalidate; native layout never calls arbitrary JS while inside an M5GFX transaction.

### 13.5 Fluent facade

The fluent surface can remain close to the proposal even if its implementation is ES5-compatible:

```js
var title = s3.text("Hello, ink.").size("xl").center();
s3.paper().page(s3.page("home").content(title)).render();
```

If arrows/modules are desired for authoring, transpile on the host into the supported subset and test the emitted script with the exact MicroQuickJS commit. Do not put a transpiler on the device.

### 13.6 Acceptance mapping

The four studio presets become cross-backend contract tests:

| Preset | Native primitives it proves |
|---|---|
| Hello | builder/tree creation, text measure, layout, full first present |
| Status | row/spacer/icon, dynamic value, scheduled quiet region, partial damage |
| Library | list composition, sorting outside native layout, hit region, route push/back |
| Reader | pagination, left/right gestures, page state, refresh policy, folio |

Run each first as a native C++ fixture, then as a MicroQuickJS script. Compare normalized draw-op traces, not framebuffer pixels alone.

---

## 14. Decision records

### Decision: Native vertical slice before JavaScript

- **Context:** The fluent API depends on display, text, storage, pagination, input, and power contracts that are not yet stable.
- **Options considered:** Bind the prototype immediately; build generic UI first; build a native reader slice first.
- **Decision:** Build and ship the native reader slice before embedding MicroQuickJS.
- **Rationale:** It keeps hardware failures attributable and lets the JS binding target proven contracts.
- **Consequences:** The fluent demo arrives later, but its implementation risk is much lower.
- **Status:** accepted

### Decision: Qualify a matrix instead of permanently freezing ESP-IDF 5.3.3

- **Context:** 5.3.3 is the factory-demo control, while upstream M5GFX 0.2.17 claims PaperS3 support on 5.4 and 0.2.25 contains later EPD safety fixes.
- **Options considered:** Stay forever on 5.3.3; immediately upgrade; run controlled A/B qualification.
- **Decision:** Keep 5.3.3 as the control and promote a newer combination only after the common hardware corpus passes.
- **Rationale:** This preserves waveform safety without ignoring upstream fixes.
- **Consequences:** Phase 0 is mandatory and produces a reproducible pin.
- **Status:** proposed

### Decision: Single UI/display owner task

- **Context:** Existing console and UI tasks can concurrently mutate one application object and call display-facing methods.
- **Options considered:** Global mutex; direct cross-task calls; message-based single ownership.
- **Decision:** Route console, touch, timers, storage completions, and future JS events through one bounded queue.
- **Rationale:** Deterministic ordering is more valuable than nominal parallelism for an EPD reader.
- **Consequences:** Long storage work must be chunked or delegated, with results posted back.
- **Status:** proposed

### Decision: Draw operations separate layout from hardware

- **Context:** The studio's best architectural idea is tree -> layout -> flat operations -> backend.
- **Options considered:** Widgets draw directly; immediate-mode M5 calls; flat POD operations.
- **Decision:** Layout emits bounded draw operations consumed by fake and M5 backends.
- **Rationale:** Enables host tests, JS-independent behavior, trace comparison, and centralized clipping.
- **Consequences:** Frame arena lifetime and capacity errors become explicit contracts.
- **Status:** proposed

### Decision: Refresh policy is centralized

- **Context:** Widgets know semantic intent but do not know accumulated panel history.
- **Options considered:** Widgets choose EPD modes; app chooses per screen; planner chooses from intent/history.
- **Decision:** Widgets emit damage and intent; one planner maps to qualified waveform/regions.
- **Rationale:** Prevents incompatible local policies and makes ghosting tests observable.
- **Consequences:** Policy context and present metrics must be persisted in memory and logged.
- **Status:** proposed

### Decision: Persist content locators, not page numbers

- **Context:** Page numbers change with typography and viewport settings.
- **Options considered:** Page number; byte offset; structured locator plus context hash.
- **Decision:** Persist a stable locator and treat page index as cache data.
- **Rationale:** Resume remains meaningful after settings changes.
- **Consequences:** Paginator and content adapters must expose locator mappings.
- **Status:** proposed

### Decision: SD-first books, internal critical state

- **Context:** 512 KB SPIFFS cannot hold a useful library, while PaperS3 includes microSD.
- **Options considered:** SPIFFS only; SD only; SD content plus internal critical metadata.
- **Decision:** Store books/caches on SD and keep minimal settings/resume redundancy internally.
- **Rationale:** Capacity, recoverability, and removable content all improve.
- **Consequences:** Card removal and mount failure become normal states the UI handles.
- **Status:** proposed

### Decision: Plain text first, EPUB adapter later

- **Context:** EPUB adds ZIP, XML, XHTML, CSS, images, and cache invalidation before core pagination is proven.
- **Options considered:** EPUB-first; TXT-only forever; common compositor with TXT first.
- **Decision:** Prove the common text/page pipeline with TXT, then add EPUB as a content adapter.
- **Rationale:** Reduces early variables without blocking the long-term reader.
- **Consequences:** First milestone has limited formatting.
- **Status:** proposed

### Decision: JavaScript callbacks never execute inside a display transaction

- **Context:** MicroQuickJS can allocate, collect, throw, or run too long; M5GFX transactions need bounded deterministic work.
- **Options considered:** Native layout calls JS lazily; JS emits immediate M5 calls; evaluate JS before render and submit data patches.
- **Decision:** Resolve callbacks at event/model-update time, freeze native render input, then transact with the display.
- **Rationale:** Protects panel state and makes failure recovery possible.
- **Consequences:** Dynamic values need explicit dependency IDs and callback registries.
- **Status:** proposed

---

## 15. Phased implementation plan

Each phase has its own exit gate. Do not begin a later phase because the previous one “mostly works.”

### Phase 0: Hardware, toolchain, and driver qualification

**Objective:** choose a reproducible baseline and preserve evidence.

**Work:**

- create a minimal PaperS3 qualification firmware;
- run matrix A-D from Section 4;
- capture component SHAs, `sdkconfig.defaults`, heap metrics, timings, serial logs, and photos;
- exercise Issue 181 geometries and the local LUT allocation condition;
- decide whether the local rotation/LUT patches are needed against M5GFX 0.2.25.

**Exit gate:** one clean pin passes repeated boot, geometry, grayscale, long partial-update, sleep, and heap-integrity tests. The pin and any patches are committed and explained.

### Phase 1: Scaffold and ownership model

**Objective:** create the new firmware with deterministic event ownership.

**Work:**

- create the project and USB Serial/JTAG REPL;
- initialize the display before large application allocations;
- add bounded `AppEvent` and reply queues;
- run all app/display methods on one UI task;
- make console commands post messages;
- add `status`, `heap`, `display`, and `events` diagnostics.

**Exit gate:** simultaneous scripted console traffic and touch cannot cause direct cross-task state mutation; queue overflow produces an explicit error.

### Phase 2: Geometry, draw operations, and fake backend

**Objective:** establish host-testable rendering primitives.

**Work:**

- implement safe half-open rectangles and EPD alignment;
- implement typed draw operations and frame arena;
- implement clipping and capacity errors;
- implement fake backend that records normalized operations;
- implement M5 backend transaction shell without policy sophistication.

**Exit gate:** host tests cover overflow and edge geometry; the same fixture produces an expected trace and a visible hardware frame.

### Phase 3: Refresh planner and visual qualification

**Objective:** make every panel update deliberate, measured, and reproducible.

**Work:**

- add damage merge/alignment;
- map semantic intents to qualified M5 modes;
- log update area, mode, busy time, history, and cleanup reason;
- implement full-clean triggers by screen switch, wake, budget, and explicit request;
- build the long-run ghosting/photo corpus.

**Exit gate:** 10,000 mixed bounded updates complete without heap corruption; the planner explains every full refresh; visual artifacts remain within an operator-approved baseline.

### Phase 4: Input, hit testing, gestures, and scheduler

**Objective:** produce stable logical events independent of raw hardware polling.

**Work:**

- normalize rotation and pointer phases;
- implement tap, long press, and cardinal swipes;
- generate hit regions from layout output;
- implement one monotonic timer scheduler;
- add input-idle and quiet-region deferral;
- test touch cancel and screen-switch edge cases.

**Exit gate:** recorded touch traces replay identically on host; page-zone taps do not double-fire; timer callbacks never draw directly.

### Phase 5: Fonts and deterministic text layout

**Objective:** render readable mixed-case UTF-8 text using measurements shared with pagination.

**Work:**

- select and package a reader font;
- implement UTF-8 decoder and fallback;
- expose glyph advances and line metrics;
- implement paragraph and line breaking;
- emit glyph-run draw operations;
- create fixtures for punctuation, long words, malformed UTF-8, and multilingual fallback.

**Exit gate:** host golden layouts and hardware output agree on line breaks; no break bisects UTF-8; lowercase and punctuation render correctly.

### Phase 6: Content, catalog, settings, and positions

**Objective:** establish safe storage contracts.

**Work:**

- implement read-only SD text source;
- scan/build versioned catalog with stable `BookId`;
- persist settings and structured locators atomically;
- handle missing/unmounted/reinserted cards;
- expose console import/list/verify diagnostics;
- never auto-format user storage.

**Exit gate:** power interruption during a test write leaves either old or new valid state; removed SD produces a recoverable UI state.

### Phase 7: Streaming pagination and caches

**Objective:** navigate large books without full pre-pagination.

**Work:**

- paginate from locators using actual glyph metrics;
- add sparse checkpoints and page cache keyed by `LayoutKey`;
- implement bounded next/previous;
- estimate progress without blocking open;
- invalidate caches on content or typography changes;
- persist disposable checkpoints to SD.

**Exit gate:** first page opens without scanning the full book; previous/next round trips preserve locators; changing font invalidates and recovers position correctly.

### Phase 8: Complete native reader vertical slice

**Objective:** deliver a useful reader before generic UI or JS work.

**Work:**

- library screen with title/author/progress;
- reading screen with body, folio, progress, and library action;
- touch page zones and bookmark action;
- resume last book/location;
- refresh behavior from Phase 3;
- empty/error/loading states;
- operator console controls through messages.

**Exit gate:** an intern can copy a TXT book to SD, open it, read forward/back, power-cycle, and resume without a script runtime.

### Phase 9: Generalize into widgets, pages, and regions

**Objective:** extract only the abstractions proven by the native reader.

**Work:**

- retained widget arena and generation-safe handles;
- `text`, `row`, `col`, `spacer`, `divider`, `progress`, `list`, and `book` nodes;
- measured layout to draw-op compilation;
- routable `page` objects and overlay `region` specs;
- dependency invalidation and structural diffing;
- native fixtures matching the studio presets.

**Exit gate:** native hello/status/library/reader fixtures produce stable traces and the real native reader runs on the generic tree without regression.

### Phase 10: Power and resume

**Objective:** make battery behavior a coordinated lifecycle, not an ad-hoc `powerOff()` call.

**Work:**

- inactivity policy and explicit sleep request;
- flush position/catalog state;
- wait for display idle and choose retained sleep image;
- quiesce SD and timers;
- configure verified RTC/button wake sources;
- restore display, card, app state, and refresh history safely.

**Exit gate:** repeated sleep/wake and low-battery shutdown tests preserve state and do not leave an in-flight EPD operation.

### Phase 11: Bounded MicroQuickJS feasibility spike

**Objective:** answer integration questions without contaminating the native reader.

**Work:**

- pin exact MicroQuickJS commit;
- cross-compile on ESP32-S3;
- instantiate fixed arenas at several budgets;
- bind one diagnostic function and one opaque widget handle;
- test GC-moving-object discipline with rooted references;
- test source and trusted relocated bytecode;
- characterize syntax/transpilation needs;
- measure startup, event callback, memory exhaustion, exception, and runaway-script behavior.

**Stop rule:** if bounded execution/cancellation or C API safety cannot be demonstrated in a short ticket, preserve the native ABI and postpone scripting rather than weakening display safety.

### Phase 12: Fluent `s3paper` JavaScript facade

**Objective:** reproduce the product API over stable primitives.

**Work:**

- generate/register the minimal native standard library;
- implement ES5-compatible fluent wrappers;
- root callback registry and generation-safe wrappers;
- map lambdas to dependency/callback IDs;
- route gestures/timers through JS before native rendering;
- port four studio acceptance programs;
- add host build/transpile/bytecode pipeline.

**Exit gate:** native and JS fixtures produce equivalent normalized draw-op and refresh-plan traces.

### Phase 13: Hardening and documentation

**Objective:** convert a demo into maintainable firmware.

**Work:**

- malformed content and storage fuzz cases;
- long-run partial/full refresh and heap checks;
- queue saturation and cancellation;
- power-loss matrix;
- script memory/exception tests;
- latency and battery measurements;
- component/license inventory;
- intern guide, operator playbook, and architecture decisions updated from measured outcomes.

**Exit gate:** `docmgr doctor`, host tests, firmware build, hardware acceptance, and review checklist pass on the final pin.

---

## 16. Testing strategy

### 16.1 Host tests

Host tests are mandatory for pure components:

- rectangle arithmetic and clipping;
- damage alignment and merge policy;
- draw-op arena capacity;
- widget layout;
- UTF-8 decoding and line breaks;
- pagination locators and cache invalidation;
- catalog/state serialization;
- gesture recognition from recorded traces;
- refresh planning from synthetic histories;
- native-vs-JS normalized traces later.

A fake display records:

```text
frame id
clip rectangle
ordered draw operations
present intent
selected plan
planner history after present
```

### 16.2 Hardware tests

Hardware is required for:

- waveform quality and ghosting;
- actual rotation/touch mapping;
- PSRAM/DMA allocation;
- panel busy timing;
- sleep/wake and power behavior;
- SD electrical behavior;
- battery usage.

### 16.3 Visual corpus

Keep fixed pages and photograph them under consistent conditions:

- black/white checkerboard;
- 16 gray bars;
- small black text on white;
- white text on black;
- repeated folio update;
- alternating page pairs with large contrast changes;
- tiny odd/even-width rectangles;
- corner/edge patterns;
- mixed text and grayscale image.

The test manifest records toolchain, M5GFX/M5Unified SHAs, board revision, temperature if available, mode, update count, and elapsed time.

### 16.4 Suggested initial performance gates

These are starting targets to measure and revise, not fabricated guarantees:

- no dropped/corrupt app events under defined queue load;
- no heap-integrity failure during 10,000 mixed updates;
- first TXT page appears without whole-book pagination;
- page turn does not block input processing for unbounded storage work;
- every present emits timing and reason metrics;
- no display call originates outside the owner task;
- persisted locator survives typography changes through recovery logic.

---

## 17. Risks and mitigations

| Risk | Why it matters | Mitigation |
|---|---|---|
| Driver/toolchain regressions | EPD output can smear, overdrive, or corrupt PSRAM | Phase 0 matrix, exact pins, visual corpus, Issue 181 tests |
| LUT internal allocation failure | Display may fail to register under fragmented DMA heap | initialize early, report largest blocks, retain narrow patch only if proven |
| Cross-task display/state access | Rare corruption and non-reproducible UI state | single owner + bounded message queue |
| Page identity drift | Resume points break after font/margin changes | locators + context hash; page index is cache only |
| Text/render metric mismatch | Different line breaks during pagination and rendering | one shared `FontMetrics`/line layout result |
| SD removal or corruption | Reader loses content mid-session | explicit mount state, recoverable errors, disposable caches |
| Excessive flash wear | page-turn persistence rewrites frequently | coalesced atomic writes and forced shutdown flush |
| Ghosting heuristic masquerades as measurement | policy looks clever but is wrong | log area/mode/history; operator visual baseline |
| MicroQuickJS moving GC misuse | native bindings retain stale JS values | root with documented C API discipline and dedicated tests |
| Unsupported JS syntax | prototype scripts fail on-device | ES5 facade or host transpilation tested against pinned runtime |
| Unbounded script execution | UI/display owner starves | bounded feasibility spike and stop rule |
| ABI overfits current JS facade | native core becomes hard to evolve | opaque handles, POD descriptors, versioned capability query |

---

## 18. Open questions

These questions should be answered by the named phase rather than by speculation:

1. Which matrix cell gives the best stable panel output on the actual board? (Phase 0)
2. Does M5GFX 0.2.25 still require the local LUT allocation patch in the new early-init firmware? (Phase 0)
3. What damage alignment minimizes Issue 181 risk while avoiding excess updates? (Phase 2/3)
4. Which packaged font gives acceptable reading quality, size, and partial-refresh speed? (Phase 5)
5. Is grayscale antialiasing worth its refresh cost for body text? (Phase 5/3)
6. What is the right sparse-checkpoint density for backward pagination? (Phase 7)
7. Which resume data belongs in NVS versus SD? (Phase 6/10)
8. Can touch or only RTC/button reliably wake this board revision? (Phase 10)
9. Can MicroQuickJS execution be bounded sufficiently for UI callbacks? (Phase 11)
10. Should authoring use a host transpilation step or a deliberately ES5-style API guide? (Phase 11/12)

---

## 19. Intern review map

Read in this order:

1. `sources/local/s3paper-api-design.md` for product semantics.
2. `sources/local/s3paper-studio.jsx:6-16`, `:78-405`, and `:406-728` for the prototype pipeline.
3. `0080-papers3-ereader/main/ereader_app.cpp` for the existing reader flow.
4. `0080-papers3-ereader/main/paginator.cpp` for the current pagination baseline.
5. `0080-papers3-ereader/main/gnosis_types.h`, `layout_engine.cpp`, `dirty_tracker.cpp`, and `widget_renderer.cpp` for retained UI mechanics.
6. `0075-papers3-touch-draw-demo/main/app_main.cpp` for responsive clipped touch drawing.
7. `0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp` for queued guest intent.
8. `../M5PaperS3-UserDemo/main/hal/hal.cpp` and `main/main.cpp` for hardware/HAL patterns.
9. The M5PS3 EPD drawing deep dive and ESP-37 crash diary for driver internals and prior failures.
10. `sources/web/06-mquickjs-readme.md` only after the native layers are understood.

The reproducible research commands and current snapshots are under `scripts/`; start with `scripts/00-research-log.md`.

---

## 20. References

### Imported local designs

- `sources/local/s3paper-api-design.md`
- `sources/local/s3paper-studio.jsx`

### Existing project evidence

- `0075-papers3-touch-draw-demo/`
- `0078-papers3-gnosis-layout/`
- `0079-papers3-wamr-assemblyscript-console/`
- `0080-papers3-ereader/`
- `0082-papers3-wamr-allocator-control/`
- `../M5PaperS3-UserDemo/`
- `ttmp/2025/12/30/M5PS3-EPD-DRAWING--m5papers3-how-epd-drawing-works-library-pipeline/`
- `ttmp/2026/03/23/ESP-37-EREADER-EPD-CRASH--papers3-e-reader-crashes-on-epd-fillscreen-null-framebuffer-pointer/`

### Captured web sources

- `sources/web/01-m5stack-papers3-hardware.md`
- `sources/web/02-m5papers3-userdemo.md`
- `sources/web/03-m5gfx-papers3-driver.md`
- `sources/web/04-m5gfx-issue-181-panel-epd-heap-corruption.md`
- `sources/web/05-m5gfx-issue-152-waveform-and-ghosting.md`
- `sources/web/06-mquickjs-readme.md`
- `sources/web/07-diy-esp32-epub-reader.md`
- `sources/web/08-diy-ebook-reader-article.md`
- `sources/web/09-crossink-architecture.md`
- `sources/web/10-m5stack-papers3-touch.md`
- `sources/web/11-m5gfx-releases.md`

### Reproducibility artifacts

- `scripts/00-research-log.md`
- `scripts/01-inventory-local-evidence.sh`
- `scripts/02-import-and-fetch-sources.sh`
- `scripts/03-query-upstream-state.sh`
- `scripts/04-collect-line-anchors.sh`
- `scripts/output/`
