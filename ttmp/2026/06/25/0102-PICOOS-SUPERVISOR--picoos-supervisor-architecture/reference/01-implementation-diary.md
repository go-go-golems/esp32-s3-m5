---
Title: Implementation Diary
Ticket: 0102-PICOOS-SUPERVISOR
Status: active
Topics:
    - esp32-p4
    - picojs
    - picoos
    - quickjs
    - firmware
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp
      Note: |-
        Key source inspected while writing diary
        PicoOS initialization and console command integration (commit ac906dc)
    - Path: components/picojs_runtime/picojs_runtime.cpp
      Note: Runtime source inspected while writing diary
    - Path: components/picoos_core/include/picoos_core.h
      Note: Phase 1 supervisor public API (commit ac906dc)
    - Path: components/picoos_core/picoos_core.cpp
      Note: Phase 1 supervisor registry and status implementation (commit ac906dc)
    - Path: ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/design-doc/01-picoos-supervisor-design-and-implementation-guide.md
      Note: Primary design deliverable described by this diary
    - Path: ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/01-supervisor-phase1-probe.py
      Note: Passing hardware probe for status/apps
ExternalSources: []
Summary: Chronological diary for the PicoOS supervisor design ticket.
LastUpdated: 2026-06-26T00:00:00Z
WhatFor: Use this diary to understand how the supervisor design was produced, what evidence was inspected, and what remains for implementation.
WhenToUse: Read before resuming the supervisor implementation or reviewing the design document.
---



# Diary

## Goal

This diary captures the creation of the `0102-PICOOS-SUPERVISOR` ticket and its design/implementation guide. The guide explains how to turn the existing ESP32-P4 PicoCalc PicoJS firmware from a console-driven single-app runtime into a daily-use PicoOS supervisor with launcher, live app ticking, app switching, and REPL integration.

## Step 1: Create the supervisor architecture ticket and intern-facing design guide

This step paused feature implementation and created a planning artifact for the next phase. The design is intended for a new intern who has not seen the firmware before, so it explains the current system boundaries, the relevant files, the runtime threading model, the proposed `picoos_core` component, and a phased path from today's console-driven PicoJS runtime to an everyday PicoCalc OS.

The design is evidence-backed by current source files instead of being a greenfield proposal. It references the actual boot path, QuickJS service ownership rules, PicoJS runtime API, display backend, keyboard routing, built-in apps, and console commands.

### Prompt Context

**User prompt (verbatim):** "Ok, create a new docmgr ticket, and Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable."

**Assistant interpretation:** Create a new docmgr ticket for the PicoOS supervisor concept, write a comprehensive technical guide suitable for onboarding an intern, include architecture diagrams, API sketches, pseudocode, implementation phases, and concrete file references, then upload the result to reMarkable.

**Inferred user intent:** Establish a clear technical plan before implementing the daily-use PicoOS layer so future work proceeds from a shared architecture rather than ad hoc widget additions.

**Commit (code):** N/A — documentation-only step.

### What I did

- Created ticket `0102-PICOOS-SUPERVISOR` with title `PicoOS Supervisor Architecture`.
- Added design document `design-doc/01-picoos-supervisor-design-and-implementation-guide.md`.
- Added this implementation diary at `reference/01-implementation-diary.md`.
- Inspected the current firmware entrypoint, PicoJS runtime API and implementation, QuickJS service API/implementation, visual REPL display API, keyboard API, and LCD API.
- Wrote an intern-facing guide with:
  - executive summary;
  - current-state architecture;
  - line-anchored file evidence;
  - proposed `components/picoos_core` architecture;
  - native C API sketches;
  - JavaScript API sketches;
  - diagrams;
  - frame-pump pseudocode;
  - app lifecycle and input routing design;
  - console command design;
  - implementation phases;
  - validation strategy;
  - risks, mitigations, and open questions.

### Why

