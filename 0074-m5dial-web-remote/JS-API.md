# 0074 M5Dial Lain OS JavaScript API Guide

This project exposes a persistent MicroQuickJS runtime on the dial. JavaScript does not paint the screen directly. Instead, it drives Lain OS by queuing high-level commands such as mode changes, knob position updates, station metadata, reveal overlays, and messages.

Use this as the supported runtime guide for new developers. The main implementation lives in [js_service.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/js_service.cpp), [mqjs_timers.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/mqjs_timers.cpp), and [esp32_stdlib_runtime.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/esp32_stdlib_runtime.c).

## Syntax compatibility

Write scripts in conservative function-style JavaScript.

- Supported variable form: `var`
- Do not use: `let` or `const`
- Supported: `function () { ... }`
- Do not use: arrow functions like `() => { ... }`

When writing callbacks for `setTimeout(...)`, `every(...)`, or `lain.after(...)`, always use the classic form:

```js
var count = 0
setTimeout(function () {
  print('ok')
}, 500)
```

## Mental model

There is one long-lived JS VM per device. Code sent from `esp_console`, the Go HTTP endpoint, or the websocket path all runs inside that same VM. Global state and timer handles persist between evaluations until the firmware reboots.

The data flow is:

```text
console / HTTP / websocket script_eval
            |
            v
      js_service_eval(...)
            |
            v
   persistent MicroQuickJS runtime
            |
            +--> timer callbacks via mqjs_timers task
            |
            v
   __lain.cmds / __lain.logs / __lain.events
            |
            v
   flushed into firmware app command queue
            |
            v
      app_main.cpp updates LCD state
```

Important consequences:

- JS state is persistent, so you can keep timer handles and counters in globals.
- Timer callbacks are real asynchronous callbacks now, but they still execute on the single JS service thread.
- A script that floods the app command queue will still overwhelm the UI path. Prefer small incremental updates.

## How to run code

### Local console

```text
js eval 1 + 2
js eval "lain.mode('radio'); lain.position(35); 'ok'"
js eval "setTimeout(function () { print('tick') }, 500); 'armed'"
```

Use `print(...)` when you want immediate text on the serial console.

### HTTP

```bash
curl -sS -X POST http://127.0.0.1:18080/api/script-eval \
  -H 'Content-Type: application/json' \
  --data '{"device_id":"m5dial-b76a94","request_id":9001,"filename":"inline","timeout_ms":1000,"code":"lain.mode(\"radio\"); lain.position(35); \"ok\""}'
```

To send a saved script file:

```bash
jq -Rs \
  --arg device_id "m5dial-b76a94" \
  --arg filename "scripts/timer-radio-sweep.js" \
  '{device_id:$device_id, request_id:9002, filename:$filename, timeout_ms:1000, code:.}' \
  /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/scripts/timer-radio-sweep.js |
curl -sS -X POST http://127.0.0.1:18080/api/script-eval \
  -H 'Content-Type: application/json' \
  --data-binary @-
```

## Supported API

### Timing primitives

These are backed by [mqjs_timers.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/mqjs_timers.cpp) and native hooks in [esp32_stdlib_runtime.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/esp32_stdlib_runtime.c).

- `setTimeout(fn, ms)`
  Schedule `fn` once. Returns a numeric timer id.
- `clearTimeout(id)`
  Cancel a one-shot timer by id.
- `every(ms, fn)`
  Convenience helper that reschedules itself after each callback. Returns a handle object with `cancel()`.
- `cancel(handleOrId)`
  Accepts either a numeric timer id or an `every(...)` handle object.
- `lain.after(ms, fn)`
  Alias for `setTimeout(fn, ms)`.
- `lain.every(ms, fn)`
  Alias for `every(ms, fn)`.
- `lain.cancel(handle)`
  Alias for `cancel(handle)`.

Timer limits:

- Delay is clamped to `0 .. 3600000` milliseconds.
- Active timer slots are capped by `CONFIG_TUTORIAL_0074_JS_MAX_TIMERS` in [Kconfig.projbuild](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/Kconfig.projbuild).
- Timer ids come from a 1024-entry id ring and will be reused after cancellation or firing.

### Lain OS DSL

These are the primary project APIs. They enqueue commands for the firmware app loop.

