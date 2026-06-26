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
        picoos launch/launcher/repl console commands (commit c687e03)
        picoos start/stop/frame console integration (commit b5378d1)
        Physical keyboard routing through PicoOS (commit e409fda)
        Visual REPL slash commands and Break-to-REPL key mapping
        Global Home/Shift+Tab keyboard shortcut implementation (commit fe454b6)
        PicoOS ps and visual /ps command implementation (commit 52c65ef)
    - Path: components/picojs_runtime/picojs_runtime.cpp
      Note: Runtime source inspected while writing diary
    - Path: components/picoos_core/include/picoos_core.h
      Note: |-
        Phase 1 supervisor public API (commit ac906dc)
        Launch/repl public APIs (commit c687e03)
        Frame pump public API (commit b5378d1)
        Key routing public API (commit e409fda)
    - Path: components/picoos_core/picoos_core.cpp
      Note: |-
        Phase 1 supervisor registry and status implementation (commit ac906dc)
        Supervisor launch and REPL surface implementation (commit c687e03)
        Live frame pump implementation (commit b5378d1)
        Supervisor key routing and global key behavior (commit e409fda)
    - Path: ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/design-doc/01-picoos-supervisor-design-and-implementation-guide.md
      Note: Primary design deliverable described by this diary
    - Path: ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/01-supervisor-phase1-probe.py
      Note: Passing hardware probe for status/apps
    - Path: ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/02-supervisor-launch-probe.py
      Note: Passing launch/repl hardware probe
    - Path: ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/03-supervisor-frame-pump-probe.py
      Note: Passing live frame pump hardware probe
    - Path: ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/04-supervisor-input-router-probe.py
      Note: Passing semantic input router probe
    - Path: ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/05-repl-slash-commands-probe.py
      Note: Passing slash-command hardware probe
    - Path: ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/06-repl-launch-crash-repro.py
      Note: Reusable slash-launch crash reproduction script
    - Path: ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/07-serial-crash-logger.py
      Note: Reusable serial crash logger
    - Path: ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/08-start-serial-crash-logger-tmux.sh
      Note: Tmux wrapper for serial crash logging
    - Path: ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/09-repl-home-key-probe.py
      Note: REPL-to-Home semantic key validation probe
    - Path: ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/10-picoos-ps-probe.py
      Note: PicoOS process view validation probe
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


## Step 4: Implement Phase 2 supervisor app launch and REPL surface commands

This step moved app launching behind the new PicoOS supervisor. The old `picojs load ...` path still exists for low-level debugging, but the new user-facing path is now `picoos launch <id>`, with convenience commands for `picoos launcher` and `picoos repl`.

This still does not make Snake live. It establishes the app registry as the source of launch metadata and records which surface/app the supervisor considers active.

### Prompt Context

**User prompt (verbatim):** (same as Step 3)

**Assistant interpretation:** Continue implementing the design phase by phase after the Phase 1 supervisor skeleton.

**Inferred user intent:** Keep making small, testable OS increments with commits and diary evidence.

**Commit (code):** c687e033819a08d519121e59d905f1aed5dd7792 — "0102: add PicoOS app launch commands"

### What I did

- Added public APIs:
  - `picoos_launch(picoos_supervisor_t *os, const char *app_id)`
  - `picoos_show_repl(picoos_supervisor_t *os)`
- Implemented launch lookup through the supervisor app registry.
- Added a small QuickJS job inside `picoos_core` to reinstall the PicoJS `OS` native object before evaluating a registered JavaScript app source.
- Updated supervisor state on successful app launch:
  - active app id;
  - active surface;
  - foreground/background app status rows.
- Added console commands:
  - `picoos launch <id>`
  - `picoos launcher`
  - `picoos repl`
- Kept `picojs load`, `picojs dump`, `picojs frame`, and `picojs key` available for debugging.
- Added hardware probe:
  - `scripts/02-supervisor-launch-probe.py`

### Why

- The supervisor must own app launching before it can own scheduling and app switching.
- Keeping app metadata in the registry avoids hard-coding launch behavior in `cmd_picojs()`.
- `picoos repl` is the first concrete step toward making the REPL a system surface rather than a separate mode flag.

### What worked

- ESP-IDF build passed with IDF 5.4.2.
- Flash passed on `/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00`.
- `picoos launcher` loaded/rendered the home launcher and `picojs dump` showed `picoOS` plus the launcher menu.
- `picoos launch snake` loaded/rendered Snake and `picoos status` reported:
  - `surface=app`
  - `active=snake`
- `picoos repl` switched the supervisor surface back to REPL and `picoos status` reported:
  - `surface=repl`
  - `active=repl`
- The launch probe passed:
  - `PICOOS_LAUNCH_PROBE PASS [True, True, True, True, True, True, True, True]`

### What didn't work

