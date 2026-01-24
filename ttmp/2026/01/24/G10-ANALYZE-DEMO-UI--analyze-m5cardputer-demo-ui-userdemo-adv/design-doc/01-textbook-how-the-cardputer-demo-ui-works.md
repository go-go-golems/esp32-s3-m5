---
Title: 'Textbook: How the Cardputer Demo UI Works'
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
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: M5Cardputer-UserDemo-ADV/components/mooncake/src/ability_manager/ability_manager.cpp
      Note: Mooncake v2 ability lifecycle and scheduling
    - Path: M5Cardputer-UserDemo-ADV/components/smooth_ui_toolkit/src/widget/select_menu/smooth_selector.cpp
      Note: SmoothSelectorMenu widget; animation + callback scheduling + input/render cadence
    - Path: M5Cardputer-UserDemo-ADV/main/main.cpp
      Note: ADV demo entrypoint; wires ui_hal tick/delay and runs main loop
    - Path: M5Cardputer-UserDemo/components/mooncake/src/app/app_manager.cpp
      Note: Old Mooncake app lifecycle FSM (create/resume/running/bg/pause/destroy)
    - Path: M5Cardputer-UserDemo/main/apps/launcher/views/menu/menu.cpp
      Note: Old launcher menu; uses Smooth_Menu; opens apps and backgrounds launcher
    - Path: M5Cardputer-UserDemo/main/apps/utils/smooth_menu/src/selector/selector.cpp
      Note: Old Smooth_Menu selector animation + click handling
    - Path: M5Cardputer-UserDemo/main/cardputer.cpp
      Note: Old demo entrypoint; installs apps and runs Mooncake update loop
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-24T13:41:42.88407406-05:00
WhatFor: ""
WhenToUse: ""
---


# Textbook: How the Cardputer Demo UI Works

## Executive Summary

- Both `M5Cardputer-UserDemo` and `M5Cardputer-UserDemo-ADV` implement a **sprite-based, single-threaded UI loop** on ESP-IDF/FreeRTOS: one task repeatedly updates input/state and redraws into offscreen sprites, then pushes those sprites to the display.
- The demos share the same “shape” (HAL → framework scheduler → apps/scenes → widgets → pixels) but differ in **generation and abstraction**:
  - **UserDemo (older)**: `MOONCAKE` v0.x + `SMOOTH_MENU` + ad-hoc blocking input loops; apps are **allocated/destroyed dynamically** via “packer” factories.
  - **UserDemo-ADV (newer)**: `mooncake` v2 (“abilities”) + `smooth_ui_toolkit` (springs/easing, callback cadence, selector menu widget); apps are **persistent abilities** that transition between open/running/sleeping.
- “Animations” are not magic: they are **state machines that turn time into geometry**. Old uses LVGL-style interpolation paths; ADV uses a small `Animate` engine with spring/easing generators and retargeting.
- “Event callbacks” are also not magic: they’re either **framework-invoked virtual methods** (lifecycle, widget hooks) or **observer signals** (`Signal`) that deliver hardware events to subscribers; both ultimately run on the main UI task in these demos.

## Problem Statement

You want a code-faithful explanation of how these demo UIs are built:

- What are the widgets, and what “kind” of UI system is this (retained vs immediate mode)?
- How are animations represented, updated, and composed?
- How are events delivered (polling vs callbacks vs signals), and how do apps respond?
- How are “scenes” (apps) loaded/unloaded (or opened/closed), and what happens to memory?
- How does all of this map onto ESP-IDF/FreeRTOS primitives and constraints?

This document answers those questions by building a reusable mental model, starting from fundamentals (time, tasks, and pixels) and ending at the concrete code paths in both projects.

## Proposed Solution

This section is the “textbook”: it is not just a tour of files; it is a reconstruction of the underlying design.

### How to Read This Document

1. Read **Part 1** to lock in the runtime model: how time flows, what a “frame” is, and where the UI code runs.
2. Read **Part 2–3** (UserDemo) to understand the “classic” Mooncake + Smooth_Menu architecture.
3. Read **Part 4–6** (ADV) to understand the “abilities + smooth_ui_toolkit” architecture.
4. Use **Part 7** as your “map”: UI ↔ FreeRTOS primitives, plus where concurrency actually enters.
5. Use **Part 8** as guidance when you want to extend or refactor.

