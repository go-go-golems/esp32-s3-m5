# 0067 ESP-C3 Matrix JavaScript API Guide

This is the developer reference for JavaScript-driven LED matrix control in `0067-esp-c3-led-matrix-http`. It is written so that a new contributor can go from zero to running custom animations on the 96x8 LED matrix over Wi-Fi without reading any firmware source code first.

The firmware runs on an M5Stack STAMP C3 (ESP32-C3) and chains 12 MAX7219 8x8 LED modules into a single 96-pixel-wide, 8-pixel-tall canvas. A MicroQuickJS virtual machine embedded in the firmware lets you send JavaScript over REST or the Wi-Fi serial console to draw pixels, trigger built-in text animations, or run complex frame-by-frame effects with precise timing control.

The document covers:

- getting started in under two minutes,
- how the runtime executes and cancels your scripts,
- the full JavaScript API surface (framebuffer, text, animations, timing),
- the animation lifecycle registry for production-quality scripts,
- REST and console control paths,
- frame budget math so you can reason about performance,
- configuration knobs and their defaults,
- diagnostic tools and troubleshooting recipes.


## 1. Hardware and Topology

The matrix hardware is a chain of MAX7219 LED driver modules connected over SPI. Each module drives one 8x8 LED grid. With 12 modules chained together, the firmware presents a single logical framebuffer of **96 columns by 8 rows** (768 pixels total, each either on or off).

Default GPIO wiring on the STAMP C3:

| Signal   | GPIO | Notes                          |
|----------|------|--------------------------------|
| DIN/MOSI | 4    | SPI data out to first MAX7219  |
| CS       | 5    | Active-low chip select         |
| CLK/SCK  | 6    | SPI clock                      |

The SPI clock defaults to 100 kHz, which means a full 192-byte frame transfer takes roughly 15 ms. At 1 MHz (configurable at runtime via `matrix.setSpiHz()` or `matrix spi` console command), that drops to about 1.5 ms. This transfer cost is a fixed per-frame overhead that matters when you target high frame rates.

**Frame budget at common rates:**

| Target FPS | Frame period | SPI @ 100 kHz | JS logic (typical) | Slack     |
|------------|-------------|----------------|---------------------|-----------|
| 15         | 66.7 ms     | ~15 ms         | ~5 ms               | ~46 ms    |
| 30         | 33.3 ms     | ~15 ms         | ~5 ms               | ~13 ms    |
| 60         | 16.7 ms     | ~15 ms         | ~5 ms               | tight     |

At default SPI speed, 30 FPS is comfortable. If you need 60 FPS, increase the SPI clock first.


## 2. Two Ways to Drive the Matrix

There are two independent control paths, each with REST and console interfaces:

**Matrix API** (`/api/matrix/*`, `matrix` console commands) -- Built-in C engine modes: static text, scrolling text, wave scroll, drop-bounce animation. These run entirely in firmware with no JS involvement. Good for simple signage and status displays.

**JS API** (`/api/js/*`, `js` console commands) -- Send JavaScript source code to the embedded MicroQuickJS VM. Scripts can draw individual pixels, run frame loops, trigger built-in animations, or combine everything. This is where the real power lives.

Both paths share the same underlying matrix engine. When a JS script calls `matrix.clear()` and `matrix.present()`, it writes to the same framebuffer and SPI bus as the built-in scroll animation. The engine tracks which mode is active (`idle`, `text`, `scroll`, `drop`, or `script`) and the two control paths cooperate through this state.


## 3. Getting Started

### 3.1 Prerequisites

- Device flashed with `0067-esp-c3-led-matrix-http` firmware.
- Device connected to Wi-Fi (configured via `wifi set --ssid "..." --pass "..." --save` over console, or pre-set in sdkconfig).
- Device reachable from your workstation on the same network.

