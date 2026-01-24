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
RelatedFiles:
    - Path: esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp
      Note: Replace 0066 custom TCA+ISR keyboard with cardputer_kb::UnifiedScanner
    - Path: esp32-s3-m5/components/cardputer_kb/include/cardputer_kb/scanner.h
      Note: UnifiedScanner public API + backend autodetect
    - Path: esp32-s3-m5/components/cardputer_kb/tca8418.h
      Note: C linkage for C driver functions
    - Path: esp32-s3-m5/components/cardputer_kb/unified_scanner.cpp
      Note: TCA8418 backend implementation + IDF 5.4 build fixes
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

## Step 15: Migrate tutorial 0066 to `cardputer_kb::UnifiedScanner` (commit fd2a01f)

This step updates tutorial `0066-cardputer-adv-ledchain-gfx-sim` to use the unified keyboard scanner component rather than its own embedded TCA8418 driver and ISR plumbing. The goal is to keep the project aligned with the “one consistent component” decision and to make 0066 runnable on both Cardputer and Cardputer-ADV without swapping keyboard code.

Functionally, the keyboard task now treats the scanner as a “keys currently down” sensor and synthesizes “press edge” events by diffing snapshots. The modifier and key legend differences between Cardputer and Cardputer-ADV are handled by checking which backend the scanner selected at runtime.

### Prompt Context

**User prompt (verbatim):** "update the firmware 0066 to use the new library."

**Assistant interpretation:** Replace 0066’s custom keyboard implementation with the unified keyboard component (`cardputer_kb::UnifiedScanner`) and remove duplicate local drivers.

**Inferred user intent:** Avoid maintaining two separate keyboard stacks; prove the unified component works in real firmware and can autodetect Cardputer vs Cardputer-ADV.

**Commit (code):** fd2a01f — "0066: use UnifiedScanner for keyboard"

### What I did
- Rewrote the 0066 keyboard task to use `cardputer_kb::UnifiedScanner` snapshot polling + edge detection:
  - `esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp`
- Removed the 0066-local TCA8418 implementation and stopped compiling it:
  - `esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/tca8418.c` (deleted)
  - `esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/tca8418.h` (deleted)
  - `esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/CMakeLists.txt` (removed `tca8418.c`)
- Updated the Kconfig help text to reflect the new scanner behavior:
  - `esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/Kconfig.projbuild`

### Why
- 0066 was effectively a “fork” of the keyboard stack (separate driver, ISR, event decode).
- The unified scanner already exists in `components/cardputer_kb`; 0066 should consume it rather than re-implement it.

### What worked
- The unified scanner provides `pressed_keynums` in picture-space (`1..56`), so 0066’s existing UI event shapes could stay the same; only the input plumbing changed.

### What didn't work
- N/A (migration was straightforward once the scanner existed)

### What I learned
- Keeping Cardputer vs Cardputer-ADV compatibility in one firmware is mostly about:
  - (1) scanning backend selection, and
  - (2) the “Fn/Shift/Caps” semantic differences around keynums `29/30`.

### What was tricky to build
- Mapping modifier semantics across boards without breaking existing shortcuts:
  - Cardputer has an explicit `fn` key; ADV does not (we treat `opt` as `fn`-like in 0066).
  - ADV has `capslock`; Cardputer does not.

### What warrants a second pair of eyes
- The key legend tables used by 0066 are intentionally different for Cardputer vs Cardputer-ADV in the “Fn/Shift/Caps” region; review that the chosen mapping matches the expected physical legends.

### What should be done in the future
- If 0066 needs lower latency or less polling, wire `UnifiedScannerConfig.int_gpio` into an ISR wakeup path for the TCA8418 backend.

### Code review instructions
- Start with `esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp` and confirm:
  - backend detection drives which legend table is used,
  - events are emitted on press edges only,
  - `mods` matches UI expectations (`UI_MOD_*`).
- Confirm the local TCA driver is gone from 0066:
  - `esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/CMakeLists.txt`

### Technical details
- Key commits: `fd2a01f`

## Step 16: Make `UnifiedScanner` build cleanly on ESP-IDF 5.4.1 (commits e9ce104, 4b4c6d6, 35e6f35)

