---
Title: Diary
Ticket: 0066-cardputer-ledchain-gfx-sim
Status: active
Topics:
    - cardputer
    - m5gfx
    - display
    - animation
    - esp32c6
    - ui
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen/design-doc/01-deep-research-led-pattern-engine-cardputer-gfx-rendering.md
      Note: Primary deep-research analysis that diary entries refer to
    - Path: ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen/tasks.md
      Note: Living implementation checklist for this ticket
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-23T08:52:46.159881623-05:00
WhatFor: ""
WhenToUse: ""
---


# Diary

## Goal

Record (with high fidelity) the research and implementation decisions for ticket `0066-cardputer-ledchain-gfx-sim`, so we can:

- Validate intent early (before coding).
- Trace each discovery back to concrete repo files/APIs.
- Resume work later without re-deriving key facts.

## Context

We have an ESP32-C6 firmware that drives a physical 50-LED WS281x chain and supports multiple patterns (rainbow/chase/breathing/sparkle/off). We want a Cardputer (ESP32-S3) “GFX simulation” that draws the same 50-LED chain on the Cardputer display so the chain state/pattern is visible on-screen.

This repo already contains:

- A reusable WS281x pattern engine and task plumbing (`components/ws281x`).
- An MLED/1 protocol stack + node firmware that maps MLED/1 patterns to the WS281x pattern engine (`components/mled_node`, `0049-xiao-esp32c6-mled-node`).
- Multiple Cardputer M5GFX examples that show stable render-loop patterns (`0011`, `0015`, `0022`, `0030`, etc).

## Quick Reference

### Key repo landmarks (first pass)

- Pattern math / rendering to a “strip”: `components/ws281x/src/led_patterns.c`
- Pattern config structs: `components/ws281x/include/led_patterns.h`
- Strip buffer format (wire-order bytes, brightness scaling): `components/ws281x/src/led_ws281x.c`
- Task loop that calls patterns each frame (timebase, cadence): `components/ws281x/src/led_task.c`
- MLED/1 wire format for pattern config (20 bytes): `components/mled_node/include/mled_protocol.h`
- Adapter from MLED/1 pattern config → `led_pattern_cfg_t`: `0049-xiao-esp32c6-mled-node/main/mled_effect_led.c`
- Cardputer M5GFX “sprite/canvas + pushSprite + waitDMA” idiom: `0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp`, `0022-cardputer-m5gfx-demo-suite/main/app_main.cpp`

## Usage Examples

When adding a new discovery:

- Link it to a concrete file + line numbers.
- State the invariant (“what must always remain true”) and why it matters for the simulator.

## Related

- `../design-doc/01-deep-research-led-pattern-engine-cardputer-gfx-rendering.md`

---

## Step 1: Validate Goal and Intent (2026-01-23)

We are building a Cardputer on-screen visualization that mirrors the 50-LED WS281x chain patterns currently running on an ESP32-C6. The intent is not “new patterns”, but rather a faithful, inspectable rendering of the same computed LED colors over time, using M5GFX on the Cardputer display.

The critical success criterion is semantic equivalence: given the same pattern config and the same timebase, the simulator should render the same per-LED RGB values as the firmware would send to the strip (modulo display color quantization and any intentional gamma correction).

### Prompt Context

---

## Step 9: Resume implementation (Phase 0 groundwork) (2026-01-23)

### What I understand the goal to be (restated for validation)

We want to evolve `0066-cardputer-adv-ledchain-gfx-sim` into a “scriptable simulator” where:

- The LED-chain **pattern engine** runs continuously (eventually in its own task at ~60 Hz).
- A **MicroQuickJS REPL** exists on the Cardputer (USB Serial/JTAG console) and can:
  - Change simulator patterns (already supported via `sim.*`).
  - Schedule work in time (**timeouts** and **periodic callbacks**).
  - Toggle two Grove-labeled pins (**G3** and **G4**) so JS can drive simple external sequences.
- Concurrency stays safe: the JS VM has a single owner task (`mqjs_service`), timers/GPIO/engine communicate via queues/jobs, and “blocking calls” mean “ack/enqueued” rather than “wait until a frame rendered”.

### Current repo state (after interrupts)

I re-checked `git status` and verified only one intended modification was present (a local edit to `0066-cardputer-adv-ledchain-gfx-sim/main/Kconfig.projbuild`). Two unrelated untracked items existed (`walkthrough-mled-protocol.sh` and `ttmp/.../0067-.../`); I did not stage or commit them.

### Work performed

1) Added Kconfig knobs to make the upcoming work configurable and explicit:

- `CONFIG_TUTORIAL_0066_JS_MEM_BYTES` (MicroQuickJS fixed arena size; default 65536).
- `CONFIG_TUTORIAL_0066_G3_GPIO` and `CONFIG_TUTORIAL_0066_G4_GPIO` (board-label mapping).

The default mapping is evidence-based: in `0047-cardputer-adv-lvgl-chain-encoder-list/main/Kconfig.projbuild`, the “G1/G2” board labels default to GPIO1/GPIO2, so we default **G3→GPIO3** and **G4→GPIO4** but keep it configurable.

### Commands / results

- `git add 0066-cardputer-adv-ledchain-gfx-sim/main/Kconfig.projbuild`
- `git commit -m "0066: add Kconfig for JS mem + G3/G4 mapping"`
  - Commit: `79c732f`

### Next

Phase 1: migrate JS execution off the `esp_console` handler thread and onto the existing `components/mqjs_service` VM-owner task, preserving the existing `js eval` and `js repl` UX.

---

## Step 10: Phase 1 — JS runs on `mqjs_service` (2026-01-23)

### Why this change

The prior `0066` JS integration (`main/mqjs/mqjs_engine.cpp`) created and owned a MicroQuickJS `JSContext` directly inside the `esp_console` command handler path.

That was fine for `js eval ...`, but it becomes unsafe once we add:

