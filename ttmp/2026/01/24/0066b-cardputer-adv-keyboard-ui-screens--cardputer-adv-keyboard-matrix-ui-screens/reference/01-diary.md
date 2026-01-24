---
Title: Diary
Ticket: 0066b-cardputer-adv-keyboard-ui-screens
Status: active
Topics:
    - cardputer
    - keyboard
    - ui
    - m5gfx
    - led-sim
    - console
    - web-ui
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/assets/app.js
      Note: Web UI SPA logic + preview + presets (commit c9b833b)
    - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/http_server.cpp
      Note: Added /api/frame
    - Path: ttmp/2026/01/24/0066b-cardputer-adv-keyboard-ui-screens--cardputer-adv-keyboard-matrix-ui-screens/scripts/04-web-smoke.sh
      Note: Smoke-test helper (commit fde611c)
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-24T00:46:15.329644186-05:00
WhatFor: ""
WhenToUse: ""
---


# Diary

## Goal

Capture (with high fidelity) the design + implementation journey for adding a Cardputer-ADV keyboard-driven UI: how we wire the keyboard matrix scan, define input semantics (modifiers, chords), and implement each on-device screen.

## Context

We are implementing a keyboard-driven on-device UI for Cardputer-ADV-style firmwares in this repo. Two “spec” documents were provided as local files and imported into this ticket workspace so the rest of the work can reference stable paths.

Likely target firmware for implementation: `0066-cardputer-adv-ledchain-gfx-sim` (already has always-on LED preview and M5GFX rendering).

## Quick Reference

N/A (this diary is the narrative record; design docs contain the durable specs).

## Usage Examples

N/A

## Related

- `../analysis/01-cardputer-adv-keyboard-matrix-setup.md`
- `../design/01-ui-screen-designs-per-screen.md`

---

## Step 1: Create ticket, import UI specs, and pick the target firmware (2026-01-24)

This step establishes the workspace and the “ground truth” inputs. Two markdown documents (`/tmp/cardputer-ui.md` and `/tmp/cardputer-webui.md`) are treated as external sources and imported into the ticket so future work can cite stable paths. I also identified the most likely code target for implementation: tutorial `0066-cardputer-adv-ledchain-gfx-sim`, which already has the always-on LED preview screen and multiple control planes (console + web + JS).

The next step is to translate those specs into a keyboard matrix setup doc and a per-screen implementation design, then implement incrementally with small commits and docmgr task checkoffs.

### Prompt Context

**User prompt (verbatim):**
> Create a new ticket 0066b-..., use docmgr import file --file ... --ticket ...
>
> Import /tmp/cardputer-ui.md and /tmp/cardputer-webui.md and then create a detailed document on how to set up the keyboard matrix for the cardputer adv, and then a set of detailed implementation design for each screen separately. create tasks and add to ticket, and then implement it, committing and checking off tasks, and keeping a detailed diary.

**Assistant interpretation:** Create ticket 0066b, import two external markdown specs, author detailed keyboard-matrix and screen-design docs, then implement the UI in the firmware with tasks, frequent diary updates, and incremental commits.

**Inferred user intent:** Move from “UI mock text” to real firmware: keyboard-driven overlays for the simulator on Cardputer-ADV, with a clear design and an auditable implementation trail.

**Commit (code):** N/A (workspace/bootstrap step; code changes will come after design docs and tasks are created).

### What I did
- Created docmgr ticket `0066b-cardputer-adv-keyboard-ui-screens`.
- Created docs:
  - `analysis/01-cardputer-adv-keyboard-matrix-setup.md`
  - `design/01-ui-screen-designs-per-screen.md`
  - `reference/01-diary.md`
- Imported external specs into the ticket sources using the requested command form:
  - `docmgr import file --ticket 0066b-cardputer-adv-keyboard-ui-screens --file /tmp/cardputer-ui.md --name "Cardputer UI mock"`
  - `docmgr import file --ticket 0066b-cardputer-adv-keyboard-ui-screens --file /tmp/cardputer-webui.md --name "Cardputer Web UI mock"`