This step was driven by an actual ESP-IDF build of tutorial 0066 using IDF 5.4.1. The build surfaced several “real toolchain” issues (strict type conversions, incomplete-type pitfalls, and C/C++ linkage) that weren’t caught by earlier edits because we hadn’t built a C++ firmware that instantiates `UnifiedScanner` in a translation unit.

The end result is that `idf.py build` for 0066 now succeeds under IDF 5.4.1, and the unified scanner remains a single component with an internal TCA backend.

### Prompt Context

**User prompt (verbatim):** "Try idf.py build again. source ~/esp/esp-idf-5.4.1/export.sh before hand"

**Assistant interpretation:** Build 0066 using ESP-IDF 5.4.1 and fix whatever breaks so the new unified keyboard library actually compiles and links.

**Inferred user intent:** Ensure the migration isn’t theoretical; validate it against the real build system/toolchain.

**Commit (code):** 35e6f35 — "cardputer_kb: fix C linkage for tca8418"

### What I did
- Ran a real build:
  - `source ~/esp/esp-idf-5.4.1/export.sh`
  - `cd esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim && idf.py build`
- Fixed strict type errors in the new I2C master bus config (IDF 5.4.1 expects `gpio_num_t`):
  - `esp32-s3-m5/components/cardputer_kb/unified_scanner.cpp` (commit `e9ce104`)
- Fixed incomplete-type problems with `std::unique_ptr<TcaState>` in `UnifiedScanner` by switching to a custom deleter for the pimpl:
  - `esp32-s3-m5/components/cardputer_kb/include/cardputer_kb/scanner.h`
  - `esp32-s3-m5/components/cardputer_kb/unified_scanner.cpp` (commit `4b4c6d6`)
- Fixed C/C++ linkage for the TCA helper functions (linker undefined references due to name mangling):
  - `esp32-s3-m5/components/cardputer_kb/tca8418.h` (commit `35e6f35`)

### Why
- IDF 5.4.1 builds with aggressive warnings-as-errors and stricter C++ type checking.
- The “unified component” must be build-stable, since it’s meant to be copy/pasted into future tutorials/firmwares.

### What worked
- After the fixes above, `idf.py build` for 0066 completes successfully and produces `build/cardputer_adv_ledchain_gfx_sim_0066.bin`.

### What didn't work
- Initial build failures (captured verbatim):
  - `invalid conversion from 'int' to 'gpio_num_t'` (I2C bus config)
  - `invalid application of 'sizeof' to incomplete type '...TcaState'` (unique_ptr + incomplete type)
  - `undefined reference to tca8418_*` (C/C++ linkage mismatch)

### What I learned
- If a public header owns a `std::unique_ptr<IncompleteType>`, use a custom deleter in the header (or define the owning destructor out-of-line very carefully) to keep toolchain behavior predictable.
- Whenever a C `.c` implementation is consumed by C++ code, the header must be explicit about `extern "C"`.

### What was tricky to build
- The incomplete-type issue is subtle because it can compile in some TUs and fail in others depending on how/where special members are instantiated.

### What warrants a second pair of eyes
- The pimpl deleter change is a low-risk pattern, but it’s worth confirming that no other headers depend on `std::unique_ptr<TcaState>` being the exact type (ABI isn’t a concern here, but API churn is).

### What should be done in the future
- Consider adding CI for at least one representative tutorial build (like 0066) so the unified keyboard component doesn’t silently regress.

### Code review instructions
- Validate the exact build locally:
  - `source ~/esp/esp-idf-5.4.1/export.sh`
  - `cd esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim && idf.py build`
- Review the fixes in order:
  - `esp32-s3-m5/components/cardputer_kb/unified_scanner.cpp`
  - `esp32-s3-m5/components/cardputer_kb/include/cardputer_kb/scanner.h`
  - `esp32-s3-m5/components/cardputer_kb/tca8418.h`

### Technical details
- Key commits: `e9ce104`, `4b4c6d6`, `35e6f35`

## Step 14: Fix C++ designated initializer portability (commit f98d5fd)