- timer callbacks,
- GPIO “sequencing” callbacks,
- future engine-task notifications,

because those events should not execute JS from arbitrary tasks/ISRs. The stable pattern in this repo (see `0048-cardputer-js-web`) is to:

- run **all JS** on a single VM-owner task,
- communicate by posting bounded “jobs” (or synchronous eval requests) to that task.

We therefore migrated 0066 to use the existing `components/mqjs_service` + `components/mqjs_vm`.

### Implementation notes

- Added `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/js_service.cpp` / `.h`:
  - Starts `mqjs_service` with the existing generated `js_stdlib` from `mqjs/esp32_stdlib_runtime.c`.
  - Binds `sim_engine_t*` via `mqjs_sim_set_engine(engine)` before service use (so `sim.*` native functions keep working).
  - Provides `js_service_eval(...)`, `js_service_dump_memory(...)`, and `js_service_reset(...)` (implemented as stop+restart).
- Updated `mqjs_console.cpp` so `js eval` and `js repl` call into `js_service_eval(...)` instead of calling MicroQuickJS directly.
- Updated `0066` project `CMakeLists.txt` to include `mqjs_vm` and `mqjs_service` as extra components and main `REQUIRES`.

### Validation

Build:

- `cd 0066-cardputer-adv-ledchain-gfx-sim && idf.py build`

Flash to the Cardputer attached on `/dev/ttyACM0`:

- `idf.py -p /dev/ttyACM0 flash`

Smoke test script (ticket-local):

- `python3 ttmp/.../scripts/serial_smoke_js_0066.py --port /dev/ttyACM0`

The resulting log shows `js eval sim.status()` and pattern changes work with the service model.

### Commits

- `45612f6` — `0066: run JS via mqjs_service`

---

## Step 11: Phase 2 — Timers (`setTimeout` + `every`) (2026-01-23)

### Goal

Provide a minimal, robust timing substrate so that JS can drive “sequenced” behavior:

- `setTimeout(fn, ms)` / `clearTimeout(id)` as the primitive.
- `every(ms, fn)` + `cancel(handle)` as the ergonomic periodic layer (implemented in JS on top of `setTimeout`).

Key constraints:

- Never run JS in a timer callback context (avoid ISR/task-crossing issues).
- Keep timing “good enough” (ms resolution; jitter acceptable).
- Avoid introducing a complicated promise/event-loop model for MVP.

### Implementation

1) A one-shot scheduler task that only *posts* work into `mqjs_service`:

- `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/mqjs_timers.cpp`
  - FreeRTOS task `0066_js_tmr` owns a bounded queue of schedule/cancel commands.
  - Maintains a small fixed pool (`kMaxTimers=32`) of pending timeouts.
  - When a timeout expires, posts a `mqjs_service` job that runs `cb()` inside the VM owner task.

2) Wiring: start/stop timers alongside the JS service:

- `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/js_service.cpp`
  - Starts the scheduler after `mqjs_service_start(...)`.
  - Runs a bootstrap script to create `globalThis.__0066.timers.cb` and define:
    - `every(ms, fn)` (recursive `setTimeout`)
    - `cancel(handle)` (accepts either numeric timeout id or `{cancel()}` handle)

3) Implement the stdlib global functions:

- `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/esp32_stdlib_runtime.c`
  - Replaces the generator stubs with working `setTimeout` and `clearTimeout`.
  - Uses `globalThis.__0066.timers.cb[id]=fn` to root callbacks.
  - When the timeout fires, the VM-owner job clears `cb[id]` to `null` (one-shot semantics).

### Semantics chosen (MVP)

- If a callback throws, we log the exception and still clear its one-shot handle.
- If the service queue is full at the moment the timer fires, the job post can fail; this is logged.
  (For MVP we accept this as “best-effort”; the API is not a hard real-time scheduler.)

### Validation

- Built and flashed:
  - `cd 0066-cardputer-adv-ledchain-gfx-sim && idf.py build`
  - `idf.py -p /dev/ttyACM0 flash`
- Extended the ticket smoke script to validate timers:
  - `ttmp/.../scripts/serial_smoke_js_0066.py`
  - The script verifies:
    - `setTimeout(function(){ __t=123 }, 100)` results in `__t == 123`
    - `every(50, ...)` increments `__c` to 5 and cancels itself

### Commits

- `e50d028` — `0066: add JS timers (setTimeout + every)`
- `1296458` — `0066: extend JS smoke script for timers`

---

## Step 12: Phase 3 — GPIO (G3/G4) from JS (2026-01-23)

### Goal

Expose two “board label” GPIOs (G3 and G4) to MicroQuickJS so that JS scripts can:

- drive external toggles/pulses using `every(...)`/`setTimeout(...)`,
- compose simple sequencers in userland JS without a bespoke C sequencer layer (for MVP).

### Implementation

1) Native primitives (`gpio.high(label)` / `gpio.low(label)`):

- Implemented by replacing the generator stubs in:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/esp32_stdlib_runtime.c`
- Parsing:
  - Accepts `"G3"/"g3"` and `"G4"/"g4"`.
  - Maps to ESP32-S3 GPIO numbers via Kconfig:
    - `CONFIG_TUTORIAL_0066_G3_GPIO`
    - `CONFIG_TUTORIAL_0066_G4_GPIO`
- Initialization:
  - Lazily configures both pins as push-pull outputs and defaults them low on first use.

2) Userland convenience (`gpio.write` / `gpio.toggle`):

- Defined in the existing JS bootstrap in:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/js_service.cpp`
- Uses a tiny JS-side state table `__0066.gpio.state` so `toggle(...)` doesn’t need a native read.

### Validation

- Built/flashed to `/dev/ttyACM0` and executed the updated smoke script:
  - `python3 ttmp/.../scripts/serial_smoke_js_0066.py --port /dev/ttyACM0`
