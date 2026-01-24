---
Title: Diary
Ticket: G10-ANALYZE-DEMO-UI
Status: active
Topics:
    - cardputer
    - ui
    - display
    - animation
    - esp-idf
    - esp32s3
    - keyboard
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-24T13:41:42.129082115-05:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Keep a detailed, chronological diary of the work done to analyze how the UI in `M5Cardputer-UserDemo` and `M5Cardputer-UserDemo-ADV` is built (widgets, animations, callbacks, composition, scene lifecycle, and mapping to FreeRTOS primitives), culminating in a “textbook” design doc stored in this ticket.

## Step 1: Create ticket + set up docs

Started by setting up a dedicated docmgr ticket so the analysis can be incremental, searchable, and linked directly to the source files that drive conclusions. This also creates the scaffolding for a long-form design doc plus this diary so subsequent steps can be recorded as the investigation proceeds.

### Prompt Context

**User prompt (verbatim):** "Create a docmgr ticket G10-ANALYZE-DEMO-UI and analyze how the UI in M5Cardputer-UserDemo-ADV and M5Cardputer-UserDemo is built, what the widgets are, how animations are handled, how event callbacks are handled, how they are composed, how scenes are loaded and unloaded, how things map to freertos primitives, etc... 

Write a detailed textbook in a peter norvig style, with callouts for fundamentals, etc... Store in the ticket. 

Keep a detailed analysis diary as you go. Frequently update it as you analyze the situation."

**Assistant interpretation:** Create a new docmgr ticket, then do a code-level architecture analysis of both demos’ UI systems and write it up as a detailed “textbook”-style doc; keep an incremental diary as the analysis progresses.

**Inferred user intent:** Build durable understanding of the demo UI frameworks (and how they relate to ESP-IDF/FreeRTOS) so the patterns can be reused, extended, or ported with confidence.

**Commit (code):** N/A

### What I did
- Verified docmgr root/config and existing vocabulary topics.
- Created ticket `G10-ANALYZE-DEMO-UI` with topics: `cardputer, ui, display, animation, esp-idf, esp32s3, keyboard`.
- Added two initial documents:
  - `design-doc/01-textbook-how-the-cardputer-demo-ui-works.md` (the eventual Norvig-style textbook)
  - `reference/01-diary.md` (this diary)

### Why
- `docmgr` ticket workspaces keep long investigations organized and make it easy to relate conclusions back to exact source files.

### What worked
- `docmgr status --summary-only` confirmed an initialized docs root under `esp32-s3-m5/ttmp`.
- `docmgr ticket create-ticket` and `docmgr doc add` succeeded and created the ticket + docs.

### What didn't work
- N/A

### What I learned
- This repo’s docmgr root is `esp32-s3-m5/ttmp` (not the top-level repo directory), so all ticket paths resolve under that tree.

### What was tricky to build
- N/A (setup-only step)

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- Keep updating this diary at each investigation milestone with exact file references and key observations.

### Code review instructions
- Review ticket creation and doc scaffolding:
  - `esp32-s3-m5/ttmp/2026/01/24/G10-ANALYZE-DEMO-UI--analyze-m5cardputer-demo-ui-userdemo-adv/index.md`
  - `esp32-s3-m5/ttmp/2026/01/24/G10-ANALYZE-DEMO-UI--analyze-m5cardputer-demo-ui-userdemo-adv/reference/01-diary.md`
  - `esp32-s3-m5/ttmp/2026/01/24/G10-ANALYZE-DEMO-UI--analyze-m5cardputer-demo-ui-userdemo-adv/design-doc/01-textbook-how-the-cardputer-demo-ui-works.md`
- Validate docmgr state with: `docmgr doctor --ticket G10-ANALYZE-DEMO-UI --stale-after 30`

### Technical details
- Commands run:
  - `docmgr status --summary-only`
  - `docmgr ticket create-ticket --ticket G10-ANALYZE-DEMO-UI --title "Analyze M5Cardputer demo UI (UserDemo + ADV)" --topics cardputer,ui,display,animation,esp-idf,esp32s3,keyboard`
  - `docmgr doc add --ticket G10-ANALYZE-DEMO-UI --doc-type reference --title "Diary"`
  - `docmgr doc add --ticket G10-ANALYZE-DEMO-UI --doc-type design-doc --title "Textbook: How the Cardputer Demo UI Works"`

