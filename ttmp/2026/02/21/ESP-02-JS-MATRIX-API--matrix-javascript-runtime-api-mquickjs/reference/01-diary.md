---
Title: Diary
Ticket: ESP-02-JS-MATRIX-API
Status: active
Topics:
    - esp32
    - esp-idf
    - mquickjs
    - javascript
    - led-matrix
    - rest
    - rtos
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/changelog.md
      Note: Changelog entries capture analysis and publication operations
    - Path: ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/design-doc/01-mquickjs-matrix-scripting-api-architecture-and-integration-blueprint.md
      Note: Primary analysis artifact recorded by diary
    - Path: ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/tasks.md
      Note: Task tracking file updated during analysis
ExternalSources: []
Summary: Step-by-step diary for ESP-02-JS-MATRIX-API analysis and delivery.
LastUpdated: 2026-02-21T16:52:04.382897721-05:00
WhatFor: ""
WhenToUse: ""
---


# Diary

## Goal

Capture the full execution trail for ticket `ESP-02-JS-MATRIX-API`: ticket lifecycle updates, codebase reconnaissance, architecture decisions for mquickjs matrix scripting over REST, and publication/upload steps.

## Step 1: Ticket transition and initialization

I began by finishing lifecycle hygiene from the previous work item and creating a clean workspace for the new JS runtime analysis. The goal was to ensure documentation continuity and avoid mixing matrix implementation tasks with the new architecture/design scope.

I then created a fresh ticket with focused topics (`esp32`, `esp-idf`, `mquickjs`, `javascript`, `led-matrix`, `rest`, `rtos`) and verified the scaffold state before writing any analysis artifacts.

### Prompt Context

**User prompt (verbatim):** "close it, then create ESP-02-JS-MATRIX-API and let's analyze the codebase on how to integrate mquickjs to provide a way to run scripts that control animations / text / pixels on the led matrix. Look at existing LED animation JS apis. The idea is that I can send over javascripts per REST and then those run to create more complex animations. They should be able to do pretty tight timing loops. 

Analyze the codebase on how to do that with the JS interpreter and real time primitives, and then design a JS API that gives me full control, from the lowest level to just triggering the predefined animations. 

Store this entire analysis 8+ pages in the same detailed textbook style in the ticket , and upload over remarkable."

**Assistant interpretation:** close the previous ticket, create a new ticket dedicated to JS matrix runtime design, deeply analyze existing firmware/runtime code, author a long-form design document (8+ pages), and deliver it to reMarkable.

**Inferred user intent:** build a robust roadmap for adding on-device scriptable LED-matrix animation with strong timing behavior and clear intern-onboarding documentation.

### What I did

- Closed ticket `ESP-01-STAMP-MATRIX` using `docmgr ticket close`.
- Created `ESP-02-JS-MATRIX-API` with explicit topic metadata.
- Verified workspace status, current docs, and task list using:
- `docmgr ticket list --ticket ESP-02-JS-MATRIX-API`
- `docmgr doc list --ticket ESP-02-JS-MATRIX-API`
- `docmgr task list --ticket ESP-02-JS-MATRIX-API`

### Why

- Keeping ticket boundaries clean preserves searchability and avoids stale context leakage across unrelated milestone phases.
- Starting with an empty, explicit task list improves traceability for future implementation rounds.

### What worked

- Ticket close/create flow completed cleanly.
- New ticket scaffold was created at:
- `ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs`

### What didn't work

- N/A in this step.

### What I learned

- The ticket scaffold is minimal by design; creating task structure early prevents documentation drift.

### What was tricky to build

- No technical complexity in this step; only workflow discipline.

### What warrants a second pair of eyes

- Confirm ticket naming/topic taxonomy aligns with your broader roadmap conventions.

### What should be done in the future

- Keep one architecture objective per ticket to maintain high signal in `docmgr doc search`.

### Code review instructions

- Review `tasks.md` in the ticket root for scope fidelity.
- Review `changelog.md` once all steps are complete.

### Technical details

- Commands executed:
- `docmgr ticket close --ticket ESP-01-STAMP-MATRIX`
- `docmgr ticket create-ticket --ticket ESP-02-JS-MATRIX-API --title "Matrix JavaScript Runtime API (mquickjs)" --topics esp32,esp-idf,mquickjs,javascript,led-matrix,rest,rtos`

## Step 2: Codebase reconnaissance and architecture extraction

I performed targeted source analysis on the current matrix firmware (`0067`) and reusable JS infrastructure (`mqjs_service`, prior tutorial `0066`, and imported `mquickjs`). This step produced the factual foundation for architecture decisions in the long-form document.

The objective was to avoid speculative design. Every major recommendation in the design doc is anchored to existing files, symbols, and proven patterns already present in the repository.

### Prompt Context

**User prompt (verbatim):** (same as Step 1)

**Assistant interpretation:** extract practical integration strategy from existing code, not greenfield assumptions.