- The first version of `02-supervisor-launch-probe.py` looked for lowercase `picoos` in the home dump, but the actual top bar text is `picoOS`. The probe failed one check:
  - `PICOOS_LAUNCH_PROBE FAIL [True, True, False, True, True, True, True, True]`
- I fixed the assertion to match the real UI text:
  - `"picoOS" in home_dump`

### What I learned

- Launching through the supervisor can reuse the current JavaScript source strings and renderer without moving all source ownership at once.
- Reinstalling the PicoJS `OS` object before launch is a pragmatic safety measure because `js reset` can replace the QuickJS context/global object.
- The current launch path still replaces the active PicoJS app in practice because `picojs_runtime` is not multi-app yet. The supervisor status model is ready for foreground/background state, but the runtime still needs the later multi-app refactor.

### What was tricky to build

- Native `repl` does not have JavaScript source, so `picoos_launch()` must treat null-source descriptors as native/system surfaces rather than invalid apps.
- `picoos_core` can update supervisor state, but `app_main.cpp` still owns LCD rendering. For now, command handlers call `render_picojs_to_lcd()` after successful JS app launches and `visual_repl_render()` after `picoos repl`.

### What warrants a second pair of eyes

- Review whether `picoos_launch()` should call `qjs_service_eval()` directly or whether app evaluation should be a single explicit `qjs_service_run()` job that combines install/eval/state updates on the JS task.
- Review whether repeated `picojs_runtime_install()` before every launch has any unwanted side effects beyond replacing the global `OS` object.
- Review the current foreground/background app status semantics. They describe supervisor intent, not true preserved JS app execution yet.

### What should be done in the future

- Phase 3: add the live frame pump (`picoos start [fps]`, `picoos stop`, possibly `picoos frame [dt_ms]`).
- Make `picoos launcher` boot default after the frame pump and input router are stable.
- Refactor `picojs_runtime` to preserve multiple app states before claiming real stateful app switching.

### Code review instructions

- Start in `components/picoos_core/picoos_core.cpp` at `picoos_launch()` and `picoos_show_repl()`.
- Check `app_main.cpp` `cmd_picoos()` for console behavior and render calls.
- Validate with:
  - `idf.py build`
  - `scripts/02-supervisor-launch-probe.py`

### Technical details

Successful launch probe included:

```text
picoos launcher: ESP_OK
picoos render after launcher: ESP_OK
[00]   picoOS                          12:01
...
picoos launch snake: ESP_OK
picoos render after launch: ESP_OK
picoos: initialized=1 running=0 surface=app active=snake cols=40 rows=20 default_fps=4 apps=7 frames=0 errors=0
...
picoos repl: ESP_OK
picoos: initialized=1 running=0 surface=repl active=repl cols=40 rows=20 default_fps=4 apps=7 frames=0 errors=0
PICOOS_LAUNCH_PROBE PASS [True, True, True, True, True, True, True, True]
```


## Step 5: Implement Phase 3 live PicoOS frame pump

This step made the first app genuinely live under PicoOS control. The supervisor now has a FreeRTOS frame task, `picoos start [fps]`, `picoos stop`, and `picoos frame [dt_ms]`. When Snake is active, `picoos start 4` advances the JavaScript `game.loop(4, ...)` callback without serial `picojs frame` or `picojs run` commands.

This is the first visible shift from a console-driven runtime to an OS-like runtime. Serial commands still start/stop the pump, but once started the firmware clocks the app itself.

### Prompt Context

**User prompt (verbatim):** (same as Step 3)

**Assistant interpretation:** Continue through the design phases after app launching by implementing the live scheduler/frame pump.

**Inferred user intent:** Make apps like Snake behave like real foreground apps instead of needing manual frame advancement over UART.

**Commit (code):** b5378d1a0eb1d2757819b7854a3d4e345872bdd6 — "0102: add PicoOS live frame pump"

### What I did

- Extended `picoos_supervisor_config_t` with a render callback:
  - `esp_err_t (*render_active)(void *user)`
  - `void *render_user`
- Added supervisor APIs:
  - `picoos_start(picoos_supervisor_t *os, uint32_t fps)`
  - `picoos_stop(picoos_supervisor_t *os)`
  - `picoos_frame(picoos_supervisor_t *os, uint32_t dt_ms)`
- Added a FreeRTOS `picoos_frame` task inside `picoos_core`.
- The frame task computes elapsed `dt_ms`, submits JavaScript frame work through `qjs_service_run()`, and calls the render callback after successful frames.
- Wired `app_main.cpp` to pass `render_picojs_to_lcd()` as the supervisor render callback.
- Added console commands:
  - `picoos start [fps]`
  - `picoos stop`
  - `picoos frame [dt_ms]`
- Added hardware probe:
  - `scripts/03-supervisor-frame-pump-probe.py`

### Why