> **Fundamental: The UI is a function.**
>
> In both demos, the UI you see at time *t* is the result of evaluating a function:
>
> `Frame(t) = Render( State(t), InputEvents(t), Assets )`
>
> “Animation” is simply *how State(t) changes with t*; “event handling” is *how InputEvents(t) changes State(t)*.
> This framing is powerful because it keeps you honest: if you can’t say what state changes on what input and what time,
> you don’t yet understand the UI.

---

## Part 1: The Runtime Model (Tasks, Time, and Pixels)

### 1.1 Where Does the UI Run?

Both demos are ESP-IDF apps. `app_main()` runs inside a FreeRTOS task created by ESP-IDF. Both demos then implement their own infinite loop:

- UserDemo: `while (1) mooncake.update();` (`M5Cardputer-UserDemo/main/cardputer.cpp`)
- ADV: `while (1) { GetHAL().feedTheDog(); GetHAL().update(); GetMooncake().update(); }` (`M5Cardputer-UserDemo-ADV/main/main.cpp`)

> **Fundamental: Single-threaded UI (by convention).**
>
> Nothing in ESP-IDF forces a single UI task. The demos choose it. This choice is pragmatic:
> - rendering code becomes simpler (no locking around the display/canvas),
> - ordering is deterministic (input → update → render),
> - but long-running operations inside the loop freeze the UI.

### 1.2 Time Sources: “millis” and the UI Tick

The UI loop needs a monotonic time source.

- UserDemo defines:
  - `millis()` as `esp_timer_get_time()/1000`
  - `delay(ms)` as `vTaskDelay(pdMS_TO_TICKS(ms))`
  in `M5Cardputer-UserDemo/main/apps/utils/common_define.h`.

- ADV uses:
  - `GetHAL().millis()` (backed by `m5gfx::millis()`) and `GetHAL().delay(ms)` (backed by `m5gfx::delay(ms)`) in `M5Cardputer-UserDemo-ADV/main/hal/hal.h`,
  - plus **injects** these into `smooth_ui_toolkit`:
    - `ui_hal::on_delay([](ms){ GetHAL().delay(ms); })`
    - `ui_hal::on_get_tick([](){ return GetHAL().millis(); })`
    in `M5Cardputer-UserDemo-ADV/main/main.cpp`.

That injection is a small design choice with a big payoff: `smooth_ui_toolkit` becomes platform-agnostic while still being able to animate in “real time”.

### 1.3 Pixels: Sprites as the UI’s Memory

Both demos draw into `LGFX_Sprite` objects (“canvas sprites”), then push them to the physical display.

- UserDemo creates sprites in `M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp`:
  - main canvas: `204x109`
  - keyboard bar: `(display.width - 204) x display.height`
  - system bar: `204 x (display.height - 109)`

- ADV creates the same geometry in `M5Cardputer-UserDemo-ADV/main/hal/hal.cpp`.

This partitions the 240×135 Cardputer display into a “T layout”:

- left strip: keyboard modifier indicators
- top bar: system status (wifi/battery/time)
- main pane: app content

> **Fundamental: Sprites are an explicit buffering strategy.**
>
> A sprite is a RAM-backed framebuffer. Drawing into a sprite and then pushing it to the display:
> - reduces tearing,
> - makes it easy to clear/redraw whole regions,
> - and turns layout into simple coordinate math (“push sprite at offset”).
>
> The cost is RAM (sprites are pixel buffers) and the discipline to redraw consistently.

---

## Part 2: UserDemo’s Framework (Mooncake v0.x)

UserDemo’s Mooncake is a tiny “activity manager”:

- a registry of installed app factories (“packers”),
- a scheduler that runs app lifecycle callbacks,
- and a simple key/value database for dependency injection.

### 2.1 Entry Point and App Installation

In `M5Cardputer-UserDemo/main/cardputer.cpp`:

1. `hal.init()` sets up display, sprites, keyboard, audio, etc.
2. The database setup callback inserts `HAL*` into Mooncake’s database:
   - `db.Add<HAL::Hal*>("HAL", &hal);`
3. Mooncake is initialized and apps are installed as packers (`moon cake.installApp(new APPS::AppWifiScan_Packer)`).
4. The launcher packer is created and then **an instance** of the launcher app is created:
   - `mooncake.createApp(launcher);`
5. The loop runs forever: `mooncake.update();`

### 2.2 App Packer vs App Instance (Why Two Types?)

Mooncake v0.x splits “an app” into:

- `APP_PACKER_BASE` (“static-ish”):
  - name/icon metadata,
  - pointer-sized `userData`,
  - factory (`newApp`) and destructor (`deleteApp`) hooks.
- `APP_BASE` (“dynamic instance”):
  - lifecycle callbacks (`onCreate`, `onResume`, `onRunning`, …),
  - internal flags (`_go_start`, `_go_close`, `_go_destroy`),
  - a pointer to its packer (to access metadata and userData).

Why this matters: it makes “apps come and go” feasible on a RAM-constrained device. The launcher can display icons for installed apps (from packers) without needing every app instance alive.

> **Fundamental: Separate identity (metadata) from execution (instance).**
>
> Many systems do this:
> - Android: manifest entry (metadata) vs activity instance,
> - desktop: `.desktop` file vs running process,
> - here: packer vs app object.
>
> It’s a memory and lifecycle control trick.

### 2.3 The App Lifecycle FSM (APP_Manager)

`APP_Manager` (`M5Cardputer-UserDemo/components/mooncake/src/app/app_manager.cpp`) owns the lifecycle transitions.

State set:

- `ON_CREATE`
- `ON_RESUME`
- `ON_RUNNING`
- `ON_RUNNING_BG`
- `ON_PAUSE`
- `ON_DESTROY`

Core idea:

- Apps don’t directly “switch states”; they set flags (`startApp()`, `closeApp()`, `destroyApp()`) and the manager interprets those flags during `update()`.

The most important branch is close semantics:

- If an app is closed and `allow_bg_running` is true, it transitions to background (`ON_PAUSE → ON_RUNNING_BG`).
- Otherwise, it is destroyed (`ON_DESTROY`) and memory is reclaimed via `APP_PACKER_BASE::deleteApp()`.

Minimal pseudocode (faithful to the implementation):

```text
for app in lifecycle_list:
  if app.go_start:  state = ON_RESUME
  if app.go_close:  state = allow_bg ? ON_PAUSE : ON_DESTROY
  if app.go_destroy: state = ON_DESTROY

  switch state:
    ON_RESUME:    app.onResume(); state = ON_RUNNING
    ON_RUNNING:   app.onRunning()
    ON_PAUSE:     app.onPause(); state = ON_RUNNING_BG
    ON_RUNNING_BG: app.onRunningBG()
    ON_DESTROY:   app.onPause(); app.onDestroy(); packer.deleteApp(app); remove
```

### 2.4 “Scenes” in UserDemo: How the Launcher Uses the FSM

The launcher (`M5Cardputer-UserDemo/main/apps/launcher/launcher.cpp`) uses the lifecycle model as a scene switcher:

- On create:
  - it grabs `HAL*` from the database,
  - starts boot/menu/system/keyboard bars,
  - calls `setAllowBgRunning(true)` so it can stay alive while another app is running,
  - and calls `startApp()` (sets the “go start” flag).

- While running (`onRunning`):
  - it updates menu and bars.

- While background (`onRunningBG`):
  - it keeps system and keyboard bars alive,
  - and watches `getCreatedAppNum()`:
    - if the only remaining app is the launcher itself, it plays a close animation and returns to foreground.

This is a crucial lesson: *“go back to launcher” is launcher policy*, not framework behavior.

---

## Part 3: UserDemo’s Widget/Animation Substrate (Smooth_Menu)

UserDemo’s launcher uses `SMOOTH_MENU` (`M5Cardputer-UserDemo/main/apps/utils/smooth_menu/src/...`) to implement the horizontally scrolling app icon selector.

### 3.1 What is a “Widget” Here?

In this codebase, widgets are closer to “controller objects” than retained UI nodes:

- they hold a small amount of state (e.g., selected index, current animation progress),
- they compute geometry (e.g., selector rectangle),
- and they invoke callbacks so the application can draw the current frame.

There is no retained scene graph, no automatic invalidation, and no layout engine. It is a minimal, explicit architecture.

### 3.2 Smooth_Menu’s Decomposition

The core pieces:

- `Menu_t`: holds `Item_t` objects (tag, keyframe rectangle, userData).
- `Selector_t`: an animated rectangle that targets an item’s keyframe.
- `Camrea_t`: an animated “viewport offset” so the target stays within a window.
- `Simple_Menu`: convenience wrapper that owns `Menu_t + Selector_t + Camrea_t` and offers `goNext/goLast/update/render`.