After adding the TCA8418 backend, I noticed `unified_scanner.cpp` used a designated initializer for `i2c_master_bus_config_t`. ESP-IDF projects commonly compile as C++17; designated initializers are C++20, so this could break builds depending on toolchain flags. Switched to explicit `memset + field assignment` to keep it portable.

### Prompt Context

**User prompt (verbatim):** "Ok, create tasks in the ticket, the implement task by task, check off tasks, commit to git, and keep a detailed diary."

**Assistant interpretation:** Keep the implementation build-friendly and reviewable; fix correctness/portability issues as they appear.

**Inferred user intent:** Avoid “looks good in docs but doesn’t compile” outcomes.

**Commit (code):** f98d5fd — "cardputer_kb: avoid C++ designated init in I2C config"

### What I did
- Replaced C++ designated init for `i2c_master_bus_config_t` with `std::memset` + explicit field assignments in:
  - `esp32-s3-m5/components/cardputer_kb/unified_scanner.cpp`

### Why
- Improves compatibility with typical ESP-IDF C++ build flags (often C++17).

### What worked
- Change is localized and doesn’t affect behavior.

### What didn't work
- N/A

## Step 13: Finalize ticket artifacts and task checkoffs (commit 337e72b)

Closed the loop by committing the remaining ticket artifacts (index, changelog, textbook) and checking off the implementation task list inside the ticket so the work is self-contained and reviewable from the docmgr workspace alone.

### Prompt Context

**User prompt (verbatim):** "Ok, create tasks in the ticket, the implement task by task, check off tasks, commit to git, and keep a detailed diary."

**Assistant interpretation:** Ensure the ticket itself is complete and accurately reflects the work done.

**Inferred user intent:** Make it easy to audit progress without spelunking git history.

**Commit (code):** 337e72b — "G10: add ticket docs and check off tasks"

### What I did
- Added the remaining ticket docs to git:
  - `index.md`, `changelog.md`, `README.md`, and the textbook design doc.
- Checked off the keyboard-unification task checklist in `tasks.md`.

### Why
- Tickets that aren’t committed tend to drift and become hard to reuse; this keeps the documentation and implementation progress synchronized.

### What worked
- Ticket now stands alone: index links → textbook + diary; tasks reflect the completed implementation sequence.

### What didn't work
- N/A

### What I learned
- N/A

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

## Step 9: Add a unified scanner facade (commit da0e436)

Implemented the first code milestone toward a single “keyboard component”: a unified scanner facade in `components/cardputer_kb` that will eventually hide whether the keyboard hardware is GPIO-matrix (Cardputer) or TCA8418 (Cardputer-ADV). This commit intentionally ships the public API skeleton and a working GPIO-matrix backend first, so subsequent tasks can add the TCA backend and runtime auto-detection without bikeshedding the surface contract.

### Prompt Context

**User prompt (verbatim):** "Ok, create tasks in the ticket, the implement task by task, check off tasks, commit to git, and keep a detailed diary."

**Assistant interpretation:** Begin implementation with small, reviewable commits, starting with the unified keyboard component’s entrypoint.

**Inferred user intent:** Build a shared keyboard component incrementally rather than as a risky big-bang refactor.

**Commit (code):** da0e436 — "cardputer_kb: add unified scanner facade"

### What I did
- Introduced a new public facade type in `components/cardputer_kb/include/cardputer_kb/scanner.h`:
  - `ScannerBackend`, `UnifiedScannerConfig`, and `UnifiedScanner`
- Implemented the facade in `components/cardputer_kb/unified_scanner.cpp`:
  - GPIO matrix backend uses the existing `MatrixScanner`
  - `Tca8418` backend currently returns `ESP_ERR_NOT_SUPPORTED` (will be implemented next)
- Wired the new source file into the component build in `components/cardputer_kb/CMakeLists.txt`.
- Committed the ticket’s implementation task list + diary updates along with the code.

### Why
- Establishing the facade early prevents “API drift” while adding backends and auto-detection; it also gives downstream firmware a stable include path to converge on.