- Located the existing keyboard matrix component we should reuse:
  - `components/cardputer_kb` (scanner + bindings + docs)

### Why
- Importing `/tmp` documents into `ttmp/.../sources/` makes them stable, reviewable inputs for the rest of the work.
- Reusing `components/cardputer_kb` avoids re-deriving the vendor matrix scan wiring and Fn-chord conventions.

### What worked
- `docmgr import file` correctly copied the markdown sources into the ticket workspace and updated source metadata.

### What didn't work
- N/A

### What I learned
- This repo already has an explicit, reusable keyboard scan component plus “how to scan” documentation; the work is more about integrating it cleanly into the target UI loop.

### What was tricky to build
- N/A (no code yet).

### What warrants a second pair of eyes
- Confirming the target firmware/screen set: I’m assuming we implement the keyboard UI into `0066-cardputer-adv-ledchain-gfx-sim` because it already has the always-on preview. If you intended a different tutorial as the target, tell me now before we implement.

### What should be done in the future
- Author the keyboard matrix setup doc with concrete pin mappings and initialization.
- Author per-screen designs (menu/pattern/params/color sliders/JS/help) with state machines and input handling.
- Turn those designs into an implementation task list and start coding.

### Code review instructions
- Review the imported sources:
  - `ttmp/2026/01/24/0066b-cardputer-adv-keyboard-ui-screens--cardputer-adv-keyboard-matrix-ui-screens/sources/local/Cardputer UI mock.md`
  - `ttmp/2026/01/24/0066b-cardputer-adv-keyboard-ui-screens--cardputer-adv-keyboard-matrix-ui-screens/sources/local/Cardputer Web UI mock.md`
- Review the keyboard component docs:
  - `components/cardputer_kb/README.md`
  - `components/cardputer_kb/REFERENCE.md`
  - `components/cardputer_kb/TUTORIAL.md`

### Technical details
- The keyboard scan API is in `components/cardputer_kb/include/cardputer_kb/scanner.h` (`MatrixScanner::init()` and `MatrixScanner::scan()`).

---

## Step 2: Write keyboard + per-screen design docs (commit 99b46ae) (2026-01-24)

This step turns the imported UI mock text into two implementable documents: a keyboard matrix setup analysis (pins, scan semantics, event generation) and a per-screen UI design spec (mode machine, input conventions, and screen-by-screen behaviors). The aim is to make the implementation phase mostly mechanical: each screen becomes a state + renderer + input handler with clear transitions and side-effects.

I also related the design docs to the specific code that anchors them (the `cardputer_kb` scanner and the existing 0066 preview UI) so future readers can jump directly from prose to source.

### Prompt Context

**User prompt (verbatim):**
> Create a new ticket 0066b-..., use docmgr import file --file ... --ticket ...
>
> Import /tmp/cardputer-ui.md and /tmp/cardputer-webui.md and then create a detailed document on how to set up the keyboard matrix for the cardputer adv, and then a set of detailed implementation design for each screen separately. create tasks and add to ticket, and then implement it, committing and checking off tasks, and keeping a detailed diary.

**Assistant interpretation:** Produce detailed keyboard-matrix and per-screen design docs grounded in existing repo components, then implement.

**Inferred user intent:** Reduce ambiguity and iteration cost by making the UI and keyboard semantics explicit before coding.

**Commit (code):** 99b46ae — "0066b: add keyboard matrix + per-screen UI designs"

### What I did
- Wrote:
  - `analysis/01-cardputer-adv-keyboard-matrix-setup.md`
  - `design/01-ui-screen-designs-per-screen.md`