- Snake already had `game.loop(4, ...)`, but no firmware scheduler called frames automatically.
- The frame pump is the smallest change that makes the current app model feel like an actual OS foreground app.
- Putting the pump in `picoos_core` keeps scheduling out of `app_main.cpp` and preserves the QuickJS ownership rule.

### What worked

- ESP-IDF build passed with IDF 5.4.2.
- Flash passed on `/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00`.
- `picoos launch snake` loaded Snake.
- `picoos start 4` started the frame pump.
- After 1.5 seconds, `picoos status` reported:
  - `running=1`
  - `surface=app`
  - `active=snake`
  - `frames=5`
  - `errors=0`
- After `picoos stop`, `picojs dump` showed the Snake head had moved from the initial position without any `picojs frame` or `picojs run` command.
- The frame-pump probe passed:
  - `PICOOS_FRAME_PUMP_PROBE PASS [True, True, True, True, True, True, True, True]`

### What didn't work

- N/A for this phase. The first implementation built and validated on hardware.
- Known limitation: the frame task renders directly through the callback from the supervisor task. This is acceptable for the first slice but should be reviewed for LCD concurrency with console/manual render commands.

### What I learned

- The existing `picojs_runtime_frame_js()` callback path was already sufficient for live app behavior once a firmware task called it periodically.
- `qjs_service_run()` works well enough for the first frame pump because it serializes QuickJS access and gives a clear completion point before rendering.
- The 4 FPS Snake loop visibly advances with low overhead; the firmware binary remains at about 77% free in the 4 MB app partition.

### What was tricky to build

- The frame task cannot touch QuickJS directly. The implementation stores `pending_dt_ms` on the supervisor and submits a `qjs_job_t` whose callback calls `picojs_runtime_frame_js(ctx, runtime, pending_dt_ms)` on the QuickJS service task.
- Rendering after the frame happens outside the QuickJS job through the render callback. This keeps QuickJS ownership clean while still updating the LCD on each successful frame.
- The frame task remains alive after `picoos stop` and simply pauses when `running=false`. This avoids task creation/deletion churn while preserving a simple stop/start API.

### What warrants a second pair of eyes

- Review the unsynchronized supervisor fields accessed by console and frame tasks (`running`, `surface`, `active_app_id`, counters). The first implementation is simple, but a mutex or critical section may be warranted before more concurrent features.
- Review LCD render safety from the frame task. If future console commands also render frequently, add a display lock.
- Review whether `picoos_stop()` should delete the frame task or leave it paused. The current implementation leaves it paused.

### What should be done in the future

- Phase 4: route physical keyboard through `picoos_key()` / supervisor global input routing.
- Add `picoos start` behavior for boot-to-launcher/app mode once input routing is stable.
- Add frame skipping or async `qjs_service_post()` if high-FPS apps ever block the scheduler.

### Code review instructions

- Start in `components/picoos_core/picoos_core.cpp` at `picoos_frame_task()`, `picoos_frame()`, `picoos_start()`, and `picoos_stop()`.
- Check `app_main.cpp` for `render_picojs_to_lcd_callback()` and the new console commands.
- Validate with:
  - `idf.py build`
  - `scripts/03-supervisor-frame-pump-probe.py`

### Technical details

Successful probe output included:

```text
picoos launch snake: ESP_OK
picoos start: ESP_OK fps=4
picoos: initialized=1 running=1 surface=app active=snake cols=40 rows=20 default_fps=4 apps=7 frames=5 errors=0
picoos stop: ESP_OK
[05] | . . . . . . . . . O . . * . . . . . .
picoos: initialized=1 running=0 surface=app active=snake cols=40 rows=20 default_fps=4 apps=7 frames=5 errors=0
PICOOS_FRAME_PUMP_PROBE PASS [True, True, True, True, True, True, True, True]
```


## Step 6: Route semantic input through the PicoOS supervisor

This step moved key routing into the supervisor path. Console-injected semantic keys now go through `picoos key <token>`, and the physical keyboard task routes non-REPL surfaces through `picoos_key()` instead of directly calling the lower-level PicoJS key helper.

This gives PicoOS its first global key behavior: `home` returns to the launcher, `escape` returns to the REPL surface, and normal keys are dispatched to the active PicoJS app.

### Prompt Context

**User prompt (verbatim):** (same as Step 3)

**Assistant interpretation:** Continue after the live frame pump by implementing the global input routing phase.

**Inferred user intent:** Make physical and console input go through the OS layer so launcher/app/REPL behavior can become consistent.

**Commit (code):** e409fda176ef429c6341da394b6580dea9979793 — "0102: route PicoOS input through supervisor"

### What I did

- Added `picoos_key(picoos_supervisor_t *os, const char *token)`.
- Added a QuickJS key job inside `picoos_core` that calls `picojs_runtime_key_js()` on the QuickJS service task.
- Added global key handling:
  - `home` launches/renders the home launcher.
  - `escape` or `repl` switches/renders the REPL surface.
  - other tokens go to the active app when the surface is `app`.