`Selector_t` is the heart: it maintains four independent `LVGL::Anim_Path` interpolators for x/y/width/height. On selection change, it retargets those paths from their current value to the new item’s keyframe.

### 3.3 Animation in Smooth_Menu: LVGL Paths

`LVGL::Anim_Path` (UserDemo’s copy in `smooth_menu/src/lv_anim`) is a **pure interpolation function**:

- You set `start_value`, `end_value`, and `time`.
- You “sample” by providing `currentTime` and reading `getValue(currentTime)`.

This is animation as math, not animation as threads.

> **Fundamental: Animation = interpolation + sampling.**
>
> Any smooth animation system has two halves:
> - **interpolation**: define the continuous path from A to B,
> - **sampling**: decide how often you evaluate it (your frame rate).
>
> Smooth_Menu defines interpolation with LVGL paths and samples it whenever the launcher calls `update(millis())`.

### 3.4 Event Handling in UserDemo: Polling + “Hold Until Release”

The launcher doesn’t receive “key events”; it actively queries keyboard state.

`M5Cardputer-UserDemo/main/apps/launcher/port/port.cpp` implements input as *blocking* helpers:

- if “next” is held:
  - while it remains held: update menu, redraw, and rescan keyboard.
  - only then return a single “true” meaning “advance once”.

This pattern trades CPU for debouncing and simplicity, while still keeping animations alive by continuing to redraw within the loop.

---

## Part 4: ADV’s Framework (Mooncake v2 Abilities)

Mooncake v2 replaces packers/app instances with a more uniform concept: an **ability** object with a type-specific lifecycle state machine.

### 4.1 The Two Managers: Apps vs Extensions

`mooncake::Mooncake` (`M5Cardputer-UserDemo-ADV/components/mooncake/src/mooncake.h`) maintains:

- an **app ability manager** (`AbilityManager`) for “apps” you open/close,
- an **extension manager** (`AbilityManager`) for any other long-lived abilities (custom, workers, utilities).

The managers are lazy-initialized on first use.

### 4.2 AbilityManager Scheduling

`AbilityManager` (`.../ability_manager.cpp`) is a scheduler for a list of `unique_ptr<AbilityBase>`.

Key implementation facts:

- `createAbility()` assigns an integer ID and pushes the ability into `_new_ability_list`.
- `updateAbilities()` merges `_new_ability_list` into `_ability_list` and iterates the list.
- Each ability has a manager-level state:
  - `StateGoCreate` → call `baseCreate()`, then immediately call `baseUpdate()`, then mark `StateUpdating`
  - `StateUpdating` → call `baseUpdate()`
  - `StateGoDestroy` → call `baseDestroy()` and erase the ability

So “creation” is amortized into the next update tick; there is no separate thread, no queue, and no blocking.

> **Fundamental: Make scheduling explicit and deterministic.**
>
> The ability manager is essentially a small interpreter:
> `for each ability: run the next lifecycle instruction`.
> This avoids many embedded pitfalls because it makes *when* things happen a matter of code, not timing accidents.

### 4.3 AppAbility: Open/Close Without Freeing

An `AppAbility` (`.../ability/ability.h`) has internal states:

- `StateSleeping`
- `StateGoOpen` → calls `onOpen()`, then `StateRunning`
- `StateRunning` → calls `onRunning()`
- `StateGoClose` → calls `onClose()`, then `StateSleeping`

This is deliberately different from UserDemo:

- closing does not destroy the object,
- state (and memory) is retained across opens.

The ADV launcher uses `GetMooncake().openApp(appId)` and `GetMooncake().getAppCurrentState(appId)` to implement “which app is currently running”.

---

## Part 5: ADV’s Animation & Widget Toolkit (smooth_ui_toolkit)

ADV’s `smooth_ui_toolkit` is a reusable “motion core”:

- it centralizes time (`ui_hal`),
- it provides an animation engine (`Animate` + generators),
- and it defines a key widget (`SmoothSelectorMenu`) used by the launcher and other apps.

### 5.1 ui_hal: Pluggable Tick and Delay

`smooth_ui_toolkit::ui_hal` (`.../utils/hal/hal.{h,cpp}`) provides:

- `get_tick()` / `get_tick_s()`
- `delay(ms)` / `delay_s(s)`

with a configurable backend (`on_get_tick`, `on_delay`). ADV injects HAL implementations at boot.

This is the interface between “platform time” (esp_timer/m5gfx) and “UI time” (animation t).

### 5.2 Animate: A Small State Machine