## Step 2: (In progress) Locate sources and build a file map

This step focused on finding the “center of gravity” files for both demos: entrypoints, UI frameworks, widget/animation toolkits, and one or two representative apps (the launcher) that demonstrate the composition patterns. The goal is to avoid getting lost in app-specific details until the common framework machinery is clear.

The important early discovery is that the two projects share conceptual roles (HAL + app lifecycle manager + a menu/selector widget + direct drawing via sprites), but they use different generations of the same ideas: the original `MOONCAKE::Mooncake` + `APP_Manager` vs. the newer `mooncake::Mooncake` + `AbilityManager`, and the older `SMOOTH_MENU` selector/menu vs. the newer `smooth_ui_toolkit::SmoothSelectorMenu` and animation primitives.

### Prompt Context

**User prompt (verbatim):** "Create a docmgr ticket G10-ANALYZE-DEMO-UI and analyze how the UI in M5Cardputer-UserDemo-ADV and M5Cardputer-UserDemo is built, what the widgets are, how animations are handled, how event callbacks are handled, how they are composed, how scenes are loaded and unloaded, how things map to freertos primitives, etc... 

Write a detailed textbook in a peter norvig style, with callouts for fundamentals, etc... Store in the ticket. 

Keep a detailed analysis diary as you go. Frequently update it as you analyze the situation."

**Assistant interpretation:** Build a source-level map of both demos’ UI systems so subsequent analysis can be organized around the correct “core” files and abstractions.

**Inferred user intent:** Get an accurate, durable mental model of the UI frameworks rather than a shallow, app-by-app tour.

**Commit (code):** N/A

### What I did
- Enumerated key directories for both demos (`main/`, `components/`, and `main/apps/...`).
- Confirmed the core frameworks and widget toolkits used:
  - `M5Cardputer-UserDemo`: `components/mooncake` + `main/apps/utils/smooth_menu`
  - `M5Cardputer-UserDemo-ADV`: `components/mooncake` (v2 abilities) + `components/smooth_ui_toolkit`
- Picked “representative” files to anchor the analysis and related them to the textbook doc via `docmgr doc relate`.

### Why
- Understanding UI architecture requires starting from lifecycle + scheduling + “what runs where”, then layering widgets/animations and finally app-specific composition on top.

### What worked
- Both demos keep their core patterns concentrated in a small set of files (entrypoint, lifecycle manager, menu widget, launcher wiring), making it feasible to write a coherent, generalized analysis.

### What didn't work
- N/A

### What I learned
- The ADV demo wires `smooth_ui_toolkit::ui_hal` tick/delay callbacks to the device HAL, so animation timing is decoupled from the platform and driven by a global “get_tick” function.
- The ADV “apps” are `AppAbility` instances stored in an `AbilityManager` and updated cooperatively from the main loop (no per-app task/thread by default).
- The original demo creates/destroys app instances dynamically through “packer” factories; the launcher itself backgrounds/foregrounds via Mooncake’s app FSM.

### What was tricky to build
- Picking a minimal set of “load-bearing” files to relate without exploding the ticket’s `RelatedFiles` list.

### What warrants a second pair of eyes
- N/A (no behavior change yet; analysis-only).

### What should be done in the future
- Continue analysis by reading the framework internals (old `APP_Manager` vs new `AbilityManager`, and old `SMOOTH_MENU` vs new `SmoothSelectorMenu` + `Animate`), then map those abstractions explicitly to FreeRTOS/ESP-IDF realities.

### Code review instructions
- Start from the related files in `esp32-s3-m5/ttmp/2026/01/24/G10-ANALYZE-DEMO-UI--analyze-m5cardputer-demo-ui-userdemo-adv/design-doc/01-textbook-how-the-cardputer-demo-ui-works.md`.

### Technical details
- Key anchoring files (also related via docmgr):
  - `M5Cardputer-UserDemo/main/cardputer.cpp`
  - `M5Cardputer-UserDemo/components/mooncake/src/app/app_manager.cpp`
  - `M5Cardputer-UserDemo/main/apps/launcher/views/menu/menu.cpp`
  - `M5Cardputer-UserDemo/main/apps/utils/smooth_menu/src/selector/selector.cpp`
  - `M5Cardputer-UserDemo-ADV/main/main.cpp`
  - `M5Cardputer-UserDemo-ADV/components/mooncake/src/ability_manager/ability_manager.cpp`
  - `M5Cardputer-UserDemo-ADV/components/smooth_ui_toolkit/src/widget/select_menu/smooth_selector.cpp`

