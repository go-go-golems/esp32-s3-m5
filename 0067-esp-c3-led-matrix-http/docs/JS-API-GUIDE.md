# 0067 ESP-C3 Matrix JavaScript API Guide

This document is the practical API spec for JavaScript-driven LED matrix control in `0067-esp-c3-led-matrix-http`.

It covers:
- how to get started quickly,
- how scripts are executed,
- all public JS matrix methods and options,
- REST and console trigger paths,
- known limits (timeouts, text length, glyph set),
- patterns for reliable animations,
- troubleshooting.

This guide is intentionally detailed so a new developer can build and debug scripts without reading all runtime source files first.

## 1. Overview

The firmware exposes a MicroQuickJS runtime that can control the MAX7219 LED chain directly.

Default matrix topology in this project:
- chain length: `12` modules
- logical resolution: `96x8`
- GPIO defaults:
  - `DIN/MOSI = 4`
  - `CS = 5`
  - `CLK/SCK = 6`

You can drive the matrix through two high-level paths:
- Matrix REST API (`/api/matrix/*`) for built-in text/anim modes.
- JS REST API (`/api/js/*`) for custom scripts (pixels, loops, timing logic).

You can also use the Wi-Fi console:
- `matrix ...` commands
- `js ...` commands

## 2. Getting Started

### 2.1 Prerequisites

- Device flashed with `0067-esp-c3-led-matrix-http`.
- Device connected to Wi-Fi and reachable at an IP (examples below use `192.168.3.119`).
- From your workstation, set:

```bash
BASE_URL=http://192.168.3.119
```

### 2.2 Verify matrix and JS runtime health

Check matrix status:

```bash
curl -sS "$BASE_URL/api/matrix/status"
```

Check JS runtime status:

```bash
curl -sS "$BASE_URL/api/js/status"
```

Expected shape (fields may differ by run state):
- matrix status includes `mode`, `width`, `chain_len`, `intensity`, `repeat_count`, orientation flags.
- JS status includes `started`, `busy`, `stop_requested`, `eval_count`, `last_eval_ms`, `last_error`.

### 2.3 Run first script (inline)

```bash
curl -sS -X POST "$BASE_URL/api/js/eval" --data-binary \
'(function(){ matrix.stop(); matrix.setText("HELLO"); return "ok"; })()'
```

### 2.4 Run script from file

```bash
curl -sS -X POST "$BASE_URL/api/js/eval" \
  --data-binary @0067-esp-c3-led-matrix-http/examples/js/01-plasma-ribbon.js
```

### 2.5 Stop/reset workflow

Stop currently running script/animation:

```bash
curl -sS -X POST "$BASE_URL/api/js/stop"
```

Soft reset runtime (default reset path):

```bash
curl -sS -X POST "$BASE_URL/api/js/reset"
```

Hard reset runtime (recreate VM):

```bash
curl -sS -X POST "$BASE_URL/api/js/reset-hard"
```

## 3. Runtime Model

### 3.1 Execution model

- One JS VM instance.
- All JS execution is serialized through a service queue.
- `setTimeout` callbacks are scheduled by a timer task and posted back into the JS service queue.
- No preemptive JS threads.

### 3.2 Script lifecycle

- `POST /api/js/eval` evaluates source text.
- The return value is converted into `output` inside a JSON envelope.
- Long-running behavior should be implemented as registered `matrix.anim` animations, using `ctx.every(...)` / `ctx.timeout(...)` inside the animation context.

### 3.3 Cancellation model

There are two distinct concepts:

1. Local matrix stop (`matrix.stop()`):
- stops current matrix mode,
- cancels pending JS timers,
- does **not** force global stop flag for future callbacks.

2. Global runtime stop (`POST /api/js/stop` or `js stop`):
- sets cooperative stop flag (`matrix.shouldStop()` becomes true),
- cancels timers,
- stops matrix mode.

Recommended animation pattern:

```javascript
(function () {
  var name = "demo";
  matrix.anim.unregister(name);
  matrix.anim.register(name, function (ctx) {
    var x = 0;
    ctx.every(40, function () {
      if (ctx.shouldStop()) return;
      ctx.matrix.clear();
      ctx.matrix.setPixel(x, 3, 1);
      ctx.matrix.present();
      x = (x + 1) % ctx.matrix.width();
    });
    return function () {
      ctx.matrix.clear();
      ctx.matrix.present();
    };
  });
  matrix.anim.start(name, {});
})()
```

## 4. REST API Reference

## 4.1 JS endpoints

### `POST /api/js/eval`
Body:
- Raw JavaScript text (not JSON).
- Max request size controlled by `CONFIG_TUTORIAL_0067_JS_MAX_BODY` (default in `sdkconfig.defaults`: `4096`).

Response envelope:

```json
{
  "ok": true,
  "output": "...",
  "error": null,
  "timed_out": false
}
```

Error envelope (example):

```json
{
  "ok": false,
  "output": "",
  "error": "TypeError: ...",
  "timed_out": false
}
```

### `POST /api/js/stop`
Stops script activity and requests cooperative stop.

Response:

```json
{"ok":true}
```

### `POST /api/js/reset`
Soft reset.

Response:

```json
{"ok":true}
```

### `POST /api/js/reset-hard`
Hard reset (recreate VM).

Response:

```json
{"ok":true}
```

### `GET /api/js/status`
Runtime status fields:
- `started`
- `busy`
- `stop_requested`
- `last_timed_out`
- `eval_count`
- `last_eval_ms`
- `timer_cb_keys`
- `timer_cb_active`
- `timer_cb_keys_high_water`
- `animations_registered`
- `active_animation`
- `heap_free_8bit`
- `heap_largest_free_8bit`
- `heap_min_free_8bit`
- `last_error`

### `GET /api/js/mem`
Returns memory dump text from VM.

## 4.2 Matrix endpoints

### `GET /api/matrix/status`
Returns matrix engine state (`mode`, `text`, `width`, `intensity`, orientation, timing values).

### `POST /api/matrix/text`
Body JSON:

```json
{"text":"HELLO"}
```

### `POST /api/matrix/anim`
Body JSON:

```json
{
  "mode": "scroll",
  "text": "HELLO",
  "fps": 20,
  "pause_ms": 250,
  "repeat_count": 0
}
```

`mode` options:
- `scroll`
- `wave`
- `drop`

`repeat_count` semantics:
- `0` means infinite loop.
- `>0` means finite cycle count.

### `POST /api/matrix/stop`
Stops matrix engine mode and returns status.

## 5. Console Command Reference

## 5.1 JS console (`js`)

```text
js status
js eval <CODE>
js reset [hard]
js stop
js mem
js examples
```

Useful examples:

```text
js eval matrix.setText('HELLO')
js eval matrix.startScroll('HELLO WIFI', {fps:20,pauseMs:250,repeat:2,wave:true})
js reset
js reset hard
```

## 5.2 Matrix console (`matrix`)

```text
matrix status
matrix examples
matrix text <TEXT>
matrix scroll on <TEXT> [fps] [pause_ms] [repeat_count]
matrix scroll wave <TEXT> [fps] [pause_ms] [repeat_count]
matrix scroll off
matrix anim drop <TEXT> [fps] [pause_ms] [repeat_count]
matrix anim off
matrix test on|off
matrix intensity <0..15>
matrix spi [hz]
matrix chain [n]
matrix reverse on|off
matrix flipv on|off
```

## 6. JavaScript API Spec

The runtime injects a `matrix` object and timing helpers.
Preferred style for reusable scripts is the `matrix.anim` lifecycle API.
Low-level global timer helpers are still available for diagnostics and quick one-offs.

## 6.1 Global helpers (low-level)

### `setTimeout(fn, ms) -> id`
- Schedules one-shot callback.
- `ms` is clamped to `0..3600000`.

### `clearTimeout(id)`
- Cancels one-shot callback.

