---
Title: PicoOS Devkit App Parity Assessment
Ticket: 0102-PICOJS-DEVICE-INTEGRATION
Status: active
Topics:
    - esp32-p4
    - quickjs
    - picocalc
    - visual-repl
    - javascript
    - firmware
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp
      Note: |-
        Current built-in firmware app sources and console load/render/input commands.
        Current built-in app load paths and console/runtime integration
    - Path: components/picojs_runtime/picojs_runtime.cpp
      Note: |-
        Current firmware-native PicoJS DSL implementation used for parity comparison.
        Current firmware runtime implementation compared against devkit
    - Path: ttmp/2026/06/25/0102-PICOJS-DEVICE-INTEGRATION--0102-picojs-device-integration/sources/picoos-devkit.jsx
      Note: |-
        Imported React devkit source containing the reference runtime, widgets, OS mocks, and nine demo apps.
        Reference app/runtime source for parity analysis
ExternalSources: []
Summary: Comparison between the imported picoOS devkit apps and the current firmware-native PicoJS implementation.
LastUpdated: 2026-06-25T17:15:00-07:00
WhatFor: Use this before planning the host SDL emulator or the next firmware DSL phases.
WhenToUse: Use when deciding which picoOS devkit apps can run on firmware today and which widgets/OS APIs remain to be implemented.
---


# PicoOS Devkit App Parity Assessment

## Executive summary

The firmware is past the proof-of-life stage, but it is not close to implementing the full `picoos-devkit.jsx` app suite yet.

Current firmware supports the core app lifecycle and a small visual subset:

- `OS.app(name)`
- `App.layout(fn)` with single-axis `row()`/`col()` regions
- `App.panel(id)`
- `App.statusbar(valueOrFn)`
- `App.mount()`
- `App.on("tick", ms, fn)`
- `App.loop(fps, fn)`
- `App.compute(fn)`
- `App.key(token, fn)`
- `Panel.frame(kind)`
- `Panel.title(valueOrFn)`
- `Panel.text(valueOrFn)`
- `Panel.gauge()`
- `Text.at()`, `Text.fg()`, `Text.bold()`
- `Gauge.at()`, `Gauge.label()`, `Gauge.value()`, `Gauge.width()`, `Gauge.showPct()`

That is enough for firmware-native `hello`, `dashboard`, and `interactive` demos, plus a very small subset of the devkit `hello`/`home`/`sysmon` concepts.

The imported devkit contains nine apps and a much richer runtime: layout, menus, tables, sparks, progress bars, rows/buttons/toggles, keypad, chip pad, grid/layers, forms, chat feed, input line, editor, viewer, focus management, tap hit testing, OS mock services, and a 40x30 screen. The hardware firmware currently renders a 40x20 ASCII-oriented screen.

## Source baseline

Imported source:

```text
ttmp/2026/06/25/0102-PICOJS-DEVICE-INTEGRATION--0102-picojs-device-integration/sources/picoos-devkit.jsx
```

The imported devkit uses:

```js
const COLS = 40;
const ROWS = 30;
```

The firmware currently uses:

```c
#define VISUAL_REPL_COLS 40
#define VISUAL_REPL_ROWS 20
```

This row-count difference matters. Some devkit apps will need adaptation for the PicoCalc hardware screen unless we implement scrolling, virtual rows, smaller cells, or a 40x30 host-only profile.

## Current firmware capability summary

### Implemented and hardware validated

- Native QuickJS installation through `qjs_service`.
- Runtime reset before QuickJS reset.
- Frame advancement on the QuickJS task.
- Timer, loop, compute, and key callbacks.
- Console commands:
  - `picojs load hello|dashboard|interactive`
  - `picojs frame [dt_ms]`
  - `picojs run <count> <dt_ms>`
  - `picojs key <token>`
  - `picojs mode app|repl`
  - `picojs dump`
  - `picojs render`
- Physical keyboard routing to app mode with Escape as the REPL escape path.
- LCD rendering of the current PicoJS 40x20 text frame.
- Screen-dump parity between `picojs dump` and `screen dump`.
- LCD SPI stabilized at 40 MHz on the current ESP32-P4 PicoCalc hardware.

### Not yet implemented