`Animate` (`.../animation/animate/animate.{h,cpp}`) is an engine parameterized by:

- timing (`delay`, `repeat`, `repeatDelay`, `repeatType`)
- generator type (`spring` or `easing`)
- callbacks (`onUpdate`, `onComplete`)

Update is explicit: calling `update()` advances the state machine using `ui_hal::get_tick_s()`.

Two key design points:

1. **Generators are swappable** (spring vs easing).
2. **Retargeting is supported**:
   - springs can be retargeted mid-flight without snapping (they carry forward velocity).

### 5.3 AnimateValue and AnimateVector2: Lazy Animation as a Value

`AnimateValue` wraps `Animate` and provides:

- `operator float()` → returns current value,
- assignment `brightness = 255` → retargets to a new value.

Critically, `AnimateValue::value()` calls `update()` internally; you get “lazy updates” when you read the value.

This enables patterns like the ADV boot fade-in:

- create `AnimateValue brightness; brightness.easingOptions().duration = 600; brightness = 255;`
- loop for ~1.2s, repeatedly doing `display.setBrightness(brightness);`

The act of reading `brightness` updates it.

> **Fundamental: Values can be time-dependent.**
>
> `AnimateValue` makes a value behave like a function of time while still looking like a scalar.
> This is elegant, but it creates a responsibility: *somebody must keep reading/updating it*,
> or the animation will never progress.

### 5.4 SmoothSelectorMenu: A Menu Widget With a Physics-Based Selector

`SmoothSelectorMenu` (`.../widget/select_menu/smooth_selector.{h,cpp}`) is the ADV equivalent of Smooth_Menu’s selector.

It stores:

- a list of options, each with a `Vector4 keyframe` and `void* userData`,
- animated selector position/shape (`AnimateVector2` for x/y and w/h),
- animated camera offset (`AnimateVector2`),
- a small scheduler for callbacks:
  - `onReadInput` every `readInputInterval` ms,
  - `onRender` every `renderInterval` ms,
  - `onClick` only after “release” and “animation done”.

The update algorithm (simplified but faithful):

```text
update(now_ms):
  onUpdate(now_ms)

  if now_ms - lastReadInput > readInputInterval:
    onReadInput()

  if selectionChanged:
    retarget selector + camera

  selector_pos.update(now_s)
  selector_shape.update(now_s)
  camera.update(now_s)

  if now_ms - lastRender > renderInterval:
    onRender()

  if wasReleased and selector done:
    onClick()
```

This solves a common embedded UI problem: it decouples “read input frequency” and “render frequency” from the app’s outer loop while still remaining cooperative.

---

## Part 6: Events and Callbacks in ADV (From ISR to Widget)

ADV uses a mix of polling and signal callbacks.

### 6.1 Keyboard Events: ISR Flag → Update → Signal Emit

`Keyboard` (`M5Cardputer-UserDemo-ADV/main/hal/keyboard/keybaord.cpp`) uses:

- a GPIO ISR (`gpio_isr_handler`) that sets a `volatile` flag `_isr_flag`,
- `Keyboard::update()` which runs in the main loop:
  - clears the “latest event” buffers,
  - if `_isr_flag` is set, reads an event from TCA8418,
  - remaps row/col,
  - emits signals:
    - `onKeyEventRaw.emit(...)`
    - `onKeyEvent.emit(...)`

Two important consequences:

1. Emitting happens in task context, not ISR context (good: mutex usage is safe).
2. Because the buffers are cleared every `update()`, polling `getLatestKeyEventRaw()` is effectively “read the most recent event this frame, if any”.

### 6.2 Two Consumption Styles

**Signal-driven (push):**

- The launcher’s keyboard bar connects a lambda to `GetHAL().keyboard.onKeyEvent` and redraws the bar when modifier states change (`view/keyboard_bar/keyboard_bar.cpp`).

This is event-driven and efficient.

**Polling-driven (pull):**

- `LauncherMenu::onReadInput()` reads `GetHAL().keyboard.getLatestKeyEventRaw()` and turns it into navigation (`goNext/goLast/press/release`) (`view/menu/menu.cpp`).

This style is simpler to reason about inside a widget but can drop events if the update cadence is wrong.

> **Fundamental: Event delivery has a contract.**
>
> “Signals” define a push contract: you’ll be called when an event occurs.
> “Polling latest event” defines a pull contract: you must poll at the right time or you miss it.
> Both can work; what matters is that the contract is explicit.