- Linked those docs to key reference files via `docmgr doc relate`:
  - `components/cardputer_kb/*`
  - `0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp`
  - `0030-cardputer-console-eventbus/main/app_main.cpp`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp`

### Why
- The keyboard matrix layer is easy to “kind of implement” but hard to get consistently right without an explicit contract (pins, active-low, chords, repeat).
- A per-screen spec prevents the classic embedded UI failure mode: adding features ad-hoc until behavior becomes inconsistent and untestable.

### What worked
- The repo already has stable, reusable keyboard infrastructure (`cardputer_kb`) so the docs could be grounded in real code rather than guesswork.

### What didn't work
- N/A

### What I learned
- The captured binding table (`bindings_m5cardputer_captured.h`) is the cleanest way to standardize Fn navigation without hardcoding keyNums in every firmware.

### What was tricky to build
- Balancing completeness vs MVP: the UI mock includes many screens; the design doc explicitly defines MVP subsets (e.g., JS Lab examples-only) so implementation can be incremental.

### What warrants a second pair of eyes
- Confirm whether the “Cardputer-ADV” keyboard wiring differs from the baseline Cardputer. The scanner supports alt IN0/IN1 autodetect, but if ADV changes more than that we should verify with the calibrator.

### What should be done in the future
- Implement the keyboard task + UI state machine in tutorial 0066.

### Code review instructions
- Start with:
  - `ttmp/2026/01/24/0066b-cardputer-adv-keyboard-ui-screens--cardputer-adv-keyboard-matrix-ui-screens/analysis/01-cardputer-adv-keyboard-matrix-setup.md`
  - `ttmp/2026/01/24/0066b-cardputer-adv-keyboard-ui-screens--cardputer-adv-keyboard-matrix-ui-screens/design/01-ui-screen-designs-per-screen.md`

### Technical details
- N/A (documentation step).

---

## Step 3: Implement keyboard scan task + UI event queue in 0066 (commit 99002e2) (2026-01-24)

This step makes keyboard input real in the target firmware. The key architectural choice is to isolate matrix scanning into its own FreeRTOS task that pushes compact key events into a bounded queue. The UI task consumes those events and (for now) simply displays the last key pressed; in the next step, the UI state machine will turn those events into overlay navigation and parameter edits.

This mirrors the repo’s proven pattern from `0030` (keyboard task + event posting), but uses a simpler queue instead of `esp_event`, since tutorial 0066 currently doesn’t use an event loop.

### Prompt Context

**User prompt (verbatim):**
> Create a new ticket 0066b-..., use docmgr import file --file ... --ticket ...
>
> Import /tmp/cardputer-ui.md and /tmp/cardputer-webui.md and then create a detailed document on how to set up the keyboard matrix for the cardputer adv, and then a set of detailed implementation design for each screen separately. create tasks and add to ticket, and then implement it, committing and checking off tasks, and keeping a detailed diary.

**Assistant interpretation:** Start implementing the on-device keyboard UI in tutorial 0066, using the repo’s keyboard scan component.

**Inferred user intent:** Get to a working keyboard-driven UI quickly, without destabilizing the existing simulator.

**Commit (code):** 99002e2 — "0066: add cardputer keyboard scan task + event queue"

### What I did
- Added `cardputer_kb` to tutorial 0066 build:
  - `0066-cardputer-adv-ledchain-gfx-sim/CMakeLists.txt` (`EXTRA_COMPONENT_DIRS`)
  - `0066-cardputer-adv-ledchain-gfx-sim/main/CMakeLists.txt` (`REQUIRES cardputer_kb`)
- Added a compact key-event layer and scanner task:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.h`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp`
- Added a Kconfig knob for scan period:
  - `CONFIG_TUTORIAL_0066_KB_SCAN_PERIOD_MS` in `0066-cardputer-adv-ledchain-gfx-sim/main/Kconfig.projbuild`
- Integrated into the existing UI loop:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp` creates a queue, starts the keyboard task, and displays the last key for quick smoke-testing.

### Why
- Keeping scanning in a dedicated task decouples input responsiveness from rendering cadence and makes it easy to add repeat/debounce later.
- A fixed-size `ui_key_event_t` avoids dynamic allocation in the message path.