Set a shell variable for convenience (substitute your device's IP):

```bash
export BASE_URL=http://192.168.3.119
```

### 3.2 Verify everything is alive

Two quick health checks tell you whether the matrix hardware and JS runtime are ready:

```bash
# Matrix engine status
curl -sS "$BASE_URL/api/matrix/status"

# JS runtime status
curl -sS "$BASE_URL/api/js/status"
```

The matrix status returns the current mode (`idle`, `text`, `scroll`, `drop`, `script`), display dimensions, intensity, and timing parameters. The JS status tells you whether the VM is started, whether it is currently busy evaluating something, how many timers are active, and heap pressure metrics.

If `started` is `false` in the JS status, the VM failed to initialize -- check serial logs for heap allocation failures. If the matrix width is 0, the SPI init did not complete.

### 3.3 Hello world: static text

```bash
curl -sS -X POST "$BASE_URL/api/js/eval" --data-binary \
  '(function(){ matrix.setText("HELLO"); return "ok"; })()'
```

The response is a JSON envelope:

```json
{"ok": true, "output": "ok", "error": null, "timed_out": false}
```

The `output` field contains whatever your script's top-level expression evaluates to (converted to a string). If evaluation fails, `ok` is `false` and `error` contains the exception message.

### 3.4 Hello world: pixel drawing

```bash
curl -sS -X POST "$BASE_URL/api/js/eval" --data-binary \
  '(function(){ matrix.clear(); matrix.setPixel(0,0,1); matrix.setPixel(95,7,1); matrix.present(); return "corners"; })()'
```

This lights up the top-left and bottom-right corners. The key concept: `setPixel` writes to an in-memory framebuffer, but nothing reaches the LEDs until you call `present()`. This lets you compose a full frame before flushing it to hardware in one SPI transaction.

### 3.5 Run a script from a file

The `examples/js/` directory contains ready-to-run animations. Send one to the device:

```bash
curl -sS -X POST "$BASE_URL/api/js/eval" \
  --data-binary @0067-esp-c3-led-matrix-http/examples/js/01-plasma-ribbon.js
```

You should see a wavy ribbon pattern sweep across the matrix.

### 3.6 Stop and reset

Three levels of "make it stop":

```bash
# 1. Stop: signal the running script to halt, cancel timers, stop matrix output
curl -sS -X POST "$BASE_URL/api/js/stop"

# 2. Soft reset: cancel timers, clear stop flag, re-bootstrap runtime (VM stays alive)
curl -sS -X POST "$BASE_URL/api/js/reset"

# 3. Hard reset: tear down VM entirely and recreate from scratch
curl -sS -X POST "$BASE_URL/api/js/reset-hard"
```

Use **stop** when you want to halt an animation but keep the runtime state intact (registered animations, variables). Use **soft reset** to get back to a clean slate without the cost of VM recreation. Use **hard reset** as a recovery path if the VM is in a bad state or you suspect memory corruption.


## 4. Runtime Architecture

Understanding how scripts execute helps you write reliable animations and debug problems faster.

### 4.1 Task ownership model

The firmware is structured around strict task boundaries. Each FreeRTOS task owns exactly one subsystem and never directly calls into another task's domain:

```
                           ┌──────────────┐
                           │  HTTP Server  │
                           │  (parse+dispatch)
                           └──────┬───────┘
                                  │ queue post
                           ┌──────▼───────┐
                           │  JS Service   │
                           │  Task         │
                           │  (VM owner)   │
                           └──────┬───────┘
                      eval/call   │   timer schedule
                 ┌────────────────┼────────────────┐
                 │                │                 │
          ┌──────▼──────┐  ┌─────▼──────┐  ┌──────▼──────┐
          │  MicroQuickJS│  │ Timer Task │  │Matrix Engine│
          │  VM          │  │ (schedule  │  │  Task       │
          │              │  │  callbacks)│  │  (SPI+anim) │
          └──────────────┘  └────────────┘  └─────────────┘
```

Key rules:
- **HTTP handlers** only parse requests and post work items to the JS service queue. They never call `JS_Eval` directly.
- **JS service task** is the sole owner of all `JS_Eval` and `JS_Call` invocations. This serializes all JS execution and prevents data races inside the VM.
- **Timer task** posts scheduled callback messages back into the JS service queue. It never executes JS directly.
- **Matrix engine task** owns the SPI bus and framebuffer hardware writes. JS controls the matrix through engine API calls that cross the task boundary safely.

This means your JavaScript always runs single-threaded, in-order, on the JS service task. There is no concurrent access to your variables, no locking needed in script code. The firmware handles all the concurrency underneath.

### 4.2 Evaluation lifecycle

When you `POST /api/js/eval`, here is what happens:

1. HTTP handler reads the request body (up to 4096 bytes by default).
2. An eval job is posted to the JS service queue with a timeout deadline (default 250 ms).
3. The JS service task picks up the job, calls `JS_Eval`, and captures the result.
4. If the script returns a value, it is serialized to a string and placed in the `output` field of the JSON response.
5. If the script throws an exception or exceeds the deadline, `ok` is `false` and `error` describes what went wrong.

**Important:** the 250 ms timeout applies to the synchronous evaluation of your script, not to timers scheduled by it. A script like `(function(){ every(40, draw); return "started"; })()` completes instantly (it just schedules a timer and returns), even though the `draw` callback will fire indefinitely. Timer callbacks have their own per-invocation timeout (also configurable, default 250 ms).

### 4.3 Timer callback system

Timers are the backbone of animations. When you call `setTimeout(fn, ms)` or `ctx.every(ms, fn)`, the firmware:

1. Allocates a timer ID from a ring of 1024 slots.
2. Starts a FreeRTOS timer with the requested delay.
3. When the timer fires, the timer task posts a callback job to the JS service queue.
4. The JS service task executes the callback function with its own timeout deadline.

The 1024-slot ring prevents unbounded ID growth during long-running animations. When all slots are occupied, `setTimeout` throws an error. In practice, `every()` reuses slots as callbacks fire and reschedule, so you will see the slot count plateau rather than grow.

You can monitor timer pressure through `/api/js/status`:
- `timer_cb_keys`: current number of allocated timer slots
- `timer_cb_active`: timers currently waiting to fire
- `timer_cb_keys_high_water`: peak slot count since last reset

### 4.4 Cancellation semantics

There are two distinct stop mechanisms, and confusing them is the most common source of "my animation does nothing" bugs:

**Local stop** (`matrix.stop()` from JS): Cancels the current matrix engine mode and pending JS timers. Does **not** set the cooperative stop flag. After calling `matrix.stop()`, a new animation can start immediately and `matrix.shouldStop()` will return `false`.

**Global stop** (`POST /api/js/stop`, `js stop` console, or external runtime stop): Sets the cooperative stop flag so that `matrix.shouldStop()` returns `true`. Also cancels timers and stops the matrix engine. This is the "hard kill switch" for runaway scripts.

The reason this distinction exists: most animation scripts call `matrix.stop()` at startup to clear any previous animation before starting their own. If `matrix.stop()` also set the cooperative stop flag, the new animation would immediately see `shouldStop() === true` and exit on its first frame. This was an actual bug that was fixed during development.

### 4.5 Memory model

The JS VM runs inside a fixed-size memory arena (default 98304 bytes / 96 KB). This is not a growing heap -- it is allocated once at boot and never resized. The firmware includes a fallback mechanism that steps the arena size down by 4 KB increments (minimum 32 KB) if the initial allocation fails due to heap fragmentation.

96 KB is enough for moderately complex scripts with a few hundred variables and small arrays, but not for large data structures. The Game of Life example (`02-life-torus.js`) allocates two 96x8 state buffers and runs comfortably, but doubling the grid size would push against the limit.

Monitor memory pressure with:

```bash
curl -sS "$BASE_URL/api/js/mem"    # Detailed VM memory breakdown
curl -sS "$BASE_URL/api/js/status"  # heap_free_8bit, heap_min_free_8bit
```

`heap_min_free_8bit` shows the lowest free heap watermark since boot -- if this approaches zero, your scripts are at risk of allocation failures.


## 5. JavaScript API Reference

The runtime injects a global `matrix` object and several timing helpers at boot. This section documents every available function.

### 5.1 Framebuffer operations

These functions write to the matrix framebuffer in **script mode**. The framebuffer is a 96x8 bit array stored in firmware memory. Nothing reaches the physical LEDs until you call `present()`.

| Function                          | Returns   | Description                                     |
|-----------------------------------|-----------|-------------------------------------------------|
| `matrix.clear()`                  | `boolean` | Set all pixels to off                           |
| `matrix.fill(on)`                 | `boolean` | Set all pixels to `on` (1) or off (0)           |
| `matrix.setPixel(x, y, on)`      | `boolean` | Set pixel at column `x`, row `y`                |
| `matrix.getPixel(x, y)`          | `boolean` | Read current pixel state                        |
| `matrix.present()`               | `boolean` | Flush framebuffer to hardware over SPI          |

Coordinate system:
- `x` ranges from `0` (leftmost column) to `95` (rightmost column, for 12-module chain).
- `y` ranges from `0` (top row) to `7` (bottom row).
- Out-of-range coordinates are silently ignored.

The clear-draw-present cycle is the fundamental pattern for all pixel-level animations:

```javascript
matrix.clear();                         // wipe the buffer
for (var x = 0; x < matrix.width(); x += 2) {
  matrix.setPixel(x, 0, 1);            // draw a dotted line
}
matrix.present();                       // push to LEDs
```

### 5.2 Geometry and status

| Function              | Returns  | Description                                       |
|-----------------------|----------|---------------------------------------------------|
| `matrix.width()`     | `number` | Canvas width in pixels (typically `96`)            |
| `matrix.height()`    | `number` | Canvas height in pixels (always `8`)               |
| `matrix.status()`    | `object` | Full engine status as a JS object                  |
| `matrix.statusJson()`| `string` | Same as `status()` but pre-serialized to JSON      |

The status object contains:

```javascript
{
  ok: true,             // engine initialized
  ready: true,          // hardware ready
  mode: "idle",         // "idle"|"text"|"scroll"|"drop"|"script"
  text: "",             // current text buffer content
  chain_len: 12,        // number of MAX7219 modules
  width: 96,            // chain_len * 8
  height: 8,
  spi_hz: 100000,       // current SPI clock speed
  intensity: 3,         // brightness 0..15
  test_mode: false,     // MAX7219 test mode flag
  fps: 15,              // animation frames per second
  pause_ms: 200,        // inter-cycle pause
  repeat_count: 0,      // 0 = infinite, >0 = finite cycles
  reverse_modules: false,
  flip_vertical: false,
  stop_requested: false // cooperative stop flag
}
```

### 5.3 Text and built-in animations

These functions delegate to the firmware's C animation engine. They are simpler than pixel-level drawing but less flexible.

| Function                           | Returns   | Description                              |
|------------------------------------|-----------|------------------------------------------|
| `matrix.setText(text)`             | `boolean` | Display static text immediately           |
| `matrix.startScroll(text, opts)`   | `boolean` | Start scrolling text animation            |
| `matrix.startDrop(text, opts)`     | `boolean` | Start drop-bounce text animation          |
| `matrix.stop()`                    | `boolean` | Stop current animation (local stop)       |

**`startScroll` options:**

| Key       | Type      | Default       | Description                                    |
|-----------|-----------|---------------|------------------------------------------------|
| `fps`     | `integer` | engine default (15) | Frame rate; clamped to `1..60`          |
| `pauseMs` | `integer` | `200`         | Pause between scroll cycles (ms)               |
| `repeat`  | `integer` | `0`           | Cycle count; `0` = infinite                    |
| `wave`    | `boolean` | `false`       | Sinusoidal vertical modulation during scroll   |

**`startDrop` options:**

| Key       | Type      | Default       | Description                                    |
|-----------|-----------|---------------|------------------------------------------------|
| `fps`     | `integer` | engine default | Frame rate; clamped to `1..60`                |
| `pauseMs` | `integer` | `200`         | Pause between drop cycles (ms)                 |
| `repeat`  | `integer` | `0`           | Cycle count; `0` = infinite                    |

Examples:

```javascript
// Static text
matrix.setText("HELLO");

// Smooth wave scroll, 3 cycles then stop
matrix.startScroll("HELLO WIFI", { fps: 20, pauseMs: 250, repeat: 3, wave: true });

// Infinite drop-bounce
matrix.startDrop("DROP", { fps: 18, pauseMs: 400, repeat: 0 });
```

Text is limited to 64 characters. Lowercase is normalized to uppercase. Characters outside printable ASCII (32..126) are rendered as spaces.

### 5.4 Device settings

| Function                                          | Returns   | Description                           |
|---------------------------------------------------|-----------|---------------------------------------|
| `matrix.setIntensity(value)`                      | `boolean` | Set brightness, `0` (dim) to `15` (max) |
| `matrix.setOrientation(reverseModules, flipVert)` | `boolean` | Mirror/flip the display               |

These take effect immediately and persist until changed or power-cycled.

```javascript
matrix.setIntensity(8);                // mid brightness
matrix.setOrientation(false, true);    // flip vertically (for upside-down mounting)
```

### 5.5 Timing primitives

These are blocking calls that run on the JS service task. Use them for tight frame-paced loops where you want deterministic timing without timer callback overhead.

| Function                         | Returns   | Description                                        |
|----------------------------------|-----------|----------------------------------------------------|
| `matrix.nowMs()`                 | `number`  | Milliseconds since boot                            |
| `matrix.nowUs()`                 | `number`  | Microseconds since boot                            |
| `matrix.sleepMs(ms)`             | `boolean` | Block JS execution for `ms` milliseconds           |
| `matrix.sleepUntilUs(targetUs)`  | `boolean` | Block until the microsecond clock reaches `targetUs` |
| `matrix.shouldStop()`            | `boolean` | Check cooperative stop flag                        |

Both sleep functions check the stop flag internally and return early if stop is requested. This makes tight loops responsive to external stop commands:

```javascript
(function () {
  var next = matrix.nowUs();
  var period = 33333;  // ~30 FPS
  var x = 0;
  while (!matrix.shouldStop()) {
    matrix.clear();
    matrix.setPixel(x, 3, 1);
    matrix.present();
    x = (x + 1) % matrix.width();
    next += period;
    matrix.sleepUntilUs(next);
  }
})();
```

**When to use blocking sleep vs. timers:** Blocking sleep is simpler and gives tighter frame pacing, but it holds the JS service task hostage -- no other eval requests can be processed while a sleep loop runs. Timer-based animations (`ctx.every(...)`) yield control between frames, allowing status queries and other eval calls to interleave. For most animations, timers are the better choice.


### 5.6 Global timer helpers

These are low-level primitives available at global scope. For production animations, prefer the `ctx.every(...)` / `ctx.timeout(...)` equivalents inside the animation registry (section 5.7), which track handles automatically and clean up on stop.

| Function                    | Returns  | Description                                            |
|-----------------------------|----------|--------------------------------------------------------|
| `setTimeout(fn, ms)`        | `number` | Schedule `fn` to run once after `ms` milliseconds      |
| `clearTimeout(id)`          | `void`   | Cancel a pending timeout by its ID                     |
| `every(ms, fn)`             | `object` | Convenience wrapper: calls `fn` every `ms` ms          |
| `cancel(handleOrId)`        | `void`   | Cancel a timeout ID or an `every()` handle             |
| `print(...args)`            | `void`   | Print to serial console (useful for debugging)         |
| `gc()`                      | `void`   | Trigger VM garbage collection                          |

`setTimeout` ms values are clamped to `0..3600000` (one hour max). The `every()` helper returns an object with a `.cancel()` method. Internally, `every()` reschedules itself with `setTimeout` after each invocation -- it is not a separate primitive.

Timer IDs are drawn from a 1024-slot ring. If all slots are in use, `setTimeout` throws an error. In practice, `every()` reuses its slot each tick, so a single repeating animation uses only one slot at steady state.


### 5.7 Animation registry API (`matrix.anim`)

The animation registry is the recommended way to write reusable, well-behaved animations. It solves three problems that raw timers leave to the script author:

1. **Resource tracking**: timers and handles created inside the animation are automatically cancelled when the animation stops.
2. **Exclusive ownership**: starting a new animation automatically stops the previous one -- no stale timers left running.
3. **Cleanup hooks**: animations can register teardown logic (clear display, restore state) that runs reliably on stop.

#### Registry methods

| Method                                | Returns              | Description                                           |
|---------------------------------------|----------------------|-------------------------------------------------------|
| `matrix.anim.register(name, startFn)` | `void`               | Register a named animation                            |
| `matrix.anim.unregister(name)`        | `void`               | Remove a registered animation                         |
| `matrix.anim.start(name, opts)`       | `void`               | Start a registered animation with options             |
| `matrix.anim.stop()`                  | `void`               | Stop the currently running animation                  |
| `matrix.anim.clear()`                 | `void`               | Unregister all animations                             |
| `matrix.anim.list()`                  | `string[]`           | Names of all registered animations                    |
| `matrix.anim.current()`              | `string` or `null`   | Name of the currently running animation               |
| `matrix.anim.status()`               | `object`             | `{ current, registered, tracked }` summary            |

#### The `startFn` contract

When you call `matrix.anim.register(name, startFn)`, the `startFn` is stored but not executed. When you later call `matrix.anim.start(name, opts)`, the registry:

1. Stops any currently running animation (cleanup hooks, timer cancellation).
2. Switches the matrix engine to script mode.
3. Calls `startFn(ctx)` where `ctx` is a fresh animation context.
4. If `startFn` returns a function, that function is registered as the cleanup callback.

`startFn` can also be an object `{ start: function(ctx) { ... } }` for compatibility, but the function form is simpler and preferred.

#### The animation context (`ctx`)

The `ctx` object passed to your start function provides everything an animation needs:

| Property / Method          | Description                                                    |
|----------------------------|----------------------------------------------------------------|
| `ctx.matrix`               | The `matrix` API object (same as global `matrix`)              |
| `ctx.opts`                 | The options object passed to `matrix.anim.start()`             |
| `ctx.shouldStop()`         | Check cooperative stop flag                                    |
| `ctx.every(ms, fn)`        | Schedule repeating callback (handle is auto-tracked)           |
| `ctx.timeout(ms, fn)`      | Schedule one-shot callback (handle is auto-tracked)            |
| `ctx.track(handleOrId)`    | Manually track a handle/ID for cleanup                         |
| `ctx.onCleanup(fn)`        | Register a cleanup callback (called in reverse order on stop)  |
| `ctx.nowMs()`              | Milliseconds since boot                                        |
| `ctx.nowUs()`              | Microseconds since boot                                        |

The key difference between `ctx.every()` and the global `every()`: handles created through `ctx` are automatically cancelled when the animation stops. With global timers, you must cancel them yourself.

#### Anatomy of a well-structured animation

```javascript
(function () {
  var name = "my-effect";

  // Remove any previous registration (safe to call if not registered)
  matrix.anim.unregister(name);

  // Register the animation
  matrix.anim.register(name, function (ctx) {
    // -- initialization --
    var w = ctx.matrix.width();
    var h = ctx.matrix.height();
    var frameMs = ((ctx.opts && ctx.opts.frameMs) | 0) || 40;
    var tick = 0;

    // -- frame loop (auto-tracked timer) --
    ctx.every(frameMs, function () {
      if (ctx.shouldStop()) return;  // respect external stop requests

      ctx.matrix.clear();
      // ... draw frame based on tick ...
      ctx.matrix.present();
      tick++;
    });

    // -- cleanup (called when animation stops) --
    return function () {
      ctx.matrix.clear();
      ctx.matrix.present();
    };
  });

  // Start the animation
  matrix.anim.start(name, { frameMs: 40 });

  // Return status for API response
  return JSON.stringify(matrix.anim.status());
})()
```

#### Why `unregister` before `register`?

If you re-send the same script to the device, the animation name is already registered from the previous eval. Calling `unregister` first is a defensive pattern that prevents "name already registered" conflicts. It is safe to call `unregister` on a name that does not exist.

#### Multiple animations in one script

You can register several animations in a single eval and switch between them:

```javascript
(function () {
  matrix.anim.register("dots", function (ctx) {
    var x = 0;
    ctx.every(50, function () {
      if (ctx.shouldStop()) return;
      ctx.matrix.clear();
      ctx.matrix.setPixel(x, 3, 1);
      ctx.matrix.present();
      x = (x + 1) % ctx.matrix.width();
    });
  });

  matrix.anim.register("fill-flash", function (ctx) {
    var on = false;
    ctx.every(200, function () {
      if (ctx.shouldStop()) return;
      on = !on;
      ctx.matrix.fill(on ? 1 : 0);
      ctx.matrix.present();
    });
  });

  matrix.anim.start("dots", {});
  return JSON.stringify(matrix.anim.list());
})()
```

Then switch from outside: `curl -sS -X POST "$BASE_URL/api/js/eval" --data-binary 'matrix.anim.start("fill-flash", {})'`


## 6. REST API Reference

All endpoints return JSON. The base path is the device's HTTP root (e.g., `http://192.168.3.119`).

### 6.1 JS runtime endpoints

#### `POST /api/js/eval`

Execute JavaScript source code on the device.

**Request body:** Raw JavaScript text (not JSON-wrapped). Max size: 4096 bytes (configurable via `CONFIG_TUTORIAL_0067_JS_MAX_BODY`).

**Success response:**

```json
{
  "ok": true,
  "output": "started",
  "error": null,
  "timed_out": false
}
```

**Error response (exception):**

```json
{
  "ok": false,
  "output": "",
  "error": "TypeError: matrix.nonExistent is not a function",
  "timed_out": false
}
```

**Error response (timeout):**

```json
{
  "ok": false,
  "output": "",
  "error": "evaluation timed out",
  "timed_out": true
}
```

The timeout applies only to the synchronous part of evaluation. Scheduled timers continue after the eval response is returned.

#### `POST /api/js/stop`

Request cooperative stop of all running JS activity. Sets the stop flag checked by `matrix.shouldStop()`, cancels pending timers, runs animation cleanup hooks, and stops the matrix engine.

```json
{"ok": true}
```

#### `POST /api/js/reset`

Soft reset: cancel all timers, clear the stop flag, re-run the bootstrap script to restore the `matrix` object and helpers. The VM itself stays alive -- global variables from previous evals may still exist, but the runtime is in a clean operational state.

```json
{"ok": true}
```

#### `POST /api/js/reset-hard`

Hard reset: destroy the VM entirely and create a new one. This is the most thorough cleanup but also the most expensive (arena reallocation). Use when soft reset does not recover a broken state.

```json
{"ok": true}
```

#### `GET /api/js/status`

Runtime observability. Returns a JSON object with these fields:

| Field                      | Type     | Description                                                       |
|----------------------------|----------|-------------------------------------------------------------------|
| `started`                  | `bool`   | VM is initialized and running                                     |
| `busy`                     | `bool`   | An eval job is currently executing                                |
| `stop_requested`           | `bool`   | Cooperative stop flag is set                                      |
| `last_timed_out`           | `bool`   | Most recent eval exceeded its deadline                            |
| `eval_count`               | `int`    | Total number of evals since last reset                            |
| `last_eval_ms`             | `int`    | Duration of the most recent eval in milliseconds                  |
| `timer_cb_keys`            | `int`    | Current timer slots in use (out of 1024 ring)                     |
| `timer_cb_active`          | `int`    | Timers currently waiting to fire                                  |
| `timer_cb_keys_high_water` | `int`    | Peak timer slot count since last reset                            |
| `animations_registered`    | `int`    | Number of animations registered via `matrix.anim.register()`      |
| `active_animation`         | `string` | Name of the currently running animation, or empty                 |
| `heap_free_8bit`           | `int`    | Current free heap bytes (8-bit capable)                           |
| `heap_largest_free_8bit`   | `int`    | Largest contiguous free block                                     |
| `heap_min_free_8bit`       | `int`    | Lowest free heap watermark since boot                             |
| `last_error`               | `string` | Most recent error message (128 chars max)                         |

This endpoint is your primary tool for monitoring long-running animations. Watch `timer_cb_keys` for slot exhaustion and `heap_min_free_8bit` for memory pressure.

#### `GET /api/js/mem`

Returns a plain-text memory dump from the QuickJS VM internals. Useful for detailed allocation debugging when `/api/js/status` heap numbers look suspicious.

### 6.2 Matrix engine endpoints

These control the built-in C animation engine directly, without involving JS.

#### `GET /api/matrix/status`

Returns the full engine state (see section 5.2 for field descriptions).

#### `POST /api/matrix/text`

Display static text. Body:

```json
{"text": "HELLO"}
```

#### `POST /api/matrix/anim`

Start a built-in animation. Body:

```json
{
  "mode": "scroll",
  "text": "HELLO WORLD",
  "fps": 20,
  "pause_ms": 250,
  "repeat_count": 0
}
```

| `mode` value | Animation style                                            |
|--------------|------------------------------------------------------------|
| `scroll`     | Smooth horizontal scroll, text re-enters from the right    |
| `wave`       | Scroll with sinusoidal vertical modulation                 |
| `drop`       | Characters drop in from top with bounce effect             |

`repeat_count`: `0` runs forever, any positive integer runs that many cycles then auto-stops to idle.

#### `POST /api/matrix/stop`

Stop the current matrix engine animation and return to idle. Returns the updated status.


## 7. Console Command Reference

The Wi-Fi serial console (accessible via `idf.py monitor` or any serial terminal at the device's USB port) provides the same capabilities as the REST API.

### 7.1 JS console commands

```
js status                    Show runtime status (same as GET /api/js/status)
js eval <CODE>               Evaluate JavaScript expression
js reset                     Soft reset (same as POST /api/js/reset)
js reset hard                Hard reset (same as POST /api/js/reset-hard)
js stop                      Stop running scripts (same as POST /api/js/stop)
js mem                       Show VM memory dump
js examples                  Print example commands
```

Console eval examples:

```
js eval matrix.setText('HELLO')
js eval matrix.startScroll('HELLO WIFI', {fps:20,pauseMs:250,repeat:2,wave:true})
js eval matrix.width()
js eval JSON.stringify(matrix.anim.list())
```

Note: console eval is limited to single-line expressions. For multi-line scripts, use the REST eval endpoint with file upload.

### 7.2 Matrix console commands

```
matrix status                        Show engine status
matrix examples                      Print example commands
matrix text <TEXT>                    Display static text
matrix scroll on <TEXT> [fps] [pause_ms] [repeat_count]    Start horizontal scroll
matrix scroll wave <TEXT> [fps] [pause_ms] [repeat_count]  Start wave scroll
matrix scroll off                    Stop scrolling
matrix anim drop <TEXT> [fps] [pause_ms] [repeat_count]    Start drop animation
matrix anim off                      Stop animation
matrix test on|off                   Toggle MAX7219 test mode (all LEDs on)
matrix intensity <0..15>             Set brightness
matrix spi [hz]                      Query or set SPI clock speed
matrix chain [n]                     Query or set chain length
matrix reverse on|off                Reverse module order
matrix flipv on|off                  Flip display vertically
```


## 8. Text and Glyph Behavior

### 8.1 Character set

The built-in font covers printable ASCII (codepoints 32 through 126). This includes:

- Uppercase letters: `A-Z`
- Digits: `0-9`
- Punctuation: `. , ; : ! ? ' "`
- Brackets: `( ) [ ] { } < >`
- Symbols: `@ # $ % ^ & * + - = _ / \ | ~`

Lowercase letters are automatically normalized to uppercase in the rendering path. Characters outside the supported range are silently replaced with a space glyph.

### 8.2 String length limits

The internal text buffer holds 64 characters (`char[65]` including null terminator). `startScroll` and `startDrop` truncate text to this limit. `setText` static mode effectively displays only as many characters as the chain can show at once (12 characters for the default 12-module chain).

### 8.3 Empty strings

- `matrix.setText("")` clears the visible text area (valid).
- `matrix.startScroll("")` and `matrix.startDrop("")` return `false` (animation requires non-empty text).

To blank the display entirely:

```javascript
matrix.stop();
matrix.clear();
matrix.present();
```


## 9. Practical Script Patterns

### 9.1 Minimal safe animation (recommended template)

This is the starting point for any new animation. Copy it, replace the draw logic, and adjust `frameMs`:

```javascript
(function () {
  var name = "my-anim";
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

Things to note:
- Wrapped in an IIFE so it evaluates cleanly.
- `unregister` before `register` for idempotent re-sends.
- `ctx.shouldStop()` checked every frame for responsive cancellation.
- Cleanup function clears the display when the animation is stopped.
- Options passed through `ctx.opts` for external control.
- Returns status JSON so the eval response confirms the animation started.

### 9.2 Bouncing text with built-in wave scroll

When you just want scrolling text, the built-in engine is simpler and more efficient than a JS pixel loop:

```javascript
(function () {
  matrix.stop();
  matrix.startScroll("BOUNCE", { fps: 20, pauseMs: 250, repeat: 0, wave: true });
  return "wave scroll started";
})()
```

### 9.3 Math-heavy pixel animation (plasma pattern)

This pattern from `01-plasma-ribbon.js` shows how to do per-pixel computation:

```javascript
(function () {
  var name = "plasma";
  matrix.anim.unregister(name);
  matrix.anim.register(name, function (ctx) {
    var w = ctx.matrix.width(), h = ctx.matrix.height();
    var tick = 0;
    ctx.every(10, function () {
      if (ctx.shouldStop()) return;
      ctx.matrix.clear();
      for (var x = 0; x < w; x++) {
        // Two sine waves with different phases create interference ribbons
        var v1 = Math.sin((x + tick) * 0.12) * 3.5;
        var v2 = Math.sin((x - tick * 0.7) * 0.08) * 3.5;
        var y1 = Math.round(3.5 + v1);
        var y2 = Math.round(3.5 + v2);
        if (y1 >= 0 && y1 < h) ctx.matrix.setPixel(x, y1, 1);
        if (y2 >= 0 && y2 < h) ctx.matrix.setPixel(x, y2, 1);
      }
      ctx.matrix.present();
      tick++;
    });
    return function () { ctx.matrix.clear(); ctx.matrix.present(); };
  });
  matrix.anim.start(name, {});
  return JSON.stringify(matrix.anim.status());
})()
```

### 9.4 Cellular automaton (Game of Life on torus)

This pattern from `02-life-torus.js` shows buffer ping-pong for state simulation:

```javascript
(function () {
  var name = "life";
  matrix.anim.unregister(name);
  matrix.anim.register(name, function (ctx) {
    var w = ctx.matrix.width(), h = ctx.matrix.height();
    // Two flat arrays for double-buffering
    var a = [], b = [];
    for (var i = 0; i < w * h; i++) {
      a[i] = Math.random() < 0.35 ? 1 : 0;
      b[i] = 0;
    }
    var gen = 0;

    ctx.every(90, function () {
      if (ctx.shouldStop()) return;
      // Compute next generation with toroidal wrapping
      for (var y = 0; y < h; y++) {
        for (var x = 0; x < w; x++) {
          var n = 0;  // neighbor count
          for (var dy = -1; dy <= 1; dy++) {
            for (var dx = -1; dx <= 1; dx++) {
              if (dx === 0 && dy === 0) continue;
              var nx = (x + dx + w) % w;
              var ny = (y + dy + h) % h;
              n += a[ny * w + nx];
            }
          }
          var alive = a[y * w + x];
          b[y * w + x] = (alive && (n === 2 || n === 3)) || (!alive && n === 3) ? 1 : 0;
        }
      }
      // Swap buffers, render, and periodically reseed
      var tmp = a; a = b; b = tmp;
      ctx.matrix.clear();
      for (var y = 0; y < h; y++)
        for (var x = 0; x < w; x++)
          if (a[y * w + x]) ctx.matrix.setPixel(x, y, 1);
      ctx.matrix.present();
      gen++;
      if (gen % 45 === 0) {  // reseed to prevent extinction
        for (var i = 0; i < 40; i++) a[(Math.random() * w * h) | 0] = 1;
      }
    });
    return function () { ctx.matrix.clear(); ctx.matrix.present(); };
  });
  matrix.anim.start(name, {});
  return JSON.stringify(matrix.anim.status());
})()
```

### 9.5 Particle system (comet trails with decay)

From `03-comet-trails.js`, this pattern shows pseudo-brightness through dithering on a 1-bit display:

```javascript
// Concept: trail[] stores per-pixel intensity (0..255).
// Each frame: decay all trails, update comet positions, stamp new trail values.
// Render: threshold trail values into on/off with dithering.

// Dithering thresholds give 3 visual brightness levels on a binary display:
//   trail > 170  →  always on   (bright core)
//   trail > 95   →  checkerboard dither (medium glow)
//   trail > 45   →  sparse dither (faint tail)
//   trail <= 45  →  off
```

The key insight: since each LED is only on or off, you simulate brightness gradients by varying pixel density using `(x + y + tick) % N === 0` patterns. This creates a convincing visual decay effect.

### 9.6 Draw border and checker (orientation diagnostic)

```javascript
(function () {
  matrix.stop();
  matrix.clear();
  var w = matrix.width(), h = matrix.height();
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

Useful for verifying module order and vertical orientation after physical rewiring.


## 10. Triggering Scripts: End-to-End Examples

### 10.1 Inline REST eval

```bash
curl -sS -X POST "$BASE_URL/api/js/eval" --data-binary \
  '(function(){ matrix.setText("READY"); return matrix.statusJson(); })()'
```

### 10.2 Multi-line script via shell heredoc

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
      for (var x = phase; x < ctx.matrix.width(); x += 2)
        ctx.matrix.setPixel(x, 2, 1);
      ctx.matrix.present();
      phase = (phase + 1) % 2;
    });
    return function () { ctx.matrix.clear(); ctx.matrix.present(); };
  });
  matrix.anim.start(name, {});
  return JSON.stringify(matrix.anim.status());
})()
JS
```

### 10.3 File-based eval

```bash
curl -sS -X POST "$BASE_URL/api/js/eval" \
  --data-binary @0067-esp-c3-led-matrix-http/examples/js/03-comet-trails.js