- `App.state()`.
- `App.dispatch()`, `App.refresh()`, `App.exit()` semantics beyond stubs/console conventions.
- Wildcard/default key handler.
- Focus fallback behavior for unbound keys.
- Arrow-token aliases (`↑↓←→`) in addition to firmware tokens (`up`, `down`, `left`, `right`).
- `Panel.titleRight()` and `Panel.footer()`.
- Most widgets:
  - `spark`
  - `table`
  - `menu` / `list`
  - `progress`
  - `row` / buttons / toggles
  - `keypad`
  - `pad`
  - `grid` layers
  - `form`
  - `feed`
  - `input`
  - `editor`
  - `viewer`
- Most devkit OS mock APIs:
  - `OS.clock()`
  - `OS.metrics`
  - `OS.history()`
  - `OS.processes()`
  - `OS.launch()`
  - music player state and FFT
  - snake engine
  - calculator eval helper
  - chat room mock
  - filesystem mock
  - settings config object
- Unicode rendering for box-drawing, block, sparkline, arrows, hearts, bullets, and other glyphs used by the devkit.

## App-by-app parity

| Devkit app | Current status | Main missing pieces |
|---|---:|---|
| `hello — minimal` | **Partial, close** | `App.state()`, `OS.clock()`, more key handling. The firmware built-in `hello` covers panel/text/statusbar/tick-like concepts but is not source-compatible with the devkit app yet. |
| `home — dashboard` | **Partial skeleton only** | `titleRight`, `OS.clock`, `body.menu()`, menu grid, `items`, `marker`, `accent`, `onPick`, `OS.launch`, `Gauge.style('blocks')`, Unicode rounded frames. |
| `sysmon — gauges + table` | **Gauge-only subset** | `OS.metrics`, `spark`, `table`, sortable rows, selection/focus, `refresh`, `exit`, `OS.history`, `OS.processes`. |
| `music — player` | **Mostly missing** | `App.state`, `row` buttons/toggles, `progress`, `spark`, music OS model, volume/player/library APIs, `bind`, `onTap`, labels/knob/track. |
| `snake — game loop` | **Frame loop exists, game widgets missing** | `grid` widget with layers, arrow aliases, `App.state`, snake OS engine, loop toggle/pause, `titleRight`. |
| `calc — keypad + live eval` | **Compute exists, UI mostly missing** | `keypad`, `pad`, `chips`, `onKey`, `onTap`, `OS.eval`, `App.state`, calculator text alignment edge cases. |
| `settings — form` | **Missing** | Full `form` builder, sliders/select/toggles, `bind`, `options`, focus/edit behavior, `OS.cfg`. |
| `chat — feed + input` | **Missing** | `feed`, `input`, submit handling, chat OS mock, room state, `titleRight`, focus input routing. |
| `notes — editor` | **Missing** | `editor`, gutter, syntax/view state, statusbar builder object, `esc` key mode switching, filesystem mock. |

## Runtime/API coverage by category

### Core lifecycle

Current coverage is good. `OS.app`, panel creation, mount, frame callbacks, and key callbacks exist.

Next compatibility work should add `App.state()` because most devkit apps rely on it for mutable app-local state. `state(o)` can be implemented cheaply by returning the original JS object and keeping a duplicate reference only if the native runtime needs it later.

### Layout

Current coverage is enough for simple single-axis layouts:

```js
app.layout(l => l.row(1, 'bar').row('*', 'body'))
```

The devkit also has a stub `Layout.cols(...pairs)` but the demos mostly use `row()`/`col()` directly. Firmware should eventually add better mixed-layout diagnostics, but single-axis support is enough for the imported presets as currently written.

### Text and gauges

Current coverage is useful but not source-complete.

Implemented:

```js
panel.text(v).at(x, y).fg(c).bold()
panel.gauge().at(x, y).label(l).value(v).width(w).showPct()
```

Missing for devkit parity:

```js
gauge.max(m)
gauge.style('bar'|'blocks')
text.dim()
```

The `style()` method is especially important because the devkit calls it in `home`, `sysmon`, and `music`.

### Interaction and focus

Current firmware handles explicit registered keys:

```js
app.key('left', fn)
app.key('a', fn)
```

The devkit expects both explicit app keys and implicit focus dispatch:

- if key is bound, call the app key handler;
- otherwise route arrows to the focused widget's `move()`;
- route Enter to `activate()`;
- route printable text/backspace to the focused input/editor's `type()`.

Firmware does not yet have focusables, default focus, widget `move()`/`activate()`/`type()`, or glyph-token aliases.

### Widgets