### What worked
- `idf.py -C 0066-cardputer-adv-ledchain-gfx-sim build` succeeds with the new component.

### What didn't work
- N/A

### What I learned
- Reusing the same key legend table as `0022` keeps key strings consistent across projects (e.g., `"tab"`, `"enter"`, `"bksp"`).

### What was tricky to build
- Avoiding accidental coupling to “ASCII”: the UI needs both semantic nav actions and raw key strings; representing those cleanly in a small struct took care.

### What warrants a second pair of eyes
- Whether we should add best-effort drop counters/logs when the UI queue is full. (Currently it silently drops events if the queue is full.)

### What should be done in the future
- Implement the overlay state machine and connect `tab/p/e/...` shortcuts and arrow navigation to actual UI modes.

### Code review instructions
- `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp`
- `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp`

### Technical details
- The keyboard task emits:
  - `UI_KEY_*` events for semantic actions (from the captured Fn binding table)
  - `UI_KEY_TEXT` for raw key edges (excluding modifiers and keys used in the active chord)

---

## Step 4: Implement on-device overlay UI state machine (commit 9e6ae46) (2026-01-24)

This step turns keyboard events into real on-device UI behavior. The LED preview remains “the floor”; overlays are drawn on top and are purely modal state. The result is a usable MVP: TAB opens a menu, P selects patterns, E edits parameters, B/F adjust brightness/frame timing, and Fn+H shows help. Color parameters open a small color editor that supports arrow adjustments, TAB to switch channel, and hex typing.

The system is intentionally conservative: all changes are immediate and applied directly to the simulator engine so the preview updates instantly. This aligns with the existing simulator architecture and reduces the need for a complex “draft/apply” model in the first pass.

### Prompt Context

**User prompt (verbatim):**
> Create a new ticket 0066b-..., use docmgr import file --file ... --ticket ...
>
> Import /tmp/cardputer-ui.md and /tmp/cardputer-webui.md and then create a detailed document on how to set up the keyboard matrix for the cardputer adv, and then a set of detailed implementation design for each screen separately. create tasks and add to ticket, and then implement it, committing and checking off tasks, and keeping a detailed diary.

**Assistant interpretation:** Implement the on-device screens described in the UI mock as a keyboard-driven overlay system.

**Inferred user intent:** Make the simulator usable without the serial console: a “console-less” control plane on the device itself.

**Commit (code):** 9e6ae46 — "0066: add keyboard-driven overlay UI (menu/pattern/params)"