## Step 3: Analyze `M5Cardputer-UserDemo` UI architecture

This step dug into the original demo’s UI framework: how the app lifecycle is modeled, how the launcher composes its “scene” out of sprites and a menu widget, and how “animations” and “callbacks” are implemented in practice. The key theme is cooperative scheduling: everything meaningful happens because `app_main()` repeatedly calls `mooncake.update()` in a tight loop.

The most useful mental model is that `MOONCAKE` is a tiny “activity manager” and the UI is immediate-mode drawing into offscreen sprites. Widgets are not a retained object tree; they are small state machines that compute geometry and then invoke render callbacks that draw the current frame.

### Prompt Context

**User prompt (verbatim):** "Create a docmgr ticket G10-ANALYZE-DEMO-UI and analyze how the UI in M5Cardputer-UserDemo-ADV and M5Cardputer-UserDemo is built, what the widgets are, how animations are handled, how event callbacks are handled, how they are composed, how scenes are loaded and unloaded, how things map to freertos primitives, etc... 

Write a detailed textbook in a peter norvig style, with callouts for fundamentals, etc... Store in the ticket. 

Keep a detailed analysis diary as you go. Frequently update it as you analyze the situation."

**Assistant interpretation:** Explain the original demo’s UI system end-to-end: lifecycle, widgets, animation timing, and scene transitions, at the level of code paths and state machines.

**Inferred user intent:** Understand the “why” and “how” of the demo UI so it can be modified confidently (e.g., new scenes, new widgets, different animation patterns) without accidental regressions.

**Commit (code):** N/A

### What I did
- Read the entrypoint and wiring:
  - `M5Cardputer-UserDemo/main/cardputer.cpp` (HAL init → Mooncake init → install apps → create launcher → `while(1) mooncake.update()`).
- Read the Mooncake v0.x framework core:
  - `components/mooncake/src/mooncake.{h,cpp}` (database setup + boot animation app + update loop)
  - `components/mooncake/src/app/app.{h}` (packer vs app instance + lifecycle callbacks)
  - `components/mooncake/src/app/app_manager.{h,cpp}` (lifecycle FSM + destroy semantics)
  - `components/mooncake/src/app/app_register.{h,cpp}` (installed packers list; userData injection)
- Read the launcher’s implementation and its “scene” composition:
  - `main/apps/launcher/launcher.{h,cpp}` (foreground/background behavior)
  - `main/apps/launcher/views/menu/menu.cpp` + `.../menu_render_callback.hpp` (Smooth_Menu usage + rendering)
  - `main/apps/launcher/port/port.cpp` (input polling + “hold until release” loops)
- Read the widget/animation substrate used by the launcher:
  - `main/apps/utils/smooth_menu/src/...` (menu + selector + camera + LVGL path interpolation)
  - `main/apps/utils/anim/anim_define.h` (app open/close circle wipe)

### Why
- The launcher exercises almost every UI “plumbing” path (input → widget update → rendering → app scene switch), making it an ideal specimen for understanding the framework.

### What worked
- The original demo has a clear separation of concerns:
  - `HAL` owns device IO and sprites.
  - `Mooncake` owns app lifetime transitions.
  - The launcher owns composition (menu + system bar + keyboard bar) and the policy of “when to open/close apps”.
  - `smooth_menu` owns selector/menu interpolation and exposes pure geometry to a render callback.

### What didn't work
- N/A (analysis-only)