### 6.3 Design Update: One Consistent Keyboard Component (Not 5 Pieces)

If you want a **single reusable keyboard component** that works across “ADV-style” and “non-ADV-style” stacks, the core requirement is to standardize **one event contract** and hide the hardware differences behind one interface.

Concretely, unify the keyboard into one component with:

- **One public API surface** (one component), not:
  - “latest event buffer” for polling,
  - plus separate signals,
  - plus separate per-project scanning logic,
  - plus separate remap logic,
  - plus separate modifier tracking.
- **Two internal backends** (implementation detail):
  - **TCA8418 backend** (ADV-like): IRQ + FIFO events over I2C (`TCA8418_DEFAULT_ADDR` = `0x34`).
  - **Matrix-scan backend** (UserDemo-like): periodic GPIO scan, with press/release events synthesized by diffing the current key set against the previous key set.

> **Fundamental: Unification happens at the boundary.**
>
> A device can have wildly different hardware implementations.
> A UI wants one thing: a reliable stream of “key pressed/released” events plus modifier state.
> The right abstraction boundary is therefore **an event stream**, not “row/col polling”.

#### What We Implemented in This Repo: `cardputer_kb::UnifiedScanner`

In this repo, that “one component” is implemented as `cardputer_kb::UnifiedScanner`:

- Public entrypoint: `esp32-s3-m5/components/cardputer_kb/include/cardputer_kb/scanner.h`
- Backends:
  - `ScannerBackend::GpioMatrix` wraps the existing `MatrixScanner`.
  - `ScannerBackend::Tca8418` drains the TCA8418 FIFO over I2C and maintains a pressed-key set.
  - `ScannerBackend::Auto` probes the TCA8418 first, then falls back to GPIO scanning.
- Stable “picture-space” output: `ScanSnapshot::pressed_keynums` uses the vendor-style key numbers `1..56` (4×14), regardless of backend.

This means UI code can standardize on a single “scan snapshot” contract and build *its own* event edges/modifiers as needed.

> **Fundamental: A scan snapshot is a “raw sensor reading”.**
>
> A snapshot is *not yet an event stream*. It’s the current “keys down” set.
> If you want “pressed this frame” semantics, you diff snapshots.

#### One Event Contract (Recommended)

Define one canonical event shape and guarantee that it is delivered the same way regardless of backend:

- `KeyEventRaw { bool pressed; uint8_t row; uint8_t col; uint32_t t_ms; uint32_t seq; }`
- `KeyEvent { bool pressed; KeyCode code; bool isModifier; uint8_t modifiers; uint32_t t_ms; uint32_t seq; }`

Then choose **one** delivery mechanism and build convenience adapters on top:

- **Primary:** a ring buffer / queue of events that consumers can drain.
- **Optional:** subscription callbacks (“signal”) that are invoked when events are enqueued.

Crucially, the “latest event” becomes a *derived view* (`peekLatest()`), not the source of truth. That eliminates the “cleared every update” footgun.

#### Programmatic Differentiation (ADV vs non-ADV): Runtime Probe

Yes—if the goal is “figure out which keyboard hardware we have” at runtime, you can do it programmatically.

In the `UnifiedScanner` implementation, this happens via `backend()` after `init()`:

- `ScannerBackend::Tca8418` means “Cardputer-ADV style” (TCA8418 present on I2C, default addr `0x34`, default SDA=GPIO8/SCL=GPIO9).
- `ScannerBackend::GpioMatrix` means “Cardputer style” (GPIO matrix scan).

Pseudocode:

```cpp
cardputer_kb::UnifiedScanner kb;
ESP_ERROR_CHECK(kb.init()); // default: Auto

if (kb.backend() == cardputer_kb::ScannerBackend::Tca8418) {
  // Cardputer-ADV keyboard: no Fn key in the vendor legend; CapsLock exists.
} else {
  // Cardputer keyboard: Fn exists; no CapsLock key.
}
```

This directly answers “can we determine which keyboard is what programmatically?” without relying on build flags or repo names.

Under the hood, the probe is a “strong signal” register read (`CFG`), because it avoids false positives compared to “device ACK only”.

#### Example: Using the Unified Scanner in a UI Task

Tutorial `0066-cardputer-adv-ledchain-gfx-sim` was migrated to the unified component so it can run on either board.

The key pattern is: `snapshot = scan();` then “press edges” by diffing against the previous snapshot.