- The prior work proved that PicoJS can render apps on the PicoCalc, but Snake and other apps are still advanced by serial `picojs frame` / `picojs run` commands.
- The user wants an everyday OS experience: launcher, REPL escape, app switching, and potentially multiple running apps.
- Adding more widgets before defining the supervisor would make the codebase harder to evolve.

### What worked

- `docmgr ticket create-ticket` created the new ticket workspace.
- `docmgr doc add` created both the design document and diary.
- Source inspection produced clear boundary evidence:
  - `app_main.cpp` owns hardware init, current built-in apps, console commands, keyboard routing, and render glue.
  - `picojs_runtime` owns the DSL and current single-active-app rendering.
  - `qjs_service` owns QuickJS and executes jobs on its service task.
  - `visual_repl` provides the 40x20 display backend.
- The design now has concrete implementation guidance rather than only high-level architecture statements.

### What didn't work

- N/A for implementation. This step did not build or flash firmware.
- The only notable constraint is that the guide intentionally leaves some choices as open questions, especially whether to implement stateful multi-app runtime support immediately or after a smaller supervisor wrapper milestone.

### What I learned

- The current code already has most low-level pieces needed for a supervisor: `qjs_service_run()`, `picojs_runtime_frame_js()`, `picojs_runtime_key_js()`, `visual_repl_render_dump_frame()`, and keyboard token mapping.
- The largest missing conceptual boundary is not a widget API; it is ownership of lifecycle, frame cadence, input routing, and foreground/background state.
- The safest first implementation is a native supervisor component that preserves the low-level `picojs` console commands while adding `picoos` commands.

### What was tricky to build

- The design had to reconcile two valid goals: quick progress toward live Snake and a longer-term desire for multiple apps running at the same time. The guide resolves this by recommending a small `picoos_core` wrapper first, while also documenting the required `picojs_runtime` multi-app refactor for state-preserving switching.
- The concurrency model needed to be stated strongly. QuickJS must remain owned by `qjs_service`; any supervisor frame pump must submit jobs rather than calling QuickJS APIs from arbitrary FreeRTOS tasks.

### What warrants a second pair of eyes

- Review the proposed boundary between `picoos_core` and `picojs_runtime`. In particular, confirm whether the first PR should include runtime multi-app state or defer it until after live single-active-app scheduling works.
- Review the global key policy. The document proposes Home/Escape/REPL/switcher behaviors, but the physical PicoCalc keyboard mapping should be validated before hard-coding user-facing shortcuts.
- Review the recommendation to keep one QuickJS context initially. It is practical, but it provides weak isolation between apps.

### What should be done in the future

- Implement Phase 1 of the guide: `components/picoos_core` skeleton plus `picoos status` and `picoos apps`.
- Then add `picoos launch`, `picoos start`, and `picoos stop` so Snake becomes live without serial frame commands.
- Create a hardware probe script for the supervisor ticket after the first implementation slice exists.

### Code review instructions

- Start by reading `design-doc/01-picoos-supervisor-design-and-implementation-guide.md` from top to bottom.
- Check that file references and proposed APIs match the current firmware source.
- Pay special attention to the `qjs_service` ownership rule and the frame pump pseudocode.
- Validate doc hygiene with:
  - `docmgr doctor --ticket 0102-PICOOS-SUPERVISOR --stale-after 30`

### Technical details

Commands used to initialize the ticket:

```bash
docmgr status --summary-only
docmgr ticket create-ticket \
  --ticket 0102-PICOOS-SUPERVISOR \
  --title "PicoOS Supervisor Architecture" \
  --topics esp32-p4,picojs,picoos,quickjs,firmware
docmgr doc add \
  --ticket 0102-PICOOS-SUPERVISOR \
  --doc-type design-doc \
  --title "PicoOS Supervisor Design and Implementation Guide"
docmgr doc add \
  --ticket 0102-PICOOS-SUPERVISOR \
  --doc-type reference \
  --title "Implementation Diary"
```

Primary files inspected for evidence:

```text
0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp
components/picojs_runtime/include/picojs_runtime.h
components/picojs_runtime/picojs_runtime.cpp
components/qjs_service/include/qjs_service.h
components/qjs_service/qjs_service.cpp
components/visual_repl/include/visual_repl.h
components/picocalc_keyboard/include/picocalc_keyboard.h
components/picocalc_lcd/include/picocalc_lcd.h
```


## Step 2: Validate the ticket and upload the guide to reMarkable

This step completed the documentation delivery loop. After writing the guide and diary, I related the important source files, resolved vocabulary warnings, validated the ticket with `docmgr doctor`, and uploaded a bundled PDF to reMarkable.

The uploaded bundle contains both the primary supervisor design guide and this implementation diary, so the reMarkable copy includes both the technical plan and the investigation trail.

### Prompt Context

**User prompt (verbatim):** (same as Step 1)

**Assistant interpretation:** Complete the requested storage and reMarkable delivery for the new supervisor ticket.

**Inferred user intent:** Make the design available for offline review and future implementation planning.

**Commit (code):** N/A — documentation-only step.

### What I did

- Related the design guide to key firmware files with `docmgr doc relate`.
- Related this diary to the primary design guide and core source files.
- Updated the ticket changelog.
- Added missing vocabulary topics:
  - `picojs`
  - `picoos`
- Ran `docmgr doctor --ticket 0102-PICOOS-SUPERVISOR --stale-after 30`.
- Ran a dry-run reMarkable bundle upload.
- Uploaded the real bundle to `/ai/2026/06/25/0102-PICOOS-SUPERVISOR`.
- Verified the remote listing.

### Why

- The ticket needed valid metadata and file relations so future `docmgr` searches can connect the design to the implementation files.
- The user explicitly requested reMarkable upload.

### What worked

- `docmgr doctor` passed after adding the `picojs` and `picoos` vocabulary entries.
- `remarquee status` returned `remarquee: ok`.
- `remarquee cloud account --non-interactive` returned `user=wesen@ruinwesen.com sync_version=1.5`.
- Dry-run upload succeeded.
- Real upload succeeded:
  - `OK: uploaded 0102 PicoOS Supervisor Architecture.pdf -> /ai/2026/06/25/0102-PICOOS-SUPERVISOR`
- Remote listing verified:
  - `[f]	0102 PicoOS Supervisor Architecture`

### What didn't work

- Initial `docmgr doctor` reported unknown topics `picojs` and `picoos`. I fixed this with:
  - `docmgr vocab add --category topics --slug picojs --description "PicoJS JavaScript DSL/runtime for PicoCalc firmware apps"`
  - `docmgr vocab add --category topics --slug picoos --description "PicoOS supervisor and app operating environment for PicoCalc"`

### What I learned

- The repository vocabulary already had `quickjs`, `esp32-p4`, `picocalc`, `repl`, and `visual-repl`, but not the new project-specific `picojs` / `picoos` terms.
- Bundle upload is the best delivery format here because the guide and diary belong together.

### What was tricky to build

- The only tricky part was metadata hygiene: topic names that are semantically correct still need vocabulary entries before `doctor` passes.

### What warrants a second pair of eyes

- Review whether the new `picojs` and `picoos` vocabulary descriptions are broad enough for future tickets.
- Review whether the remote path date should remain `/ai/2026/06/25/...` for continuity with the rest of the 0102 work or move to a later date if this ticket is revisited.

### What should be done in the future

- Commit the ticket docs and vocabulary updates if this documentation should be preserved in git.
- Start implementation with Phase 1 from the design guide.

### Code review instructions

- Inspect the ticket directory:
  - `ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/`
- Confirm the design, diary, tasks, changelog, and vocabulary update are intentional.
- Re-run:
  - `docmgr doctor --ticket 0102-PICOOS-SUPERVISOR --stale-after 30`

### Technical details

Validation command:

```bash
docmgr doctor --ticket 0102-PICOOS-SUPERVISOR --stale-after 30
```

Upload commands:

```bash
remarquee upload bundle --dry-run \
  ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/design-doc/01-picoos-supervisor-design-and-implementation-guide.md \
  ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/reference/01-implementation-diary.md \
  --name "0102 PicoOS Supervisor Architecture" \
  --remote-dir "/ai/2026/06/25/0102-PICOOS-SUPERVISOR" \
  --toc-depth 2

remarquee upload bundle \
  ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/design-doc/01-picoos-supervisor-design-and-implementation-guide.md \
  ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/reference/01-implementation-diary.md \
  --name "0102 PicoOS Supervisor Architecture" \
  --remote-dir "/ai/2026/06/25/0102-PICOOS-SUPERVISOR" \
  --toc-depth 2

remarquee cloud ls /ai/2026/06/25/0102-PICOOS-SUPERVISOR --long --non-interactive
```


## Step 3: Implement Phase 1 PicoOS supervisor skeleton

This step implemented the first supervisor phase from the design guide. The firmware now has a dedicated `components/picoos_core` component, creates a native PicoOS supervisor at boot, registers built-in app descriptors, and exposes `picoos status` plus `picoos apps` console commands.

This is deliberately not the live scheduler yet. The goal was to establish the OS boundary and make app registry state visible over UART before moving launch, frame pump, and input routing behind the supervisor.

### Prompt Context

**User prompt (verbatim):** "go ahead. implement task by task, phase by phase, commit at appropriate intervals, keep a diary as you work. I assume you can test things in the host side emulator / harness ?"

**Assistant interpretation:** Begin implementing the supervisor plan in small phases, committing sensible units, maintaining the ticket diary, and using available validation harnesses where possible.

**Inferred user intent:** Move from architecture planning into working firmware while preserving reviewability and validation evidence.

**Commit (code):** ac906dcc58af130a6bcbdacd5f634fb8a3618903 — "0102: add PicoOS supervisor skeleton"

### What I did

- Added a new ESP-IDF component:
  - `components/picoos_core/CMakeLists.txt`
  - `components/picoos_core/include/picoos_core.h`
  - `components/picoos_core/picoos_core.cpp`
- Added supervisor data types for:
  - surfaces (`launcher`, `app`, `repl`, `switcher`, `crash`);
  - app states (`stopped`, `starting`, `running`, `foreground`, `background`, `paused`, `crashed`);
  - app descriptors;
  - app status rows;
  - supervisor status.
- Added supervisor APIs:
  - `picoos_supervisor_create()`
  - `picoos_supervisor_destroy()`
  - `picoos_register_app()`
  - `picoos_get_status()`
  - `picoos_list_apps()`
  - `picoos_surface_name()`
  - `picoos_app_state_name()`
- Integrated `picoos_core` into the 0102 project and main component CMake files.
- Added `g_picoos_os` in `app_main.cpp` and initialized it after QuickJS and PicoJS runtime startup.
- Registered built-in app descriptors for:
  - `home`
  - `repl`
  - `hello`
  - `dashboard`
  - `interactive`
  - `sysmon`
  - `snake`
- Added UART console commands:
  - `picoos status`
  - `picoos apps`
- Added hardware probe script:
  - `scripts/01-supervisor-phase1-probe.py`

### Why

- A daily-use PicoOS needs a native supervisor boundary before adding live scheduling or app switching.
- The first safe milestone is read-only: create the supervisor, register apps, and report state without changing existing `picojs` behavior.
- Keeping `picojs` commands untouched preserves all current validation paths while `picoos` commands come online.

### What worked

- ESP-IDF build passed with IDF 5.4.2:
  - `idf.py build`
- Flash passed on the stable ESP32-P4 by-id port:
  - `/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00`
- `picoos status` on hardware reported:
  - `initialized=1`
  - `running=0`
  - `surface=repl`
  - `apps=7`