- Extended supervisor config with a `render_repl` callback.
- Wired `app_main.cpp` to pass `visual_repl_render()` as the REPL render callback.
- Added `picoos key <token>` console command.
- Updated `key_to_picojs_token()` to map the physical Escape byte to the semantic `escape` token.
- Updated `keyboard_task()` so non-REPL PicoOS surfaces route physical keys through `picoos_key()`.
- Added hardware probe:
  - `scripts/04-supervisor-input-router-probe.py`

### Why

- The OS supervisor must see global keys before apps do.
- Returning to launcher/REPL should not depend on each JavaScript app registering its own key callbacks.
- Physical keyboard routing needs to follow the same semantic path as console-injected keys so UART probes remain useful.

### What worked

- ESP-IDF build passed with IDF 5.4.2.
- Flash passed on `/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00`.
- The input router probe passed:
  - `PICOOS_INPUT_ROUTER_PROBE PASS [True, True, True, True, True, True, True, True, True]`
- `picoos key left` moved the Snake head left.
- `picoos key home` switched to the home launcher and `picoos status` reported:
  - `surface=app`
  - `active=home`
- `picoos key escape` switched to REPL and `picoos status` reported:
  - `surface=repl`
  - `active=repl`

### What didn't work

- N/A for the console-injected semantic key path.
- Physical key behavior was compiled and routed through the supervisor, but I did not do a manual human keypress validation in this step. UART semantic key injection is the automated acceptance path.

### What I learned

- The supervisor can now own global behavior without removing the old `picojs mode app` compatibility path. The keyboard task checks PicoOS surface first and falls back to the older PicoJS app-mode branch only when PicoOS is in REPL or unavailable.
- Adding `render_repl` to the supervisor config makes `picoos_key("escape")` self-contained enough to update the display, not just internal state.

### What was tricky to build

- REPL rendering is still owned by `visual_repl`, while app rendering goes through the PicoJS dump-frame path. The supervisor now has two render callbacks so it can switch surfaces without depending on `app_main.cpp` command-specific rendering.
- `picoos_key()` must special-case global tokens before dispatching to app callbacks. If the order is reversed, an app could swallow `home` or `escape` and trap the user.

### What warrants a second pair of eyes

- Review the keyboard task's fallback order: PicoOS non-REPL surface first, old PicoJS app-mode second, editor third.
- Review whether `escape` should always mean REPL or should sometimes mean launcher/back. The current implementation chooses REPL as the safest escape hatch.
- Review the same unsynchronized supervisor-state caveat from the frame-pump phase; input and frame tasks can now both call into the supervisor.

### What should be done in the future

- Add a display/input lock or supervisor mutex before more concurrent display updates are added.
- Add a manual physical keyboard validation note once someone confirms Home/Escape/arrows on the actual PicoCalc keyboard.
- Implement state-preserving multi-app support in `picojs_runtime` next, or pause and refactor concurrency before doing so.

### Code review instructions

- Start in `components/picoos_core/picoos_core.cpp` at `picoos_key()`.
- Then inspect `app_main.cpp` around `keyboard_task()` and `key_to_picojs_token()`.
- Validate with:
  - `idf.py build`
  - `scripts/04-supervisor-input-router-probe.py`

### Technical details

Successful probe output included:

```text
picoos key: ESP_OK token=left
[05] | . . . O . . . . . . . . * . . . . . .
picoos key: ESP_OK token=home
picoos: initialized=1 running=0 surface=app active=home cols=40 rows=20 default_fps=4 apps=7 frames=0 errors=0
picoos key: ESP_OK token=escape
picoos: initialized=1 running=0 surface=repl active=repl cols=40 rows=20 default_fps=4 apps=7 frames=0 errors=0
PICOOS_INPUT_ROUTER_PROBE PASS [True, True, True, True, True, True, True, True, True]
```


## Step 7: Add visual REPL slash commands and Break-to-REPL mapping

This step made the on-device visual REPL more useful as a PicoOS control surface. The LCD REPL now accepts slash commands that mirror the most important `esp_console` PicoOS actions, so a user can launch apps, return home, start/stop the frame pump, recover the keyboard, and return to the REPL without needing the UART console.

It also maps the PicoCalc Break key (`Shift+Esc`, key code `0xd0`) to the same semantic `escape` token as Escape, so it can be used as a global return-to-REPL escape hatch from PicoOS app surfaces.

### Prompt Context

**User prompt (verbatim):** "can you add /commands to the REPL to launch the apps? maybe mirror the things on esp_console? 

And add shift + esc (brk) to go back to the REPL?"

**Assistant interpretation:** Add LCD visual REPL slash commands for PicoOS app control and map the PicoCalc Break/Shift+Esc key to the supervisor escape path.