The devkit's real app coverage is widget-bound. Implementing widgets will unlock more apps than adding individual ad-hoc app sources.

Priority order should be:

1. `App.state`, `OS.clock`, `Panel.titleRight`, `Gauge.max/style`, `Text.dim`.
2. `Menu`/`List` focus and rendering: unlocks `home` launcher shape.
3. `Table` and `Spark`: unlocks most of `sysmon`.
4. `Grid` layers: unlocks `snake` board rendering.
5. `Progress`, `Row`, button/toggle: unlocks `music` enough to render.
6. `Keypad`, `Pad`, `OS.eval`: unlocks `calc`.
7. `Form`: unlocks `settings`.
8. `Feed` and `Input`: unlocks `chat`.
9. `Editor` and `Viewer`: unlocks `notes` and file viewer behavior.

### OS mock surface

The devkit includes a desktop mock OS. Firmware currently has only a special native gauge source string (`'battery'`) and no general OS data model beyond `OS.app()`.

For source portability, we should add an `OS` object with methods/properties matching the devkit. On firmware, values can be deterministic or connected to real telemetry later.

Minimum portable mock phase:

```js
OS.clock(fmt)
OS.battery
OS.metrics
OS.history(k, n)
OS.processes()
OS.launch(name)
OS.cfg
OS.eval(expr)
```

Then domain-specific phases:

```js
OS.library / OS.playing / OS.position / OS.volume / OS.fft()
OS.snake / OS.food / OS.step() / OS.turn() / OS.reset()
OS.room / OS.send() / OS.colorOf()
OS.cwd / OS.ls() / OS.selected / OS.select()
```

## How far along are we?

A fair estimate:

- **Core QuickJS + app lifecycle:** 70–80% of the devkit's core model.
- **Frame/timer/key execution model:** 60–70%, because callbacks work but focus/default key routing is missing.
- **Layout/text/gauge subset:** 35–45%.
- **Full widget library:** 15–20%.
- **OS mock APIs required by the apps:** 10–15%.
- **Actual app source compatibility for all nine devkit presets:** about **15–25%**.

The firmware is therefore strong as a native runtime foundation, but still early as a complete picoOS devkit implementation.

## Recommended next implementation milestones before host SDL

Before extracting a host SDL emulator, it is worth adding the minimum compatibility methods that make the imported source start loading without extensive edits.

### Milestone A: Compatibility aliases and cheap methods

Implement:

- `App.state(o)`
- `App.refresh()` no-op
- `App.exit()` state flag / status marker
- `Text.dim()`
- `Gauge.max()`
- `Gauge.style()`
- `Panel.titleRight()`
- `Panel.footer()`
- `OS.clock()`
- arrow token aliases (`↑↓←→`) mapped to `up/down/left/right`, or firmware key token output changed to devkit glyph tokens.

This should make `hello` much closer and reduce syntax failures in `home`, `sysmon`, `snake`, and `music`.

### Milestone B: Menu + focus dispatch

Implement:

- `Panel.menu()` and `Panel.list()`
- `Menu.grid()`, `items()`, `marker()`, `accent()`, `onPick()`, `frame()`, `title()`
- one focusable list/menu at a time
- default key routing for arrows and Enter

This unlocks the `home` launcher as a real interactive app.

### Milestone C: Sysmon widgets and OS metrics

Implement:

- `Panel.spark()`
- `Panel.table()`
- `OS.metrics`, `OS.history()`, `OS.processes()`
- minimal sorting/select rendering

This unlocks a recognizable `sysmon`.

### Milestone D: Snake/grid

Implement:

- `Panel.grid()`
- `Grid.size()`, `cell()`, `layer()`
- native or JS `OS.snake` mock state
- arrow aliases and `loop.toggle()`

This unlocks the first game-like app and is a good stress test for frame loop + keyboard.

## Host SDL implication

The host SDL emulator should not clone the React devkit as-is. It should share the firmware-compatible runtime surface.

The best path is:

1. Keep `sources/picoos-devkit.jsx` as the reference specification.
2. Continue implementing firmware DSL compatibility in small phases.
3. Extract the model/render core once the widget API shape stabilizes.
4. Build SDL as a backend for the same model:
   - QuickJS source loader
   - semantic keyboard tokens
   - 40x20 or configurable 40x30 screen buffer
   - SDL renderer for glyph cells

If we extract too early, the host emulator will preserve today's incomplete API instead of the devkit-compatible API we actually want.