### What I did
- Added an overlay UI module:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/ui_overlay.h`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/ui_overlay.cpp`
- Integrated overlays into the existing render loop:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp`
- Implemented these screens/modes:
  - Live (always-on preview + top status + bottom hint strip)
  - Menu (TAB)
  - Pattern select (P or Menu→Pattern)
  - Params editor (E or Menu→Params)
  - Brightness slider (B or Menu→Brightness)
  - Frame slider (F or Menu→Frame)
  - Help (Fn+H or Menu→Help)
  - Color editor (Enter on color params; TAB switches channel; hex typing supported)

### Why
- The UI mock’s core value is that the preview is always visible and control is modal + keyboard-driven.
- Keeping the preview always-on ensures edits feel safe: you see changes immediately and can back out quickly.

### What worked
- The overlay design integrates cleanly with the existing M5Canvas rendering (single sprite).
- Immediate apply produces fast feedback and avoids “state divergence” between UI draft and engine config.

### What didn't work
- Presets and JS Lab are currently implemented as “MVP stub” modes that just show a hint and close on Enter; they’re tracked as a later task.

### What I learned
- It’s worth representing “semantic keys” separately from raw text keys; it keeps overlay navigation consistent even if the physical chord mapping changes.

### What was tricky to build
- Managing color editing without a full text widget: hex typing had to be “single-character events” with an internal buffer.
- Ensuring Back/Esc semantics are consistent: Back always cancels the color editor first, then closes overlays.

### What warrants a second pair of eyes
- Parameter list completeness: Chase currently exposes `fg` but not `bg` in the params list (an oversight to fix next); the design doc expects both.
- Pattern-specific parameter ranges: the current clamps are pragmatic but may need tuning.

### What should be done in the future
- Implement Presets (RAM slots) and JS Lab (examples-only) screens.
- Expand the web UI into multiple screens matching the imported web spec.

### Code review instructions
- Start with:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/ui_overlay.cpp`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp`

### Technical details
- Modifier step sizes are implemented via `step_for_mods()`:
  - Fn → micro step
  - Alt → medium step
  - Ctrl → large step

---

## Step 5: Implement on-device Presets + JS Lab (examples-only) overlays (commits 584f586, dbba9ef) (2026-01-24)

This step upgrades the “stub” Presets/JS Lab overlays into a usable MVP. The goal is not to build a full editor on-device, but to provide a reliable keyboard-driven way to (a) save/restore pattern configurations quickly, and (b) run a small library of canned JavaScript “recipes” that exercise the simulator + GPIO API without depending on network access.

It also makes the system more coherent: the JS service is started early (in `app_main`) so the on-device overlays can run examples even when the console/web UI haven’t been started yet.

### Prompt Context

**User prompt (verbatim):**
> Create a new ticket 0066b-..., use docmgr import file --file ... --ticket ...
>
> Import /tmp/cardputer-ui.md and /tmp/cardputer-webui.md and then create a detailed document on how to set up the keyboard matrix for the cardputer adv, and then a set of detailed implementation design for each screen separately. create tasks and add to ticket, and then implement it, committing and checking off tasks, and keeping a detailed diary.

**Assistant interpretation:** Continue implementing each UI screen from the mock specs, starting with simple but functional on-device Presets and JS Lab flows.

**Inferred user intent:** Be able to demo UI flows and pattern changes without needing the web UI first; keep iteration fast on hardware via keyboard.

**Commit (code):**
- 584f586 — "0066: implement presets and JS Lab examples overlay"
- dbba9ef — "0066: start JS service early for on-device UI"

### What I did
- Implemented Presets overlay (RAM-only) with basic actions (save/load/clear) and keyboard navigation.
- Implemented JS Lab overlay as an “examples runner” (select example → run → show output).
- Started the JS service early in `app_main` so UI overlays can call `js_service_eval()` regardless of console/http startup ordering.

### Why
- Presets and examples are the fastest route to “cool demos” and debugging: they turn repeated multi-step interactions into 1–2 keystrokes.
- Starting JS early avoids UI behavior depending on whether the web server (or console) has already initialized.

### What worked
- The on-device UI can now run JS examples and apply pattern changes directly.

### What didn't work
- Persistence (NVS/device storage) is not implemented; presets are RAM-only.

### What I learned
- UI-driven JS is best treated as a “service dependency” (start once, always available) rather than being tied to a specific control plane (console/http).

### What was tricky to build
- Keeping the JS example runner safe: examples should be short and predictable, and failures need to surface clearly in a small overlay.

### What warrants a second pair of eyes
- Whether presets should be backed by NVS now (more complexity) or remain RAM-only until the UX stabilizes.

### What should be done in the future
- Add NVS-backed presets (optional), once the exact preset schema is stable.

### Code review instructions
- Start with:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/ui_overlay.cpp`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/app_main.cpp`

### Technical details
- Presets are stored as an in-memory list of pattern configs; no JSON or filesystem is used on-device.

---

## Step 6: Expand web UI into multi-screen layout + add REST endpoints for frame/JS reset/GPIO state (commit c9b833b) (2026-01-24)

This step turns the original “two panels + a raw websocket log” page into a multi-screen single-page UI matching the imported web mock: Dashboard, Patterns, JS Lab, GPIO, Presets, and System. The emphasis is on a simple implementation with no bundler: pure HTML/CSS/JS served as embedded assets.

To support a real browser preview and a richer JS Lab workflow, I added a few small endpoints: `GET /api/frame` for raw RGB bytes (to draw the preview grid), `POST /api/js/reset` to restart the VM and stop runaway timers, `GET /api/js/mem` for memory introspection, and `GET /api/gpio/status` so the browser can show G3/G4 levels without relying on JS-side bookkeeping.

### Prompt Context

**User prompt (verbatim):**
> Create a new ticket 0066b-..., use docmgr import file --file ... --ticket ...
>
> Import /tmp/cardputer-ui.md and /tmp/cardputer-webui.md and then create a detailed document on how to set up the keyboard matrix for the cardputer adv, and then a set of detailed implementation design for each screen separately. create tasks and add to ticket, and then implement it, committing and checking off tasks, and keeping a detailed diary.

**Assistant interpretation:** Implement the remaining “web UI screens” portion of the UI mock, using the simplest viable approach (embedded static assets + a few REST endpoints).

**Inferred user intent:** Be able to control patterns, run JS, and interact with GPIO from a browser during demos, without any extra tooling.

**Commit (code):** c9b833b — "0066: expand web UI screens; add frame/js reset/gpio endpoints"

### What I did
- Updated embedded assets:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/assets/index.html` (tabs + screen sections)
  - `0066-cardputer-adv-ledchain-gfx-sim/main/assets/app.js` (SPA router, polling, preview canvas, presets in localStorage)
  - `0066-cardputer-adv-ledchain-gfx-sim/main/assets/app.css` (topbar + tabs + layout)