**Inferred user intent:** Make the device usable without a serial console by letting the built-in REPL launch/control apps and by providing a reliable physical key chord to escape back to the REPL.

**Commit (code):** pending — implemented and hardware validated; commit follows this diary update.

### What I did

- Changed `evaluate_visual_input()` to return whether the visual REPL should render after evaluating a line. App-launching commands can now render the app display without being immediately overwritten by the REPL redraw.
- Added visual REPL slash commands:
  - `/help`
  - `/status`
  - `/apps`
  - `/reset`
  - `/launch <id>` and `/app <id>`
  - `/home` and `/launcher`
  - `/repl`
  - `/start [fps]`
  - `/stop`
  - `/frame [dt_ms]`
  - `/key <token>`
  - `/kbd` and `/kbd recover`
- Added `/picoos ...` aliasing so commands like `/picoos status` can mirror the UART console naming.
- Added `screen eval <source>` as a UART diagnostic path to exercise LCD REPL slash commands without physical typing.
- Mapped PicoCalc key code `0xd0` (`break`, i.e. Shift+Esc) to the semantic `escape` token in `key_to_picojs_token()`.
- Updated editor/app-mode handling so `0xd0` also behaves like Escape in legacy branches.
- Added hardware probe:
  - `scripts/05-repl-slash-commands-probe.py`

### Why

- A daily-use device should not require UART for routine app launching.
- The visual REPL is already a trusted system surface; slash commands make it a practical command shell.
- Break/Shift+Esc is a better explicit escape hatch than relying only on Escape behavior, especially if apps consume ordinary keys.

### What worked

- ESP-IDF build passed with IDF 5.4.2.
- Flash passed on `/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00`.
- `screen eval /help` rendered the slash-command help text on the visual REPL.
- `screen eval /apps` rendered the PicoOS app registry.
- `screen eval /launch snake` launched Snake and left the app display visible (`render_repl=0`).
- `screen eval /repl` returned the supervisor to `surface=repl active=repl`.
- `screen eval /kbd recover` ran keyboard recovery and rendered `KBD RECOVER: ESP_OK`.
- The slash-command probe passed:
  - `REPL_SLASH_COMMANDS_PROBE PASS [True, True, True, True, True, True, True, True, True, True]`

### What didn't work

- The first build failed on a `snprintf` truncation warning treated as an error for the visual `/status` line:
  - `error: '%d' directive output may be truncated ... [-Werror=format-truncation=]`
- I fixed this by shortening the visual status line to:
  - `OS <surface> APP=<id> R=<0|1>`
- I also had to add forward declarations for helpers that are defined later in `app_main.cpp` because the slash-command parser now calls them from `evaluate_visual_input()`.

### What I learned

- Visual REPL commands that switch to an app need a different render contract than ordinary JavaScript eval. A boolean `render_repl` return is enough for now.
- `screen eval` is a useful bridge for automated tests of physical-screen behavior because it exercises the same visual REPL command parser over UART.
- The keyboard driver already names `0xd0` as `break`, so mapping it to `escape` is straightforward.

### What was tricky to build

- `/launch snake` must render the app and then suppress the normal REPL render in `submit_input_line()`. Without that, the app would launch internally but the LCD would immediately redraw the REPL surface.
- The slash-command parser supports both short forms (`/launch snake`) and `/picoos ...` aliases by shifting the token array when the first token is `picoos`.
- The REPL command output must fit 40 columns. Long status strings can trigger compile-time truncation warnings and also look bad on the device.

### What warrants a second pair of eyes

- Review which slash commands should remain long-term. `/kbd recover` is pragmatic but exposes hardware recovery in the user shell.
- Review whether `/reset` should reset only QuickJS/PicoJS or the full PicoOS supervisor state. It currently resets QuickJS and reinstalls PicoJS.
- Review the Break/Shift+Esc physical behavior on hardware with a human keypress; automated validation confirms mapping/code path but not the physical chord timing.

### What should be done in the future

- Add a stronger keyboard reset/reinit command if `kbd recover` still does not avoid power cycling.
- Consider adding tab-completion/history for slash commands once storage/persistence exists.
- Consider adding `/ps` after real multi-app state exists.

### Code review instructions

- Start in `app_main.cpp` at `evaluate_visual_input()` for the slash-command parser.
- Then review `submit_input_line()` for the `render_repl` behavior.
- Review `key_to_picojs_token()` and `handle_editor_key()` for the `0xd0` Break mapping.
- Validate with:
  - `idf.py build`
  - `scripts/05-repl-slash-commands-probe.py`

### Technical details

Successful probe output included:

```text
screen eval /help
screen eval: ESP_OK render_repl=1

screen eval /launch snake
picoos_core: launched app id=snake elapsed=8ms
screen eval: ESP_OK render_repl=0

screen eval /kbd recover
KBD RECOVER: ESP_OK

REPL_SLASH_COMMANDS_PROBE PASS [True, True, True, True, True, True, True, True, True, True]
```