- The log shows the IDF GPIO driver configured GPIO3/GPIO4 as outputs and JS calls succeeded.

### Commits

- `d22d698` — `0066: add JS GPIO control for G3/G4`
- `097bf16` — `0066: extend JS smoke script for GPIO`

---

## Step 13: Upload ticket bundle to reMarkable (2026-01-23)

### What was uploaded

To make review easy on the device, I bundled the key ticket documents into a single PDF (with ToC):

- ticket `index.md`
- `tasks.md`
- `reference/02-js-postmortem.md`
- `reference/01-diary.md`
- all `design-doc/*.md` (deep research + JS design docs)

### Commands / result

- `remarquee upload bundle --dry-run ... --name "0066 Cardputer LEDChain GFX Sim (Ticket Bundle)" --remote-dir "/ai/2026/01/23/0066-cardputer-ledchain-gfx-sim/" --toc-depth 3`
- `remarquee upload bundle ...` (same args)

Verified remote dir listing includes the new file:

- `/ai/2026/01/23/0066-cardputer-ledchain-gfx-sim/0066 Cardputer LEDChain GFX Sim (Ticket Bundle).pdf`

### Next

Resume Phase 5: refactor the simulator so the pattern engine renders in its own task (~60 Hz) and publishes a latest-frame snapshot; migrate UI + console/web/JS control paths to enqueue commands instead of locking the engine mutex directly.

---

## Step 14: Phase 5 — Engine task + latest-frame snapshot (2026-01-23)

### Why this change

Before this step, the UI task called `sim_engine_render(...)` directly. That meant:

- UI pacing and pattern rendering were effectively the same loop.
- Any future “sequencer” callbacks, timers, web handlers, etc. that wanted to drive the engine would either:
  - take the same mutex used during render, or
  - risk races with rendering.

The design direction for 0066 is to make the pattern engine a proper *service*:

- One owner task computes frames at a fixed cadence (~60 Hz).
- Everyone else (console/web/js) sends control messages (ack/enqueued semantics).
- The UI reads a *latest-frame snapshot* and never calls into pattern math.

### What changed (code)

1) `sim_engine_t` became a “control + snapshot” object:

- Engine task owns `led_patterns_t` and the internal render buffer.
- `sim_engine_t` now stores:
  - current `cfg` + `frame_ms`,
  - a double-buffered `frames[2]` snapshot,
  - a control `QueueHandle_t`,
  - a mutex protecting snapshot metadata.

2) New engine task (`0066_engine`) renders continuously:

- `0066-cardputer-adv-ledchain-gfx-sim/main/sim_engine.cpp`
  - renders patterns every ~16 ms (`kEngineTickMs=16`)
  - publishes `frames[next]` then swaps `active_frame`
  - drains control queue and applies config updates (`led_patterns_set_cfg`)

3) UI now copies the latest snapshot:

- `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp`
  - replaced `sim_engine_render(...)` with `sim_engine_copy_latest_pixels(...)`
  - uses `sim_engine_get_order(...)` and `sim_engine_get_led_count(...)`

4) JS + HTTP status adjusted to read `led_count` through accessors:

- `mqjs/esp32_stdlib_runtime.c` now uses `sim_engine_get_led_count(s_engine)`
- `http_server.cpp` now reports `led_count` via `sim_engine_get_led_count(s_engine)`

### Validation

Build:

- `cd 0066-cardputer-adv-ledchain-gfx-sim && idf.py build`

Flash (note: the first attempt failed because `/dev/ttyACM0` was still held open by a python process; I killed it and retried):

- `lsof /dev/ttyACM0` → showed a python PID holding the device
- `kill <pid>` → released the port
- `idf.py -p /dev/ttyACM0 flash` → success

Smoke tests:

- `python3 ttmp/.../scripts/serial_smoke_0066.py --port /dev/ttyACM0`
- `python3 ttmp/.../scripts/serial_smoke_js_0066.py --port /dev/ttyACM0`

Both passed: console pattern changes, JS control (including timers + GPIO) still work, and UI continues to animate because it reads the published snapshot.

### Commit

- `4d4c356` — `0066: move pattern rendering into engine task`

---

## Step 15: Phase 4 — JS “sequencer cookbook” (2026-01-23)

### Goal

Before adding any native “declarative sequencer” layer, capture a usable set of JS idioms (copy/paste level) for:

- `setTimeout` one-shots,
- `every` periodic loops with cancellation,
- GPIO pulse/toggle patterns on `G3/G4`,
- simulator “playlist” style pattern changes,
- and combined “GPIO edge drives sim change” recipes.

### Output

Added:

- `ttmp/.../reference/03-js-sequencer-cookbook.md`

This doc is intentionally practical: it encodes the current JS surface (`every`, `cancel`, `gpio.write/toggle`, `sim.*`) into patterns that are easy to adapt.

---

## Step 16: Web server smoke test + `/api/apply` consistency fix (2026-01-23)

### Goal

Make sure the web control plane remains a reliable way to drive the simulator (even after the engine refactor):

- confirm `/api/status` works end-to-end from the workstation,
- confirm `/api/apply` visibly applies and responds consistently.

### What I did

1) Added a ticket-local script:

- `ttmp/.../scripts/http_smoke_0066.py`

It:

- opens the USB Serial/JTAG console on `/dev/ttyACM0`,
- runs `wifi status` to parse `ip=...`,
- performs:
  - `GET http://<ip>/api/status`
  - `POST http://<ip>/api/apply` with a small JSON payload,
- and stores a transcript under `ttmp/.../various/http-smoke-0066-*.log`.

2) Found a subtle behavior regression after Phase 5:

After moving the engine into its own task, `POST /api/apply` would enqueue changes, but the immediate response could still reflect *previous* config because:

- engine commands were marked “applied” (seq ack) as soon as they were dequeued,
- but the shared `e->cfg` snapshot used by `status_get` was only updated during the next frame publish.