### What I learned
- **Widget model (old):** `SMOOTH_MENU::Simple_Menu` is a coordinator: it holds a `Menu_t` (items), a `Selector_t` (animated rectangle that targets an item), and a `Camrea_t` (animated viewport offset). It does not draw itself; it calls a `SimpleMenuCallback_t::renderCallback(...)` with three snapshots (menu items + selector rect + camera rect). Rendering is entirely application-defined.
- **Animation model (old):** `LVGL::Anim_Path` is an integer interpolation function adapted from LVGL’s “anim path” code. Each animated property is “pure”: given `(start, end, time, act_time)` it returns the interpolated value. `Selector_t` updates its four animators (x/y/width/height) on target change and then renders by sampling those animators at `currentTime`.
- **Event handling (old):** there’s no centralized event bus for UI. Input is polled from the keyboard matrix (`updateKeyList()`), then launcher-specific “port” helpers convert key states into navigation events. Those helpers deliberately block (busy-wait) until release, while continuing to redraw the menu so animations don’t freeze.
- **Scene lifecycle (old):** apps are created/destroyed dynamically via `APP_PACKER_BASE::newApp/deleteApp`. The launcher backgrounds itself (`closeApp()`) after starting an app; later, while running in background (`onRunningBG()`), it detects “only the launcher remains” and brings itself back to foreground with a close animation.
- **FreeRTOS mapping (old):** the UI scheduler is effectively the `app_main` task + “polling loops + time checks”. There’s no dedicated UI task created by the framework. When delays occur, they typically go through `delay(...)` which ultimately yields to FreeRTOS.

### What was tricky to build
- Separating “framework responsibility” from “launcher policy”: some behaviors (e.g. “go back to launcher when others exit”) are not generic Mooncake features; they are implemented by the launcher app itself by inspecting `getCreatedAppNum()`.

### What warrants a second pair of eyes
- The launcher’s `selected_item++` in `views/menu/menu.cpp` is a subtle index convention (“skip launcher” vs “selector index”), and it’s easy to get wrong when adding/removing apps.

### What should be done in the future
- When writing the textbook, include explicit state-machine diagrams for:
  - `APP_Manager` lifecycle states and transitions (foreground/background/destroy).
  - `Selector_t` and `LVGL::Anim_Path` timing (“retargeting” behavior on selection changes).

### Code review instructions
- Start from `M5Cardputer-UserDemo/main/cardputer.cpp` and follow calls into Mooncake + launcher + Smooth_Menu.

### Technical details
- Key files for the old demo analysis:
  - `M5Cardputer-UserDemo/components/mooncake/src/app/app_manager.cpp`
  - `M5Cardputer-UserDemo/main/apps/launcher/views/menu/menu.cpp`
  - `M5Cardputer-UserDemo/main/apps/utils/smooth_menu/src/selector/selector.cpp`
  - `M5Cardputer-UserDemo/main/apps/launcher/port/port.cpp`

## Step 4: Analyze `M5Cardputer-UserDemo-ADV` UI architecture

This step analyzed the ADV demo’s “modernized” UI stack. The key change relative to the original demo is not the drawing primitives (it’s still sprite-based immediate drawing), but the architecture around them: Mooncake moves from “factory + per-open allocation” to “ability objects updated every frame”, and the UI widget layer uses `smooth_ui_toolkit` to centralize animation timing, easing/spring behavior, and callback cadence (read input/render/click).

The resulting system is closer to a small, deterministic game loop: one thread of control (`app_main`’s task) repeatedly updates HAL → processes queued hardware events (keyboard ISR) → updates apps/abilities → draws to sprites → pushes sprites to the display. Concurrency exists, but mostly underneath (drivers/ISRs) rather than at the UI composition level.

### Prompt Context

**User prompt (verbatim):** "Create a docmgr ticket G10-ANALYZE-DEMO-UI and analyze how the UI in M5Cardputer-UserDemo-ADV and M5Cardputer-UserDemo is built, what the widgets are, how animations are handled, how event callbacks are handled, how they are composed, how scenes are loaded and unloaded, how things map to freertos primitives, etc... 

Write a detailed textbook in a peter norvig style, with callouts for fundamentals, etc... Store in the ticket. 

Keep a detailed analysis diary as you go. Frequently update it as you analyze the situation."

**Assistant interpretation:** Explain ADV’s UI architecture in terms of its “ability” lifecycle, widget primitives, animation engine, and the event/callback mechanisms that wire input to UI transitions.

**Inferred user intent:** Understand the newer stack well enough to extend it (new apps, new widgets, richer animations) while staying aligned with ESP-IDF/FreeRTOS realities.

**Commit (code):** N/A

