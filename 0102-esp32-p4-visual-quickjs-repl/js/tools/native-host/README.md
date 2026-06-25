# native-host

`native-host` is a desktop C++ QuickJS embedder for the PicoCalc visual REPL API.
It is intentionally split so the QuickJS binding/runtime code in `src/pico_native_api.*`
can later be ported into ESP-IDF firmware pieces, while `src/main.cpp` remains a
host-only ANSI terminal event loop.

The API direction is the same as native module providers in `go-go-goja`/`goja-text`,
but implemented with QuickJS C/C++ bindings: JavaScript sees an `OS` object and fluent
builders, while C++ owns screen state, app objects, widgets, timers, key dispatch, and
host globals.

## Build

```bash
make -C 0102-esp32-p4-visual-quickjs-repl/js/tools/native-host all
```

## Run

Prefer the wrapper so `PICO_JS_DIR` points at the JS tree:

```bash
0102-esp32-p4-visual-quickjs-repl/js/tests/run-native-host.sh hello-native
0102-esp32-p4-visual-quickjs-repl/js/tests/run-native-host.sh dashboard-native
```

Keys:

- arrows/typeable characters are forwarded to the JS app as semantic key tokens,
- `q` quits the host emulator,
- the host runs a simple timed redraw loop.

## Portability boundary

- Portable/firmware-oriented: `src/pico_native_api.hpp`, `src/pico_native_api.cpp`.
- Host-only: `src/main.cpp`, termios/raw keyboard, ANSI rendering, file loading.

This checkpoint implements a small API surface: `print`, `millis`, `gc`, `OS.app`,
`OS.clock`, `OS.launch`, `App.state/panel/on/key/statusbar/mount/exit`, `Panel.frame/title/titleRight/text/gauge`,
and `Text/Gauge` fluent methods.

Native structs store duplicated QuickJS values for callbacks and reactive literals via
RAII (`StoredValue`). `runtime_destroy()` tears down native app/widget/timer state before
freeing the QuickJS context/runtime, so scripted host runs can exit cleanly. JS wrapper
objects are non-owning views over native objects; the native runtime owns the actual app,
panels, and widgets.

## Smoke test

```bash
0102-esp32-p4-visual-quickjs-repl/js/tests/run-native-smoke.sh
```
