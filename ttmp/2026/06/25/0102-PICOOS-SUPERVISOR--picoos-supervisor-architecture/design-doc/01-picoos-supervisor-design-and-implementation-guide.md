---
Title: PicoOS Supervisor Design and Implementation Guide
Ticket: 0102-PICOOS-SUPERVISOR
Status: active
Topics:
    - esp32-p4
    - picojs
    - picoos
    - quickjs
    - firmware
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp
      Note: Current firmware entrypoint
    - Path: components/picocalc_keyboard/include/picocalc_keyboard.h
      Note: Physical keyboard event API for supervisor input routing
    - Path: components/picocalc_lcd/include/picocalc_lcd.h
      Note: LCD geometry and primitive display API
    - Path: components/picojs_runtime/include/picojs_runtime.h
      Note: PicoJS runtime public API that the supervisor will call
    - Path: components/picojs_runtime/picojs_runtime.cpp
      Note: Native PicoJS DSL
    - Path: components/qjs_service/include/qjs_service.h
      Note: Serialized QuickJS eval/job API and ownership boundary
    - Path: components/qjs_service/qjs_service.cpp
      Note: QuickJS service task
    - Path: components/visual_repl/include/visual_repl.h
      Note: 40x20 text display backend and dump-frame rendering API
    - Path: ttmp/2026/06/25/0102-PICOJS-DEVICE-INTEGRATION--0102-picojs-device-integration/analysis/01-picoos-devkit-app-parity-assessment.md
      Note: Prior parity analysis that motivates supervisor-before-more-widgets planning
    - Path: ttmp/2026/06/25/0102-PICOJS-DEVICE-INTEGRATION--0102-picojs-device-integration/sources/picoos-devkit.jsx
      Note: Imported picoOS devkit source compatibility reference
ExternalSources: []
Summary: Design for turning the console-driven PicoJS runtime into a daily-use PicoOS supervisor with launcher, live scheduler, app switching, and REPL integration.
LastUpdated: 2026-06-26T00:00:00Z
WhatFor: Use this to onboard an intern to the current ESP32-P4 PicoCalc PicoJS firmware and guide the next supervisor implementation phase.
WhenToUse: Read before implementing live apps, launcher/app switching, global input routing, or REPL-as-system-app behavior.
---


# PicoOS Supervisor Design and Implementation Guide

## Executive summary

The current firmware is a strong prototype for a PicoCalc JavaScript UI runtime, but it is still shaped like a **console-driven single-app REPL**. The user can load a built-in PicoJS app over UART, manually advance frames with `picojs frame` or `picojs run`, inject keys with `picojs key`, and dump the rendered text framebuffer for validation. That is excellent for development, but it is not yet an everyday operating environment.

This document designs the next layer: a **PicoOS supervisor**. The supervisor is a native firmware service that sits above `picojs_runtime` and below user-facing apps. It owns app lifecycle, the launcher, active app selection, live frame ticking, global input routing, crash containment, and REPL escape. The goal is that the PicoCalc boots into a launcher, can run Snake live without serial commands, can switch between apps, can drop into the JavaScript REPL, and can report app state over the UART console.

The recommended implementation is intentionally incremental. Do not begin by building a general multi-process OS. Start with one QuickJS service task and one QuickJS context, then support multiple **cooperative app records** inside the native supervisor. This keeps memory usage and concurrency manageable on ESP32-P4 while preserving a path to stronger isolation later.

## What exists today

### Repository and target

The main firmware worktree is:

```text
/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5
```

The target project for this work is:

```text
0102-esp32-p4-visual-quickjs-repl/
```

The target board is the ESP32-P4 PicoCalc path. Build and flash with ESP-IDF 5.4.2:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0102-esp32-p4-visual-quickjs-repl
source ~/esp/esp-idf-5.4.2/export.sh
idf.py build
idf.py -p /dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00 flash
```

### Current boot shape

`app_main.cpp` initializes the LCD, visual REPL, keyboard task, QuickJS service, and PicoJS runtime. The current boot screen is explicitly the visual QuickJS REPL, not a launcher. Evidence:

- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:1098-1109` initializes the LCD and renders the initial text `ESP32-P4 VISUAL QUICKJS REPL`.
- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:1112-1118` initializes the keyboard and starts `keyboard_task`.
- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:1120-1128` starts the QuickJS service and creates `g_picojs`.

Current boot flow:

```text
app_main()
  ├─ picocalc_lcd_init()
  ├─ visual_repl_init()
  ├─ render "ESP32-P4 VISUAL QUICKJS REPL"
  ├─ picocalc_keyboard_init()
  ├─ xTaskCreate(keyboard_task)
  ├─ start_quickjs_service()
  └─ picojs_runtime_create()
```

The proposed supervisor changes the last part of boot:

```text
app_main()
  ├─ initialize hardware and QuickJS services
  ├─ picojs_runtime_create()
  ├─ picoos_supervisor_create()
  ├─ picoos_register_builtin_apps()
  └─ picoos_boot_to_launcher()
```

### Display geometry and renderer

The display path is text-cell based today. `visual_repl.h` defines a 40-column by 20-row display using 8x16 cells:

- `components/visual_repl/include/visual_repl.h:13-18` defines `VISUAL_REPL_COLS 40`, `VISUAL_REPL_ROWS 20`, `VISUAL_REPL_CELL_W 8`, and `VISUAL_REPL_CELL_H 16`.
- `components/visual_repl/include/visual_repl.h:40-49` exposes `visual_repl_render()`, `visual_repl_render_input()`, `visual_repl_render_dump_frame()`, and `visual_repl_dump_text()`.

The current PicoJS LCD render path converts the PicoJS text dump into a full display frame:

- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:412-418` calls `picojs_runtime_dump_text()` and then `visual_repl_render_dump_frame()`.

This means the first supervisor should use the existing 40x20 framebuffer. Do not introduce a pixel compositor yet. A text compositor is enough for launcher, app switching, REPL, Snake, Sysmon, and settings.

### QuickJS ownership and threading

QuickJS must be treated as owned by `qjs_service`. The service creates the QuickJS runtime and context on its task and serializes evaluation/jobs through a FreeRTOS queue:

- `components/qjs_service/include/qjs_service.h:60-66` defines `qjs_job_t`, the native job callback shape that receives `JSContext *ctx`.
- `components/qjs_service/include/qjs_service.h:71-80` exposes `qjs_service_eval()`, `qjs_service_run()`, `qjs_service_post()`, and `qjs_service_reset()`.
- `components/qjs_service/qjs_service.cpp:65-79` stores `JSRuntime *rt`, `JSContext *ctx`, queue, and task state in `Service`.
- `components/qjs_service/qjs_service.cpp:348-358` executes native jobs on the service task by calling `p->fn(s->ctx, p->user)`.
- `components/qjs_service/qjs_service.cpp:369-420` creates the queue and FreeRTOS task.

This is the most important concurrency rule for the intern:

> Any code that touches QuickJS values, QuickJS callbacks, or `JSContext *` must run through `qjs_service_run()` or `qjs_service_post()`.

Do not call `picojs_runtime_frame_js()` or `picojs_runtime_key_js()` directly from arbitrary FreeRTOS tasks. Current code obeys this by wrapping frame and key actions in jobs:

- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:361-386` wraps `picojs_runtime_frame_js()` in `run_picojs_frame()` via `qjs_service_run()`.
- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:389-399` wraps `picojs_runtime_key_js()` in `send_picojs_key_token()` via `qjs_service_run()`.

### PicoJS runtime today

`picojs_runtime` is the native runtime boundary for the JavaScript UI DSL. Its public API is in `components/picojs_runtime/include/picojs_runtime.h`:

- `components/picojs_runtime/include/picojs_runtime.h:21-30` defines default geometry and runtime config.
- `components/picojs_runtime/include/picojs_runtime.h:32-43` defines runtime status: initialized, JS installed, app mode, frame count, app counts, last frame, and errors.
- `components/picojs_runtime/include/picojs_runtime.h:45-56` exposes create/destroy/install/reset/status/frame/key/app-mode/dump calls.

Internally, the runtime currently has one active `App` in `rt->app`. It supports panels, text, gauges, generic widgets, timers, loops, computes, and keys:

- `components/picojs_runtime/picojs_runtime.cpp:126-146` defines `GenericWidget` for spark/table/menu/list/grid-like widgets.
- `components/picojs_runtime/picojs_runtime.cpp:148-158` defines `Panel` with title, title-right, footer, text, gauges, and widgets.
- `components/picojs_runtime/picojs_runtime.cpp:160-188` defines callbacks and `App` state.
- `components/picojs_runtime/picojs_runtime.cpp:196-205` begins the `picojs_runtime` state object.

The renderer walks the active app and writes into the runtime screen buffer:

- `components/picojs_runtime/picojs_runtime.cpp:536-588` recomputes layout, draws panels/widgets, and writes the status bar.
- `components/picojs_runtime/picojs_runtime.cpp:1442-1449` advances a JS-aware frame by running callbacks and then rendering.
- `components/picojs_runtime/picojs_runtime.cpp:1461-1482` sends a key token to the app's registered key callbacks and renders afterward.

The current DSL surface includes:

- `OS.app(name)` from `components/picojs_runtime/picojs_runtime.cpp:1391-1398`.
- App methods from `components/picojs_runtime/picojs_runtime.cpp:1124-1140`: `state`, `layout`, `panel`, `statusbar`, `mount`, `on`, `loop`, `compute`, `key`, `refresh`, `dispatch`, `exit`.
- Panel methods from `components/picojs_runtime/picojs_runtime.cpp:1143-1158`: `frame`, `title`, `titleRight`, `footer`, `text`, `gauge`, `spark`, `table`, `menu`, `list`, `grid`.
- Text and gauge methods from `components/picojs_runtime/picojs_runtime.cpp:1161-1183`.
- Generic widget methods from `components/picojs_runtime/picojs_runtime.cpp:1186-1211`.
- OS compatibility properties/methods from `components/picojs_runtime/picojs_runtime.cpp:1315-1343`.

### Built-in demo apps today

The built-in app sources live as C++ string constants in `app_main.cpp`:

- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:29-35` defines `hello`.
- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:37-47` defines `dashboard`.
- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:49-70` defines `interactive`.
- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:72-82` defines `home`.
- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:84-94` defines `sysmon`.
- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:96-108` defines `snake`.

The `snake` source already declares `game.loop(4, ...)`, but the loop only runs when native code sends frame time into `picojs_runtime_frame_js()`. There is no autonomous frame pump yet. This is why Snake is currently controlled by serial `picojs run` / `picojs frame`, not live gameplay.

### Console commands today

`picojs` console commands live in `cmd_picojs()`:

- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:842-853` prints runtime status.
- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:860-902` loads one built-in app by evaluating the selected source string.
- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:904-913` dumps the current PicoJS framebuffer.
- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:915-932` advances one frame.
- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:934-957` advances several frames.
- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:959-970` injects a key token.
- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:977-986` switches between app input mode and REPL input mode.

These commands should remain during the supervisor phase, but several should become wrappers around supervisor actions rather than direct runtime manipulation.

### Keyboard routing today

`keyboard_task()` has a binary mode split:

- In app mode, keys go to `send_picojs_key_token()` except Escape, which returns to REPL mode.
- In REPL mode, keys edit the text input line.

Evidence:

- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:421-445` maps keyboard bytes to semantic PicoJS tokens such as `left`, `up`, `down`, `right`, `enter`, `home`, `delete`, and printable ASCII.
- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:494-548` polls the keyboard, handles recovery, and routes keys based on `picojs_runtime_app_mode(g_picojs)`.

The supervisor should replace the binary app/repl split with a global input router:

```text
physical key event
  ├─ global hotkey?          -> supervisor action
  ├─ active surface is REPL? -> REPL editor
  └─ active surface is app?  -> foreground app key callback
```

## Problem statement and scope

### User-facing problem

The user wants the PicoCalc to feel like a small everyday computer, not a test harness. That means:

- Boot to a useful launcher.
- Start apps without serial commands.
- Run apps continuously when appropriate.
- Switch visible apps without losing their state.
- Drop to a REPL for exploration and debugging.
- Recover from app errors without rebooting the device.
- Keep enough console observability to debug the system headlessly.

### Engineering problem

The firmware currently has the ingredients, but not the OS layer:

- There is a QuickJS service task, but no app scheduler above it.
- There is a PicoJS runtime, but it owns one active app, not a set of app instances.
- There are built-in app sources, but no registry or lifecycle state.
- There is a keyboard task, but no global hotkey/input policy.
- There is a display renderer, but no display manager that knows about foreground apps, launcher, REPL, and app switcher.
- There are console commands, but they directly manipulate the runtime instead of controlling an OS supervisor.