So we fixed the ordering:

- when the engine dequeues a config command, it now immediately publishes the applied `cfg`/`frame_ms` into the shared snapshot under the same mutex used for status reads.

### Validation

- Flash:
  - `cd 0066-cardputer-adv-ledchain-gfx-sim && idf.py build && idf.py -p /dev/ttyACM0 flash`
- Run:
  - `python3 ttmp/.../scripts/http_smoke_0066.py --port /dev/ttyACM0`

The latest smoke log shows:

- `/api/status` returns JSON (ok)
- `/api/apply` response now reflects the updated `global_brightness_pct` immediately

---

## Step 17: Phase 0 mapping notes (partial) (2026-01-23)

### What we know vs what we don’t

We confirmed in-repo precedent that on Cardputer-ADV projects in this repo:

- `G1` defaults to GPIO1 and `G2` defaults to GPIO2 (see tutorial 0047 Kconfig).

We did *not* find any other “G3/G4” usage in this repo beyond ticket 0066 itself, so we cannot fully prove the mapping from local sources alone.

### What we implemented (pragmatic approach)

To avoid blocking the rest of the simulator work on a schematic lookup, we made the mapping explicit and user-overridable:

- `CONFIG_TUTORIAL_0066_G3_GPIO` (default 3)
- `CONFIG_TUTORIAL_0066_G4_GPIO` (default 4)

and documented how to validate on hardware in the ticket README.

**User prompt (verbatim):** "Create a new docmgr ticket 0066-... (you chose the name) where we are going to build a cardputer adv GFX simulation of the 50 led chain we are currently controlling from the esp32c6. We want to display the chain on the screen. 

Analyze the existing codebase to find the relevant parts of the codebase where we compute the different patterns (rainbow/chase/etc...) and also existing GFX code for the cardputer that we can use to display the leds.

Do a detailed and deep research and store the analysis document in the docmgr ticket, use a detailed textbook style as if written by peter norvig, with references to APIs and files and pseudocode and explaining fundamentals in callouts and using prose paragraphs and bullet points etc...

Keep a detailed and frequent diary as you go along, starting with a first entry that writes up what you undertood of the goal and my intent, so I can validate."

**Assistant interpretation:** Create a docmgr ticket and produce a deep, file-referenced analysis of (1) where the WS281x patterns are computed and (2) which Cardputer M5GFX rendering scaffolds we can reuse to draw a 50-LED chain on screen; keep a detailed diary.

**Inferred user intent:** You want a “bridge artifact”: a research-backed design doc that makes it easy to implement a faithful simulator (and later debug mismatches) without rediscovering where pattern semantics and rendering idioms live in the repo.

**Commit (code):** N/A

### What I did
- Created docmgr ticket `0066-cardputer-ledchain-gfx-sim`.
- Created initial docs: a design doc for deep research and this diary.

### Why
- Establish a durable workspace (ticket + docs) before making implementation decisions.

### What worked
- Ticket scaffolding created successfully under `ttmp/2026/01/23/...`.

### What didn't work
- N/A

### What I learned
- N/A (goal-validation step).

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- Clarify expected data source: does Cardputer compute patterns locally, or must it receive/mirror the live ESP32-C6’s config/timebase over MLED/1?

### What should be done in the future
- Confirm the simulator’s input contract (local vs mirrored).

### Code review instructions
- Start at `ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen/reference/01-diary.md:1`.

### Technical details
- N/A

---

## Step 2: Initial Codebase Recon (2026-01-23)

The repo already contains both halves of what we need: a reusable pattern engine for producing per-LED RGB values, and Cardputer M5GFX “sprite/canvas” examples that demonstrate stable full-frame redraw loops. The immediate shape of the simulator becomes clear: run the existing pattern engine on a virtual 50-LED buffer and draw that buffer as 50 colored glyphs each frame.

The highest-value finding is that the pattern logic is centralized in `components/ws281x/src/led_patterns.c`, and the on-wire protocol used by the ESP32-C6 “MLED node” maps directly onto those same pattern parameters; this makes “mirror the ESP32-C6” tractable by reusing the MLED/1 pattern config representation.

### Prompt Context

**User prompt (verbatim):** "Create a new docmgr ticket 0066-... (you chose the name) where we are going to build a cardputer adv GFX simulation of the 50 led chain we are currently controlling from the esp32c6. We want to display the chain on the screen. 

Analyze the existing codebase to find the relevant parts of the codebase where we compute the different patterns (rainbow/chase/etc...) and also existing GFX code for the cardputer that we can use to display the leds.

Do a detailed and deep research and store the analysis document in the docmgr ticket, use a detailed textbook style as if written by peter norvig, with references to APIs and files and pseudocode and explaining fundamentals in callouts and using prose paragraphs and bullet points etc...

Keep a detailed and frequent diary as you go along, starting with a first entry that writes up what you undertood of the goal and my intent, so I can validate."

**Assistant interpretation:** Perform repo archaeology to locate the “truth” sources for pattern computation and Cardputer rendering idioms; capture them in ticket docs.

**Inferred user intent:** Minimize future ambiguity: if visuals differ, we should know exactly which modules define semantics (patterns, brightness scaling, time stepping) and which define rendering performance constraints (canvas, DMA waits).

**Commit (code):** N/A

### What I did
- Located pattern computation code (canonical component + duplicates):
  - `components/ws281x/src/led_patterns.c` (rainbow/chase/breathing/sparkle/off).
  - Tutorial-local copies in `0044-xiao-esp32c6-ws281x-patterns-console/main/led_patterns.c` and `0046-xiao-esp32c6-led-patterns-webui/main/led_patterns.c`.
- Located the strip “framebuffer” representation and brightness scaling:
  - `components/ws281x/src/led_ws281x.c` (RGB scaling + GRB/RGB order into `pixels[3*led_count]`).
