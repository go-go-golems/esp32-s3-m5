---
Title: Investigation Diary
Ticket: ESP-23-PHOTO-TIMER
Status: active
Topics:
    - cardputer
    - cardputer-adv
    - esp32-s3
    - firmware
    - timer
    - photo-development
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0071-cardputer-adv-photo-timer/CMakeLists.txt
      Note: Step 2 scaffold commit f401c35
    - Path: 0071-cardputer-adv-photo-timer/main/app_main.cpp
      Note: Step 4 implementation commit d50bdef
    - Path: 0071-cardputer-adv-photo-timer/main/preset_store.cpp
      Note: Step 3 implementation commit d662c86
    - Path: 0071-cardputer-adv-photo-timer/main/timer_engine.cpp
      Note: Step 3 implementation commit d662c86
    - Path: ttmp/2026/02/28/ESP-23-PHOTO-TIMER--cardputer-adv-photo-development-timer-firmware/design-doc/01-cardputer-adv-photo-development-timer-implementation-plan.md
      Note: Diary step produced this design plan
    - Path: ttmp/2026/02/28/ESP-23-PHOTO-TIMER--cardputer-adv-photo-development-timer-firmware/tasks.md
      Note: Diary step established detailed task breakdown
ExternalSources: []
Summary: Chronological implementation diary for ESP-23-PHOTO-TIMER.
LastUpdated: 2026-03-01T00:00:00Z
WhatFor: Record implementation steps, outcomes, failures, and review guidance.
WhenToUse: Use during and after implementation to continue work without re-discovery.
---



# Diary

## Goal

Capture the step-by-step implementation work for the Cardputer-ADV photo timer firmware, including technical decisions, failures, and validation instructions.

## Step 1: Ticket Setup and Evidence Collection

This step established a dedicated ticket workspace and collected concrete, file-backed evidence from existing firmwares to avoid speculative design. The goal was to define a build plan that can be implemented directly with known-good modules rather than inventing new infrastructure.

The output of this step is a detailed design document anchored to code references from `0038`, `0047`, `0048`, `0039`, and shared components (`wifi_mgr`, `wifi_console`, `httpd_assets_embed`). This gives a defensible foundation for implementation tasks.

### Prompt Context

**User prompt (verbatim):** "Create a new ticket for creating a timer for photo development on the cardputer-adv. The idea is to have times for development like C41 cinestill and you can select a preset and all those times are stored in config files that get loaded on boot start (or can be uploaded through a little web interface). There is an encoder attached to navigate things and all.

Analyze from existing firmwares what you can use to build this new firmware and then write a detailed implementation plan on how to build the functionality and how it is going to work.

Upload the implementation plan to remarkable. TGhen create tasks for the firmware (detailed) and work on them one by one and commit as appropriate and keep a detailed diary as you work."

**Assistant interpretation:** Create a new ticket, produce an evidence-backed implementation plan from existing firmware code, upload that plan to reMarkable, and then implement the firmware through detailed tracked tasks with commits and ongoing diary entries.

**Inferred user intent:** Build a real, usable Cardputer-ADV photo process timer firmware with persistent presets and web-assisted configuration, while preserving strong execution traceability.

### What I did

- Created ticket `ESP-23-PHOTO-TIMER` and initialized docs (`index.md`, `tasks.md`, `changelog.md`, design-doc, diary).
- Audited reusable projects/components with line-anchored evidence:
  - encoder + LVGL: `0047-cardputer-adv-lvgl-chain-encoder-list/main/*`
  - Cardputer-ADV runtime/display/console: `0038-cardputer-adv-serial-terminal/main/*`
  - Wi-Fi + HTTP patterns: `0048-cardputer-js-web/main/*`
  - shared net/web helpers: `components/wifi_mgr/*`, `components/wifi_console/*`, `components/httpd_assets_embed/*`
  - SPIFFS pattern: `0039-cardputer-adv-js-gpio-exercizer/main/storage/Spiffs.*`
- Wrote comprehensive implementation plan in ticket design doc.
- Expanded `tasks.md` with detailed execution checklist.

### Why

- The requested firmware spans UI, input, persistence, and networking; existing implementations already cover each capability. Reuse reduces integration risk and time.
- A detailed task plan is required before coding to support staged commits and continuous diary updates.

### What worked

- `docmgr` ticket creation and doc scaffolding worked as expected.
- Existing firmware coverage was sufficient to define a full architecture without relying on external references.
- The current repo patterns already align with USB Serial/JTAG console guidance, reducing console/UART conflict risk for encoder integration.

### What didn't work

- `sed -n '1,260p' 0039-cardputer-adv-js-gpio-exercizer/README.md` failed because that file does not exist in that project path.

### What I learned

- `0047` and `0048` contain two close variants of chain-encoder UART handling; `0048` includes click-kind handling (`single/double/long`) which is useful for richer timer controls.
- Shared components (`wifi_mgr`, `wifi_console`, `httpd_assets_embed`) are mature enough to avoid firmware-local rewrites.

### What was tricky to build