### What I did
- Read the entrypoint + main loop:
  - `M5Cardputer-UserDemo-ADV/main/main.cpp` (HAL init; `ui_hal::on_delay/on_get_tick`; install apps; main loop calls `GetHAL().feedTheDog()`, `GetHAL().update()`, `GetMooncake().update()`).
- Read Mooncake v2 internals:
  - `components/mooncake/src/mooncake.{h,cpp}` (singleton + app/extension managers)
  - `components/mooncake/src/ability_manager/ability_manager.{h,cpp}` (ability list + create/destroy scheduling)
  - `components/mooncake/src/ability/ability.h` + `ui_ability.cpp` + `worker_ability.cpp` + `app_ability.cpp` (type-specific state machines)
  - `components/mooncake/src/ability_manager/ability_api_wrapping.cpp` (type-checked API wrappers like `openAppAbility`)
- Read the widget/animation layer used by launcher and apps:
  - `components/smooth_ui_toolkit/src/widget/select_menu/smooth_selector.{h,cpp}` (SmoothSelectorMenu)
  - `components/smooth_ui_toolkit/src/animation/animate/animate.{h,cpp}` + generators (spring/easing) + `animate_value.{h,cpp}`
  - `components/smooth_ui_toolkit/src/utils/hal/hal.{h,cpp}` (pluggable time + delay)
- Read how the launcher composes its UI and wires events:
  - `main/apps/app_launcher/app_launcher.{h,cpp}` (menu open/close policy; “running app id” bookkeeping)
  - `main/apps/app_launcher/view/menu/menu.{h,cpp}` (LauncherMenu as SmoothSelectorMenu subclass)
  - `main/apps/app_launcher/view/system_bar/system_bar.cpp` (periodic render to system bar sprite)
  - `main/apps/app_launcher/view/keyboard_bar/keyboard_bar.cpp` (signal-driven updates based on key events)
- Read the keyboard HAL path to see where events originate:
  - `main/hal/keyboard/keyboard.h` + `main/hal/keyboard/keybaord.cpp` (TCA8418 IRQ → ISR flag → update reads event → emits signals)

### Why
- ADV’s “UI behavior” is split across three layers (HAL events, widget update cadence, and app ability state machines); understanding it requires reading across boundaries.

### What worked
- The abstraction boundaries are explicit:
  - `smooth_ui_toolkit::ui_hal` isolates timing sources and delays from the platform.
  - `AbilityManager` isolates “when callbacks run” from app code.
  - Widgets like `SmoothSelectorMenu` isolate “how to animate selection and detect click” from the rendering details.

### What didn't work
- N/A (analysis-only)

### What I learned
- **Lifecycle model (ADV):** apps are `AppAbility` objects. They are created once (installed into `AbilityManager` as `unique_ptr`) and then updated every frame; “open/close” is an internal state machine (`StateGoOpen → StateRunning → StateGoClose → StateSleeping`). Closing does *not* free memory unless the ability is destroyed/uninstalled.
- **Widget model (ADV):** `SmoothSelectorMenu` owns animated state (`AnimateVector2` selector position/shape, camera offset) and provides virtual callbacks (`onReadInput`, `onRender`, `onClick`, etc.). It schedules those callbacks with millisecond intervals and triggers `onClick` only after “release” and after transitions are complete.
- **Animation model (ADV):** the core `Animate` engine is a small state machine driven by `ui_hal::get_tick_s()` and a `KeyFrameGenerator` (spring or easing). “Retargeting” is a first-class operation: springs update their velocity and re-init so mid-flight changes remain smooth.
- **Event callbacks (ADV):**
  - “UI framework callbacks”: virtual methods invoked by `AbilityManager` (`onCreate`, `onRunning`, etc.) and by widgets (`onReadInput`, `onRender`, `onClick`).
  - “Signal callbacks”: `Keyboard::onKeyEvent` uses a mutex-protected observer list; the launcher’s keyboard bar connects a lambda to update UI state immediately on modifier changes.
  - “Ad-hoc callbacks”: `LauncherMenu` exposes `std::function<void(int,int)> onAppOpen` to hand selection events back to `Launcher`.
- **Scene switching (ADV):** the launcher does not background itself; it keeps running and uses a simple policy:
  - if an app is open, it watches that app’s state; when it returns to sleeping, it plays a close animation and resumes menu updates.
  - opening an app is `GetMooncake().openApp(appId)` plus a UI animation (`ANIM_APP_OPEN()`).