### In scope for this ticket

This ticket should design and guide implementation of:

1. A native `picoos_core` component.
2. App registry for built-in apps.
3. App lifecycle state machine.
4. Live frame pump driven by firmware time.
5. Foreground/background app switching.
6. Global input routing.
7. Launcher as a system surface.
8. REPL as a first-class system surface.
9. Console commands for inspection and control.
10. Hardware validation probes.

### Out of scope for the first implementation slice

Do **not** implement these in the first supervisor slice unless the basics are already stable:

- One QuickJS context per app.
- Preemptive JavaScript execution.
- Pixel-level compositor or overlapping windows.
- Network stack integration.
- SD-card app packages.
- Security sandbox for untrusted third-party code.
- Full compatibility with every app in `picoos-devkit.jsx`.

## Target mental model for the intern

Think of PicoOS as a small cooperative kernel. It does not preempt JavaScript in the way a desktop OS preempts processes. Instead, it owns a list of app records and explicitly calls app callbacks on the QuickJS service task.

```text
+--------------------------------------------------+
| User-visible PicoOS                              |
|                                                  |
|  Launcher  App switcher  REPL  Settings  Apps    |
+--------------------------+-----------------------+
                           |
+--------------------------v-----------------------+
| picoos_core supervisor                           |
|                                                  |
|  App registry       Lifecycle state machine      |
|  Active app id      Frame pump                   |
|  Input router       Display manager             |
|  Crash accounting   Console control API          |
+--------------------------+-----------------------+
                           |
+--------------------------v-----------------------+
| picojs_runtime                                    |
|                                                  |
|  OS object bindings  App/Panel/Widget objects    |
|  callbacks           40x20 text framebuffer      |
+--------------------------+-----------------------+
                           |
+--------------------------v-----------------------+
| qjs_service                                      |
|                                                  |
|  One QuickJS Runtime/Context on service task      |
|  Queue of eval/job/status/reset messages         |
+--------------------------+-----------------------+
                           |
+--------------------------v-----------------------+
| Hardware/services                                |
| LCD, keyboard, UART console, timers, storage     |
+--------------------------------------------------+
```

The supervisor should be native C/C++ because it controls tasks, timers, and hardware. PicoJS apps should remain JavaScript because the whole point is a quick on-device app DSL.

## Proposed architecture

### New component: `components/picoos_core`

Create a component:

```text
components/picoos_core/
  CMakeLists.txt
  include/picoos_core.h
  picoos_core.cpp
```

Initial dependencies:

```text
picoos_core
  ├─ picojs_runtime
  ├─ qjs_service
  ├─ visual_repl
  ├─ esp_timer or FreeRTOS task/timer APIs
  └─ esp_err/log
```

The component owns OS-level state, but it does not own the low-level hardware drivers directly. It receives handles or callback hooks from `app_main.cpp`.

### Primary native types

The first version can use fixed-size arrays to avoid heap surprises, or `std::vector` if the existing C++ style remains acceptable. Since `picojs_runtime.cpp` already uses `std::vector` and `std::unique_ptr`, either is consistent. For firmware predictability, start with a capped registry.

```cpp
#define PICOOS_MAX_APPS 12
#define PICOOS_APP_ID_MAX 24
#define PICOOS_TITLE_MAX 40

typedef enum {
    PICOOS_APP_STOPPED = 0,
    PICOOS_APP_STARTING,
    PICOOS_APP_RUNNING,
    PICOOS_APP_FOREGROUND,
    PICOOS_APP_BACKGROUND,
    PICOOS_APP_PAUSED,
    PICOOS_APP_CRASHED,
} picoos_app_state_t;

typedef enum {
    PICOOS_SURFACE_LAUNCHER = 0,
    PICOOS_SURFACE_APP,
    PICOOS_SURFACE_REPL,
    PICOOS_SURFACE_SWITCHER,
    PICOOS_SURFACE_CRASH,
} picoos_surface_t;

typedef struct {
    char id[PICOOS_APP_ID_MAX];
    char title[PICOOS_TITLE_MAX];
    const char *source;
    const char *filename;
    bool system;
    bool autostart;
    bool allow_background_ticks;
    uint32_t preferred_fps;
} picoos_app_descriptor_t;

typedef struct {
    char id[PICOOS_APP_ID_MAX];
    picoos_app_state_t state;
    bool loaded;
    bool dirty;
    bool visible;
    uint32_t frame_count;
    uint32_t error_count;
    uint32_t last_frame_ms;
    uint32_t last_tick_us;
} picoos_app_status_t;
```

The descriptor describes installable apps. The status describes running instances.

### Supervisor object

```cpp
typedef struct picoos_supervisor picoos_supervisor_t;

typedef struct {
    qjs_service_t *qjs;
    picojs_runtime_t *runtime;
    uint16_t cols;
    uint16_t rows;
    uint32_t default_fps;
    esp_err_t (*render_active)(void *user);
    esp_err_t (*render_repl)(void *user);
    void *render_user;
} picoos_supervisor_config_t;
```

The supervisor should not call `visual_repl_render_dump_frame()` directly at first. Instead, accept a render callback. In the current project, `app_main.cpp` can pass the existing `render_picojs_to_lcd()` wrapper.

### Public C API sketch

```cpp
esp_err_t picoos_supervisor_create(const picoos_supervisor_config_t *cfg,
                                   picoos_supervisor_t **out);
void picoos_supervisor_destroy(picoos_supervisor_t *os);

esp_err_t picoos_register_app(picoos_supervisor_t *os,
                              const picoos_app_descriptor_t *desc);
esp_err_t picoos_boot(picoos_supervisor_t *os);

esp_err_t picoos_start(picoos_supervisor_t *os);
esp_err_t picoos_stop(picoos_supervisor_t *os);
bool picoos_is_running(picoos_supervisor_t *os);

esp_err_t picoos_launch(picoos_supervisor_t *os, const char *app_id);
esp_err_t picoos_switch_to(picoos_supervisor_t *os, const char *app_id);
esp_err_t picoos_close(picoos_supervisor_t *os, const char *app_id);
esp_err_t picoos_show_launcher(picoos_supervisor_t *os);
esp_err_t picoos_show_repl(picoos_supervisor_t *os);

esp_err_t picoos_frame(picoos_supervisor_t *os, uint32_t dt_ms);
esp_err_t picoos_key(picoos_supervisor_t *os, const char *token);

esp_err_t picoos_get_status(picoos_supervisor_t *os,
                            picoos_status_t *out);
esp_err_t picoos_list_apps(picoos_supervisor_t *os,
                           picoos_app_status_t *out,
                           size_t cap,
                           size_t *count);
```