- `lain.mode(mode)`
  `mode` may be `'debug'`, `'radio'`, or the corresponding integer mode.
- `lain.position(value)`
  Set the current radio position / knob position.
- `lain.band(name)`
  Set the current band label.
- `lain.station(pos, type, name)`
  Update one station slot.
  `type` may be `'clear'`, `'static'`, `'hidden'`, or `'distorted'`.
- `lain.message(text)`
  Show a transient status/debug message.
- `lain.reveal(text)`
  Trigger reveal text.
- `lain.emit(name, detail)`
  Emit a script event for remote observers.
- `lain.nowMs()`
  Convenience alias for `Date.now()`.

### Utility functions

- `print(...)`
  Writes directly to serial. Use this for local REPL debugging.
- `console.log(...)`
- `console.warn(...)`
- `console.error(...)`
  These go into the remote script log stream rather than straight to serial.
- `Date.now()`
  Millisecond clock from `esp_timer`.
- `performance.now()`
  Also millisecond clock.
- `gc()`
  Request a JS garbage collection pass.
- `load(path)`
  Evaluate a file from SPIFFS if mounted. This exists, but it is not the main `0074` workflow.

### Out-of-scope advanced runtime helpers

The imported stdlib also exposes lower-level helpers such as `gpio.*` and `i2c.*` in [esp32_stdlib_runtime.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/esp32_stdlib_runtime.c). They are real, but they are not the primary supported Lain OS surface for this project.

## Examples

### Enter radio mode

```js
lain.mode('radio')
lain.position(35)
lain.band('wired')
lain.station(35, 'clear', 'navi_broadcast')
```

### One-shot timer

```js
print('arming timeout')
setTimeout(function () {
  print('timeout fired')
  lain.reveal('HELLO FROM TIMER')
}, 750)
```

### Repeating motion with cleanup

```js
var pos = 0
var handle = every(200, function () {
  lain.mode('radio')
  lain.position(pos)
  pos = (pos + 8) % 120
})

setTimeout(function () {
  cancel(handle)
  lain.message('sweep stopped')
}, 5000)
```

### Persistent global handle

```js
globalThis.demoSweep && cancel(globalThis.demoSweep)

var pos = 0
globalThis.demoSweep = every(150, function () {
  lain.position(pos)
  pos = (pos + 5) % 120
})
```

### Seed stations, then animate

```js
lain.mode('radio')
lain.band('wired')
lain.station(5, 'clear', 'phantom_relay')
lain.station(25, 'distorted', 'wire_temple')
lain.station(50, 'static', 'STATIC')
lain.station(75, 'hidden', '???')
lain.station(100, 'clear', 'layer_07_hub')

var step = 0
every(400, function () {
  lain.position((step * 5) % 120)
  step++
})
```

## Example scripts

Saved examples are in [scripts](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/scripts):

- [timer-radio-sweep.js](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/scripts/timer-radio-sweep.js)
- [timer-reveal-demo.js](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/scripts/timer-reveal-demo.js)

## Debugging

### If timers seem dead

Run this first:

```text
js eval "setTimeout(function () { print('timer ok') }, 500); 'armed'"
```

If that prints, the timer scheduler is alive and your problem is in the callback body.

### If the UI does not change

Check app-side state:

```text
display status
js eval "lain.mode('radio'); 'ok'"
display status
display redraw
```

If `mode=radio` but the LCD does not change, the display renderer is the problem, not the JS command path.

### If local console looks quiet

- `print(...)` goes to serial.
- `console.log(...)` goes to the remote script log stream.

### If you flood the queue

Bad:

```js
every(50, function () {
  for (var i = 0; i < 60; i++) {
    lain.station(i * 2, 'clear', 'station-' + i)
  }
})
```

Better:

```js
var i = 0
every(250, function () {
  lain.station((i * 7) % 120, 'clear', 'station-' + i)
  i++
})
```

## Relevant files

- [js_service.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/js_service.cpp)
- [mqjs_timers.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/mqjs_timers.cpp)
- [esp32_stdlib_runtime.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/esp32_stdlib_runtime.c)
- [js_console.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/js_console.cpp)
- [remote_client.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/remote_client.cpp)
- [scripts](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/scripts)