- **FreeRTOS mapping (ADV):**
  - The “UI thread” is still the `app_main` task, and explicit yielding is done via `GetHAL().feedTheDog()` → `vTaskDelay(1)`.
  - Hardware input uses an ISR (`gpio_isr_handler`) to set a volatile flag; events are then drained in the main loop (`Keyboard::update()`).
  - `std::mutex` used by `Signal` maps to the ESP-IDF pthread layer and ultimately FreeRTOS synchronization primitives.

### What was tricky to build
- There are *two* distinct kinds of “event-ness”:
  - edge-triggered hardware events (keyboard IRQ),
  - frame-based UI updates (SmoothSelectorMenu polling `getLatestKeyEventRaw()`), which can miss events if you don’t call `GetHAL().update()` frequently enough.

### What warrants a second pair of eyes
- `Keyboard::update()` clears the “latest key event” buffers at the start of every call, so consumers that poll `getLatestKeyEventRaw()` are implicitly depending on being called in the same frame as `Keyboard::update()` captured an IRQ event.

### What should be done in the future
- In the textbook, include a “dataflow diagram” for input:
  - IRQ edge → ISR flag → `Keyboard::update` reads FIFO event → `onKeyEvent.emit` (signal) + “latest event buffer” update → widget/app consumers.

### Code review instructions
- Read `M5Cardputer-UserDemo-ADV/main/main.cpp` and follow into `GetHAL().update()` and `GetMooncake().update()`.
- For UI behavior, start at `main/apps/app_launcher/app_launcher.cpp` and then inspect `view/menu/menu.cpp` and `view/keyboard_bar/keyboard_bar.cpp`.

### Technical details
- Key files for the ADV demo analysis:
  - `M5Cardputer-UserDemo-ADV/components/mooncake/src/ability_manager/ability_manager.cpp`
  - `M5Cardputer-UserDemo-ADV/components/mooncake/src/ability/ability.h`
  - `M5Cardputer-UserDemo-ADV/components/smooth_ui_toolkit/src/widget/select_menu/smooth_selector.cpp`
  - `M5Cardputer-UserDemo-ADV/components/smooth_ui_toolkit/src/animation/animate/animate.cpp`
  - `M5Cardputer-UserDemo-ADV/main/hal/keyboard/keybaord.cpp`

## Step 5: Write the Norvig-style “textbook” design doc

With the source map and both architectural analyses complete, this step translated the findings into a single long-form, reusable document. The emphasis is on fundamentals first (time/tasks/pixels), then layering the two generations (UserDemo vs ADV) as two different instantiations of the same underlying UI ideas.

The resulting doc is structured to be readable as a “textbook chapter sequence” but still fits docmgr’s design-doc conventions (executive summary, decisions, alternatives, implementation guidance, open questions, and references).

### Prompt Context

**User prompt (verbatim):** "Create a docmgr ticket G10-ANALYZE-DEMO-UI and analyze how the UI in M5Cardputer-UserDemo-ADV and M5Cardputer-UserDemo is built, what the widgets are, how animations are handled, how event callbacks are handled, how they are composed, how scenes are loaded and unloaded, how things map to freertos primitives, etc... 

Write a detailed textbook in a peter norvig style, with callouts for fundamentals, etc... Store in the ticket. 

Keep a detailed analysis diary as you go. Frequently update it as you analyze the situation."

**Assistant interpretation:** Produce a detailed, durable writeup that explains the two demo UI stacks from first principles and ties each claim back to code.

**Inferred user intent:** Have a high-quality reference that can be used to onboard new contributors and guide future UI work.

**Commit (code):** N/A

### What I did
- Wrote a complete multi-part analysis in:
  - `esp32-s3-m5/ttmp/2026/01/24/G10-ANALYZE-DEMO-UI--analyze-m5cardputer-demo-ui-userdemo-adv/design-doc/01-textbook-how-the-cardputer-demo-ui-works.md`
- Included explicit callouts for fundamentals (single-threaded UI, sprites as buffering, animation = interpolation+sampling, ISR-to-task handoff, event contract).
- Added a concrete UI ↔ FreeRTOS mapping table and a practical “how to add a new app” cookbook for both generations.