The important implementation rule is that `picoos_frame()` and `picoos_key()` submit QuickJS jobs when they need to execute JavaScript. The API can be called from the keyboard task, console task, or supervisor timer task without those callers knowing QuickJS details.

### Supervisor state model

First version:

```text
PicoOS supervisor
  surface = LAUNCHER | APP | REPL | SWITCHER | CRASH
  running = true | false
  active_app = "home" | "snake" | ... | none
  app_table[]
    descriptor
    state
    loaded flag
    error count
```

State transitions:

```text
          launch(app)
STOPPED  -----------> STARTING
                         |
                         | eval app source OK
                         v
                     BACKGROUND
                         |
                         | switch_to(app)
                         v
                     FOREGROUND
                         |
                         | switch away
                         v
                     BACKGROUND
                         |
                         | close(app)
                         v
                      STOPPED

Any JS callback/eval failure above threshold:
  RUNNING/FOREGROUND/BACKGROUND -> CRASHED
```

Text diagram:

```text
+---------+   launch    +----------+   eval OK   +------------+
| STOPPED | ----------> | STARTING | ----------> | BACKGROUND |
+---------+             +----------+             +------------+
     ^                                                 |
     | close                                           | switch_to
     |                                                 v
+---------+   crash     +---------+   switch away +------------+
| CRASHED | <---------  | RUNNING | <-----------  | FOREGROUND |
+---------+             +---------+               +------------+
```

Use explicit state names in console output so failures are diagnosable.

## JavaScript API design

### Preserve current PicoJS DSL

Existing apps should continue to work:

```js
var app = OS.app('snake');
var st = app.state({ score: 0, x: 4, y: 3 });
var board = app.panel('board').frame('single').title(' snake ');
board.grid().at(1, 1).size(19, 9).cell('. ');
app.loop(4, function () { st.x = (st.x + 1) % 19; });
app.key('left', function () { st.x = Math.max(0, st.x - 1); });
app.mount();
```

### Add OS-level app APIs

The launcher and REPL need OS APIs that talk to the supervisor instead of acting as no-ops.

```js
OS.apps();              // returns descriptors/status for launcher
OS.running();           // returns running app summaries
OS.launch('snake');     // start app if needed, switch foreground
OS.switchTo('sysmon');  // switch active visible app
OS.close('snake');      // stop app and release its JS/native app state
OS.repl();              // show REPL surface
OS.home();              // show launcher surface
OS.ps();                // process/app table for sysmon
```

Return shapes should be small and display-oriented:

```js
OS.apps() => [
  { id: 'home', title: 'Home', system: true, state: 'foreground' },
  { id: 'snake', title: 'Snake', system: false, state: 'background' }
]
```

### Add lifecycle events

Current `App.on('tick', ms, fn)` exists. Extend event names gradually:

```js
app.on('start', function (app) {});
app.on('resume', function (app) {});
app.on('pause', function (app) {});
app.on('stop', function (app) {});
app.on('error', function (app, error) {});
app.on('tick', 1000, function (app) {});
```

Internally, the runtime already stores timer callbacks. The supervisor adds lifecycle calls around load/switch/close.

### Add global key conventions

Reserve global keys before app dispatch:

| Token | Supervisor behavior |
|---|---|
| `home` | Show launcher |
| `escape` | Back: app -> launcher, launcher -> REPL or previous surface |
| `ctrl+r` or configured token | Show REPL |
| `tab` | Show app switcher |
| `ctrl+q` | Close foreground app |

Current keyboard mapping handles arrows, enter, home/delete/end, and printable ASCII. The intern should extend `key_to_picojs_token()` or add a new supervisor-level mapper so special combinations can be represented as tokens.

## Frame pump design

### Why this is needed

`snake` declares `game.loop(4, ...)` in its app source, but the app only moves when native code calls `picojs_runtime_frame_js(ctx, rt, dt_ms)`. The serial console command does that today:

- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:915-932` implements `picojs frame`.
- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:934-957` implements `picojs run`.

The supervisor must call frames automatically when the OS is running.

### Recommended implementation: FreeRTOS task, not timer callback executing JS

Do not run JavaScript directly inside an ESP timer callback. Use a small FreeRTOS task or timer callback that posts work to a task. Simpler first implementation:

```cpp
void picoos_frame_task(void *arg) {
    auto *os = static_cast<picoos_supervisor_t *>(arg);
    int64_t last_us = esp_timer_get_time();

    while (!os->stop_requested) {
        vTaskDelay(pdMS_TO_TICKS(os->frame_period_ms));

        int64_t now_us = esp_timer_get_time();
        uint32_t dt_ms = (uint32_t)((now_us - last_us) / 1000);
        last_us = now_us;

        if (!os->running) continue;
        picoos_frame(os, dt_ms);
    }
}
```

`picoos_frame()` then submits a QuickJS job:

```cpp
static esp_err_t picoos_frame_job(JSContext *ctx, void *user) {
    auto *os = static_cast<picoos_supervisor_t *>(user);

    PicoAppInstance *active = find_active_app(os);
    if (!active) return ESP_OK;

    esp_err_t err = picojs_runtime_frame_js(ctx, os->runtime, os->last_dt_ms);
    if (err == ESP_OK) {
        active->frame_count++;
        active->dirty = true;
    } else {
        active->error_count++;
        maybe_mark_crashed(active);
    }
    return err;
}

esp_err_t picoos_frame(picoos_supervisor_t *os, uint32_t dt_ms) {
    os->last_dt_ms = dt_ms;
    qjs_job_t job = {};
    job.fn = picoos_frame_job;
    job.user = os;
    job.timeout_ms = os->job_timeout_ms;
    esp_err_t err = qjs_service_run(os->qjs, &job);
    if (err == ESP_OK && os->render_active) {
        return os->render_active(os->render_user);
    }
    return err;
}
```