## Step 8: Preserve ad-hoc crash/repro scripts in the ticket scripts directory

This step cleaned up the investigation workflow after the `/launch` crash report. The temporary scripts that were created under `/tmp` for crash reproduction and serial logging have been promoted into the ticket's `scripts/` directory so future sessions can reuse them and so the investigation trail is visible in git.

The logging work also surfaced a tmux-specific environment issue: an older default tmux server did not see `/dev/serial/by-id`, while a fresh `tmux -L repro` server did. The reusable tmux launcher records that workaround.

### Prompt Context

**User prompt (verbatim):** "store all the scripts you write in the scripts/ folder of the ticket (retroactively too) so that you can reuse them when appropriate and so that we can trace all the work you did"

**Assistant interpretation:** Move the one-off `/tmp` scripts into the active docmgr ticket workspace and keep them executable/traceable.

**Inferred user intent:** Avoid losing debugging tools and make the implementation/debugging history auditable.

**Commit (code):** pending — documentation/script-only step.

### What I did

- Added `scripts/06-repl-launch-crash-repro.py` from the earlier `/tmp/picoos_repl_launch_repro.py` one-off repro script.
- Added `scripts/07-serial-crash-logger.py` from the earlier `/tmp/picoos_physical_crash_logger.py` one-off serial logger.
- Added `scripts/08-start-serial-crash-logger-tmux.sh` as a reusable tmux wrapper for the logger.
- Marked all three scripts executable.
- Started the tmux logger through the reusable wrapper.

### Why

- The ticket already contained formal probes, but the ad-hoc crash/debug scripts were still in `/tmp`.
- Reproduction tools are part of the work product for hardware debugging; they should be versioned next to probes and diary entries.

### What worked

- The tmux wrapper started a logger and wrote to:
  - `/tmp/picoos_physical_crash.log`
- The wrapper prints the attach command:
  - `tmux -L repro attach -t picocalc-0102-physlog`
- The logger saw the serial console prompt.

### What didn't work

- Starting the earlier repro script in the default tmux server failed with:
  - `FileNotFoundError: ... /dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00`
- A diagnostic `tmux new-session` under the default tmux server also reported:
  - `ls: cannot access '/dev/serial/by-id': No such file or directory`
- Starting a fresh tmux server with `tmux -L repro` did see `/dev/serial/by-id` correctly. The wrapper therefore uses `tmux -L repro` by default.

### What I learned

- For this environment, do not assume an old tmux server has the same `/dev` view as the current shell.
- Repro and logger scripts should live under the ticket immediately, even if they begin as quick one-off scripts.

### What was tricky to build

- The serial port is single-owner, so logger/repro scripts can conflict with each other. The tmux wrapper kills its named logger session before starting a new one, but it still prints `lsof` output first to show any existing holder.
- The crash report appears tied to the physical REPL/keyboard path rather than the UART `screen eval` path: the UART repro did not reproduce a panic. This made a persistent logger more useful than a one-shot command-only repro.

### What warrants a second pair of eyes

- Review whether the tmux `-L repro` workaround should become standard in all hardware logging scripts.
- Review whether the serial logger should also timestamp each received line; currently it preserves raw firmware output with only a session-start timestamp.

### What should be done in the future

- If the user can reproduce the physical `/launch` crash while `07-serial-crash-logger.py` is running, inspect `/tmp/picoos_physical_crash.log` and add the panic/backtrace to this diary.
- If the crash is gone after increasing the keyboard task stack, update this diary with that validation.

### Code review instructions

- Inspect the scripts under:
  - `ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/06-repl-launch-crash-repro.py`
  - `ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/07-serial-crash-logger.py`
  - `ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/08-start-serial-crash-logger-tmux.sh`
- To start logging:
  - `scripts/08-start-serial-crash-logger-tmux.sh`
- To attach:
  - `tmux -L repro attach -t picocalc-0102-physlog`

### Technical details

Reusable commands:

```bash
ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/06-repl-launch-crash-repro.py

ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/08-start-serial-crash-logger-tmux.sh

tmux -L repro attach -t picocalc-0102-physlog

tail -f /tmp/picoos_physical_crash.log
```


## Step 9: Increase keyboard task stack after physical `/launch` crash report

This step records the immediate mitigation for the user-reported physical `/launch` crash. The UART `screen eval /launch home|hello|snake` reproduction did not crash, which suggests the failure path may be specific to the physical keyboard task calling the now-larger visual REPL command parser on its own stack.

As a defensive fix, I increased the keyboard task stack from 4096 words to 12288 words and flashed the firmware. The change is committed, but it still needs physical validation from the device keyboard path while the serial crash logger is running.

### Prompt Context

**User prompt (verbatim):** "can you read the logs of why it crashed? that's why you should run in a tmux or so"