- Added endpoints:
  - `GET /api/frame` → `application/octet-stream` RGB bytes (size = `3 * led_count`)
  - `POST /api/js/reset` → reset VM (kills timers)
  - `GET /api/js/mem` → plain-text memory dump
  - `GET /api/gpio/status` → current levels of configured G3/G4 pins
- Extended `GET /api/status` with uptime + heap stats + frame seq metadata.

### Why
- The mock implies a “desktop-ish” multi-screen UI; tabs are the simplest way to deliver that without extra dependencies.
- `GET /api/frame` avoids JSON bloat and is trivial to render in JS (50 LEDs → 150 bytes).
- `POST /api/js/reset` is the pragmatic “stop everything” escape hatch when timers are running.

### What worked
- `idf.py build` succeeded after changes.

### What didn't work
- N/A (implementation stayed within existing architecture).

### What I learned
- For 50 LEDs, a binary “frame fetch” endpoint is a clean middle ground: efficient over the wire and simple to consume in the browser.

### What was tricky to build
- Avoiding accidental “param stomping”: dashboard quick controls should not implicitly overwrite pattern-specific parameters unless the Patterns screen is used.

### What warrants a second pair of eyes
- HTTP API surface area: confirm these endpoint names are acceptable long-term (especially `/api/frame` format + headers).

### What should be done in the future
- If we want *streaming* preview, add a websocket “latest-frame” message to avoid polling.

### Code review instructions
- Start with:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/http_server.cpp`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/assets/app.js`
- Validate:
  - `idf.py -C 0066-cardputer-adv-ledchain-gfx-sim build`
  - Visit `http://<device-ip>/` and verify tabs + preview update.

### Technical details
- `/api/frame` uses headers:
  - `x-led-count: <N>`
  - `x-frame-seq: <seq>`

---

## Step 7: Add ticket scripts for build/flash/smoke workflows (commit fde611c) (2026-01-24)

This step adds small, ticket-local scripts for the tight inner loop: build, flash, monitor, and a curl-based web smoke test. The scripts intentionally live under the ticket so they can evolve along with the docs and stay discoverable during reviews.

### Prompt Context

**User prompt (verbatim):** "Store all scripts you write, even temporary ones, in the scripts/ folder of the ticket."

**Assistant interpretation:** Put any helper scripts under the ticket workspace rather than scattering them in the repo root.