- Located MLED/1 wire format and mapping into the pattern engine:
  - `components/mled_node/include/mled_protocol.h` (20-byte `mled_pattern_config_t`).
  - `0049-xiao-esp32c6-mled-node/main/mled_effect_led.c` (maps MLED pattern → `led_pattern_cfg_t`).
- Located Cardputer M5GFX display + sprite rendering idioms:
  - `0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp` (full-screen sprite + `pushSprite` + optional `waitDMA()`).
  - `0022-cardputer-m5gfx-demo-suite/main/app_main.cpp` + `.../demo_primitives.cpp` (multi-sprite layout + UI scaffolding).
  - `0030-cardputer-console-eventbus/main/app_main.cpp` (dirty-flag full-screen redraw + `waitDMA()`).

### Why
- These files define “what colors should be” (pattern engine) and “how to show them” (M5GFX rendering loops).

### What worked
- Found a reusable, centralized `components/ws281x` implementation that multiple projects already mirror/use.
- Confirmed the MLED/1 protocol’s pattern config maps naturally onto the same set of patterns.

### What didn't work
- Attempted to open `0049-xiao-esp32c6-mled-node/main/mled_protocol.h` (doesn’t exist); the canonical header is `components/mled_node/include/mled_protocol.h`.

### What I learned
- The pattern engine is time-driven (`now_ms`) but some patterns internally use a `last_step_ms` delta (not all are purely absolute-time functions). The simulator must preserve call cadence/timebase to match visuals.

### What was tricky to build
- Deciding what constitutes “the” canonical implementation: there are tutorial-local copies, but `components/ws281x` appears intended as the reusable base.

### What warrants a second pair of eyes
- Confirm whether the Cardputer simulator should:
  - Recompute patterns locally (given a config), or
  - Subscribe to/mirror a live MLED/1 show clock (`execute_at_ms`) for deterministic sync with the ESP32-C6.

### What should be done in the future
- Capture a “semantic contract” table in the design doc: for each pattern, list required params, time dependence, and brightness semantics.

### Code review instructions
- Pattern engine: `components/ws281x/src/led_patterns.c:1`
- Strip buffer semantics: `components/ws281x/src/led_ws281x.c:1`
- Protocol semantics: `components/mled_node/include/mled_protocol.h:1`
- Cardputer rendering idioms: `0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp:1`, `0022-cardputer-m5gfx-demo-suite/main/app_main.cpp:1`

### Technical details
- Key grep queries used (representative):
  - `rg -n "rainbow|chase|pattern|ws281x" ...`
  - `rg -n "M5GFX|M5Canvas|pushSprite|waitDMA" ...`

---

## Step 3: Write Deep Research Doc + Upload to reMarkable (2026-01-23)

I consolidated the repository findings into a “design-doc as research artifact”: a codebase map, pattern semantic contract, and a proposed simulator architecture (virtual strip → render glyphs via M5Canvas). I also recorded the work in ticket metadata (related files + changelog) so future exploration stays anchored to canonical sources.

After the docs were written, I exported them as a single bundled PDF (with ToC) and uploaded them to your reMarkable under a ticket-stable folder for today’s date (`/ai/2026/01/23/0066-cardputer-ledchain-gfx-sim`).

### Prompt Context

**User prompt (verbatim):** "Create a new docmgr ticket 0066-... (you chose the name) where we are going to build a cardputer adv GFX simulation of the 50 led chain we are currently controlling from the esp32c6. We want to display the chain on the screen. 

Analyze the existing codebase to find the relevant parts of the codebase where we compute the different patterns (rainbow/chase/etc...) and also existing GFX code for the cardputer that we can use to display the leds.

Do a detailed and deep research and store the analysis document in the docmgr ticket, use a detailed textbook style as if written by peter norvig, with references to APIs and files and pseudocode and explaining fundamentals in callouts and using prose paragraphs and bullet points etc...

Keep a detailed and frequent diary as you go along, starting with a first entry that writes up what you undertood of the goal and my intent, so I can validate."

**Assistant interpretation:** Produce the deep research document and diary in the ticket workspace; then upload the resulting docs to reMarkable.

**Inferred user intent:** You want the research readable on the reMarkable (offline, distraction-free), and also preserved in-repo with durable links to the code.

**Commit (code):** N/A

### What I did
- Wrote the deep research document:
  - `ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen/design-doc/01-deep-research-led-pattern-engine-cardputer-gfx-rendering.md`
- Updated ticket overview and tasks:
  - `ttmp/.../index.md`
  - `ttmp/.../tasks.md`
- Related key source files to the design doc via `docmgr doc relate` and updated ticket changelog.
- Uploaded a bundled PDF to reMarkable using `remarquee upload bundle` (ToC depth 2) into:
  - `/ai/2026/01/23/0066-cardputer-ledchain-gfx-sim`

### Why
- The simulator will go faster (and be less error-prone) if we have a single canonical reference for:
  - where pattern semantics live,
  - how brightness/time are applied,
  - which M5GFX render-loop idioms are known to be stable on Cardputer.

### What worked
- `docmgr` ticket + docs updated cleanly; related files recorded.
- `remarquee` successfully uploaded the PDF bundle to the target folder.

### What didn't work
- `remarquee` cloud access failed in the restricted environment (DNS/network blocked) until run with network-enabled execution:
  - `dial tcp: lookup internal.cloud.remarkable.com: no such host`

### What I learned
- The MLED/1 node implementation uses a scheduled “cue fire” model (`execute_at_ms` in header); if we want *true mirroring* of a live node, show-time sync matters, not just pattern math.

### What was tricky to build
- Balancing “docmgr related files should be small” vs “deep research needs many citations”; I kept the related list tight and embedded additional file pointers inside the doc text.

### What warrants a second pair of eyes
- Decide whether the simulator should mirror show time (`execute_at_ms`) or simply run patterns locally from config; this affects architecture and test strategy.