```cpp
auto snap = kb.scan();
auto down = snap.pressed_keynums;           // "keys down" now
auto pressed_edges = diff(down, prev_down); // "pressed this frame"
prev_down = down;
```

#### Mapping to FreeRTOS Primitives (When You Implement It)

There are two solid implementation strategies:

1. **ISR sets a flag; task drains FIFO; task enqueues events** (similar to current ADV):
   - simplest, avoids doing I2C work in ISR context.
2. **ISR pushes minimal metadata into a FreeRTOS queue** (advanced):
   - `xQueueSendFromISR()` + `portYIELD_FROM_ISR()`,
   - still do I2C reads in task context; the ISR just wakes the task.

Both approaches keep heavy work out of ISR context while still making event delivery robust.

---

## Part 7: Mapping the UI to FreeRTOS and ESP-IDF

The UI layers are written in C++, but the platform is FreeRTOS + ESP-IDF. Here’s the practical mapping you care about.

### 7.1 The Big Picture (Who Runs What?)

| Concern | UserDemo | ADV | FreeRTOS/ESP-IDF Primitive |
|---|---|---|---|
| “UI thread” | `app_main` loop + `mooncake.update()` | `app_main` loop + `GetHAL().update()` + `GetMooncake().update()` | A FreeRTOS task created by ESP-IDF |
| Yielding / delay | `delay(ms)` macro | `feedTheDog()` + HAL delay | `vTaskDelay()` or equivalent |
| Monotonic time | `millis()` macro | `GetHAL().millis()` | `esp_timer_get_time()` or equivalent |
| Keyboard input | GPIO matrix scanned in task | GPIO ISR sets flag; task drains event | ISR + task-level processing |
| “Signals” | (not central) | `mclog::Signal` (mutex-protected observers) | `std::mutex` → pthread → FreeRTOS sync |
| Wi-Fi connect state | apps/utilities | HAL uses ESP event loop + event groups | `esp_event` + `EventGroupHandle_t` |

### 7.2 A Practical Rule of Thumb

If code runs inside the main UI loop, treat it as part of the UI scheduler:

- avoid long blocking operations,
- avoid calling `delay()` inside tight loops unless you *want* to freeze rendering,
- prefer incremental updates and time slicing.

If you must do long work, either:

- move it into a background task and communicate via queues/events, or
- convert it into an incremental state machine and run it cooperatively.

---

## Part 8: Comparing the Two Demos (Tradeoffs and Guidance)

### 8.1 Lifecycle Tradeoffs

UserDemo:

- **Pros:** memory is reclaimed when apps close; packer metadata supports a launcher UI cheaply.
- **Cons:** app startup cost happens on every open; app state is lost unless persisted; lifecycle is more manual.

ADV:

- **Pros:** apps can keep state in memory; open/close is cheap; lifecycle logic is uniform across ability types.
- **Cons:** memory footprint grows with installed apps; “closing” doesn’t reduce memory pressure unless abilities are destroyed.

### 8.2 Widget/Animation Tradeoffs

Smooth_Menu (UserDemo):

- **Pros:** simple integer interpolation; easy to reason about; no hidden scheduling.
- **Cons:** callback cadence and click detection are app-managed; animation composition is mostly manual.

SmoothSelectorMenu + Animate (ADV):

- **Pros:** standardized callback cadence; spring/easing engines; retargeting is built-in; click detection is integrated.
- **Cons:** lazy updates mean you must understand “when value() is read”; polling patterns can silently miss events if cadence slips.

### 8.3 How to Add a New App (Conceptual Cookbook)

UserDemo pattern:

1. Create a new `APP_PACKER_BASE` subclass that returns name/icon and implements `newApp/deleteApp`.
2. Implement an `APP_BASE` subclass and override lifecycle callbacks.
3. Install the packer in `cardputer.cpp`.
4. Decide whether the app should:
   - destroy itself on close (`allow_bg_running=false`, default), or
   - keep running in background (`allow_bg_running=true`).

ADV pattern:

1. Create an `AppAbility` subclass, set `setAppInfo().name` (and optionally `userData` icon).
2. Implement `onOpen/onRunning/onClose` and decide what state transitions you want (`open()` / `close()` calls).
3. Install the app via `GetMooncake().installApp(std::make_unique<MyApp>())`.

> **Fundamental: A “scene” is just a lifecycle boundary.**
>
> In both demos, “scenes” are not a graphics abstraction—they are lifecycle and scheduling boundaries.
> A scene is “what code is allowed to run and draw right now”.