**Inferred user intent:** Keep the ticket self-contained and auditable: reviewers can reproduce results without guessing which commands were run.

**Commit (code):** fde611c — "0066b: add build/flash/web-smoke scripts"

### What I did
- Added:
  - `ttmp/.../scripts/01-build-0066.sh`
  - `ttmp/.../scripts/02-flash-0066.sh`
  - `ttmp/.../scripts/03-monitor-0066.sh`
  - `ttmp/.../scripts/04-web-smoke.sh`
  - `ttmp/.../scripts/README.md`

### Why
- Reduce “command drift” across runs and make it easy to validate on another machine (or later date).

### What worked
- The scripts are minimal wrappers around `idf.py` and `curl`, with configurable `PORT`/`HOST`.

### What didn't work
- N/A

### What I learned
- `idf.py monitor` requires a TTY stdin; scripts should call that out explicitly.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- Add a websocket smoke script if we standardize on a tool like `websocat`.

### Code review instructions
- Read `ttmp/.../scripts/README.md` and sanity-check the defaults (`PORT=/dev/ttyACM0`, `HOST=http://192.168.4.1`).

### Technical details
- Scripts compute `ROOT_DIR` relative to the ticket folder so they can be run from anywhere.

---

## Step 8: Flash 0066 onto Cardputer (/dev/ttyACM0) and attempt validation (2026-01-24)

This step flashes the current `0066-cardputer-adv-ledchain-gfx-sim` firmware to the attached Cardputer device. In this environment, we can invoke `idf.py flash`, but `idf.py monitor` fails because stdin is not a TTY in the automation harness. That means full runtime verification (keyboard + screen + web UI) still needs a human terminal for monitoring and a network connection to the device to hit HTTP endpoints.

### Prompt Context

**User prompt (verbatim):** "btw I attached the cardputer to ttyACM0 so if you want to flash and at least verify you can start the simulations and all, feel free. You can use tmux to read from idf.py monitor / restart the monitoring, etc..."

**Assistant interpretation:** Flash the firmware to `/dev/ttyACM0` and do a basic smoke test that the app runs.

**Inferred user intent:** Confirm the changes are not theoretical: they boot and can be interacted with on hardware.

**Commit (code):** N/A (flash/validation step)

### What I did
- Flashed:
  - `idf.py -C 0066-cardputer-adv-ledchain-gfx-sim -p /dev/ttyACM0 flash`
- Attempted monitor (expected failure in this harness):
  - `idf.py -C 0066-cardputer-adv-ledchain-gfx-sim -p /dev/ttyACM0 monitor`
- Attempted a host-side HTTP probe (failed because the host wasn’t connected to the device’s AP):
  - `curl http://192.168.4.1/api/status`

### Why
- Flashing is the fastest “integration test” for an embedded UI: it catches missing Kconfig, linking issues, or runtime asserts that unit tests won’t.

### What worked
- `idf.py flash` succeeded (USB Serial/JTAG).

### What didn't work
- `idf.py monitor` failed with:
  - "Error: Monitor requires standard input to be attached to TTY."
- HTTP probe timed out (likely host not on the device network).

### What I learned
- For automated runs, we can reliably flash, but monitor output requires a real interactive terminal.

### What was tricky to build
- N/A (environment limitation).

### What warrants a second pair of eyes
- Confirm on-device behavior after flash:
  - keyboard overlays open/close and apply changes
  - web UI tabs load and preview updates

### What should be done in the future
- N/A (remaining work is manual validation).

### Code review instructions
- Run in a real terminal:
  - `PORT=/dev/ttyACM0 ttmp/.../scripts/03-monitor-0066.sh`
- Connect host to the device AP (or STA IP) and open `/` in a browser, then run:
  - `HOST=http://<device-ip> ttmp/.../scripts/04-web-smoke.sh`

### Technical details
- `idf.py monitor` TTY requirement is a hard constraint of `idf_monitor.py` (expects interactive keystrokes).