### `every(ms, fn) -> handle`
- Convenience repeated scheduler built on `setTimeout`.
- Returns object with:
  - `handle.cancel()`
- In normal animation code, prefer `ctx.every(...)` inside `matrix.anim.register(...)` so handles are tracked automatically.

### `cancel(handleOrId)`
- Accepts timeout ID or object exposing `.cancel()`.
- Mostly useful for low-level scripts; not usually needed with `matrix.anim` lifecycle tracking.

### `print(...)`
- Console print helper.

### `gc()`
- Triggers VM garbage collection.

## 6.2 `matrix` object

### Geometry / state

- `matrix.width() -> number`
- `matrix.height() -> number`
- `matrix.status() -> object`
- `matrix.statusJson() -> string`

Status object fields include:
- `ok`, `ready`, `mode`, `text`,
- `chain_len`, `width`, `height`,
- `spi_hz`, `intensity`, `test_mode`,
- `fps`, `pause_ms`, `repeat_count`,
- `reverse_modules`, `flip_vertical`,
- `stop_requested`.

### Framebuffer control (script mode)

- `matrix.clear() -> boolean`
- `matrix.fill(on) -> boolean`
- `matrix.setPixel(x, y, on) -> boolean`
- `matrix.getPixel(x, y) -> boolean`
- `matrix.present() -> boolean`

Typical pattern:

```javascript
matrix.clear();
for (var x = 0; x < matrix.width(); x++) {
  matrix.setPixel(x, 3, 1);
}
matrix.present();
```

### Text and built-in animations

- `matrix.setText(text) -> boolean`
- `matrix.startScroll(text, opts) -> boolean`
- `matrix.startDrop(text, opts) -> boolean`
- `matrix.stop() -> boolean`

`startScroll` options:
- `fps` (integer, `0` means use default)
- `pauseMs` (integer)
- `repeat` (integer, `0 = infinite`)
- `wave` (boolean)

`startDrop` options:
- `fps`
- `pauseMs`
- `repeat`

Example:

```javascript
matrix.startScroll("HELLO WIFI", { fps: 20, pauseMs: 250, repeat: 2, wave: true });
```

### Device settings

- `matrix.setIntensity(value) -> boolean` where value is `0..15`
- `matrix.setOrientation(reverseModules, flipVertical) -> boolean`

Example:

```javascript
matrix.setOrientation(false, true);
matrix.setIntensity(8);
```

### Animation registry API (`matrix.anim`)

This API lets scripts register named animations with explicit lifecycle hooks so resource cleanup is consistent.

Methods:
- `matrix.anim.register(name, specOrStartFn)`
- `matrix.anim.unregister(name)`
- `matrix.anim.start(name, opts)`
- `matrix.anim.stop()`
- `matrix.anim.clear()`
- `matrix.anim.list() -> string[]`
- `matrix.anim.current() -> string|null`
- `matrix.anim.status() -> { current, registered, tracked }`

`specOrStartFn` options:
- function form: `register("name", function(ctx) { ... })`
- object form: `register("name", { start: function(ctx){ ... } })`

`ctx` provides:
- `ctx.matrix` (same as `matrix`)
- `ctx.opts` (start options)
- `ctx.shouldStop()`
- `ctx.every(ms, fn)` (tracked automatically)
- `ctx.timeout(ms, fn)` (tracked automatically)
- `ctx.track(handleOrId)` (manual tracking)
- `ctx.onCleanup(fn)` (register cleanup callbacks)
- `ctx.nowMs()`, `ctx.nowUs()`

Cleanup behavior:
- Starting a new registered animation automatically stops the previous one.
- `matrix.anim.stop()` cancels tracked timers/handles, runs cleanup callbacks (reverse order), and stops matrix output.
- Runtime stop/reset paths call animation cleanup hooks as well.

Example:

```javascript
(function () {
  matrix.anim.register("pulse", function (ctx) {
    var on = false;
    ctx.every(120, function () {
      if (ctx.shouldStop()) return;
      on = !on;
      ctx.matrix.fill(on ? 1 : 0);
      ctx.matrix.present();
    });
    ctx.onCleanup(function () {
      ctx.matrix.clear();
      ctx.matrix.present();
    });
  });

  matrix.anim.start("pulse", {});
  return JSON.stringify(matrix.anim.status());
})()
```

### Timing primitives

- `matrix.nowMs() -> number`
- `matrix.nowUs() -> number`
- `matrix.sleepMs(ms) -> boolean`
- `matrix.sleepUntilUs(targetUs) -> boolean`
- `matrix.shouldStop() -> boolean`

For tight loop timing:

```javascript
(function () {
  var next = matrix.nowUs();
  var period = 20000; // 50 Hz
  while (!matrix.shouldStop()) {
    // draw/update frame here
    next += period;
    matrix.sleepUntilUs(next);
  }
})();
```

## 7. Text/Glyph Behavior and Limits

## 7.1 Character set

Font table supports printable ASCII range `32..126`.

This includes punctuation such as:
- `. , ; : ! ?`
- `[](){}<>`
- `@ # $ % ^ & * + - = _ / \\ | ~ ' "`

Notes:
- Lowercase input is normalized to uppercase in rendering path.
- Non-printable / out-of-range characters are replaced with space.

## 7.2 String length limits

- Internal text buffer is 64 chars (`char[65]`).
- `startScroll` and `startDrop` text is truncated to at most 64 chars.
- `setText` static mode effectively displays only up to chain length characters (12 by default).

## 7.3 Empty strings

- `matrix.setText("")` is allowed and clears visible text content.
- `matrix.startScroll("")` / `matrix.startDrop("")` returns false (invalid animation payload).

If you want a blank display, use:

```javascript
matrix.stop();
matrix.clear();
matrix.present();
```

## 8. Practical Script Patterns

## 8.1 Minimal safe animation template

```javascript
(function () {
  var name = "runner";
  matrix.anim.unregister(name);

  matrix.anim.register(name, function (ctx) {
    var x = 0;
    var frameMs = ((ctx.opts && ctx.opts.frameMs) | 0) || 50;
    ctx.every(frameMs, function () {
      if (ctx.shouldStop()) return;
      ctx.matrix.clear();
      ctx.matrix.setPixel(x, 3, 1);
      ctx.matrix.present();
      x = (x + 1) % ctx.matrix.width();
    });
    return function () {
      ctx.matrix.clear();
      ctx.matrix.present();
    };
  });

  matrix.anim.start(name, { frameMs: 50 });
  return JSON.stringify(matrix.anim.status());
})()
```

## 8.2 Bouncing text using built-in scroll mode

```javascript
(function () {
  matrix.stop();
  matrix.startScroll("BOUNCE", { fps: 20, pauseMs: 250, repeat: 0, wave: true });
  return "wave scroll started";
})()
```

## 8.3 Punctuation demo

```javascript
(function () {
  matrix.stop();
  matrix.startScroll(".?[]()@{}<>!", { fps: 18, pauseMs: 200, repeat: 0, wave: false });
  return "punctuation demo";
})()
```

## 8.4 Draw border + checker for orientation checks

```javascript
(function () {
  matrix.stop();
  matrix.clear();
  var w = matrix.width();
  var h = matrix.height();

  for (var x = 0; x < w; x++) {
    matrix.setPixel(x, 0, 1);
    matrix.setPixel(x, h - 1, 1);
  }
  for (var y = 0; y < h; y++) {
    matrix.setPixel(0, y, 1);
    matrix.setPixel(w - 1, y, 1);
  }
  matrix.present();
  return "border";
})()
```

## 9. Triggering Scripts: End-to-End Examples

## 9.1 Inline REST eval

```bash
curl -sS -X POST "$BASE_URL/api/js/eval" --data-binary \
'(function(){ matrix.setText("READY"); return matrix.statusJson(); })()'
```

## 9.2 Multi-line script via heredoc