```

### 10.4 Playback helper script

The project includes a convenience script under the ticket workspace:

```bash
PLAY=ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_play_js_example.sh

$PLAY 01-plasma-ribbon     # wavy ribbon
$PLAY 02-life-torus        # cellular automaton
$PLAY 03-comet-trails      # bouncing particles with trails
```

### 10.5 Console eval (serial)

```
js eval matrix.setText('HELLO')
js eval matrix.startScroll('HELLO WIFI', {fps:20,pauseMs:250,repeat:0,wave:true})
```

Console eval accepts single-line expressions. For anything longer, use the REST endpoint.


## 11. Diagnostics and Troubleshooting

### 11.1 Visual diagnostic sequence

When something is not working, start with the structured diagnostic scripts rather than guessing. The sequence progresses from the simplest possible operation to complex animations, isolating failures precisely:

| Step | Script                     | What it tests                        | Expected result         |
|------|----------------------------|--------------------------------------|-------------------------|
| 0    | `diag/00-env-status.js`    | Runtime health and dimensions        | JSON with `width:96`    |
| 1    | `diag/01-all-on.js`        | Full framebuffer write + present     | All 768 LEDs lit        |
| 2    | `diag/02-all-off.js`       | Clear + present                      | All LEDs dark           |
| 3    | `diag/03-single-pixel.js`  | Coordinate mapping                   | Top-left pixel only     |
| 4    | `diag/04-border.js`        | Edge pixel addressing                | Rectangle outline       |
| 5    | `diag/05-checkerboard.js`  | Alternating pattern                  | Checkerboard grid       |
| 6    | `diag/06-walk-dot.js`      | Timer callbacks + frame pacing       | Dot sweeps left to right|
| 7    | `diag/07-text-test.js`     | Glyph rendering                      | Static "TEST" text      |
| 8    | `diag/08-scroll-test.js`   | Scroll animation engine              | Scrolling "HELLO 123"   |
| 9    | `diag/09-wave-test.js`     | Wave modulation flag                 | Wavy scrolling text     |
| 10   | `diag/10-stop-reset.js`    | Stop and cleanup semantics           | Display clears          |

Scripts are in `0067-esp-c3-led-matrix-http/examples/js/diag/`. Run them with:

```bash
curl -sS -X POST "$BASE_URL/api/js/eval" \
  --data-binary @0067-esp-c3-led-matrix-http/examples/js/diag/01-all-on.js