**Inferred user intent:** minimize reinvention, maximize reuse, and keep runtime stable under script load.

### What I did

- Inspected 0067 matrix firmware internals:
- `main/app_main.c`
- `main/http_server.c`
- `main/matrix_engine.c`
- `main/matrix_console.c`
- `main/Kconfig.projbuild`
- Inspected JS runtime support components:
- `components/mqjs_service/include/mqjs_service.h`
- `components/mqjs_service/mqjs_service.cpp`
- `components/mqjs_service/include/mqjs_vm.h`
- `components/mqjs_service/mqjs_vm.cpp`
- Inspected prior working JS+REST+timers integration from 0066:
- `main/mqjs/js_service.cpp`
- `main/mqjs/mqjs_timers.cpp`
- `main/mqjs/esp32_stdlib_runtime.c`
- `main/http_server.cpp`
- Compared 0066 and 0067 CMake component wiring.

### Why

- 0067 already has matrix rendering and REST control.
- `mqjs_service` already solves VM ownership and deadline interruption.
- 0066 already solves timer callback orchestration and REST eval flow.
- Combining these three facts yields the shortest safe path to delivery.

### What worked

- Found complete reusable patterns for:
- JS service queue ownership.
- Timer callback posting into JS task.
- HTTP eval/reset/mem endpoint style.
- Bootstrap helper functions (`every`, `cancel`).
- Confirmed mquickjs APIs available for context, eval, call, interrupt, and memory dump.

### What didn't work

- One non-critical probe failed because the file does not exist:
- Command: `sed -n '1,340p' 0067-esp-c3-led-matrix-http/README.md`
- Error: `sed: can't read 0067-esp-c3-led-matrix-http/README.md: No such file or directory`

### What I learned

- 0067 is already advanced in matrix features (glyphs, orientation, repeat-count animations).
- Integration complexity is primarily orchestration and safety, not rendering feature gaps.

### What was tricky to build

- The main complexity was reducing a large codebase to only the files that directly determine architecture decisions while avoiding noisy scans from generated/build/ttmp artifacts.

### What warrants a second pair of eyes

- Validate whether VM arena defaults should be conservative (stability-first) or aggressive (script capability-first) for initial rollout.

### What should be done in the future

- Add a compact architecture diagram directly in firmware docs after implementation lands, so runtime ownership rules are visible near source code.

### Code review instructions

- Start with `components/mqjs_service/mqjs_service.cpp` for ownership model.
- Then compare `0066.../main/mqjs/mqjs_timers.cpp` and `0067.../main/matrix_engine.c`.
- Verify CMake diffs once integration starts.

### Technical details

- Main discovery commands used:
- `rg -n "matrix_console|esp_console|argtable|matrix" 0067-esp-c3-led-matrix-http/main -g'*.c' -g'*.h'`
- `sed -n '1,340p' components/mqjs_service/mqjs_service.cpp`
- `sed -n '1,340p' 0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/mqjs_timers.cpp`
- `sed -n '1,340p' imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h`

## Step 3: Author textbook-style design doc and prepare publication

I then wrote the full design artifact under the new ticket as a long-form, intern-friendly textbook document. It includes architecture diagrams, API layering, pseudocode, endpoint contracts, timing strategy, risk controls, and phased rollout guidance.

I expanded the content to exceed 8 pages in practical render length by adding deep-dive sections on SPI throughput budgeting, runtime state machine semantics, observability, and validation matrices.

### Prompt Context

**User prompt (verbatim):** (same as Step 1)

**Assistant interpretation:** deliver a comprehensive, actionable blueprint, not a short summary.

**Inferred user intent:** give future contributors enough fundamentals and concrete steps to execute implementation safely.

### What I did

- Created design doc:
- `design-doc/01-mquickjs-matrix-scripting-api-architecture-and-integration-blueprint.md`
- Added sections:
- Fundamentals, codebase state, architecture, API contract, REST contract, console contract.
- Real-time design with cooperative timing primitives.
- Implementation plan with file-level scope.
- Deep-dive throughput and validation matrices.
- Confirmed size:
- `wc -w` => 4088 words.

### Why

- The user requested 8+ pages and intern-ready depth.
- The design needs to be immediately actionable for next implementation phase.

### What worked

- Document now contains all requested structure types:
- prose paragraphs
- bullet points
- pseudocode
- file names
- code snippets
- ASCII diagrams

### What didn't work

- N/A for content generation step.

### What I learned

- Throughput math (rows/bytes/frame) is very effective for grounding timing expectations and explaining why certain architectural constraints exist.

### What was tricky to build

- Balancing deep detail with consistency: the document needed to remain implementation-relevant while still teaching fundamentals to new contributors.

### What warrants a second pair of eyes

- API naming choices (`matrix.*` surface and `/api/matrix/js/*` routes) should be reviewed with downstream CLI/tooling maintainers to avoid churn.

### What should be done in the future