```bash
cat <<'JS' | curl -sS -X POST "$BASE_URL/api/js/eval" --data-binary @-
(function () {
  var name = "stripe";
  matrix.anim.unregister(name);
  matrix.anim.register(name, function (ctx) {
    var phase = 0;
    ctx.every(120, function () {
      if (ctx.shouldStop()) return;
      ctx.matrix.clear();
      for (var x = phase; x < ctx.matrix.width(); x += 2) ctx.matrix.setPixel(x, 2, 1);
      ctx.matrix.present();
      phase = (phase + 1) % 2;
    });
    return function () {
      ctx.matrix.clear();
      ctx.matrix.present();
    };
  });
  matrix.anim.start(name, {});
  return JSON.stringify(matrix.anim.status());
})()
JS
```

## 9.3 File-based eval

```bash
curl -sS -X POST "$BASE_URL/api/js/eval" \
  --data-binary @0067-esp-c3-led-matrix-http/examples/js/03-comet-trails.js
```

## 9.4 Console eval

```text
js eval matrix.startScroll('HELLO', {fps:20,pauseMs:250,repeat:0,wave:true})
```

## 10. Diagnostics and Troubleshooting

## 10.1 First-line visual diagnostic sequence

Use:
- `0067-esp-c3-led-matrix-http/examples/DIAGNOSTIC-SEQUENCE.md`
- `0067-esp-c3-led-matrix-http/examples/js/diag/*.js`

Run step-by-step from known-good patterns (`all-on`, `all-off`, `single-pixel`) before testing complex animations.

## 10.2 Common failure causes

1. API is healthy but no pixels change:
- likely script logic cancelled itself early.
- check `matrix.shouldStop()` usage and whether you accidentally set global stop.

2. Script starts but dies quickly:
- check `/api/js/status` and `last_error`.
- check serial logs for callback interruption or JS exceptions.

3. `413 body too large` on `/api/js/eval`:
- script exceeds `CONFIG_TUTORIAL_0067_JS_MAX_BODY`.

4. Blank output from animation with empty text:
- `startScroll`/`startDrop` requires non-empty text.

## 10.3 Useful debug commands

```bash
curl -sS "$BASE_URL/api/js/status"
curl -sS "$BASE_URL/api/matrix/status"
curl -sS "$BASE_URL/api/js/mem"
curl -sS -X POST "$BASE_URL/api/js/stop"
curl -sS -X POST "$BASE_URL/api/js/reset"
```

## 11. Configuration Knobs (Kconfig)

Defined in `main/Kconfig.projbuild`:
- `CONFIG_TUTORIAL_0067_JS_MEM_BYTES`
- `CONFIG_TUTORIAL_0067_JS_MAX_BODY`
- `CONFIG_TUTORIAL_0067_JS_EVAL_TIMEOUT_MS`
- `CONFIG_TUTORIAL_0067_JS_MAX_TIMERS`
- matrix pin/chain/SPI defaults and animation defaults

Current project defaults in `sdkconfig.defaults` include:
- `CONFIG_TUTORIAL_0067_JS_MEM_BYTES=98304`
- `CONFIG_TUTORIAL_0067_JS_MAX_BODY=4096`
- `CONFIG_TUTORIAL_0067_JS_EVAL_TIMEOUT_MS=250`
- `CONFIG_TUTORIAL_0067_JS_MAX_TIMERS=32`

## 12. Quick Reference Cheatsheet

Start file script:

```bash
curl -sS -X POST "$BASE_URL/api/js/eval" --data-binary @script.js
```

Stop everything:

```bash
curl -sS -X POST "$BASE_URL/api/js/stop"
```

Soft reset:

```bash
curl -sS -X POST "$BASE_URL/api/js/reset"
```

Static text:

```javascript
matrix.setText("HELLO")
```

Wave scroll infinite:

```javascript
matrix.startScroll("HELLO", {fps:20,pauseMs:250,repeat:0,wave:true})
```

Per-frame low-level drawing:

```javascript
matrix.clear(); matrix.setPixel(0,0,1); matrix.present();
```