- `picoos apps` listed all seven app descriptors with expected metadata.
- The phase-1 probe passed:
  - `PICOOS_PHASE1_PROBE PASS [True, True, True, True, True, True, True, True]`

### What didn't work

- First build failed because the new component was not listed in the project `EXTRA_COMPONENT_DIRS`:
  - `Failed to resolve component 'picoos_core' required by component 'main': unknown name.`
- I fixed this by adding:
  - `"${CMAKE_CURRENT_LIST_DIR}/../components/picoos_core"`
  to `0102-esp32-p4-visual-quickjs-repl/CMakeLists.txt`.
- The first manual console helper invocation accidentally sent `picoos`, `status`, `picoos`, and `apps` as separate commands. I reran it with quoted commands: `'picoos status' 'picoos apps'`.

### What I learned

- The project does not auto-discover all repository-level components. New components must be added to the project-level `EXTRA_COMPONENT_DIRS`.
- The current console probe helper accepts each shell argument as a separate command, so multi-word commands must be shell-quoted.
- A Phase 1 supervisor can be introduced without disrupting the existing PicoJS REPL/app test flow.

### What was tricky to build

- The supervisor needs to know about app source strings that currently live in `app_main.cpp`. For Phase 1, I kept those strings in place and registered descriptors from `app_main.cpp` rather than prematurely moving source ownership into `picoos_core`.
- The `repl` descriptor is a native/system surface rather than JavaScript source. I allowed `picoos_register_app()` to accept a `nullptr` source so native system surfaces can still appear in `picoos apps`.

### What warrants a second pair of eyes

- Review whether built-in app descriptors should remain in `app_main.cpp` for Phase 2 or move into a `picoos_builtin_apps` module.
- Review the fixed app cap `PICOOS_MAX_APPS=12`; it is enough for current built-ins but should eventually become storage-backed.
- Review whether `repl` should be called an app, a surface, or both in the public console output.

### What should be done in the future

- Phase 2: add `picoos launch <id>` so app loading moves behind the supervisor.
- Phase 3: add `picoos start [fps]` / `picoos stop` frame pump so Snake becomes live.
- Add host-side unit coverage only after the supervisor core is split enough to test registry/state without ESP-IDF hardware handles.

### Code review instructions

- Start with `components/picoos_core/include/picoos_core.h` to understand the new public API.
- Then inspect `components/picoos_core/picoos_core.cpp` for registry/status behavior.
- Review `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp` for `init_picoos_supervisor()`, `register_picoos_builtin_apps()`, and `cmd_picoos()`.
- Validate with:
  - `cd 0102-esp32-p4-visual-quickjs-repl && source ~/esp/esp-idf-5.4.2/export.sh && idf.py build`
  - `ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/01-supervisor-phase1-probe.py`

### Technical details

Successful probe output included:

```text
picoos: initialized=1 running=0 surface=repl active=- cols=40 rows=20 default_fps=4 apps=7 frames=0 errors=0
picoos apps: ESP_OK count=7
[0] id=home title=PicoOS Home state=stopped system=1 autostart=1 bg_ticks=0 fps=1 frames=0 errors=0
[1] id=repl title=QuickJS REPL state=stopped system=1 autostart=0 bg_ticks=0 fps=4 frames=0 errors=0
[6] id=snake title=Snake state=stopped system=0 autostart=0 bg_ticks=0 fps=4 frames=0 errors=0
PICOOS_PHASE1_PROBE PASS [True, True, True, True, True, True, True, True]
```

Host-side validation note: there is not yet a useful host-side harness for `picoos_core` because the component currently accepts ESP-IDF `qjs_service_t *` and `picojs_runtime_t *` handles and is built as an ESP-IDF component. The old host/native harness is still useful for portable PicoJS API shape, but supervisor validation currently relies on `idf.py build` and UART hardware probes. A future refactor can split a pure registry/state reducer from ESP-IDF handles to make host unit tests practical.