A future optimization can use `qjs_service_post()` to avoid blocking the frame task. Start with `run()` because it makes error handling and rendering order easier to reason about.

### Frame policy

First version policy:

- Only the foreground app receives high-frequency loop frames.
- Background apps receive no loop frames by default.
- System apps such as clock/sysmon may receive low-frequency ticks later.
- If a frame job fails repeatedly, mark the app crashed and show a crash surface.

Why this policy: it keeps CPU/heap use predictable and makes the display behavior clear.

## App switching design

### Current limitation

`picojs_runtime` currently has one active `rt->app`. Loading a new app overwrites the current app. That is acceptable for the first prototype but not for daily use.

### Minimal viable app switching

Do not implement full multiple app state in `picojs_runtime` first. The fastest path is a native supervisor that stores app descriptors and can reload apps by ID while preserving a small native status table. This gives a launcher and live frame pump quickly, but switching away from Snake loses JavaScript heap state.

However, the user explicitly wants "potentially multiple apps running at the same time and can be switched on the display". To support stateful switching, use a better first design:

1. Change `picojs_runtime` from one active `std::unique_ptr<App> app` to a vector/map of apps.
2. Keep a pointer/index to the active mounted app.
3. `OS.app(name)` returns an existing app if one exists, otherwise creates it.
4. `App.mount()` marks the app as mounted and sets it active if there is no active app.
5. `picojs_runtime_set_active_app(rt, id)` changes which app the renderer uses.
6. `picojs_runtime_frame_js()` runs callbacks for the active app first; background callbacks can be added later.

Proposed runtime additions:

```cpp
esp_err_t picojs_runtime_set_active_app(picojs_runtime_t *rt, const char *app_id);
const char *picojs_runtime_active_app(picojs_runtime_t *rt);
esp_err_t picojs_runtime_close_app_js(JSContext *ctx, picojs_runtime_t *rt, const char *app_id);
esp_err_t picojs_runtime_list_apps(picojs_runtime_t *rt, picojs_runtime_app_info_t *out, size_t cap, size_t *count);
```

Internal shape:

```cpp
struct picojs_runtime {
    bool initialized;
    std::vector<std::unique_ptr<App>> apps;
    App *active_app;
    // existing screen, frame_count, error_count, etc.
};
```

Migration pattern:

```cpp
// Current style
if (!rt || !rt->app || !rt->app->mounted) render_banner(rt);

// Supervisor-ready style
App *app = rt->active_app;
if (!rt || !app || !app->mounted) render_banner(rt);
```

This is a larger change than the pure supervisor wrapper, but it is the right step if stateful switching matters.

## Launcher design

### Launcher as a system app

The launcher should be a PicoJS app because this dogfoods the runtime and keeps the UI editable. It should be registered as a system app, autostarted, and protected from accidental close.

Example launcher source:

```js
var home = OS.app('home');
var st = home.state({ selected: 0 });

home.layout(function (l) {
  l.row(1, 'bar').row('*', 'body');
});

home.panel('bar')
  .frame('single')
  .title(' PicoOS ')
  .titleRight(function () { return OS.clock('HH:mm'); });

var body = home.panel('body').frame('single').title(' apps ');
var menu = body.menu()
  .frame('single')
  .title(' launcher ')
  .at(2, 2)
  .grid(3)
  .items(function () { return OS.apps().map(function (a) { return a.id; }); })
  .marker('>')
  .onPick(function (id) { OS.launch(id); });

home.key('left', function () { home.dispatch('menu.left'); });
home.key('right', function () { home.dispatch('menu.right'); });
home.key('up', function () { home.dispatch('menu.up'); });
home.key('down', function () { home.dispatch('menu.down'); });
home.key('enter', function () { home.dispatch('menu.pick'); });
home.key('r', function () { OS.repl(); });

home.statusbar('arrows select | enter launch | r repl');
home.mount();
```

The current menu widget has `onPick()` as a no-op compatibility method. The supervisor phase should decide whether selection/focus belongs in widgets or the app. For daily use, implement focus in the runtime so menus and lists can be navigated consistently.

### Launcher acceptance behavior

On boot:

```text
[00]   PicoOS                         12:00
[01] +- apps ------------------------------+
[02] | > REPL     Snake    Sysmon          |
[03] |   Calc     Notes    Settings        |
...
[19] arrows select | enter launch | r repl
```

Serial validation should be able to run:

```bash
picoos status
picoos apps
picoos launcher
picojs dump
```

Expected dump contains `PicoOS`, `REPL`, `Snake`, and `Sysmon`.

## REPL integration design

### Current REPL

The current REPL is the default display/input mode. It uses `visual_repl` history and input rendering. JavaScript evaluation goes through `qjs_service_eval()`. Keyboard input goes to `handle_editor_key()` when not in PicoJS app mode.

### Target behavior

The REPL should become a system surface:

```text
launcher -> REPL
app -> global REPL hotkey -> REPL
REPL -> command or hotkey -> return to previous app/launcher
```

Initial implementation can keep the existing `visual_repl` editor code in `app_main.cpp`, but route mode changes through the supervisor:

```cpp
picoos_show_repl(os) {
    os->surface = PICOOS_SURFACE_REPL;
    os->previous_app = os->active_app;
    visual_repl_render();
}

picoos_return_from_repl(os) {
    if (os->previous_app) picoos_switch_to(os, os->previous_app);
    else picoos_show_launcher(os);
}
```

Later, a JS-facing REPL app could render a richer prompt inside PicoJS. Do not do that first; reuse the working REPL.

## Input routing design

### Current keys

Current key token mapping:

```cpp
0xb4 -> left
0xb5 -> up
0xb6 -> down
0xb7 -> right
0x0a/0x0d -> enter
0xd2 -> home
0xd4 -> delete
0xd5 -> end
printable ASCII -> one-character token
```

This lives at `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:421-445`.

### Supervisor router

Replace direct `if (picojs_runtime_app_mode(...))` logic with:

```cpp
void handle_key_event(picocalc_key_event_t ev) {
    char token[16];
    if (!key_to_picoos_token(ev.key, ev.mods, token, sizeof(token))) return;

    picoos_key_result_t result = {};
    esp_err_t err = picoos_handle_key(g_picoos_os, token, &result);

    if (err != ESP_OK) log;
    if (result.render_repl) visual_repl_render();
    if (result.render_app) render_picojs_to_lcd();
}
```