### What should be done in the future
- If you want “mirror the live node”, add a dedicated section specifying the simulator’s timebase and how it syncs (or doesn’t) with the controller epoch/show clock.

### Code review instructions
- Start with the research doc:
  - `ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen/design-doc/01-deep-research-led-pattern-engine-cardputer-gfx-rendering.md:1`
- Then review the diary for intent and traceability:
  - `ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen/reference/01-diary.md:1`

### Technical details
- Upload command pattern (bundled PDF + ToC):
  - `remarquee upload bundle <md...> --name \"...\" --remote-dir \"/ai/2026/01/23/0066-cardputer-ledchain-gfx-sim\" --toc-depth 2`

---

## Step 4: MVP Implementation — Cardputer firmware + console-driven simulator (2026-01-23)

I implemented the first MVP firmware as a new ESP-IDF project (`0066-cardputer-adv-ledchain-gfx-sim`). It reuses the canonical pattern engine (`components/ws281x`) to compute the per-LED RGB values, then renders a 10×5 grid of “LED cells” onto the Cardputer display using M5GFX (`M5Canvas` sprite + `pushSprite`).

The simulator is controlled entirely via `esp_console` over **USB Serial/JTAG** (no network). The console provides `sim status`, `sim pattern set ...`, and per-pattern `sim <pattern> set ...` commands to update parameters.

### Prompt Context

**User prompt (verbatim):** "for a first MVP, we want simulator to just run patterns from the console. no need for network protocol. 

Create the tasks, and then start implementing. Keep a detailed implementation diary, and chek off tasks as you go. Commit code to git at each step. "

**Assistant interpretation:** Implement a Cardputer firmware that renders a virtual 50-LED chain and exposes pattern control via `esp_console` REPL, tracking progress in tasks + diary and committing in small steps.

**Inferred user intent:** You want a fast, reliable MVP that proves end-to-end “pattern math → visible on screen → configurable at runtime” before adding any network mirroring.

**Commit (code):** `eb39f4b` — "0066: add cardputer ledchain gfx sim skeleton"

### What I did
- Added a new project: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim`
- Implemented:
  - `sim_engine` (wraps `led_patterns_t` + virtual `led_ws281x_t` pixel buffer).
  - `sim_ui` (M5GFX sprite rendering of 50 LEDs as a 10×5 grid + small text overlay).
  - `sim_console` (REPL with `sim` commands to drive patterns and parameters).
- Ensured console backend default is USB Serial/JTAG in `sdkconfig.defaults`.

### Why
- Reusing `components/ws281x` preserves semantics with the ESP32-C6 implementation and avoids “two pattern engines”.
- A console-driven MVP minimizes UI work and keeps iteration tight.

### What worked
- `idf.py set-target esp32s3` + `idf.py build` succeeded for the new firmware.

### What didn't work
- N/A in this step (hardware validation is recorded in Step 5).

### What I learned
- The simplest stable shape is: compute into a virtual strip byte buffer (wire order) and render from that buffer; this keeps brightness scaling semantics identical to the WS281x sender path.

### What was tricky to build
- Keeping the pattern config parsing permissive enough for iterative use while still mapping cleanly to the existing param ranges.

### What warrants a second pair of eyes
- Console UX: argument parsing is intentionally minimal; if you want richer UX (optional args, defaults, validation messages), we should refactor command parsing once behavior stabilizes.

### What should be done in the future
- Add keyboard shortcuts (optional) once console control is solid.

### Code review instructions
- Entry points:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/app_main.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/sim_console.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/sim_engine.cpp`

### Technical details
- Pattern engine calls:
  - `led_patterns_render_to_ws281x(&patterns, now_ms, &virtual_strip)`
- Rendering:
  - `rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)`

---

## Step 5: Hardware Smoke Test + Fix crash (M5GFX init stack) (2026-01-23)

I flashed the MVP to the Cardputer on `/dev/ttyACM0` and validated the REPL responds to commands. The first on-device run crashed shortly after `display.init()` with a panic whose backtrace implicated `esp_task_wdt_reset` dereferencing a NULL watchdog object; this symptom is consistent with memory corruption, and in this case it was caused by running M5GFX init in a too-small FreeRTOS task stack.

Increasing the UI task stack (from 4096 to 8192) removed the crash. After that change, the console prompt appeared and `sim help` / `sim status` executed successfully.

### Prompt Context

**User prompt (verbatim):** "btw I attached the cardputer to ttyACM0 so if you want to flash and at least verify you can start the simulations and all, feel free. You can use tmux to read from idf.py monitor / restart the monitoring, etc..."

**Assistant interpretation:** Flash the new firmware to `/dev/ttyACM0` and perform an end-to-end smoke test, using tooling that works in a non-interactive environment.

**Inferred user intent:** You want confidence that the MVP is not just “it builds” but actually runs on the real Cardputer and accepts pattern commands.

**Commit (code):** `78efc2e` — "0066: fix M5GFX init crash (increase UI task stack)" (plus smoke scripts in `71ca90c`, `a9e6c4b`)

### What I did
- Built + flashed:
  - `idf.py set-target esp32s3`
  - `idf.py -p /dev/ttyACM0 flash`
- Captured crash log and symbolized the backtrace with `xtensa-esp32s3-elf-addr2line` against the ELF.
- Fixed the crash by increasing the UI task stack to 8192.
- Added non-interactive verification scripts (per user instruction to keep scripts in the ticket’s `scripts/` dir):
  - `ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen/scripts/serial_smoke_0066.py`
  - `ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen/scripts/tmux_monitor_0066.sh`

### Why
- `idf.py monitor` needs a TTY; the smoke script allows automated “send commands + capture output” checks without an interactive terminal.
- The stack increase is a direct mitigation for stack corruption during M5GFX init.

### What worked
- Flash over `/dev/ttyACM0` works.
- After increasing the UI task stack, the REPL responds (example command sequence):
  - `help`
  - `sim help`
  - `sim status`
  - `sim pattern set rainbow`
  - `sim rainbow set --speed 5 --sat 100 --spread 10 --bri 25`