### Why
- A long-form narrative is easier to retain than a pile of disconnected notes, especially when the code uses multiple layers (HAL/framework/widgets/apps) that cross-cut.

### What worked
- The demos’ internal state machines map cleanly to textbook-style explanations (both are explicit FSMs).

### What didn't work
- N/A

### What I learned
- Writing the analysis as “two generations of the same architecture” avoids duplicating explanations and makes differences (dynamic app instances vs persistent abilities; LVGL-path interpolation vs spring retargeting) much clearer.

### What was tricky to build
- Balancing “design-doc required structure” with a “textbook” narrative without turning either into a shallow checklist.

### What warrants a second pair of eyes
- The ADV keyboard event buffering semantics (clear-on-update + polling consumers) is a subtle contract; it’s a good place for a reviewer to sanity-check the analysis and/or propose a more robust pattern.

### What should be done in the future
- If this analysis is used to drive changes, spin a follow-up ticket focused on one concrete improvement (e.g., introduce a ring-buffered key event queue for widgets).

### Code review instructions
- Read the textbook doc first, then spot-check key claims by opening the linked “References” file list at the end of the doc.

### Technical details
- The textbook doc is the primary deliverable:
  - `esp32-s3-m5/ttmp/2026/01/24/G10-ANALYZE-DEMO-UI--analyze-m5cardputer-demo-ui-userdemo-adv/design-doc/01-textbook-how-the-cardputer-demo-ui-works.md`

## Step 6: Validate ticket hygiene

Finished by running docmgr hygiene checks to ensure the ticket is well-formed (frontmatter valid, no docmgr doctor warnings) and by updating the ticket index/tasks/changelog so reviewers can navigate the work quickly.

### Prompt Context

**User prompt (verbatim):** "Create a docmgr ticket G10-ANALYZE-DEMO-UI and analyze how the UI in M5Cardputer-UserDemo-ADV and M5Cardputer-UserDemo is built, what the widgets are, how animations are handled, how event callbacks are handled, how they are composed, how scenes are loaded and unloaded, how things map to freertos primitives, etc... 

Write a detailed textbook in a peter norvig style, with callouts for fundamentals, etc... Store in the ticket. 

Keep a detailed analysis diary as you go. Frequently update it as you analyze the situation."

**Assistant interpretation:** Ensure the final artifacts are discoverable and validate cleanly under docmgr.

**Inferred user intent:** Make the documentation durable and easy to reuse, not a one-off dump of notes.

**Commit (code):** N/A

### What I did
- Ran `docmgr doctor --ticket G10-ANALYZE-DEMO-UI --stale-after 30`.
- Validated frontmatter for:
  - `design-doc/01-textbook-how-the-cardputer-demo-ui-works.md`
  - `reference/01-diary.md`
- Updated ticket navigation files:
  - `index.md` (overview + links)
  - `tasks.md` (checked off work)
  - `changelog.md` (recorded deliverables)

### Why
- Docmgr hygiene checks prevent subtle metadata drift that makes tickets hard to search and maintain over time.

### What worked
- `docmgr doctor` reported all checks passed and frontmatter validated successfully.

### What didn't work
- Initial `docmgr validate frontmatter` attempts used doc paths prefixed with the docs root, causing a duplicated path; rerunning with paths relative to docmgr root fixed it.

### What I learned
- For `docmgr validate frontmatter`, pass paths relative to docmgr’s configured docs root (e.g. `2026/.../doc.md`), not workspace-relative paths that already include `esp32-s3-m5/ttmp`.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- N/A

### Code review instructions
- Open `esp32-s3-m5/ttmp/2026/01/24/G10-ANALYZE-DEMO-UI--analyze-m5cardputer-demo-ui-userdemo-adv/index.md` and follow links to the textbook + diary.

## Step 7: Unify the keyboard design + re-upload

This step updated the design doc to match a new constraint: we want **one consistent keyboard component** rather than a set of partially overlapping mechanisms (signals vs polling, latest-event buffers, per-project scanning logic, and separate remapping/modifier tracking). The doc now proposes a single “keyboard component” with a unified event contract and two internal backends.

It also answers the question “can we tell ADV vs non-ADV programmatically?” with a practical runtime detection strategy: probe the TCA8418 keypad controller on I2C (`0x34`) and fall back to GPIO matrix scanning if the probe fails.