Inside supervisor:

```cpp
picoos_handle_key(os, token):
  if token is global:
    execute global action
    return render request

  switch os.surface:
    case REPL:
      return PICOOS_KEY_TO_REPL_EDITOR
    case LAUNCHER:
    case APP:
      submit qjs key job to active app
      return render app
```

Practical shortcut: Keep `handle_editor_key()` in `app_main.cpp`. If `picoos_handle_key()` returns `PICOOS_KEY_TO_REPL_EDITOR`, call the existing function.

## Console command design

Keep existing `picojs` commands for low-level runtime debugging, but add `picoos` commands for OS behavior.

Proposed commands:

```bash
picoos status
picoos start [fps]
picoos stop
picoos apps
picoos ps
picoos launch <id>
picoos switch <id>
picoos close <id>
picoos launcher
picoos repl
picoos key <token>
picoos frame [dt_ms]
```

Command responsibilities:

| Command | Purpose |
|---|---|
| `picoos status` | Supervisor status: running, surface, active app, fps, frame count, errors. |
| `picoos start [fps]` | Start live frame pump. |
| `picoos stop` | Stop live frame pump. |
| `picoos apps` | Registered app descriptors. |
| `picoos ps` | Running app instances and states. |
| `picoos launch <id>` | Start/switch to app. |
| `picoos switch <id>` | Switch display to running app. |
| `picoos close <id>` | Stop app unless protected system app. |
| `picoos launcher` | Show launcher. |
| `picoos repl` | Show REPL surface. |
| `picoos key <token>` | Inject key through supervisor router. |
| `picoos frame [dt_ms]` | Manual frame for deterministic tests. |

The old `picojs frame/run/key/mode` commands should remain until the supervisor is stable, but tests should move to `picoos` commands.

## Error handling and crash containment

QuickJS exceptions can occur during app load, frame callbacks, key callbacks, and OS API calls. The current runtime increments `last_error_count` for callback exceptions but does not isolate app failure deeply:

- `components/picojs_runtime/picojs_runtime.cpp:1472-1474` increments `last_error_count` when a key callback returns an exception.

The supervisor should track errors per app:

```cpp
if (job_result != ESP_OK || js_exception_detected) {
    app->error_count++;
    app->last_error = captured_error;
    if (app->error_count >= app->error_limit) {
        app->state = PICOOS_APP_CRASHED;
        if (os->active_app == app) os->surface = PICOOS_SURFACE_CRASH;
    }
}
```

Crash screen:

```text
[00] +- app crashed: snake -----------------+
[01] | Error: boom                          |
[02] |                                      |
[03] | enter: restart   home: launcher      |
[19] PicoOS protected REPL still available  |
```

Do not reset the entire QuickJS service for every app exception in the first implementation. A full reset would kill all apps and should be an explicit recovery action.

## Persistence and daily-use requirements

The first supervisor can run without persistence, but daily use eventually needs storage.

Prioritized storage plan:

1. **NVS settings**: brightness, theme, boot target, frame pump enabled, default app.
2. **REPL history**: small ring buffer in NVS or file storage.
3. **App user data**: notes, calculator history, game scores.
4. **App packages**: load JavaScript sources from LittleFS/SD.

Do not block the supervisor implementation on storage. Add interfaces that can be backed by stubs first:

```js
OS.settings.get('theme')
OS.settings.set('theme', 'amber')
OS.files.read('/notes/today.txt')
OS.files.write('/notes/today.txt', text)
```

## Implementation phases

### Phase 0: keep the current system working

Goal: no regression. Before changing behavior, verify:

```bash
idf.py build
idf.py -p /dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00 flash
# run existing probes as appropriate
```

Must still work:

- `picojs load home`
- `picojs load sysmon`
- `picojs load snake`
- `picojs dump`
- `picojs run 2 250`
- `picojs key left`

### Phase 1: add `picoos_core` skeleton

Files:

```text
components/picoos_core/CMakeLists.txt
components/picoos_core/include/picoos_core.h
components/picoos_core/picoos_core.cpp
0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp
```

Tasks:

1. Create supervisor config and handle.
2. Register built-in app descriptors for `home`, `sysmon`, `snake`, `repl`.
3. Add `picoos status` and `picoos apps` commands.
4. Keep boot behavior unchanged initially.

Acceptance:

```bash
picoos status
picoos apps
```

Expected: lists registered apps without changing display behavior.

### Phase 2: move built-in loading behind supervisor

Tasks:

1. Add `picoos launch <id>`.
2. Implement launch by evaluating app descriptor source through `qjs_service_eval()` or an equivalent QuickJS job.
3. Render active app after launch.
4. Add `picoos launcher` and `picoos repl`.

Acceptance:

```bash
picoos launch home
picojs dump
picoos launch snake
picojs dump
picoos repl
```

Expected: same visual results as current `picojs load`, but controlled by supervisor state.

### Phase 3: live frame pump

Tasks:

1. Add frame task to `picoos_core`.
2. Add `picoos start [fps]` and `picoos stop`.
3. Make `picoos start 4` advance Snake without serial `picojs run`.
4. Render after frames, but throttle LCD redraw if needed.

Acceptance:

```bash
picoos launch snake
picoos start 4
# wait 2 seconds
picojs dump
picoos stop
```

Expected: Snake head position changes over time without manual frame commands.

### Phase 4: global input router

Tasks:

1. Add `picoos_key()` and route keyboard task through supervisor.
2. Reserve `home`, `escape`, and REPL hotkey behavior.
3. Send unreserved keys to foreground app.
4. Keep console key injection through `picoos key <token>`.

Acceptance:

```bash
picoos launch snake
picoos key left
picojs dump
picoos key home
picojs dump
picoos repl
```

Physical acceptance:

1. Boot/launch Snake.
2. Arrow keys steer Snake.
3. Home returns to launcher.
4. REPL remains reachable.

### Phase 5: runtime multi-app support

Tasks:

1. Convert `picojs_runtime` from one `rt->app` to app table + active app pointer.
2. Add runtime API for active app selection and app listing.
3. Ensure `OS.app(name)` reuses existing app records.
4. Update render/frame/key functions to operate on active app.
5. Add supervisor `picoos switch <id>`.

Acceptance:

```bash
picoos launch snake
picoos start 4
picoos launch sysmon
picoos switch snake
picojs dump
```

Expected: Snake's state is preserved across switching.