### What didn't work
- `idf.py monitor` cannot run in this non-interactive context because it requires stdin to be attached to a TTY.

### What I learned
- M5GFX `display.init()` can be stack-hungry; initializing it in a 4k task can corrupt memory and produce misleading crashes elsewhere (here: task watchdog code).

### What was tricky to build
- The failure mode looked like a task watchdog bug, but was actually memory corruption from a too-small task stack.

### What warrants a second pair of eyes
- If you want long-term robustness, consider moving M5GFX init to the main task (known 8k stack) rather than a separate UI task, or make the UI task stack configurable via Kconfig.

### What should be done in the future
- Add a tiny “on-screen OK banner” / heartbeat counter so headless smoke tests have a visual confirmation to pair with the console validation.

### Code review instructions
- Crash fix:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp`
- Smoke scripts:
  - `ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen/scripts/serial_smoke_0066.py`
  - `ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen/scripts/tmux_monitor_0066.sh`

### Technical details
- Backtrace symbolization:
  - `xtensa-esp32s3-elf-addr2line -pfiaC -e build/cardputer_adv_ledchain_gfx_sim_0066.elf <pcs...>`

---

## Step 6: Add Wi-Fi console + Web Server (2026-01-23)

This step adds a minimal web control plane (HTML + JSON API) while preserving the existing console-driven simulator UX.

### Prompt Context

**User prompt (verbatim):** "now add the web server"

**Assistant interpretation:** Integrate Wi-Fi STA management + an `esp_http_server` instance that exposes the simulator’s current state and allows applying pattern settings via JSON.

**Inferred user intent:** You want quick remote control/inspection (from a browser/curl) without committing yet to the full ESP32-C6↔Cardputer mirroring protocol.

### Why this shape
- `esp_console` supports only one active REPL backend at a time; starting a second REPL is an easy foot-gun.
- The repo already contains reusable building blocks (`wifi_mgr`, `wifi_console`, `httpd_assets_embed`) that keep this small and consistent with earlier tutorials (notably `0065-xiao-esp32c6-gpio-web-server`).
- Using `wifi_console`’s `register_extra` hook lets us keep `sim ...` and `wifi ...` in a single REPL.

### What I did
- Added dependencies to the 0066 project:
  - `EXTRA_COMPONENT_DIRS` now includes `components/wifi_mgr`, `components/wifi_console`, `components/httpd_assets_embed`.
- Added a minimal embedded HTML page:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/assets/index.html`
  - Uses `fetch("/api/status")` and `POST /api/apply` for a tiny interactive control loop.
- Implemented a small HTTP server module:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/http_server.cpp`
  - Endpoints:
    - `GET /` → embedded `index.html`
    - `GET /api/status` → JSON snapshot of `pattern`, `frame_ms`, `led_count`
    - `POST /api/apply` → JSON apply for `type`, `brightness_pct`, `frame_ms`, plus optional nested per-pattern objects
- Refactored console ownership to avoid “two REPLs”:
  - `sim_console_register_commands(sim_engine_t*)` registers only the `sim` command.
  - `wifi_console_start(...)` owns the REPL and also registers `wifi ...`.
- Updated `app_main.cpp` so:
  - UI starts immediately.
  - Wi-Fi manager starts.
  - Web server starts only after STA gets an IP (got-IP callback).
  - A single REPL is started via `wifi_console_start`, with `sim` commands registered via `register_extra`.

### Commits
- `729e39d` — "0066: add HTTP server module scaffold"
- `e4313b3` — "0066: split sim console into register vs REPL"
- `512f705` — "0066: start wifi console and HTTP server on got-IP"

### What worked
- Code is structured to keep the original MVP console UX, while enabling web control whenever Wi-Fi is connected.

### What didn't work (yet)
- Not yet validated on hardware in this step (Wi-Fi join + `curl /api/status`). That validation is a dedicated task in `tasks.md`.

### Review notes / follow-ups
- If we later add a richer UI, we can switch from embedded HTML to `httpd_assets_embed` serving bundled static assets, without changing the API.

---

## Step 7: Add MicroQuickJS (`mquickjs`) + `js` REPL + `sim.*` JS APIs (2026-01-23)

This step adds a programmable surface on the Cardputer: a MicroQuickJS context + a small JS REPL command, and native JS APIs that directly control the simulator patterns and parameters.

### Prompt Context

**User prompt (verbatim):** "Ok, add the microquickjs + REPL that we have with the encoder demo, but this time register the different APIs for controlling the sim patterns."

**Assistant interpretation:** Bring MicroQuickJS into the 0066 firmware and expose a JS-friendly API to drive `sim_engine` (pattern type/params/brightness/frame timing), in a way that coexists with the existing `esp_console` REPL (`wifi ...`, `sim ...`).

### Key discovery (important constraint)

MicroQuickJS in this repo is **table-driven**: the set of native-callable JS functions is described by a generated `JSSTDLibraryDef` stdlib table (and a matching atom table), not via dynamic runtime registration like upstream QuickJS. That means:

- to add a new global object `sim` with methods like `sim.setPattern(...)`,
- we must **regenerate** the stdlib header so the C-function table contains those symbols.

This is the same structural constraint documented in:
- `imports/esp32-mqjs-repl/mqjs-repl/docs/js.md`
- `0039-cardputer-adv-js-gpio-exercizer/main/esp32_stdlib_runtime.c`

### Design decision

I deliberately avoided running a second “raw serial” REPL task (0039-style), because 0066 already uses `wifi_console_start()` which owns an `esp_console` REPL over USB Serial/JTAG.

Two concurrent readers on `/dev/ttyACM*` is a reliable way to get heisenbugs (interleaved reads, broken line editing, missing bytes). Instead, I integrated MicroQuickJS as a **new console command**:

- `js eval ...`
- `js repl`
- `js stats`
- `js reset`

### What I did (code)

- Added a `mqjs/` module under 0066 `main/`:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/mqjs_console.cpp`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/mqjs_engine.cpp`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/esp32_stdlib_runtime.c`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/esp32_stdlib.h` (generated)
- Wired `mqjs_console_register_commands(&engine)` into the existing REPL via:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/app_main.cpp`
- Added the `mquickjs` component to the 0066 project by extending:
  - `0066-cardputer-adv-ledchain-gfx-sim/CMakeLists.txt` (`EXTRA_COMPONENT_DIRS`)
  - `0066-cardputer-adv-ledchain-gfx-sim/main/CMakeLists.txt` (`REQUIRES mquickjs`)