- Once implementation begins, split this design into “accepted architecture” and “implementation status” docs to keep long-term maintainability.

### Code review instructions

- Review the design doc top-down once for conceptual coherence.
- Review the “Implementation Plan” and “Validation Matrix” sections as the primary execution checklist.

### Technical details

- File created:
- `ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/design-doc/01-mquickjs-matrix-scripting-api-architecture-and-integration-blueprint.md`
- Word count command:
- `wc -w <design-doc-path>`

## Step 4: Ticket bookkeeping and publication prep

This step tracks the doc artifacts back to the ticket metadata so they remain discoverable in future searches and triage work. It also prepares the reMarkable upload packaging workflow.

### Prompt Context

**User prompt (verbatim):** (same as Step 1)

**Assistant interpretation:** ensure work is not only written but stored correctly in ticket workflow and delivered externally.

**Inferred user intent:** maintain rigorous documentation operations, not ad hoc local files.

### What I did

- Added detailed tasks in `tasks.md` for traceability.
- Created both design and diary docs using `docmgr doc add`.
- Prepared for file relations, changelog entries, task checkoff, and `remarquee` upload.

### Why

- Ticket metadata and relations are essential for future onboarding and search-based reuse.

### What worked

- Ticket docs exist in canonical locations under ticket workspace.

### What didn't work

- N/A so far.

### What I learned

- Doing `docmgr` scaffolding first reduces editorial rework when documents grow large.

### What was tricky to build

- Maintaining strict traceability while iterating quickly on long-form content.

### What warrants a second pair of eyes

- Confirm upload destination path convention on reMarkable (`/ai/YYYY/MM/DD/TICKET-ID`) matches your preferred taxonomy.

### What should be done in the future

- Keep diary updates incremental if implementation starts immediately after design approval.

### Code review instructions

- Verify ticket contains both docs and that relations/changelog are present.
- Validate upload artifact title and target folder after transfer.

### Technical details

- Scaffold commands used:
- `docmgr doc add --ticket ESP-02-JS-MATRIX-API --doc-type design-doc --title "mquickjs Matrix Scripting API Architecture and Integration Blueprint"`
- `docmgr doc add --ticket ESP-02-JS-MATRIX-API --doc-type reference --title "Diary"`

## Quick Reference

- Primary design doc:
- `ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/design-doc/01-mquickjs-matrix-scripting-api-architecture-and-integration-blueprint.md`
- Diary doc:
- `ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/reference/01-diary.md`
- Ticket root:
- `ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs`

## Usage Examples

- List docs:

```bash
docmgr doc list --ticket ESP-02-JS-MATRIX-API
```

- Search by topic:

```bash
docmgr doc search --query "matrix js runtime" --topics mquickjs,led-matrix
```

- Validate metadata:

```bash
docmgr validate frontmatter --doc ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/design-doc/01-mquickjs-matrix-scripting-api-architecture-and-integration-blueprint.md --suggest-fixes
```

## Related

- `ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/design-doc/01-mquickjs-matrix-scripting-api-architecture-and-integration-blueprint.md`

## Step 5: Implementation and device validation (runtime_bridge + soft-reset model)

After design delivery, implementation started directly in firmware `0067-esp-c3-led-matrix-http`. The focus was to integrate the reusable `mqjs_service` component with matrix runtime primitives, expose REST/console control paths, and validate everything on the Stamp C3 over Wi-Fi.

### What I implemented

- JS runtime integration in 0067:
- Added `main/mqjs/js_runtime_bridge.cpp` + `main/mqjs/js_runtime_bridge.h` (renamed from `js_service` naming at file level; API names kept stable as `js_service_*` for C callers).
- Added `main/mqjs/mqjs_timers.cpp` + `main/mqjs/mqjs_timers.h`.
- Added `main/mqjs/esp32_stdlib_runtime.c` and wired generated `main/mqjs/esp32_stdlib.h`.

- Build/config wiring:
- Updated `0067-esp-c3-led-matrix-http/CMakeLists.txt` to include reusable component paths.
- Updated `0067-esp-c3-led-matrix-http/main/CMakeLists.txt` for new sources/dependencies.
- Added JS Kconfig knobs in `main/Kconfig.projbuild`:
- memory arena bytes
- max HTTP body
- eval timeout
- max timers

- Matrix engine integration for JS control:
- Added script mode and framebuffer APIs in `matrix_engine.h/.c`:
- `frame_clear`, `frame_fill`, `frame_set_pixel`, `frame_get_pixel`, `frame_present`
- `width/height` helpers
- Updated status rendering in console and HTTP for `script` mode.

- REST + console surfaces:
- Added JS HTTP endpoints in `main/http_server.c`:
- `/api/js/eval`
- `/api/js/reset`
- `/api/js/reset-hard`
- `/api/js/stop`
- `/api/js/mem`
- `/api/js/status`
- Added `main/js_console.c` command parser:
- `js status`
- `js eval <code>`
- `js reset` (soft default)
- `js reset hard` (explicit hard reset)
- `js stop`
- `js mem`
- `js examples`