**Assistant interpretation:** Capture serial logs around the crash instead of guessing, and keep the logging process alive independently.

**Inferred user intent:** Diagnose the actual panic/backtrace for physical `/launch` crashes and avoid losing logs when the device resets.

**Commit (code):** e1c943ef539c394cef74af58b9a19f637665a6d4 — "0102: enlarge visual REPL keyboard task stack"

### What I did

- Tried to reproduce the crash over UART with `screen eval /launch home`, `screen eval /launch hello`, and `screen eval /launch snake`.
- The UART reproduction did not crash.
- Increased the physical keyboard task stack in `app_main.cpp`:
  - from `4096` words
  - to `12288` words
- Built and flashed the firmware.
- Started a persistent serial crash logger in tmux through the new script wrapper.

### Why

- The slash-command parser grew substantially and is called from the keyboard task when the user types into the LCD REPL.
- The UART `screen eval` path runs through the console task, not the physical keyboard task, so a crash only seen when typing physically is plausibly stack-related.

### What worked

- Build and flash passed after the stack change.
- UART launch repro did not crash.
- The persistent logger is running and can capture a future physical repro.

### What didn't work

- I did not capture the user's original panic/backtrace because logging was not running at the time of the physical crash.
- The default tmux server could not see `/dev/serial/by-id`; this was fixed by starting a fresh tmux server with `tmux -L repro`.

### What I learned

- There are two different paths into visual REPL commands:
  - UART `screen eval`, running on the console task.
  - Physical typing, running from the keyboard task.
- A repro that passes through UART does not fully validate keyboard-task stack safety.

### What was tricky to build

- The serial logger itself owns the UART port, so it can conflict with command probes. For physical reproduction, that is fine because the user interacts through the keyboard while the logger only reads serial output.

### What warrants a second pair of eyes

- Review whether `12288` words is the right stack size or whether the visual REPL command parser should avoid large stack allocations instead.
- Review the remaining local stack buffers in `evaluate_visual_input()`; future commands should keep buffers small or move larger work off the keyboard task.

### What should be done in the future

- Ask the user to retry physical `/launch home`, `/launch hello`, and `/launch snake` while the logger is active.
- If it still crashes, inspect `/tmp/picoos_physical_crash.log` and decode the backtrace.

### Code review instructions

- Inspect `app_main.cpp` around `xTaskCreate(keyboard_task, "kbd0102", 12288, ...)`.
- Validate physically by typing `/launch snake` into the LCD REPL while the logger is active.

### Technical details

Logger attach command:

```bash
tmux -L repro attach -t picocalc-0102-physlog
```

Log file:

```text
/tmp/picoos_physical_crash.log
```

## Step 10: Make the Home key open PicoOS Home from the REPL

This step closes a usability gap in the physical keyboard path. PicoCalc Shift+Tab emits the Home key code, and the user expects that key to open the PicoOS Home launcher even while the native visual REPL editor is active.

The fix makes Home a global keyboard shortcut before the keyboard task branches into PicoOS app mode, legacy PicoJS app mode, or REPL text editing. The old editor-only behavior of moving the cursor to column zero no longer wins over the system Home action when PicoOS is available.

### Prompt Context

**User prompt (verbatim):** "continue. also, even from the repl, shift+tab (== home key) should go to home."

**Assistant interpretation:** Continue the PicoOS/PicoJS integration work and specifically ensure that the physical Home key opens the PicoOS launcher from the REPL surface, not just from app surfaces.

**Inferred user intent:** Make the launcher feel like a real system shell with a reliable hardware Home shortcut from anywhere.

**Commit (code):** fe454b6827bddebcf2bccccb1e44ee67c186369e — "0102: make Home key open PicoOS launcher"

### What I did
- Updated `keyboard_task()` in `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp`.
- Added an early global handler for key code `0xd2` (`home`, Shift+Tab on PicoCalc).
- The handler calls `picoos_key(g_picoos_os, "home")`, which launches the `home` app and renders the PicoOS Home surface.
- Added `scripts/09-repl-home-key-probe.py` to validate the semantic Home path from the REPL surface.
- Built, flashed, and ran the new probe.

### Why
- Before this change, Home from the REPL editor path moved the input cursor to the beginning of the line.
- PicoOS already treated semantic `home` as a global launcher shortcut, but physical REPL input bypassed that path.
- A system Home key should not depend on whether the current surface is an app or the native REPL editor.

### What worked
- Build passed with binary size `0xe9780`, leaving 77% of the app partition free.
- Flash to `/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00` succeeded.
- `REPL_HOME_KEY_PROBE PASS` confirmed that semantic `picoos key home` transitions from `surface=repl` to `surface=app active=home` and renders the launcher.

### What didn't work
- I did not physically press Shift+Tab in this automated pass, because the independent probe can only exercise the same semantic `picoos_key("home")` target that the new keyboard branch calls.
- Full physical-key confirmation should still be done when convenient, especially given the existing keyboard reliability concerns.