- The codebase has many historical firmwares with similar capabilities but different implementation style (C vs C++, LVGL vs canvas, per-project networking stacks). The challenge was selecting a minimal, coherent baseline and avoiding a hybrid that increases maintenance burden.

### What warrants a second pair of eyes

- Final preset JSON schema should be reviewed for forward compatibility/versioning.
- Thread-safety approach for shared timer/config state across LVGL/UI loop and HTTP handlers should be reviewed before implementation is finalized.

### What should be done in the future

- Convert current open questions in the design doc into explicit acceptance criteria before closing implementation.

### Code review instructions

- Start with the design doc to validate architecture and reuse rationale:
  - `ttmp/2026/02/28/ESP-23-PHOTO-TIMER--cardputer-adv-photo-development-timer-firmware/design-doc/01-cardputer-adv-photo-development-timer-implementation-plan.md`
- Then inspect task breakdown:
  - `ttmp/2026/02/28/ESP-23-PHOTO-TIMER--cardputer-adv-photo-development-timer-firmware/tasks.md`

### Technical details

- Key evidence files (line-anchored) are listed in the design doc `References` section.
- Ticket path:
  - `ttmp/2026/02/28/ESP-23-PHOTO-TIMER--cardputer-adv-photo-development-timer-firmware`

## Step 2: Scaffold Firmware Project 0071

This step created the new firmware project directory and baseline build/runtime configuration as a concrete execution target for the ticket. The objective was to establish a stable project frame before introducing timer logic and networking behavior.

The resulting scaffold included the project-level CMake wiring, partition defaults, sdkconfig defaults, a build helper script, Kconfig options, and an embedded minimal web page skeleton.

### Prompt Context

**User prompt (verbatim):** (same as Step 1)

**Assistant interpretation:** Start implementation with a dedicated firmware project and wire build/dependency configuration first.

**Inferred user intent:** Ensure implementation is happening in a real, runnable firmware project, not only documentation.

**Commit (code):** `f401c3535c78ab54e46b8a56c0503da042680ca7` - "0071: scaffold photo timer firmware project"

### What I did

- Created `0071-cardputer-adv-photo-timer` from a 0047-style project skeleton.
- Added project/runtime config files:
  - `CMakeLists.txt`, `partitions.csv`, `sdkconfig.defaults`, `build.sh`, `README.md`
  - `main/CMakeLists.txt`, `main/Kconfig.projbuild`, `main/idf_component.yml`
  - `main/assets/index.html`
- Wired shared components for Wi-Fi and embedded HTTP assets in project CMake.

### Why

- A clean scaffold is required before implementing runtime modules, otherwise later changes become hard to review and split into meaningful commits.

### What worked

- Project creation and initial component wiring succeeded without conflicts.
- Commit boundary stayed focused on structural scaffolding only.

### What didn't work

- N/A

### What I learned

- `0071` can reuse the same repository-level component directories used by previous cardputer projects without additional path normalization.

### What was tricky to build

- Maintaining a scaffold that supports both LVGL + encoder runtime and future HTTP/Wi-Fi integration required selecting dependencies carefully up front to avoid immediate rework.

### What warrants a second pair of eyes

- Verify that the chosen dependency surface in `main/CMakeLists.txt` is minimal and does not pull unnecessary components.

### What should be done in the future

- Run a full build in an environment with a valid ESP-IDF Python env to confirm no missing manifest/dependency edges.

### Code review instructions

- Start with:
  - `0071-cardputer-adv-photo-timer/CMakeLists.txt`
  - `0071-cardputer-adv-photo-timer/main/CMakeLists.txt`
  - `0071-cardputer-adv-photo-timer/main/Kconfig.projbuild`
- Then verify default runtime assumptions:
  - `0071-cardputer-adv-photo-timer/sdkconfig.defaults`
  - `0071-cardputer-adv-photo-timer/partitions.csv`

### Technical details

- Embedded web skeleton introduced at:
  - `0071-cardputer-adv-photo-timer/main/assets/index.html`

## Step 3: Implement Preset Storage and Timer Engine Core

This step implemented the core firmware domain logic: persistent preset storage and the timer execution engine. It converted the design doc schema/state-machine into concrete C++ modules with validation, default seeding, and runtime action APIs.

The result was a complete core backend that can load/save presets, bind active presets, and execute timer controls independent of UI transport.

### Prompt Context

**User prompt (verbatim):** (same as Step 1)

**Assistant interpretation:** Implement the core behavior first: preset persistence and timed-step execution.

**Inferred user intent:** Make the firmware functional at its core before layering controls/UI.

**Commit (code):** `d662c86fd97aaa63ff587776370b590292c05b34` - "0071: add preset storage and timer state engine"

### What I did

- Added domain and engine modules:
  - `main/photo_timer_types.h`
  - `main/timer_engine.h`
  - `main/timer_engine.cpp`
  - `main/preset_store.h`
  - `main/preset_store.cpp`
  - `main/app_state.h`
  - `main/app_state.cpp`
- Implemented JSON schema parse/serialize and validation with `cJSON`.
- Implemented SPIFFS-backed load-or-seed behavior for `/spiffs/presets.json`.
- Added default presets including C41 CineStill-style workflow.
- Implemented timer state machine actions: start, pause, resume, toggle, next, reset.
- Added app-wide synchronized state wrapper exposing action/snapshot/config APIs.