### Debugging outcome: root cause and fix

First device smoke tests failed with:
- `{"ok":false,"error":"busy"}` (later improved to `js init failed`)
- `/api/js/mem` returning `dump failed`

I instrumented `components/mqjs_service/mqjs_service.cpp` with heap diagnostics and captured serial logs using a tracked script. Root cause was contiguous heap pressure on C3:

- failure sample:
- `arena alloc failed: bytes=131072 free8=240200 largest8=114688`

This proved total free heap was high enough, but largest contiguous block was below requested arena size.

Fixes applied:
- reduced default arena in `sdkconfig.defaults` from `131072` to `98304`
- added arena fallback logic in `mqjs_service`:
- automatically steps down arena size by 4KB until allocation/context init succeeds (min 32KB)
- added diagnostic warning when fallback is used

Observed success after fix:
- `arena fallback: requested=98304 actual=94208 ...`
- JS eval/memory dump succeeded before and after resets.

### Reset behavior change (user-guided decision)

Based on user preference and observed heap churn, reset semantics were changed:

- `js reset` now performs **soft reset** by default:
- cancel timers
- clear runtime stop flag
- prepare/reset runtime namespace
- rerun bootstrap in the same VM

- `js reset hard` (and `/api/js/reset-hard`) performs full teardown/recreate:
- stop service task + VM + queue
- start fresh service

This keeps default reset lightweight and avoids unnecessary reallocations, while preserving a recovery path when needed.

### Validation scripts added under ticket `scripts/`

Per request, all helper scripts used for bring-up/testing were stored under:

- `ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_flash.sh`
- `ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_monitor_capture.sh`
- `ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_http_js_smoke.sh`
- `ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_http_matrix_smoke.sh`
- `ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_serial_capture.py`
- `ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_console_js_smoke.py`

### Commands executed (representative)

```bash
idf.py -C 0067-esp-c3-led-matrix-http build
ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_flash.sh
ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_http_js_smoke.sh
ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_http_matrix_smoke.sh
ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_console_js_smoke.py
```

### Final validation status

- Build: successful (`idf.py build`)
- Flash: successful to `/dev/serial/by-id/usb-1a86_USB_Single_Serial_575E072431-if00`
- JS HTTP flow: successful (eval, mem, stop, soft reset, hard reset)
- Matrix HTTP flow: successful (status, text, scroll/wave anim, stop)
- Console parser: successful (`js examples`, `js eval`, `js reset`, `js reset hard`)

No blocking runtime issues remain for this implementation phase.

## Step 6: Add complex JS animation examples and playback tooling

User asked for richer JavaScript animations stored in the project examples folder, plus practical commands to play them. I implemented three non-trivial scripts under `0067-esp-c3-led-matrix-http/examples/js` and a tracked helper script under ticket `scripts/`.

### What I added

- Project examples:
- `0067-esp-c3-led-matrix-http/examples/js/01-plasma-ribbon.js`
- `0067-esp-c3-led-matrix-http/examples/js/02-life-torus.js`
- `0067-esp-c3-led-matrix-http/examples/js/03-comet-trails.js`
- `0067-esp-c3-led-matrix-http/examples/README.md`

- Tracked playback helper:
- `ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_play_js_example.sh`

### Animation behavior summary

- `01-plasma-ribbon.js`:
- dual-sine ribbon interference across width with moving phase and spark accents
- runs as periodic callback via `every(40, ...)`

- `02-life-torus.js`:
- Conway's Game of Life on toroidal 96x8 grid
- periodic reseeding to avoid deadlock/static extinction
- runs via `every(90, ...)`

- `03-comet-trails.js`:
- multiple moving comets with velocity jitter and decaying trail field
- pseudo-brightness dithering from trail strength thresholds
- runs via `every(45, ...)`

All scripts cancel prior `__matrix_anim`, switch to script control, and publish a startup string result for API confirmation.

### Validation details

After reconnecting workstation to `CLUB:LINK`, I validated all examples over REST:

```bash
PLAY=ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_play_js_example.sh
$PLAY 01-plasma-ribbon
$PLAY 02-life-torus
$PLAY 03-comet-trails
curl -sS -X POST http://192.168.3.119/api/js/stop
curl -sS -X POST http://192.168.3.119/api/js/reset
```

Observed results:
- each play call returned `{"ok":true,... "started" ...}`
- `/api/matrix/status` reported `mode:"script"`
- `/api/js/status` remained healthy (`busy:false`, no timeout flags)

### Operational note

During initial testing, HTTP calls timed out because the workstation had switched SSID away from `CLUB:LINK`; device logs still showed correct target IP. Rejoining the correct network restored reachability and playback validation.

## Step 7: Visual diagnostics pass and root-cause fix for inert JS animations