### What I did (stdlib generation tooling)

To avoid forking the whole mqjs-repl project, I wrote a small ticket-local generator script that:

1) reads the upstream `mqjs_stdlib.c` input,
2) injects a `sim` object definition and adds it to the global object,
3) compiles a temporary generator using `mquickjs_build.c`,
4) writes `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/esp32_stdlib.h`.

Script:
- `ttmp/.../scripts/gen_esp32_stdlib_0066.py`

### JS API surface (implemented)

The `sim` object contains:

- `sim.status()`
- `sim.setFrameMs(ms)`
- `sim.setBrightness(pct)`
- `sim.setPattern("off"|"rainbow"|"chase"|"breathing"|"sparkle")`
- `sim.setRainbow(speed, sat, spread)`
- `sim.setChase(speed, tail, gap, trains, "#RRGGBB", "#RRGGBB", dir, fade, briPct)`
- `sim.setBreathing(speed, "#RRGGBB", min, max, curve)`
- `sim.setSparkle(speed, "#RRGGBB", density, fade, mode, "#RRGGBB")`

All setters ultimately mutate `led_pattern_cfg_t` and call `sim_engine_set_cfg(...)`, so they preserve the same semantics as the existing `sim ...` console commands.

### What worked (hardware)

- Built:
  - `idf.py -C 0066-cardputer-adv-ledchain-gfx-sim build`
- Flashed (after killing a stray Python process that had the TTY locked):
  - `idf.py -C 0066-cardputer-adv-ledchain-gfx-sim -p /dev/ttyACM0 flash`
- Smoke test (USB Serial/JTAG):
  - `js help` prints usage
  - `js eval sim.status()` returns an object
  - `js eval sim.setBrightness(100)` updates brightness and shows `100`

Smoke script + log:
- `ttmp/.../scripts/serial_smoke_js_0066.py`
- `ttmp/.../various/serial-smoke-js-20260123-173504.log`

### Commits

- `d3f54a2` — "0066: add MicroQuickJS console + sim JS API"

### What I learned

The main “gotcha” is the MicroQuickJS generation model: *bindings are not ad-hoc*, they are part of the stdlib table. Once you accept that, the cleanest integration is:

- keep `sim_engine` as the single source of truth,
- keep the JS API as thin wrappers,
- avoid console transport contention by using one REPL.

---

## Step 8: Design doc — MQJS timers + GPIO sequencing (2026-01-23)

This step is **documentation-only**: no firmware changes, only a design/analysis write-up for the next feature set:

- `setTimeout` / periodic JS callbacks
- GPIO toggling for two pins (“G3” / “G4”)
- higher-level sequencing layer options built on timers

### What I did

- Reviewed prior art in the repo:
  - `components/mqjs_service` / `components/mqjs_vm` (single-owner VM + job execution + deadlines)
  - `0048-cardputer-js-web/main/js_service.cpp` (bootstrap namespace, callback storage, `JS_Call` pattern)
  - `0047-cardputer-adv-lvgl-chain-encoder-list/main/lvgl_port_m5gfx.cpp` (periodic `esp_timer` pattern)
- Wrote a detailed design document:
  - `ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen/design-doc/03-mqjs-timers-and-gpio-sequencer.md`

### Key conclusions

- Timers are fundamentally an **execution model** problem: JS must run on a single VM owner task, never inside `esp_timer` callbacks.
- The cleanest implementation path is to reuse `mqjs_service` and treat timers as “enqueue jobs to the VM thread when due”.
- Sequencing can start as pure JS (userland) once timers exist, and only later graduate to a native “sequencer” object if needed.

---

## Step 9: Design doc — Engine task + control queue + JS sync/async (2026-01-23)

You asked whether we can make the chain pattern engine its own task with a queue, while still allowing:

- blocking (synchronous) JS calls, and
- async work in MicroQuickJS (callbacks / timers / sequences).

### Answer (summary)

- Yes: **blocking JS calls become an RPC** to the engine task (enqueue + wait for ack with timeout).
- Yes: async JS is supported by **posting jobs to the JS VM owner task** (reuse `mqjs_service`), never by calling JS directly from timers/engine context.
- Avoid deadlock by rule: **engine never waits on JS**, only posts notifications.

### What I did

- Wrote the design doc capturing this architecture and trade-offs:
  - `ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen/design-doc/04-pattern-engine-task-rpc-and-js-async.md`

### Key references used

- Current 0066 rendering loop (UI task directly calls engine methods):
  - `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp`
- Existing single-owner JS VM mechanism:
  - `components/mqjs_service`
- Existing “event → JS callback” pattern:
  - `0048-cardputer-js-web/main/js_service.cpp`

---

## Step 10: Task breakdown — timers, GPIO, engine task refactor (2026-01-23)

Captured your decisions (board-label G3/G4, ms resolution, jitter OK, JS moves to `mqjs_service`, timer API is `every`, engine blocking = ack/enqueued, engine tick 60Hz, UI consumes latest-frame snapshot) and expanded them into a detailed implementation task list in:

- `ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen/tasks.md`

This is intended to be the checklist we execute phase-by-phase with small commits and frequent hardware smoke tests.