### Phase 6: REPL as system surface

Tasks:

1. Make `picoos repl` switch display/input to existing visual REPL.
2. Add return command/hotkey to previous app or launcher.
3. Ensure REPL eval does not corrupt active app state unless explicitly desired.

Acceptance:

1. Launch Snake.
2. Switch to REPL.
3. Evaluate `print('hello')`.
4. Return to Snake.
5. Snake still renders.

### Phase 7: polish for daily use

Tasks:

1. Boot to launcher by default.
2. Add settings app stubs.
3. Add crash screen.
4. Add `picoos ps`.
5. Add NVS-backed settings for boot behavior.
6. Add probe scripts.

Acceptance:

Power-cycle behavior:

1. Device boots to launcher.
2. Launch Snake with physical keys.
3. Play with arrows.
4. Return to launcher.
5. Open REPL.
6. Return to launcher.

## Suggested file-level changes

### `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp`

Current responsibilities in this file are too broad. It owns demo app sources, keyboard routing, console commands, QuickJS job wrappers, and render glue. During the supervisor phase, split responsibilities:

Keep in `app_main.cpp`:

- Hardware initialization.
- Console command registration.
- Existing REPL editor functions until they are moved.
- Render callback that calls `picojs_runtime_dump_text()` + `visual_repl_render_dump_frame()`.

Move into `picoos_core`:

- Built-in app registry.
- App launch/switch/close logic.
- Frame pump.
- Supervisor state.
- Most `picoos` command helpers if practical.

### `components/picojs_runtime/include/picojs_runtime.h`

Add APIs needed by supervisor:

```cpp
typedef struct {
    char id[24];
    bool mounted;
    bool active;
    uint32_t frame_count;
    uint32_t error_count;
} picojs_runtime_app_info_t;

esp_err_t picojs_runtime_set_active_app(picojs_runtime_t *rt, const char *id);
esp_err_t picojs_runtime_get_active_app(picojs_runtime_t *rt, char *dst, size_t dst_len);
esp_err_t picojs_runtime_list_apps(picojs_runtime_t *rt,
                                   picojs_runtime_app_info_t *out,
                                   size_t cap,
                                   size_t *count);
esp_err_t picojs_runtime_close_app_js(JSContext *ctx,
                                      picojs_runtime_t *rt,
                                      const char *id);
```

### `components/picojs_runtime/picojs_runtime.cpp`

Refactor points:

- Replace `rt->app` with `rt->apps` and `rt->active_app`.
- Update `js_os_app()` to find-or-create by ID.
- Update `render_app()` to render active app.
- Update `run_callbacks()` to accept an `App *` or active/background policy.
- Update `picojs_runtime_key_js()` to dispatch to active app.
- Preserve existing behavior for single-app tests.

### `components/qjs_service`

Do not change unless the supervisor needs async job results. `qjs_service_run()` and `qjs_service_post()` are already enough for the first phase.

### `components/visual_repl`

Do not change for phase 1-4. It already provides both REPL rendering and dump-frame rendering. Later, consider renaming or layering it because it will no longer be "just a REPL"; it is also the text display backend.

## Decision records

### Decision: Add a native supervisor instead of extending `app_main.cpp`

- **Context:** `app_main.cpp` already owns hardware init, REPL, keyboard routing, console commands, QuickJS wrappers, and built-in app source strings. Adding app lifecycle and scheduling there would make the file harder to reason about.
- **Options considered:** Keep adding logic to `app_main.cpp`; add `picoos_core`; move everything into `picojs_runtime`.
- **Decision:** Add `components/picoos_core` as a native supervisor component.
- **Rationale:** App lifecycle, frame pump, global hotkeys, launcher state, and app switching are OS concerns, not low-level DSL rendering concerns.
- **Consequences:** Requires a new component and integration API, but creates a clearer boundary for future storage, app packages, and host emulator parity.
- **Status:** proposed.

### Decision: Keep one QuickJS service/context initially

- **Context:** Multiple apps should eventually run and switch, but ESP32-P4 heap and QuickJS complexity make one context per app risky for the first supervisor milestone.
- **Options considered:** One QuickJS context per app; one QuickJS runtime with multiple contexts; one QuickJS context with multiple cooperative app records.
- **Decision:** Start with one QuickJS service/context and multiple cooperative app records.
- **Rationale:** The current system already has a working `qjs_service` and native object bindings. Keeping one context minimizes memory overhead and preserves simple JS object sharing for OS APIs.
- **Consequences:** Isolation is weak. A bad app can still pollute global JS state or consume memory. Crash containment is cooperative rather than security-grade.
- **Status:** proposed.

### Decision: Text compositor before pixel compositor

- **Context:** The current display and probes use a 40x20 text framebuffer. Daily-use apps can be useful within that model.
- **Options considered:** Keep text framebuffer; introduce pixel widgets; implement window compositor.
- **Decision:** Keep text framebuffer and render only one foreground surface at a time.
- **Rationale:** It preserves console dump parity and enables deterministic UART probes. It also fits PicoCalc's small display and existing visual REPL implementation.
- **Consequences:** UI is terminal-like. Rich graphics and overlapping windows wait until the OS loop is stable.
- **Status:** proposed.

### Decision: Frame pump in a task, JS execution via `qjs_service_run()`

- **Context:** Snake needs autonomous motion, but QuickJS must only be touched from the service task.
- **Options considered:** Run JS from ESP timer callback; run JS from keyboard/console tasks; have a FreeRTOS frame task submit `qjs_service` jobs.
- **Decision:** Use a FreeRTOS frame task that submits jobs to `qjs_service_run()`.
- **Rationale:** This respects QuickJS ownership and keeps timing logic separate from JavaScript execution.
- **Consequences:** Frame cadence may block if JS jobs run long. Later, switch to `qjs_service_post()` or drop frames if needed.
- **Status:** proposed.

### Decision: Preserve low-level `picojs` commands during migration

- **Context:** Existing probes and debugging flows use `picojs load`, `picojs dump`, `picojs frame`, and `picojs key`.
- **Options considered:** Replace all commands immediately; add new `picoos` commands while keeping `picojs` commands.
- **Decision:** Add `picoos` commands and keep `picojs` commands until tests are migrated.
- **Rationale:** Reduces risk and preserves hardware observability while supervisor behavior is being debugged.
- **Consequences:** There will be two command surfaces temporarily. Documentation and probes must be explicit about which one is authoritative.
- **Status:** proposed.