User reported that complex JS examples (`plasma-ribbon`, `life-torus`, `comet-trails`) appeared to do nothing on the matrix. I switched from assumption-based debugging to deterministic visual/runtime isolation.

### What I added for operator-guided verification

- Project diagnostic scripts under:
- `0067-esp-c3-led-matrix-http/examples/js/diag/00-env-status.js` ... `10-stop-reset.js`
- Diagnostic guide:
- `0067-esp-c3-led-matrix-http/examples/DIAGNOSTIC-SEQUENCE.md`
- Guided runner in ticket scripts:
- `ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_run_js_diagnostics.sh`

I executed the full sequence against `http://192.168.3.119`; all steps returned `ok:true` and expected mode transitions.

### Findings

1. JS timers were alive (independent probe): `ticks` increased from 0 to 13 in ~1.2s.
2. Complex examples still showed zero lit pixels when sampled from JS framebuffer.
3. `diag/06-walk-dot.js` also reported `lit=0`, indicating timer callbacks were starting but not rendering.

This pointed to control-flow cancellation rather than transport/wifi issues.

### Root cause A: `matrix.stop()` poisoned `shouldStop()`

`matrix.stop()` in `esp32_stdlib_runtime.c` set the global cooperative stop flag (`s_stop_requested = true`).

Many animation callbacks start with:

```js
if (matrix.shouldStop()) { handle.cancel(); return; }
```

So animations canceled on their first tick after calling `matrix.stop()` at script startup.

Fix applied:
- Updated `op: "stop"` behavior to cancel timers + stop matrix mode without latching the cooperative stop flag.
- Kept global cancellation semantics for external stop paths (`js_service_request_stop`).

### Root cause B: timer callback hard timeout too low

`life-torus` still failed after Root cause A fix. Serial capture using tracked script showed:

- `W (...) 0067_js_timers: timeout callback threw: InternalError: interrupted`

Cause:
- Timer callback jobs in `mqjs_timers.cpp` used hardcoded `timeout_ms = 100`.
- Heavier frame callbacks exceeded 100ms and were interrupted, then self-cancelled by the JS `every()` wrapper.

Fix applied:
- `job.timeout_ms = CONFIG_TUTORIAL_0067_JS_EVAL_TIMEOUT_MS` (default 250ms).

### Validation after both fixes

After rebuild + flash to `/dev/serial/by-id/usb-1a86_USB_Single_Serial_575E072431-if00`:

- `diag/06-walk-dot.js`: sampled lit pixels became non-zero (`lit=1`).
- `01-plasma-ribbon.js`: sampled lit counts changed over time (e.g. 177, 183, 189).
- `02-life-torus.js`: sampled lit counts changed over time (e.g. 208, 174, 75).
- `03-comet-trails.js`: sampled lit counts changed over time (e.g. 63, 81, 89).

These measurements confirm active frame updates and animation progression on device.

### Trace artifacts

- `ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/various/serial-capture-0067-js-examples-throw.log`
- `ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/various/serial-capture-0067-life-debug.log`

### Representative commands

```bash
AUTO=1 BASE_URL=http://192.168.3.119 ttmp/.../scripts/0067_run_js_diagnostics.sh
BASE_URL=http://192.168.3.119 ttmp/.../scripts/0067_play_js_example.sh 0067-esp-c3-led-matrix-http/examples/js/01-plasma-ribbon.js
idf.py -C 0067-esp-c3-led-matrix-http build
ttmp/.../scripts/0067_flash.sh
```

## Step 8: Checkpoint commit and handoff

Packaged the diagnostics assets + JS runtime fixes into commit:

- `62a04e4` — Fix JS animation stop semantics and timer callback deadlines; add matrix diagnostics

Handoff expectation:
- operator runs `diag` sequence and confirms first visually failing step (if any) for final hardware-side triage.

## Step 9: Project docs API spec write-up

Added a detailed project-local documentation page for JS scripting in `0067`, focused on intern onboarding and day-to-day script authoring.

New file:
- `0067-esp-c3-led-matrix-http/docs/JS-API-GUIDE.md`

Scope covered:
- verbose getting started walkthrough
- REST API endpoints for matrix and JS runtime
- `js` and `matrix` console command surfaces
- complete `matrix` JS object API (pixels, text, animations, timing, status)
- timer helper semantics (`setTimeout`, `clearTimeout`, `every`, `cancel`)
- limits (body size, timeout, text length, glyph set)
- troubleshooting and diagnostic flow references

Also linked from examples entry point:
- `0067-esp-c3-led-matrix-http/examples/README.md` now references `../docs/JS-API-GUIDE.md`

## Step 10: Upload JS API guide to reMarkable

Uploaded project docs file to reMarkable cloud:
- local: `0067-esp-c3-led-matrix-http/docs/JS-API-GUIDE.md`
- remote dir: `/ai/2026/02/21/ESP-02-JS-MATRIX-API`

Verification:
- `remarquee cloud ls /ai/2026/02/21/ESP-02-JS-MATRIX-API --non-interactive`
- observed entries include `JS-API-GUIDE`.