### What I learned
- The PicoOS semantic key layer already had the correct Home behavior; the bug was that REPL edit mode handled physical Home locally before reaching PicoOS.
- Global system shortcuts should run before surface-specific editor behavior in the keyboard task.

### What was tricky to build
- The subtlety is ordering. App surfaces already used `key_to_picojs_token()` and `picoos_key()`, but the REPL branch called `handle_editor_key()` directly. Adding Home inside `handle_editor_key()` would have worked, but placing it earlier makes the shortcut explicitly global and prevents legacy PicoJS app mode from shadowing it.

### What warrants a second pair of eyes
- Review whether any other keys should be elevated to global shortcuts before mode dispatch.
- Review whether Home should be ignored on key repeat to avoid relaunching Home repeatedly if Shift+Tab is held.

### What should be done in the future
- Physically validate Shift+Tab/Home on the device keyboard path.
- Consider adding a general global-shortcut table for Home, Escape/Break, and future system keys.

### Code review instructions
- Start in `app_main.cpp` inside `keyboard_task()` around the early `ev.key == 0xd2` branch.
- Validate with `ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/09-repl-home-key-probe.py`.

### Technical details
- Build command: `source ~/esp/esp-idf-5.4.2/export.sh && idf.py build`.
- Flash command: `idf.py -p /dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00 flash`.
- Probe command: `ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/09-repl-home-key-probe.py`.

## Step 11: Add PicoOS process/status views

This step adds a lightweight process-list view as a stepping stone toward the multi-app-state work. The supervisor already tracks registered apps, foreground/background state, frame counts, and error counts; this makes that state visible through both UART and the visual REPL.

The new `picoos ps` and `/ps` commands are intentionally observational. They do not preserve separate QuickJS contexts yet, but they give us a stable operator-facing view for validating foreground/background transitions as the runtime model evolves.

### Prompt Context

**User prompt (verbatim):** (same as Step 10)

**Assistant interpretation:** Continue building the next PicoOS supervisor capabilities after fixing the Home key shortcut.

**Inferred user intent:** Keep advancing from launcher/app demos toward a real PicoOS operating surface with inspectable app state.

**Commit (code):** 52c65ef83ef58a2971344a171fe7abc19deac9ed — "0102: add PicoOS ps views"

### What I did
- Added `picoos ps` as a concise UART process/status listing.
- Added `/ps` in the visual REPL with 40-column rows showing app ID, state, frames, and errors.
- Updated `/help` and console help text to include `ps`.
- Added `scripts/10-picoos-ps-probe.py`.
- Built, flashed, and validated the process views on hardware.

### Why
- Multi-app state preservation needs better visibility into foreground/background/stopped/crashed state.
- `picoos apps` is a verbose registry dump; `picoos ps` is a compact runtime status view.
- The visual REPL needs the same process view so the LCD can be used without a host terminal.

### What worked
- `picoos ps` reports all 10 registered apps with state, frames, errors, and preferred FPS.
- `/ps` renders the same information compactly on the 40x20 display.
- `PICOOS_PS_PROBE PASS` confirmed both UART and visual REPL views.

### What didn't work
- This is not true state-preserving multi-app support yet. Apps still share the single current PicoJS runtime behavior and relaunched JS source is evaluated again.
- `/ps` can fill most of the visual history when all 10 apps are shown; future pagination may be useful as the app list grows.

### What I learned
- The existing `picoos_app_info_t` structure already contains enough information for a useful `ps` view.
- A separate `ps` command lets `/apps` remain a compact app-discovery list while `/ps` focuses on runtime state.

### What was tricky to build
- The LCD rows have to stay within 40 columns while still showing useful state. The implementation uses bounded app ID/state columns plus compact `fN eN` counters.
- The visual history can show previous `/ps` output above the new command because it is a scrolling REPL history, not a full-screen table surface.

### What warrants a second pair of eyes
- Review whether `ps` should sort foreground/background apps first or keep registry order.
- Review whether `/ps` should paginate once multi-app state adds more metadata.

### What should be done in the future
- Implement actual state-preserving multi-app runtime records.
- Add `picoos switch <id>` or a task-switcher surface once background app state is real.

### Code review instructions
- Start in `app_main.cpp` around `evaluate_visual_input()` handling for `apps`/`ps` and the `cmd_picoos()` `apps`/`ps` branch.
- Validate with `ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/10-picoos-ps-probe.py`.

### Technical details
- Build command: `source ~/esp/esp-idf-5.4.2/export.sh && idf.py build`.
- Flash command: `idf.py -p /dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00 flash`.
- Probe command: `ttmp/2026/06/25/0102-PICOOS-SUPERVISOR--picoos-supervisor-architecture/scripts/10-picoos-ps-probe.py`.