## Test and validation strategy

### Build validation

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0102-esp32-p4-visual-quickjs-repl
source ~/esp/esp-idf-5.4.2/export.sh
idf.py build
```

### Hardware flash

Use the stable by-id port:

```bash
idf.py -p /dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00 flash
```

If the port disappears, do not switch blindly to `/dev/ttyACM0`. Re-check:

```bash
ls -l /dev/serial/by-id
```

### Probe scripts

Continue writing probes under the ticket's `scripts/` directory or child ticket directories. Use the existing helper:

```text
0102-esp32-p4-visual-quickjs-repl/tools/picocalc_console.py
```

Minimum supervisor probe sequence:

```text
picoos status
picoos apps
picoos launch home
picojs dump
picoos launch snake
picoos start 4
wait 2 seconds
picojs dump
picoos key left
picojs dump
picoos launcher
picojs dump
picoos repl
```

Assertions:

- Launcher dump contains `PicoOS`, `REPL`, `Snake`, `Sysmon`.
- Snake position changes between dumps while frame pump is running.
- `picoos key left` affects Snake direction/position.
- `picoos launcher` returns to launcher.
- `picoos status` reports running state and active surface.
- No QuickJS service crash occurs.

### Regression tests

Existing behavior should still pass until intentionally replaced:

- `picojs load home`
- `picojs load sysmon`
- `picojs load snake`
- `picojs run 2 250`
- `picojs key left`
- `picojs dump`

### Manual physical validation

Manual test for daily use:

1. Flash firmware.
2. Device boots to launcher.
3. Use physical arrows to select Snake.
4. Press Enter to launch.
5. Snake moves without serial commands.
6. Use arrows to steer.
7. Press Home to return to launcher.
8. Open REPL.
9. Evaluate `print('hello picoos')`.
10. Return to launcher or previous app.

## Risks and mitigations

### Risk: QuickJS global pollution between apps

With one context, apps share global state unless the runtime wraps app source. Mitigation: evaluate built-ins inside a function wrapper or conventionally scoped source.

Example wrapper:

```js
(function(OS) {
  // app source here
})(OS);
```

### Risk: frame pump starves console or keyboard

If frames block on `qjs_service_run()`, other tasks may wait. Mitigation: start at low FPS, collect `last_eval_ms`, and skip frames when QuickJS is busy.

### Risk: LCD redraw cost is too high

Rendering every frame may be expensive. Mitigation: render at lower FPS than logic, dirty-check frames, or only redraw changed rows later.

### Risk: keyboard I2C recovery warnings

Keyboard recovery warnings have appeared during prior validation. Mitigation: keep existing recovery logic and make supervisor key injection testable through UART so development is not blocked by physical keyboard instability.

### Risk: app crashes corrupt runtime

A severe app may leave JS state inconsistent. Mitigation: track per-app errors first; add `picoos reset-app <id>` and full `qjs_service_reset()` recovery path later.

## Open questions

1. Should background apps receive low-frequency ticks in phase 1, or should all background execution wait until app switching is stable?
2. Which physical key should reliably mean "REPL" on the PicoCalc keyboard?
3. Should the launcher be implemented as a PicoJS app immediately, or should the very first launcher be native text for recovery simplicity?
4. How much JS source isolation is required before loading apps from storage?
5. Should `picojs_runtime` maintain multiple app records now, or should supervisor launch/switch first reload apps and add stateful switching in the next phase?

## Recommended first implementation PR

The first PR should be intentionally small:

1. Add `components/picoos_core` skeleton.
2. Add built-in app registry.
3. Add `picoos status` and `picoos apps`.
4. Add `picoos launch <id>` using the same sources as current `picojs load`.
5. Add `picoos start` / `picoos stop` frame pump for the active app.
6. Keep `picojs` commands untouched.
7. Add `03-picoos-supervisor-probe.py` validating status/apps/launch/start/stop.

This will make Snake live and establish the supervisor boundary without forcing multi-app state preservation in the same change.

## Reference map

| Area | File | Why it matters |
|---|---|---|
| Firmware entrypoint | `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp` | Initializes hardware/services, owns current commands, keyboard routing, built-in app strings. |
| PicoJS public API | `components/picojs_runtime/include/picojs_runtime.h` | Runtime boundary the supervisor will call. |
| PicoJS implementation | `components/picojs_runtime/picojs_runtime.cpp` | Native JS DSL, app/panel/widget models, frame/key callbacks, framebuffer renderer. |
| QuickJS service | `components/qjs_service/include/qjs_service.h` | Defines serialized QuickJS jobs/eval/status/reset APIs. |
| QuickJS service implementation | `components/qjs_service/qjs_service.cpp` | Owns QuickJS task/context and executes jobs on the service task. |
| Display backend | `components/visual_repl/include/visual_repl.h` | Defines 40x20 text display and dump-frame rendering. |
| Keyboard backend | `components/picocalc_keyboard/include/picocalc_keyboard.h` | Defines keyboard events and diagnostics used by input router. |
| LCD backend | `components/picocalc_lcd/include/picocalc_lcd.h` | Provides 320x320 LCD primitive operations. |
| Devkit reference | `ttmp/2026/06/25/0102-PICOJS-DEVICE-INTEGRATION--0102-picojs-device-integration/sources/picoos-devkit.jsx` | Source compatibility reference for future widgets/apps. |
| Parity assessment | `ttmp/2026/06/25/0102-PICOJS-DEVICE-INTEGRATION--0102-picojs-device-integration/analysis/01-picoos-devkit-app-parity-assessment.md` | Documents current compatibility and next DSL gaps. |

## Glossary

- **PicoJS**: The JavaScript DSL and native runtime used to describe text UI apps.
- **PicoOS**: The intended daily-use operating environment on the PicoCalc.
- **Supervisor**: Native service that owns lifecycle, scheduling, input routing, app switching, and REPL integration.
- **Surface**: The visible mode: launcher, app, REPL, switcher, or crash screen.
- **Foreground app**: The app that receives normal input and owns the display.
- **Background app**: A running app not currently visible. Background ticks are optional in early phases.
- **Frame pump**: Firmware task that periodically advances active app callbacks without serial commands.
- **QuickJS service task**: The FreeRTOS task that owns `JSRuntime *` and `JSContext *`.