## Step 11: Investigate long-run JS animation stall after minutes

User reported `03-comet-trails.js` stopping after a couple of minutes while manual matrix animations still worked.

### Reproduction probes and findings

- Confirmed stop-flag was not poisoned:
- `matrix.stop(); matrix.shouldStop()` returned `false`.
- `js/status` stayed healthy (`last_timed_out=false`, `last_error=""`).

I then inspected the internal timer callback table from JS:

```js
Object.keys(globalThis.__0067.timers.cb).length
```

Observed unbounded growth during a running animation:
- ~58 keys at 5s
- ~113 keys at 10s
- ~224 keys at 20s
- ~280 keys at 25s

Root cause:
- each `every()` tick schedules a new `setTimeout` ID.
- callback slots were set to `null` after fire/cancel, not deleted/reused.
- keyspace growth in `__0067.timers.cb` was effectively unbounded over long runs.

This can eventually increase VM pressure and make timer callbacks less reliable over time.

### Fix applied

File:
- `0067-esp-c3-led-matrix-http/main/mqjs/esp32_stdlib_runtime.c`

Change:
- replaced monotonic timeout ID allocation with bounded ID-ring reuse (`1..1024`).
- allocator now scans for free (`null/undefined`) slots and reuses IDs.
- `setTimeout` now throws if no ring slot is available.

### Validation

After rebuild/flash, long-run probe results:
- `keys=327` at 30s
- `keys=649` at 60s
- `keys=970` at 90s
- `keys=1024` at 120s
- `keys=1024` at 150s
- `keys=1024` at 180s

Key result: timer callback table now plateaus at ring size instead of growing without bound.

Post-run script handoff check:
- switching from long-running `03-comet-trails.js` to `01-plasma-ribbon.js` succeeded with live non-zero lit pixels.

## Step 12: Add JS runtime observability and animation registry API

User requested two additions:
1. richer status observability to spot memory/resource growth quickly,
2. a script-level animation registration API with structured cleanup semantics.

### Runtime/API changes implemented

Files:
- `0067-esp-c3-led-matrix-http/main/mqjs/js_runtime_bridge.h`
- `0067-esp-c3-led-matrix-http/main/mqjs/js_runtime_bridge.cpp`
- `0067-esp-c3-led-matrix-http/main/http_server.c`
- `0067-esp-c3-led-matrix-http/main/js_console.c`

Added `js_service_status_t` fields:
- `timer_cb_keys`
- `timer_cb_active`
- `timer_cb_keys_high_water`
- `animations_registered`
- `active_animation`
- `heap_free_8bit`
- `heap_largest_free_8bit`
- `heap_min_free_8bit`

Status endpoint (`/api/js/status`) and `js status` now include these fields.

### New script API: `matrix.anim`

Bootstrapped JS runtime now provides a lifecycle-managed animation registry:
- `matrix.anim.register(name, specOrStartFn)`
- `matrix.anim.unregister(name)`
- `matrix.anim.start(name, opts)`
- `matrix.anim.stop()`
- `matrix.anim.clear()`
- `matrix.anim.list()`
- `matrix.anim.current()`
- `matrix.anim.status()`

Resource behavior:
- starting a new registered animation stops the previous one,
- tracked handles/timers are cancelled automatically,
- cleanup callbacks run in reverse order,
- runtime stop path calls animation stop hook before clearing timers.

### Reliability fixes discovered while implementing

- Bootstrapping briefly failed (`SyntaxError: catch variable already exists`) after adding larger JS bootstrap logic. I fixed this by using unique catch variable names in all catch blocks.
- Increased bootstrap timeout from 250ms to 1000ms for startup/reset to avoid parser/init deadline issues on C3.
- Added status probe fallback in `js_service_get_status()` using `mqjs_service_eval` when direct status job path cannot collect stats in time.

### Validation highlights

- `matrix` and `matrix.anim` are now present immediately after boot.
- `examples/js/04-anim-registry-demo.js` registers and starts animations successfully.
- `/api/js/status` shows live observability values during animation:
  - non-zero `timer_cb_keys`,
  - non-zero `animations_registered`,
  - populated `active_animation`.

Sample observed status (runner active):
- `timer_cb_keys: 10`
- `timer_cb_active: 1`
- `animations_registered: 2`
- `active_animation: "runner"`

## Step 13: Migrate core example scripts to `matrix.anim` lifecycle API

User asked to make the script surface more elegant and avoid ad-hoc animation handle ownership via `globalThis.__matrix_anim`.

### What I changed

Rewrote core example scripts to use registry/lifecycle primitives:
- `examples/js/01-plasma-ribbon.js`
- `examples/js/02-life-torus.js`
- `examples/js/03-comet-trails.js`