### Why

- This is the minimum logic necessary to satisfy the feature request independent of UI choices.

### What worked

- Module boundaries stayed clean: storage, engine, and app-state orchestration are separated.
- Commit remained scoped to backend/runtime logic only.

### What didn't work

- N/A

### What I learned

- Using a copyable `TimerConfig` and an `app_state` facade keeps HTTP/UI integration straightforward while preserving thread-safe update points.

### What was tricky to build

- Balancing strict JSON validation with resilient boot behavior required a fallback strategy: malformed storage restores defaults to keep device usable.

### What warrants a second pair of eyes

- Review timer transition logic in `TimerEngine::advance_step_locked` and pause/resume timing math.
- Review validation error paths in `preset_store_parse_json` for missing edge cases.

### What should be done in the future

- Add host/unit tests for parser validation and timer transitions once build env is available.

### Code review instructions

- Start with:
  - `0071-cardputer-adv-photo-timer/main/preset_store.cpp`
  - `0071-cardputer-adv-photo-timer/main/timer_engine.cpp`
  - `0071-cardputer-adv-photo-timer/main/app_state.cpp`
- Validate interfaces in corresponding `.h` files.

### Technical details

- Config path constant:
  - `preset_store_path() -> /spiffs/presets.json`
- Action API surface:
  - `app_state_timer_action(action, preset_id)`

## Step 4: Wire Encoder/LVGL UI and HTTP Control Surface

This step integrated all control surfaces: on-device encoder navigation with LVGL and remote preset/control APIs over HTTP. It completed the end-to-end functional flow from boot-loaded presets to interactive timer controls and web-uploaded configuration replacement.

The implementation also included an attempted build verification, which surfaced an ESP-IDF environment blocker unrelated to source code changes.

### Prompt Context

**User prompt (verbatim):** (same as Step 1)

**Assistant interpretation:** Complete user-facing interaction layers (encoder UI + web upload/control) and checkpoint via focused commit.

**Inferred user intent:** A usable firmware with both physical and web-based interaction paths.

**Commit (code):** `d50bdefbc5bff4b8452729c62576924fc70123c5` - "0071: wire LVGL UI, encoder input, and HTTP control surface"

### What I did

- Added interaction/runtime integration modules:
  - `main/app_main.cpp`
  - `main/http_server.h`
  - `main/http_server.cpp`
  - `main/chain_encoder_uart.h`
  - `main/chain_encoder_uart.cpp`
  - `main/lvgl_port_m5gfx.h`
  - `main/lvgl_port_m5gfx.cpp`
- Implemented LVGL list UI:
  - action rows (`Start/Pause`, `Next`, `Reset`)
  - preset selection rows (active preset marker)
  - status labels with live countdown.
- Wired chain-encoder input to LVGL encoder indev read callback.
- Added HTTP endpoints:
  - `GET /api/status`
  - `GET /api/presets`
  - `POST /api/presets`
  - `POST /api/control`
- Wired Wi-Fi startup (`wifi_mgr` + `wifi_console`) and HTTP start-on-got-IP callback.

### Why

- This layer is required to satisfy the request for both encoder navigation and web-based preset upload/editing.

### What worked

- End-to-end control path is now present in source: storage -> state engine -> UI/API actions.
- Commit remained focused on integration surfaces (UI, input, HTTP).

### What didn't work

- Build attempt failed due missing local ESP-IDF Python env:
  - Command: `cd 0071-cardputer-adv-photo-timer && ./build.sh build`
  - Error: `ERROR: ESP-IDF Python virtual environment "/home/manuel/.espressif/python_env/idf5.4_py3.13_env/bin/python" not found. Please run the install script to set it up before proceeding.`

### What I learned

- The current workspace shell has `ESP_IDF_VERSION` context but not the expected Python env path required by `build.sh`.

### What was tricky to build

- Coordinating config-revision-driven UI rebuilds with live timer updates required careful separation between state snapshots and list regeneration so focus/navigation remains stable.

### What warrants a second pair of eyes

- UI event binding lifecycle in `app_main.cpp` (`rebuild_preset_list` + callbacks) should be reviewed for any duplicate callback or stale-object edge cases.
- HTTP API error responses should be reviewed for consistency and client diagnostics quality.

### What should be done in the future

- Re-run `./build.sh build` in a machine/session with a valid ESP-IDF Python env and execute on-device smoke tests.

### Code review instructions

- Start with:
  - `0071-cardputer-adv-photo-timer/main/app_main.cpp`
  - `0071-cardputer-adv-photo-timer/main/http_server.cpp`
  - `0071-cardputer-adv-photo-timer/main/chain_encoder_uart.cpp`
- Then verify display/flush integration:
  - `0071-cardputer-adv-photo-timer/main/lvgl_port_m5gfx.cpp`

### Technical details

- Wi-Fi and HTTP start path:
  - `on_wifi_got_ip -> photo_http_server_start()`
- UI refresh path:
  - LVGL timer callback updates labels and rebuilds preset list on config revision changes.