```

The first step that fails tells you exactly which subsystem is broken.

### 11.2 Common failure causes and fixes

**"API returns ok but no pixels change"**

Most likely cause: the animation callback cancelled itself on the first tick. This happens when `matrix.shouldStop()` returns `true` unexpectedly. Check whether something called global stop (`POST /api/js/stop`) before starting the animation. Fix: call `POST /api/js/reset` to clear the stop flag, then re-send the script.

**"Script starts but dies after a few seconds"**

Check `/api/js/status` -- look at `last_error` and `last_timed_out`. If `last_timed_out` is `true`, the timer callback exceeded the per-callback deadline (default 250 ms). This happens with computationally heavy frames. Possible fixes:
- Optimize the frame computation (fewer per-pixel operations, precomputed lookup tables).
- Increase `CONFIG_TUTORIAL_0067_JS_EVAL_TIMEOUT_MS` if the computation is inherently expensive.
- Reduce frame rate (longer `ctx.every()` interval).

Check serial logs for `InternalError: interrupted` -- this is the JS exception thrown when the timeout fires mid-execution.

**"Animation runs for minutes then stops"**

Check `timer_cb_keys` in `/api/js/status`. If it has reached 1024, timer slots are exhausted. This was a known issue with an earlier timer implementation and should be fixed in current firmware (IDs now reuse from a ring). If you see this, ensure you are running the latest firmware build.

**"`413 body too large` on eval"**

Your script exceeds the 4096-byte request body limit. Options:
- Minify the script (remove comments and whitespace).
- Increase `CONFIG_TUTORIAL_0067_JS_MAX_BODY` in Kconfig and rebuild.
- Split logic: send a setup script first, then trigger with a short eval.

**"Blank display from scroll/drop animation"**

`startScroll` and `startDrop` require non-empty text. Passing `""` returns `false` without starting anything. Also verify the text does not consist entirely of unsupported characters (which would render as spaces).

**"JS init failed" or "arena alloc failed"**

The VM could not allocate its memory arena at boot. The firmware will attempt progressively smaller arenas (stepping down by 4 KB) until reaching 32 KB minimum. If even that fails, free heap is critically low. Reduce other memory consumers or increase the ESP-IDF heap configuration.

### 11.3 Runtime health checks

A quick three-command health probe:

```bash
# 1. Is the runtime alive and idle?
curl -sS "$BASE_URL/api/js/status" | python3 -m json.tool