Each script now:
- uses `matrix.anim.unregister(name)` to avoid stale registrations,
- calls `matrix.anim.register(name, function(ctx){ ... })`,
- runs frames via `ctx.every(...)` and `ctx.shouldStop()`,
- returns cleanup closure to clear/present the matrix,
- starts with `matrix.anim.start(name, opts)` and returns `matrix.anim.status()`.

### Documentation updates

Updated docs to match the new pattern:
- replaced old "globalThis handle" template in `docs/JS-API-GUIDE.md` section 8.1,
- updated cancellation guidance section with `matrix.anim`-first template,
- added example-note in `examples/README.md` that `01`..`03` are registry-based.

### Validation

- JS syntax checks pass for all migrated examples (`node --check`).
- search confirms no `globalThis.__matrix_anim` usage remains in `01`..`03` and updated guide sections.

## Step 14: Normalize JS API documentation to `matrix.anim`-first style

User requested docs be aligned with the new structured script style.

### Changes made

Updated `0067-esp-c3-led-matrix-http/docs/JS-API-GUIDE.md` so production script guidance consistently uses animation lifecycle APIs instead of low-level timer ownership patterns.

Specific edits:
- Script lifecycle section now explicitly recommends `matrix.anim` + `ctx.every/ctx.timeout`.
- Global timer helper section is labeled low-level and notes preferred `ctx.*` alternatives.
- `matrix.anim` example removed unnecessary explicit `h.cancel()` usage in callback.
- Multi-line heredoc example in trigger section rewritten to register/start a named animation via `matrix.anim`.

### Validation

- Searched docs for legacy manual animation handle pattern (`globalThis.__matrix_anim`): no remaining occurrences in markdown docs.

## Step 15: Upload updated JS API guide to reMarkable

Uploaded updated `0067-esp-c3-led-matrix-http/docs/JS-API-GUIDE.md` via `remarquee`.

Notes:
- Existing document already present in `/ai/2026/02/21/ESP-02-JS-MATRIX-API` and same-name upload was skipped (non-force mode).
- To avoid destructive overwrite (`--force`), uploaded to a new dated folder instead.

Upload result:
- `JS-API-GUIDE.pdf` -> `/ai/2026/02/22/ESP-02-JS-MATRIX-API`

## Step 16: Boot UX polish (boot animation + IP preview until first animation)

Implemented requested startup behavior directly in firmware `0067`.

### What changed

1. Added boot animation support in matrix engine:
- new API: `matrix_engine_play_boot_animation()`
- effect: short sweep + pulse sequence at boot
- invoked from `app_main()` immediately after `matrix_engine_init()`

2. Added boot IP preview support:
- new API: `matrix_engine_show_boot_ip(const char *ip_text, uint32_t fps, uint32_t pause_ms)`
- called from Wi-Fi got-IP callback in `app_main.c`
- displays `IP a.b.c.d` as a scrolling preview

3. Added first-animation claim semantics in matrix engine:
- engine tracks whether the first real user animation has started
- first call to `matrix_engine_start_scroll`, `matrix_engine_start_drop`, or script framebuffer animation path (`matrix_engine_frame_*`) marks first animation as started
- once first animation is claimed, subsequent boot-IP preview attempts are ignored

### Build / validation

- `idf.py build` in `0067-esp-c3-led-matrix-http` succeeds.
- No hardware flash performed in this step because no USB serial device (`/dev/serial/by-id`, `/dev/ttyACM*`, `/dev/ttyUSB*`) was present in the environment.

## Step 17: Add scroll loop behavior modes (`gap` / `wrap` / `right_exit`) and apply same model to boot IP preview

User requested configurable loop-around behavior for wave/scroll rendering instead of only clearing at end, and asked for the same treatment on boot IP rotation.

### Implemented behavior modes

Added a new loop mode enum in matrix engine:
- `MATRIX_SCROLL_LOOP_GAP` (legacy behavior)
- `MATRIX_SCROLL_LOOP_WRAP` (direct looparound, no blank clear gap)
- `MATRIX_SCROLL_LOOP_RIGHT_EXIT` (move right and fully exit right before restarting)

Engine updates:
- scroll renderer now branches by loop mode in `anim_task`
- repeat-cycle accounting updated for each mode (`repeat_count` still honored)
- status now reports current `scroll_loop`

### Interfaces updated

1. REST API `/api/matrix/anim`
- accepts optional `loop_mode` string: `gap|wrap|right_exit`
- aliases accepted: `loop|direct` => `wrap`, `exit_right` => `right_exit`
- `/api/matrix/status` now includes `loop_mode`

2. Console `matrix scroll ...`
- optional arg added:
  - `matrix scroll wave <TEXT> [fps] [pause_ms] [repeat_count] [loop_mode]`
- `matrix status` now prints active loop mode

3. JS runtime bridge (`matrix.startScroll`)
- `opts.loopMode` is now passed through to the runtime bridge (`loop_mode` op field)
- JS `matrix.status()` now includes `loop_mode`