### Prompt Context

**User prompt (verbatim):** "we want one consistent component, not split up in 5 smaller ones.
  - can we determine which keyboard is what programmatically? to differentiate between ADV and non-ADV?
 
  Update the design, overwrite the pdf on remarkable."

**Assistant interpretation:** Update the documentation to propose a unified keyboard component and a runtime keyboard-hardware detection approach, then regenerate and overwrite the uploaded reMarkable PDF.

**Inferred user intent:** Standardize keyboard handling across codebases and make the doc reflect the intended “single component” direction, with a clean reMarkable artifact.

**Commit (code):** N/A

### What I did
- Updated the textbook doc with a new section proposing:
  - one keyboard component,
  - a single event-stream contract (buffered queue/ring buffer as source of truth),
  - runtime probe-based backend selection (TCA8418 → matrix scan fallback).

### Why
- The existing “split” approach works in each demo but doesn’t compose well if you want one shared component: polling semantics and buffer-clearing rules become subtle and easy to misuse.

### What worked
- The codebase already contains enough evidence to justify a robust runtime probe (TCA8418 default address and `begin()` contract exist in the Adafruit driver used by ADV).

### What didn't work
- N/A

### What I learned
- The cleanest way to unify the behavior is to treat signals and “latest event” as *views* on top of a buffered event stream, not as the primary delivery mechanism.

### What was tricky to build
- Updating the doc without implying we had already implemented the unified component in firmware (this ticket remains analysis/design).

### What warrants a second pair of eyes
- The runtime probe strategy is straightforward, but reviewers should consider whether any supported hardware variants could cause a false-positive probe or require a configurable probe order.

### What should be done in the future
- If the unified component becomes a real implementation task, create a follow-up ticket focused on:
  - event buffering semantics,
  - ISR/task handoff strategy,
  - and backward-compatible row/col remapping.

### Code review instructions
- Review the new keyboard unification section in:
  - `esp32-s3-m5/ttmp/2026/01/24/G10-ANALYZE-DEMO-UI--analyze-m5cardputer-demo-ui-userdemo-adv/design-doc/01-textbook-how-the-cardputer-demo-ui-works.md`

## Step 8: Create implementation tasks and start iterative delivery

This step transitions the ticket from “analysis only” into “design + implementation”. The user asked to create explicit tasks inside the ticket, then implement them one-by-one with checkoffs, git commits, and diary updates. I kept the tasks scoped to the concrete ask: unify keyboard handling into one consistent component and make the firmware consumers converge on it.

Key scope decision: implement the unification in `esp32-s3-m5/components/cardputer_kb`, since this repo already contains a reusable GPIO matrix scanner (Cardputer) and multiple ad-hoc TCA8418 handlers (Cardputer-ADV). That’s the natural “one component” home.

### Prompt Context

**User prompt (verbatim):** "Ok, create tasks in the ticket, the implement task by task, check off tasks, commit to git, and keep a detailed diary."

**Assistant interpretation:** Add a concrete implementation task list to this ticket and begin executing it with incremental commits and diary updates.

**Inferred user intent:** Turn the analysis/design into shipped, reviewable code changes with traceability.

**Commit (code):** N/A

### What I did
- Added an “Implementation TODO (Keyboard Unification)” checklist to the ticket tasks:
  - `esp32-s3-m5/ttmp/2026/01/24/G10-ANALYZE-DEMO-UI--analyze-m5cardputer-demo-ui-userdemo-adv/tasks.md`

### Why
- Explicit tasks let us implement incrementally without losing the thread, and they give reviewers a clear “what’s done/what’s next” view.

### What worked
- The repo already has strong building blocks (`components/cardputer_kb` for GPIO scanning and `0036` for TCA8418 register-level I2C access), so task decomposition is straightforward.

### What didn't work
- N/A

### What I learned
- `esp32-s3-m5` is a git repo, while the workspace root is not; commits should be made under `esp32-s3-m5`.

### What was tricky to build
- N/A (planning-only step)

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- Execute the new tasks one-by-one, committing after each meaningful milestone.

### Code review instructions
- Start by reading the new checklist in `esp32-s3-m5/ttmp/2026/01/24/G10-ANALYZE-DEMO-UI--analyze-m5cardputer-demo-ui-userdemo-adv/tasks.md`.