## Design Decisions

This ticket is analysis, but the demos embody design decisions worth naming explicitly:

1. **Single-threaded UI loop:** simplifies rendering correctness and ordering; pushes long work into either cooperative state machines or background subsystems.
2. **Sprite partitioning:** makes layout and partial redraw explicit; reduces tearing.
3. **State-machine lifecycles:** both demos implement explicit lifecycle FSMs (apps or abilities), making scene transitions code-driven instead of ad-hoc.
4. **Animation as “time → value”:** both demos implement animation as explicit interpolation sampled in the main loop (no animation threads).
5. **Two event styles (ADV):** mixes signals (push) and polling (pull); flexible but demands careful cadence decisions.

## Alternatives Considered

These are not “rejected by this ticket” (we’re not changing the code), but they are the natural alternatives to understand:

- **Retained-mode UI framework (e.g., full LVGL object tree):**
  - Pros: widgets/layout/state managed by framework; events are standardized.
  - Cons: heavier RAM/flash; more complex integration; different programming model than the demos’ “immediate drawing”.

- **Dedicated UI task + message queue:**
  - Pros: clearer separation between UI and background work; predictable render cadence.
  - Cons: introduces cross-thread synchronization around the display and UI state; more failure modes (deadlocks, missed wakeups).

- **Pure signal/event-driven UI (no polling):**
  - Pros: can be more power-efficient; reacts directly to events.
  - Cons: still needs a render schedule for animations; “draw on every event” can become chaotic without a frame clock.

## Implementation Plan

If you want to *apply* this understanding (rather than rewrite the demos), this is a practical plan:

1. Decide which lifecycle model you want for your next UI (dynamic instances like UserDemo vs persistent abilities like ADV).
2. Standardize input flow:
   - Either prefer signals (push) end-to-end, or
   - define a clear polling contract (buffer events, don’t clear too aggressively).
3. Choose one animation core and use it consistently:
   - LVGL-path interpolation (simple, integer, manual), or
   - `smooth_ui_toolkit::Animate` (spring/easing, retargeting, callbacks).
4. Keep the UI loop responsive:
   - avoid long work inside lifecycle callbacks (`onRunning` / `onForeground`),
   - time-slice heavy operations or move them to background tasks with clear handoff boundaries.

## Open Questions

- Should “latest key event” buffers in ADV be cleared unconditionally at the start of every `Keyboard::update()`, or should events be buffered (ring buffer) so widget polling can’t miss them?
- In UserDemo, is the launcher’s `selected_item++` indexing convention documented anywhere, and is it robust to future changes in app installation order?
- Should ADV’s app close semantics ever destroy abilities to reclaim memory (especially for heavy apps), or is “restart to clear” the intended design?
- If we unify keyboard handling, do we want the runtime probe order to prefer `TCA8418` (I2C expander) over GPIO scan always, or should it be configurable (Kconfig) to avoid mis-detection in unusual hardware variants?

## References

- Diary for this investigation: `esp32-s3-m5/ttmp/2026/01/24/G10-ANALYZE-DEMO-UI--analyze-m5cardputer-demo-ui-userdemo-adv/reference/01-diary.md`
- UserDemo entry + framework:
  - `M5Cardputer-UserDemo/main/cardputer.cpp`
  - `M5Cardputer-UserDemo/components/mooncake/src/mooncake.cpp`
  - `M5Cardputer-UserDemo/components/mooncake/src/app/app_manager.cpp`
- UserDemo widget substrate:
  - `M5Cardputer-UserDemo/main/apps/utils/smooth_menu/src/selector/selector.cpp`
  - `M5Cardputer-UserDemo/main/apps/utils/smooth_menu/src/lv_anim/lv_anim.cpp`
- ADV entry + framework:
  - `M5Cardputer-UserDemo-ADV/main/main.cpp`
  - `M5Cardputer-UserDemo-ADV/components/mooncake/src/ability_manager/ability_manager.cpp`
  - `M5Cardputer-UserDemo-ADV/components/mooncake/src/ability/ability.h`
- ADV widget/animation substrate:
  - `M5Cardputer-UserDemo-ADV/components/smooth_ui_toolkit/src/widget/select_menu/smooth_selector.cpp`
  - `M5Cardputer-UserDemo-ADV/components/smooth_ui_toolkit/src/animation/animate/animate.cpp`