4. Boot IP preview
- `matrix_engine_show_boot_ip(...)` now accepts loop mode
- app currently uses `MATRIX_SCROLL_LOOP_WRAP` for IP rotation (continuous looparound)

### Build

- `idf.py build` for `0067-esp-c3-led-matrix-http` passed after changes.

## Step 18: Add `matrix fliph` console command

User requested a dedicated horizontal flip command to match `flipv` naming.

### Changes

Updated matrix console command parser/help:
- new command: `matrix fliph on|off`
- `fliph` is handled as an alias to existing horizontal orientation toggle path (same behavior as `reverse`)
- usage/examples now include `fliph`

### Validation

- `idf.py build` succeeds after command parser update.

## Step 19: Make default `fliph` / `flipv` configurable via Kconfig

User requested startup orientation defaults to be sdkconfig-driven.

### Added Kconfig options

In `main/Kconfig.projbuild`:
- `CONFIG_TUTORIAL_0067_MATRIX_DEFAULT_FLIPH` (bool, default `n`)
- `CONFIG_TUTORIAL_0067_MATRIX_DEFAULT_FLIPV` (bool, default `y`)

### Engine/App wiring

- Extended `matrix_engine_config_t` with:
  - `default_fliph`
  - `default_flipv`
- `matrix_engine_init()` now applies these defaults to orientation state before initial flush.
- boot log now prints orientation defaults.
- `app_main.c` now maps Kconfig bools to explicit `true/false` via preprocessor guards and passes them in config.

### Defaults file

- `sdkconfig.defaults` now sets:
  - `CONFIG_TUTORIAL_0067_MATRIX_DEFAULT_FLIPH=n`
  - `CONFIG_TUTORIAL_0067_MATRIX_DEFAULT_FLIPV=y`

### Validation

- Initial build failed due undefined bool macro when option is `n`; fixed by using `#if` wrappers in `app_main.c`.
- `idf.py build` succeeds after fix.

## Step 20: Fix `fliph` pixel mapping and add explicit `rot180` flag

User observed `fliph` appeared broken/interleaved. Root cause: previous implementation only reversed module order and did not reverse bit order within each 8-pixel module, so horizontal mirror was incomplete.

### Fix: true horizontal mirror

Updated row flush mapping to treat horizontal flip as full X-axis mirror:
- reverse module order
- reverse bits inside each module byte (`reverse_bits8`)

This produces a real global horizontal flip instead of chunk-level reordering artifacts.

### Added rotate 180 support

Implemented explicit rotate-180 flag in matrix engine:
- state: `rotate_180`
- effective orientation at flush time uses XOR composition:
  - `eff_fliph = reverse_modules XOR rotate_180`
  - `eff_flipv = flip_vertical XOR rotate_180`

This keeps rotate-180 as an independent toggle on top of existing flip settings.

### Interfaces updated

1. Matrix engine/status:
- added `rotate_180` to `matrix_status_t`
- added API: `matrix_engine_set_rotate_180(bool)`

2. Console:
- new command: `matrix rot180 on|off` (alias: `matrix rotate180 on|off`)
- `matrix status` now prints `rot180`

3. JS API:
- added `matrix.setRotate180(on)`
- runtime op `rotate180`
- `matrix.status()` now includes `rotate_180`

4. HTTP status:
- `/api/matrix/status` now includes `rotate_180`

5. Kconfig defaults:
- added `CONFIG_TUTORIAL_0067_MATRIX_DEFAULT_ROTATE_180` (default `n`)
- startup config now supports `default_rotate_180`

### Validation

- `idf.py build` succeeds after all changes.

## Step 21: Build embedded web control panel with animation presets + JS editor + onboard API guide

User requested a richer static HTML server experience: preset triggers, JS editor with loadable scripts, and embedded full JS documentation.

### Web UI changes

Replaced minimal landing page with a full control panel in `main/assets/index.html` featuring:
- Live polling of matrix and JS runtime status.
- Matrix animation preset buttons (`wave`, `scroll`, `drop`) using existing REST API.
- Manual animation form with `mode/text/fps/pause/repeat/loop_mode`.
- JS editor with preset loader and controls for:
  - run (`/api/js/eval`)
  - stop (`/api/js/stop`)
  - soft reset (`/api/js/reset`)
  - hard reset (`/api/js/reset-hard`)
- Embedded API guide viewer panel pulling markdown from a firmware-served endpoint.

### New docs endpoint

Added endpoint:
- `GET /docs/js-api-guide.md`

Implementation details:
- added `main/assets/js-api-guide.md` (copied from `docs/JS-API-GUIDE.md`)
- embedded via `EMBED_TXTFILES` in `main/CMakeLists.txt`
- served by new handler in `main/http_server.c`

### Debugging note

Build initially failed due incorrect symbol names for embedded markdown asset; fixed by using generated symbol names (`_binary_js_api_guide_md_start/_end`).

### Validation

- `idf.py build` succeeds with new web UI + docs endpoint.