# 2. Is the matrix engine ready?
curl -sS "$BASE_URL/api/matrix/status" | python3 -m json.tool

# 3. How is VM memory looking?
curl -sS "$BASE_URL/api/js/mem"
```

For ongoing monitoring of a long-running animation, poll status periodically:

```bash
while true; do
  curl -sS "$BASE_URL/api/js/status" | python3 -c "
import sys,json; d=json.load(sys.stdin)
print(f\"keys={d['timer_cb_keys']} active={d['timer_cb_active']} hw={d['timer_cb_keys_high_water']} heap_min={d['heap_min_free_8bit']} anim={d['active_animation']}\")"
  sleep 5
done
```


## 12. Configuration Reference (Kconfig)

All configuration knobs are defined in `main/Kconfig.projbuild` and can be changed via `idf.py menuconfig` under the `Tutorial 0067` menu, or by setting values in `sdkconfig.defaults` before building.

### 12.1 JavaScript runtime settings

| Config key                               | Default  | Description                                              |
|------------------------------------------|----------|----------------------------------------------------------|
| `CONFIG_TUTORIAL_0067_JS_MEM_BYTES`      | `98304`  | JS VM arena size in bytes (96 KB). Larger = more script headroom, but competes with system heap. |
| `CONFIG_TUTORIAL_0067_JS_MAX_BODY`       | `4096`   | Maximum HTTP request body for `/api/js/eval`. Increase for larger scripts. |
| `CONFIG_TUTORIAL_0067_JS_EVAL_TIMEOUT_MS`| `250`    | Deadline for synchronous eval and per-timer-callback execution. Increase for heavy compute frames. |
| `CONFIG_TUTORIAL_0067_JS_MAX_TIMERS`     | `32`     | Maximum simultaneously active FreeRTOS timers for JS callbacks. |

### 12.2 Matrix hardware settings

| Config key                               | Default   | Description                                             |
|------------------------------------------|-----------|---------------------------------------------------------|
| `CONFIG_TUTORIAL_0067_MATRIX_PIN_MOSI`   | `4`       | SPI MOSI (DIN) GPIO                                    |
| `CONFIG_TUTORIAL_0067_MATRIX_PIN_CS`     | `5`       | SPI CS GPIO                                             |
| `CONFIG_TUTORIAL_0067_MATRIX_PIN_SCK`    | `6`       | SPI clock GPIO                                          |
| `CONFIG_TUTORIAL_0067_MATRIX_CHAIN_LEN`  | `12`      | Number of chained MAX7219 modules                       |
| `CONFIG_TUTORIAL_0067_MATRIX_SPI_HZ`     | `100000`  | SPI clock frequency in Hz                               |
| `CONFIG_TUTORIAL_0067_MATRIX_DEFAULT_FPS`| `15`      | Default animation frame rate                            |

### 12.3 HTTP server settings

| Config key                               | Default  | Description                                              |
|------------------------------------------|----------|----------------------------------------------------------|
| `CONFIG_TUTORIAL_0067_HTTP_MAX_BODY`     | `512`    | Max body for non-JS REST endpoints (matrix text/anim)    |

### 12.4 Tuning tips

**Need more script headroom?** Increase `JS_MEM_BYTES`, but watch `heap_min_free_8bit` -- if system heap drops below ~20 KB, Wi-Fi stability may degrade.

**Animations running too fast for the SPI bus?** Either reduce FPS or increase `MATRIX_SPI_HZ`. At 1 MHz, SPI transfer drops from ~15 ms to ~1.5 ms per frame, enabling smooth 60 FPS.

**Scripts too large for the body limit?** Increase `JS_MAX_BODY` to 8192 or higher. Memory cost is one-time per request buffer.

**Heavy frame computation timing out?** Increase `JS_EVAL_TIMEOUT_MS` to 500 or 1000. The trade-off is that a genuinely stuck script takes longer to detect.


## 13. Quick Reference Cheatsheet

```bash
# -- Lifecycle --
curl -sS -X POST "$BASE_URL/api/js/eval" --data-binary @script.js   # Run script
curl -sS -X POST "$BASE_URL/api/js/stop"                            # Stop everything
curl -sS -X POST "$BASE_URL/api/js/reset"                           # Soft reset
curl -sS -X POST "$BASE_URL/api/js/reset-hard"                      # Hard reset

# -- Monitoring --
curl -sS "$BASE_URL/api/js/status"                                   # Runtime health
curl -sS "$BASE_URL/api/matrix/status"                               # Engine state
curl -sS "$BASE_URL/api/js/mem"                                      # VM memory dump

# -- Built-in animations (no JS needed) --
curl -sS -X POST "$BASE_URL/api/matrix/text" \
  -H 'Content-Type: application/json' -d '{"text":"HELLO"}'
curl -sS -X POST "$BASE_URL/api/matrix/anim" \
  -H 'Content-Type: application/json' \
  -d '{"mode":"wave","text":"SCROLLING","fps":20,"pause_ms":250,"repeat_count":0}'
curl -sS -X POST "$BASE_URL/api/matrix/stop"
```

```javascript
// -- JS one-liners --
matrix.setText("HELLO")                                              // static text
matrix.startScroll("HI", {fps:20,pauseMs:250,repeat:0,wave:true})   // wave scroll
matrix.clear(); matrix.setPixel(0,0,1); matrix.present();           // pixel draw
matrix.setIntensity(8);                                              // brightness
matrix.width()                                                       // → 96
JSON.stringify(matrix.anim.status())                                 // anim state
```