### What worked
- The unified facade compiles conceptually with zero changes to existing `MatrixScanner` users (we’ll migrate consumers in later tasks).

### What didn't work
- N/A

### What I learned
- The cleanest incremental path is: facade → TCA backend → auto-detect → migrate consumers, rather than attempting to refactor all callers up front.

### What was tricky to build
- Keeping the facade small enough that it doesn’t prematurely bake in an I2C/interrupt strategy, while still being a meaningful “single entrypoint”.

### What warrants a second pair of eyes
- The facade currently treats “Auto” as “GPIO matrix” until TCA support lands; reviewers should confirm this is acceptable for the intermediate milestone.

### What should be done in the future
- Implement `Tca8418` backend and then switch `Auto` to “probe TCA first, else matrix”.

### Code review instructions
- Review the new facade API and stub behavior:
  - `esp32-s3-m5/components/cardputer_kb/include/cardputer_kb/scanner.h`
  - `esp32-s3-m5/components/cardputer_kb/unified_scanner.cpp`

## Step 10: Implement TCA8418 backend + runtime autodetect (commit 3f917aa)

This step delivered the core of the “one keyboard component” goal: `cardputer_kb::UnifiedScanner` now supports both keyboard hardware acquisition models and chooses between them at runtime. The guiding principle is “one public scanner contract” (`scan() -> ScanSnapshot` in picture-space), even though the underlying hardware differs radically (snapshot GPIO scan vs edge FIFO events).

### Prompt Context

**User prompt (verbatim):** "Ok, create tasks in the ticket, the implement task by task, check off tasks, commit to git, and keep a detailed diary."

**Assistant interpretation:** Implement the missing Cardputer-ADV keyboard backend and auto-detection so downstream firmware can stop hardcoding assumptions.

**Inferred user intent:** Make “Cardputer vs Cardputer-ADV keyboard hardware” a runtime implementation detail rather than an app-level fork.

**Commit (code):** 3f917aa — "cardputer_kb: add TCA8418 backend + autodetect"

### What I did
- Added a minimal, register-level TCA8418 I2C helper into the component:
  - `esp32-s3-m5/components/cardputer_kb/tca8418.h`
  - `esp32-s3-m5/components/cardputer_kb/tca8418.c`
- Extended `cardputer_kb::UnifiedScanner`:
  - Added a private `TcaState` implementation with I2C init/probe, event draining, and a “pressed keys” bitset.
  - Implemented TCA event decode → picture-space `(x,y)` → `keyNum` updates.
  - Implemented `ScannerBackend::Auto` as “probe TCA8418 first, else fall back to GPIO matrix”.
- Updated `components/cardputer_kb/CMakeLists.txt` to build the new sources and depend on `esp_driver_i2c`.

### Why
- This makes the keyboard hardware choice explicit and centralized, so UI/framework code can be written once against `ScanSnapshot` without carrying device-specific forks.

### What worked
- The TCA backend produces the same picture-space `keyNum` values (1..56) as the existing GPIO scanner by applying the same remap formula used in the ADV demo code.

### What didn't work
- N/A

### What I learned
- A practical “unification seam” is the existing `ScanSnapshot` contract: if we can express both hardware models as a stable set of currently-pressed keys in picture-space, a lot of higher-level decoding and UI logic becomes reusable.

### What was tricky to build
- Avoiding pin-ownership conflicts: the unified scanner probes TCA8418 via I2C first and only configures GPIO matrix scanning if the probe fails, so it can safely reuse the same physical pins across variants.

### What warrants a second pair of eyes
- The TCA8418 event bit7 “pressed” convention varies across docs; this code follows the existing ADV firmware convention (bit7 set → pressed). A reviewer should confirm this against real hardware traces.

### What should be done in the future
- Migrate firmware consumers to use `UnifiedScanner` so the unification is real in practice, not just an available API.

### Code review instructions
- Focus review on the unification logic and the remap:
  - `esp32-s3-m5/components/cardputer_kb/unified_scanner.cpp`
  - `esp32-s3-m5/components/cardputer_kb/tca8418.c`

## Step 11: Migrate firmware consumers to the unified scanner (commit 721b665)

This step made the “one component” story real by migrating multiple firmware examples in `esp32-s3-m5` away from direct `MatrixScanner` usage and onto `cardputer_kb::UnifiedScanner`. This reduces the number of places that encode “how the keyboard works” and ensures the runtime autodetect behavior is exercised by real call sites.

### Prompt Context

**User prompt (verbatim):** "Ok, create tasks in the ticket, the implement task by task, check off tasks, commit to git, and keep a detailed diary."

**Assistant interpretation:** After adding the unified scanner implementation, update downstream code to use it so the refactor is complete in practice.

**Inferred user intent:** Avoid a “library exists but nobody uses it” outcome; keep consumers converged.

**Commit (code):** 721b665 — "Use cardputer_kb::UnifiedScanner in demos"

### What I did
- Updated the keyboard wrappers in:
  - `esp32-s3-m5/0025-cardputer-lvgl-demo/main/input_keyboard.h`
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/input_keyboard.h`
  to use `cardputer_kb::UnifiedScanner` instead of `MatrixScanner` and propagate `init()` errors.
- Updated tasks that scan the keyboard directly:
  - `esp32-s3-m5/0030-cardputer-console-eventbus/main/app_main.cpp`
  - `esp32-s3-m5/0028-cardputer-esp-event-demo/main/app_main.cpp`
  - `esp32-s3-m5/0023-cardputer-kb-scancode-calibrator/main/app_main.cpp`
  to instantiate `cardputer_kb::UnifiedScanner` and added `esp_err.h` where needed for `ESP_ERROR_CHECK`.

### Why
- A unified component only improves maintainability if consumers actually adopt it; otherwise, the old “split components” problem persists.

### What worked
- Because `UnifiedScanner` preserves the `ScanSnapshot` contract (pressed keys in picture-space + keynums), most call sites required only a type swap.

### What didn't work
- N/A

### What I learned
- The existing higher-level decoding logic (bindings, modifiers, chords) composes cleanly with the unified scanner because it already operates on `pressed_keynums` rather than on electrical details.

### What was tricky to build
- Ensuring all call sites that use `ESP_ERROR_CHECK(...)` include `esp_err.h` explicitly.

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- Update the `components/cardputer_kb` documentation to recommend `UnifiedScanner` as the primary entrypoint and clarify the autodetect behavior.

### Code review instructions
- Review the migration diffs first (they should be boring):
  - `esp32-s3-m5/0025-cardputer-lvgl-demo/main/input_keyboard.h`
  - `esp32-s3-m5/0030-cardputer-console-eventbus/main/app_main.cpp`

## Step 12: Update `cardputer_kb` docs to point to UnifiedScanner (commit dacd92d)

This step updates the component documentation so the “one consistent component” path is obvious to future readers. The docs now describe `UnifiedScanner` as the primary entrypoint and explicitly call out that it supports both Cardputer GPIO-matrix scanning and Cardputer-ADV’s TCA8418.

### Prompt Context

**User prompt (verbatim):** "Ok, create tasks in the ticket, the implement task by task, check off tasks, commit to git, and keep a detailed diary."

**Assistant interpretation:** After implementing and migrating, update the docs so the repo doesn’t regress into split/duplicated keyboard handling.

**Inferred user intent:** Institutionalize the “one component” decision through documentation as well as code.

**Commit (code):** dacd92d — "cardputer_kb docs: recommend UnifiedScanner"

### What I did
- Updated:
  - `esp32-s3-m5/components/cardputer_kb/README.md`
  - `esp32-s3-m5/components/cardputer_kb/REFERENCE.md`
  - `esp32-s3-m5/components/cardputer_kb/TUTORIAL.md`
  to describe `cardputer_kb::UnifiedScanner` as the default scanner API and note TCA8418 support.

### Why
- Without doc updates, new firmware examples tend to copy older snippets (MatrixScanner-only), recreating the split-component situation over time.

### What worked
- The docs changes were small and localized; no code behavior changes.

### What didn't work
- N/A

### What I learned
- Documentation is part of the interface: once the API is unified, the docs should stop framing the component as “matrix scan only”.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- N/A
