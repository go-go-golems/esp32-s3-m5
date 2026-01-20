---
Title: LED Pattern Engine + esp_console REPL (WS281x)
Ticket: MO-032-ESP32C6-LED-PATTERNS-CONSOLE
Status: active
Topics:
    - esp32
    - animation
    - console
    - serial
    - esp-idf
    - usb-serial-jtag
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0030-cardputer-console-eventbus/main/app_main.cpp
      Note: esp_console USB Serial/JTAG REPL bootstrap example
    - Path: 0043-xiao-esp32c6-ws2811-4led-d1/inspiration/led_patterns.h
      Note: Reference config structs and state split
    - Path: 0043-xiao-esp32c6-ws2811-4led-d1/main/main.c
      Note: Known-good WS281x driver + rainbow loop
    - Path: 0043-xiao-esp32c6-ws2811-4led-d1/main/ws281x_encoder.c
      Note: Custom RMT WS281x encoder
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/led_console.c
      Note: Implementation of the proposed esp_console verbs
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/led_patterns.c
      Note: Implementation of the pattern algorithms described in this doc
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/led_task.c
      Note: Implementation of the queue-driven single-owner animation task
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/led_ws281x.c
      Note: Implementation of WS281x driver layer described in this doc
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/main.c
      Note: Implementation target for this design
    - Path: ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/sources/local/patterns.jsx
      Note: Simulator patterns + parameter baselines
ExternalSources: []
Summary: 'Design for a WS281x LED animation engine on ESP32-C6: pattern configs (rainbow/chase/breathing/sparkle), a realtime task with queue-driven reconfiguration, and an esp_console REPL over USB Serial/JTAG including staged driver config + apply.'
LastUpdated: 2026-01-20T14:51:22.075430826-05:00
WhatFor: Design a WS281x LED pattern engine for ESP32-C6 with realtime animation task + queue-driven live reconfiguration and an esp_console REPL over USB Serial/JTAG.
WhenToUse: Use when implementing LED animations + runtime console control for ESP-IDF firmwares targeting WS2811/WS2812-style addressable LED chains.
---







# LED Pattern Engine + esp_console REPL (WS281x)

## Executive Summary

This document proposes a portable LED pattern engine for ESP-IDF (ESP32-C6 focus) that:

- Drives WS2811/WS2812-style 1-wire RGB chains (“WS281x”) via the ESP-IDF **RMT TX** peripheral.
- Renders multiple animation patterns in a **dedicated realtime task** with a stable, time-based update loop.
- Supports **live reconfiguration** (pattern changes + parameter tweaks + driver/timing tweaks) using a **single-owner task model** and a **FreeRTOS queue**.
- Exposes an interactive **`esp_console` REPL** over **USB Serial/JTAG** with a small set of verbs designed for “operator ergonomics” (fast typing, consistent defaults, introspection via `status`).

The immediate scope is: patterns **rainbow**, **chase**, **breathing**, **sparkle**, inspired by:

- A simulator implementation in `sources/local/patterns.jsx`.
- A known-good ESP32-C6 rainbow driver in `0043-xiao-esp32c6-ws2811-4led-d1/main/main.c`.
- A pattern-engine reference design in `0043-xiao-esp32c6-ws2811-4led-d1/inspiration/led_patterns.[ch]`.

## Problem Statement

We need a firmware-side system that can:

1. Render multiple WS281x LED animations at a stable refresh cadence.
2. Switch patterns and adjust their parameters at runtime, *without rebooting*.
3. Allow runtime tuning of the WS281x driver configuration (GPIO, LED count, timings, and byte order), while keeping the driver behavior safe and comprehensible.
4. Provide an operator-facing interactive control plane that works reliably on ESP32 devices where UART pins may be repurposed (e.g., Cardputer/S3); therefore the console should default to **USB Serial/JTAG**.

Constraints and engineering realities:

- WS281x waveforms are timing-sensitive, but using the RMT peripheral means *bit timing is hardware-driven*; the realtime risk is mostly about **frame scheduling** and **avoiding long blocking work** in the wrong thread.
- Runtime mutability differs by parameter:
  - Pattern parameters (speed, colors, density, etc.) should be safe to change at any time.
  - Driver parameters like GPIO, LED count, timing constants require **driver reinitialization**, which needs careful sequencing.
- The pattern engine should be structured so that “control plane” code (console handlers) never needs to directly touch internal pattern state, to avoid concurrency hazards.

> **Callout — Why queue + single owner task?**
>
> A queue enforces a single-threaded ownership model: the animation task “owns” the pattern state, frame buffer, and hardware driver. All other code talks to it via messages, eliminating the need for mutex-heavy designs and reducing the likelihood of tearing/stalls.

## Proposed Solution

### High-level Architecture

**One animation task** owns:

- A **driver** object: RMT channel + WS281x encoder + pixel buffer + color ordering.
- A **pattern engine** object: active pattern type + config union + runtime state.
- A stable time base (`esp_timer_get_time()`), and a frame period (`frame_ms`).

**Other tasks** (including `esp_console` command handlers) do not mutate these objects directly. They send **messages** to the animation task via a **FreeRTOS queue**.

```
esp_console cmd handler(s)
   |
   |  xQueueSend(led_q, led_msg)
   v
LED animation task (single owner)
   |
   | render pattern -> fill pixel buffer -> rmt_transmit()
   v
WS281x LED chain
```

### Reference Baselines in This Repo

This design intentionally anchors itself to existing working code:

- **Driver baseline** (RMT + WS281x encoder + time-based rainbow):
  - `0043-xiao-esp32c6-ws2811-4led-d1/main/main.c`
  - `0043-xiao-esp32c6-ws2811-4led-d1/main/ws281x_encoder.c`
- **Pattern config schema baseline** (unified config union + separate runtime state):
  - `0043-xiao-esp32c6-ws2811-4led-d1/inspiration/led_patterns.h`
- **Console baseline** (USB Serial/JTAG `esp_console` setup and a simple argv-style command):
  - `0030-cardputer-console-eventbus/main/app_main.cpp`
- **Simulator baseline** (pattern math + user-facing knobs):
  - `sources/local/patterns.jsx` (imported into this ticket)

### Fundamentals: WS281x “frames” and “colors”

> **Fundamental — A WS281x “frame”**
>
> A “frame” is the complete LED buffer written to the chain: `led_count × 3 bytes` for RGB (commonly GRB order on the wire). A reset/“latch” low pulse follows the frame (commonly ~50–80µs).

> **Fundamental — Brightness scaling**
>
> Most patterns are easiest to write in “full-scale” RGB and then apply:
> 1) optional per-pattern intensity modulation and
> 2) a global brightness scale.
>
> In embedded firmware, prefer integer math and avoid floats in hot loops.

### Pattern Configuration Schema (C)

The firmware needs a stable, inspectable configuration format that:

- supports multiple pattern types (union),
- decouples configuration (static-ish) from state (dynamic),
- is easy to “set partially” (from console),
- is safe to send over a queue.

The inspiration engine in `inspiration/led_patterns.h` already suggests a good base. Below is a slightly extended, “firmware-friendly” variant that explicitly separates *driver config* from *pattern config*.

```c
typedef enum {
    LED_PATTERN_OFF = 0,
    LED_PATTERN_RAINBOW,
    LED_PATTERN_CHASE,
    LED_PATTERN_BREATHING,
    LED_PATTERN_SPARKLE,
} led_pattern_type_t;

typedef struct { uint8_t r, g, b; } led_rgb8_t;

typedef struct {
    // “time knobs”
    uint8_t speed;          // normalized 1..10 (interpretation depends on pattern)

    // “color knobs”
    uint8_t saturation;     // 0..100
    uint8_t spread_x10;     // 10 => 1.0× 360° across strip
} led_rainbow_cfg_t;

typedef enum {
    LED_DIR_FORWARD = 0,
    LED_DIR_REVERSE,
    LED_DIR_BOUNCE,
} led_direction_t;

typedef struct {
    uint8_t speed;          // 1..10
    uint8_t tail_len;       // LEDs in tail (>=1)
    led_rgb8_t fg;          // head/tail color
    led_rgb8_t bg;          // background color
    led_direction_t dir;
    bool fade_tail;         // fade tail brightness by distance
} led_chase_cfg_t;

typedef enum {
    LED_CURVE_SINE = 0,
    LED_CURVE_LINEAR,
    LED_CURVE_EASE_IN_OUT,
} led_curve_t;

typedef struct {
    uint8_t speed;          // 1..10
    led_rgb8_t color;
    uint8_t min_bri;        // 0..255
    uint8_t max_bri;        // 0..255
    led_curve_t curve;
} led_breathing_cfg_t;

typedef enum {
    LED_SPARKLE_FIXED = 0,
    LED_SPARKLE_RANDOM,
    LED_SPARKLE_RAINBOW,
} led_sparkle_color_mode_t;

typedef struct {
    uint8_t speed;          // 1..10 (interpreted as fade/spawn dynamics)
    led_rgb8_t color;       // used for FIXED mode
    uint8_t density_pct;    // 0..100 (% chance per LED per frame)
    uint8_t fade_speed;     // 1..255 (brightness decrement per frame)
    led_sparkle_color_mode_t color_mode;
    led_rgb8_t background;
} led_sparkle_cfg_t;

typedef union {
    led_rainbow_cfg_t   rainbow;
    led_chase_cfg_t     chase;
    led_breathing_cfg_t breathing;
    led_sparkle_cfg_t   sparkle;
} led_pattern_cfg_u;

typedef struct {
    led_pattern_type_t type;
    uint8_t global_brightness_pct; // 1..100, applied last
    led_pattern_cfg_u u;
} led_pattern_cfg_t;
```

> **Callout — “Percent vs 0..255”: pick one and stick to it**
>
> The repo already uses both representations:
> - `0043/.../main.c` scales brightness in **percent** (`brightness_pct`, `scale_u8()`).
> - `inspiration/led_patterns.h` uses **0..255** for brightness.
>
> For console ergonomics, percent is friendly. For per-LED internal intensity math, 0..255 is efficient. This design uses percent for *global brightness* and 0..255 for “internal intensity” values (breathing min/max, sparkle brightness).

### Pattern Implementations (Algorithm Notes + Embedded Considerations)

This section describes how each of the four target patterns should work, grounded in `patterns.jsx` and the existing C implementations.

#### 1) Rainbow

**Simulator (`patterns.jsx`) behavior:**

- Hue is a function of LED index and “frame”:
  - `hue = (i * 360/LED_COUNT + frame * speed) % 360`
- Saturation fixed at 100, lightness fixed at 50.
- Brightness is applied as a multiplier in `rgbToString(...)`.

**Prior firmware behavior (`0043/.../main.c`):**

- Hue offset is time-based:
  - `hue_offset = ((now_ms * speed * 360) / 10000) % 360`
- Hue spread across strip is adjustable via `spread_x10`.
- Saturation is a configurable knob.

**Recommended embedded semantics:**

- Use time-based hue offset (`now_ms`) rather than frame counters to avoid “speed changes with CPU load”.
- Keep `saturation` and `spread_x10` as explicit knobs (they are already used in the prior firmware and Kconfig).
- Implement HSL/HSV conversion using integer math (see `hsv2rgb()` in `0043/.../main.c` or `hsl_to_rgb()` in the inspiration engine).

> **Fundamental — “Spread”**
>
> Spread controls how much hue range is distributed along the strip:
> - `spread_x10 = 10` → full 360° rainbow across the strip
> - `spread_x10 = 5`  → 180° hue range across the strip (more “solid-ish”)

#### 2) Chase

**Simulator behavior:**

- Fixed `chaseLength = 5`.
- Head position advances with `pos = floor(frame * speed / 3) % LED_COUNT`.
- Tail fades linearly: `fade = 1 - dist/chaseLength`.
- Background is a dim version of `color2` (scaled to 10% then scaled by brightness).

**Inspiration engine behavior:**

- Tail length and direction modes are configurable (`forward`, `reverse`, `bounce`).
- Optional fade tail, background color explicit.

**Recommended embedded semantics:**

- Promote `tail_len` to config (default 5 to match the simulator).
- Promote `bg` to config (default to a dimmed color2 like simulator, or explicitly `rgb(5,5,5)`).
- Support direction modes; they’re very useful for testing wiring direction.
- Speed should affect **update interval** or **position delta per frame**, but use time-based units when possible:
  - e.g., “LEDs per second” or “ms per step” derived from `speed`.

> **Callout — Integer-friendly chase fading**
>
> Replace `fade = 1 - dist/tail_len` with an 8-bit scale:
> `fade_u8 = 255 - (dist * 255 / tail_len)`
> then `rgb_scale(fg, fade_u8)`.

#### 3) Breathing

**Simulator behavior:**

- `breath = (sin(frame * speed * 0.05) + 1)/2` and brightness scales by `breath*0.9+0.1`.
- Applies to all LEDs uniformly.

**Inspiration engine behavior:**

- Uses a phase accumulator + curve selection (sine/linear/ease).
- Maps to a min/max brightness range.

**Recommended embedded semantics:**

- Use an 8-bit phase accumulator driven by time:
  - `phase_u8 = (now_ms % period_ms) * 256 / period_ms`
- Use a quarter-wave sine table (already present in `0043/.../main.c` and inspiration code).
- Model breathing as *pattern intensity* that multiplies the pattern’s base color.

> **Fundamental — Curve selection**
>
> Using different curves is not “nice-to-have” only; it helps verify timing and brightness scaling:
> - Sine: visually smooth
> - Linear triangle: reveals gamma and quantization artifacts
> - Ease-in-out: reveals plateau/roll-off tuning

#### 4) Sparkle

**Simulator behavior:**

- Each LED per frame: with probability `p = 0.08` (since `random() > 0.92`) it becomes fully bright, otherwise dim (`0.05×`).
- No explicit fade; sparkles are instantaneous per frame.

**Inspiration engine behavior:**

- Maintains per-LED sparkle brightness state (`sparkle_brightness[i]`).
- Each frame: fade existing brightness, then spawn new ones based on density.
- Supports fixed/random/rainbow sparkle colors.

**Recommended embedded semantics:**

- Prefer the inspiration engine’s stateful model; it looks better and avoids “strobing” if frame rate changes.
- Use:
  - `density_pct` as the spawn probability per LED per frame
  - `fade_speed` as how fast they decay (brightness decrement)
- Provide `color_mode` so we can do “white sparkle”, “random sparkle”, and “position rainbow sparkle” easily.
- Seed RNG once (e.g., from `esp_random()` or time) and keep RNG inside the animation task.

> **Callout — Determinism vs nondeterminism**
>
> If a console user asks “why does it look different every time?”, nondeterminism is often the reason. Consider supporting an optional `rng_seed` command and exposing it in `status`.

### Realtime Animation Task Design

#### Goals

- Keep animation timing stable (frame period).
- Ensure console commands reconfigure the running pattern quickly (low latency).
- Avoid concurrency hazards: no shared mutable state across tasks.

#### Task loop shape

The animation task should:

1. Block waiting for messages **with a timeout** equal to the next frame deadline.
2. Apply any pending configuration changes.
3. Render a frame and transmit it.
4. Sleep until the next frame boundary (or compute the next deadline and continue).

Pseudocode (conceptual):

```c
static void led_task(void *arg) {
  led_ctx_t ctx = led_ctx_init_from_kconfig_or_nvs();

  TickType_t last_wake = xTaskGetTickCount();
  const TickType_t frame_ticks = pdMS_TO_TICKS(ctx.frame_ms);

  for (;;) {
    // 1) Drain config/control messages (non-blocking or bounded blocking).
    led_msg_t msg;
    while (xQueueReceive(ctx.q, &msg, 0) == pdTRUE) {
      led_apply_msg(&ctx, &msg);
    }

    // 2) Render pattern (uses time-based now_ms).
    const uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
    led_render_frame(&ctx, now_ms);

    // 3) Transmit frame (RMT handles waveform timing).
    led_driver_show(&ctx.driver, ctx.pixels, ctx.led_count);

    // 4) Delay until next frame boundary.
    vTaskDelayUntil(&last_wake, frame_ticks);
  }
}
```

> **Fundamental — Where does “realtime” matter?**
>
> With RMT, WS281x bit timing is not dependent on task scheduling. “Realtime” here is about:
> - stable frame cadence (smooth motion)
> - bounded control-plane latency (commands apply quickly)
> - avoiding long blocks that starve other subsystems

#### Task priority and core pinning

Guidance:

- Start with a modest priority (e.g., 5) and avoid pinning unless needed.
- If other high-frequency tasks exist (WiFi, BLE, audio), test and adjust.
- The RMT transmit itself can block until done; if that becomes an issue, consider:
  - using `rmt_transmit()` without `rmt_tx_wait_all_done()` every frame (queue depth permitting), or
  - double-buffering and letting the RMT driver pipeline frames.

### Queue Protocol for Live Reconfiguration

Define a message type that:

- supports “set full config” for easy atomic updates,
- supports partial updates for console ergonomics,
- supports driver reinit commands.

Example:

```c
typedef enum {
  LED_MSG_SET_PATTERN_CFG = 1,   // replace pattern cfg atomically
  LED_MSG_SET_PATTERN_TYPE,      // change type and reset state
  LED_MSG_SET_GLOBAL_BRIGHTNESS, // percent 1..100

  LED_MSG_SET_RAINBOW,           // partial updates by pattern
  LED_MSG_SET_CHASE,
  LED_MSG_SET_BREATHING,
  LED_MSG_SET_SPARKLE,

  LED_MSG_DRIVER_SET_CFG,        // staging config only (does not apply)
  LED_MSG_DRIVER_APPLY_CFG,      // reinit driver using staged cfg

  LED_MSG_PAUSE,
  LED_MSG_RESUME,
  LED_MSG_CLEAR,
  LED_MSG_STATUS_SNAPSHOT,       // optional: return status via a reply queue
} led_msg_type_t;

typedef struct {
  led_msg_type_t type;
  union {
    led_pattern_cfg_t pattern;      // for SET_PATTERN_CFG
    led_pattern_type_t pattern_type;
    uint8_t brightness_pct;
    led_rainbow_cfg_t rainbow;
    led_chase_cfg_t chase;
    led_breathing_cfg_t breathing;
    led_sparkle_cfg_t sparkle;
    led_driver_cfg_t driver;
  } u;
} led_msg_t;
```

> **Callout — Avoid “half-updated config”**
>
> Either send a complete `led_pattern_cfg_t` (atomic replace) or send partial updates that only touch the relevant sub-struct. Do not require multiple commands to form a valid configuration unless you have an explicit “apply” barrier.

#### Queue sizing, drops, and “latest wins” semantics

Typical queue patterns for interactive control:

1. **Small queue + “latest wins” for parameter updates**  
   If users type multiple commands quickly, we generally care about the *latest* config, not a historical backlog.

   Options:
   - Use a queue length of 1 and `xQueueOverwrite()` for “set cfg” style messages.
   - Or use a small queue (e.g., 8) and drain it fully each frame, applying messages in order.

2. **Separate queues for control vs telemetry (optional)**  
   - Control queue: set config, apply driver, pause/resume.
   - Telemetry queue: status snapshots/logging.

Recommended baseline for MO-032:

- One control queue of length 8.
- Animation task drains all pending messages each frame (`while (xQueueReceive(..., 0))`).
- Console command handler prints `ok` if `xQueueSend(..., 0)` succeeds; otherwise prints `busy` and suggests retry.

> **Callout — Never block the console task**
>
> `esp_console` runs its own REPL task. If a command handler blocks waiting for the animation task, the REPL becomes sluggish. Prefer non-blocking sends and fast failure (`busy`) under pressure.

#### Message application semantics (pattern/state invariants)

Key invariant: switching patterns should reset runtime state, but keep driver config intact.

Pseudocode:

```c
static void led_apply_msg(led_ctx_t *ctx, const led_msg_t *m) {
  switch (m->type) {
    case LED_MSG_SET_PATTERN_CFG:
      ctx->pattern_cfg = m->u.pattern;
      led_pattern_state_reset(ctx);
      break;

    case LED_MSG_SET_PATTERN_TYPE:
      ctx->pattern_cfg.type = m->u.pattern_type;
      led_pattern_state_reset(ctx);
      break;

    case LED_MSG_SET_RAINBOW:
      ctx->pattern_cfg.type = LED_PATTERN_RAINBOW;
      ctx->pattern_cfg.u.rainbow = m->u.rainbow;
      led_pattern_state_reset(ctx);
      break;

    case LED_MSG_DRIVER_SET_CFG:
      if (validate_driver_cfg(&m->u.driver)) ctx->driver_cfg_staged = m->u.driver;
      break;

    case LED_MSG_DRIVER_APPLY_CFG:
      led_driver_apply(ctx);
      led_pattern_state_reset(ctx);
      break;

    case LED_MSG_PAUSE:  ctx->paused = true; break;
    case LED_MSG_RESUME: ctx->paused = false; break;
    case LED_MSG_CLEAR:  led_pixels_clear(ctx); break;
    default: break;
  }
}
```

#### Driver configuration: staged vs applied

Some driver fields are not safe to change “in place” because they affect allocation and peripheral initialization. A practical solution is to:

1. Maintain a `driver_cfg_staged` struct in the task.
2. Allow console to edit staged fields.
3. Apply them with a separate “apply/reinit” command.

```c
typedef enum { LED_ORDER_GRB = 0, LED_ORDER_RGB } led_color_order_t;

typedef struct {
  int gpio_num;
  uint16_t led_count;
  led_color_order_t order;

  // WS281x timing, in ns/us
  uint32_t t0h_ns, t0l_ns;
  uint32_t t1h_ns, t1l_ns;
  uint32_t reset_us;

  // RMT resolution, in Hz (tick size)
  uint32_t resolution_hz;
} led_driver_cfg_t;
```

Recommended behavior:

- `LED_MSG_DRIVER_SET_CFG`: validates and stores into `driver_cfg_staged` but does not disrupt output.
- `LED_MSG_DRIVER_APPLY_CFG`: stops output, deinitializes RMT+encoder, reallocates pixel buffer, initializes with staged config, then resumes.

### WS281x Driver Integration Details (ESP-IDF + RMT)

This section makes the “WS281x communication” story explicit so the console’s `led ws ...` verbs map cleanly onto driver fields.

#### RMT resolution and timing conversion

The RMT peripheral encodes durations in “ticks”. The conversion is:

```
ticks = ceil(duration_seconds * resolution_hz)
```

In the prior firmware, the baseline is:

- `resolution_hz = 10_000_000` (10 MHz), so 1 tick = 0.1 µs.

Reference implementation (tick conversion + reset symbol construction):

- `0043-xiao-esp32c6-ws2811-4led-d1/main/ws281x_encoder.c`

> **Callout — Round up, never 0 ticks**
>
> If a duration is too small for the chosen resolution, naive conversion can produce 0 ticks, yielding illegal pulses. The encoder’s conversion routines should clamp/round up to at least 1 tick.

#### Typical WS2811/WS2812 timing defaults

The prior firmware exposes Kconfig parameters (ns/us) for:

- `T0H`, `T0L`, `T1H`, `T1L` (bit 0/1 pulse widths)
- `RESET_US` (latch/reset low time after a frame)

Reference:

- `0043-xiao-esp32c6-ws2811-4led-d1/main/Kconfig.projbuild`

> **Fundamental — “WS2811 vs WS2812”**
>
> In practice, many strips tolerate a range of timings as long as the bit period is roughly correct and reset is long enough. The safest approach for a tuning console is:
> - ship sane defaults
> - allow tuning but require an explicit `apply` barrier
> - always keep a `ws status` command that prints the currently applied timings

#### Color order and buffer layout

WS281x LEDs commonly interpret bytes as **GRB** even though we colloquially say “RGB”.

The prior firmware uses a helper that writes bytes in GRB order:

- `set_pixel_grb(...)` in `0043-xiao-esp32c6-ws2811-4led-d1/main/main.c`

Recommended rule:

- Store pixels in “wire order” in the buffer (`order == GRB` → buffer bytes are `[G,R,B]` per LED).
- Keep pattern computations in logical RGB.
- Have a single conversion step `pixel_pack(order, rgb)` to avoid scattering order logic across all patterns.

Pseudocode:

```c
static inline void pixel_pack(uint8_t *buf, uint32_t i, led_rgb8_t rgb, led_color_order_t order, uint8_t bri_pct) {
  uint8_t r = scale_u8(rgb.r, bri_pct);
  uint8_t g = scale_u8(rgb.g, bri_pct);
  uint8_t b = scale_u8(rgb.b, bri_pct);

  switch (order) {
    case LED_ORDER_GRB: buf[i*3+0]=g; buf[i*3+1]=r; buf[i*3+2]=b; break;
    case LED_ORDER_RGB: buf[i*3+0]=r; buf[i*3+1]=g; buf[i*3+2]=b; break;
  }
}
```

#### Driver (re)initialization sequencing

Applying staged driver config should be a carefully ordered mini-state-machine:

```c
static esp_err_t led_driver_apply(led_ctx_t *ctx) {
  // 0) Stop pattern output (optional: clear LEDs first)
  ctx->paused = true;

  // 1) Deinit old objects
  if (ctx->chan)   rmt_disable(ctx->chan), rmt_del_channel(ctx->chan);
  if (ctx->enc)    rmt_del_encoder(ctx->enc);
  if (ctx->pixels) free(ctx->pixels), ctx->pixels=NULL;

  // 2) Allocate new pixel buffer based on staged count
  ctx->led_count = ctx->driver_cfg_staged.led_count;
  ctx->pixels = heap_caps_malloc(ctx->led_count * 3, MALLOC_CAP_DEFAULT);
  memset(ctx->pixels, 0, ctx->led_count * 3);

  // 3) Create + enable RMT channel
  rmt_new_tx_channel(...gpio_num..., ...resolution_hz..., ...queue_depth..., &ctx->chan);
  rmt_enable(ctx->chan);

  // 4) Create encoder using staged timing fields
  rmt_new_ws281x_encoder(...t0h/t0l/t1h/t1l/reset..., &ctx->enc);

  // 5) Resume
  ctx->paused = false;
  return ESP_OK;
}
```

> **Callout — Clearing LEDs on apply**
>
> Consider doing a “clear frame” after reinit (`all zeros` + transmit) to avoid stale colors if the previous stream was interrupted mid-frame.

#### Memory sizing and performance

Memory:

- Pixel buffer is `led_count * 3` bytes (plus overhead).
- Sparkle pattern state can require `led_count` bytes for per-LED brightness.

Performance hotspots:

- Color space conversion inside the per-LED render loop (rainbow).
- Repeated floating-point operations (avoid; use LUTs / integer math).

Mitigations:

- Keep conversion routines integer-based (`hsv2rgb()` as in `0043/.../main.c`).
- If LED count is large, consider precomputing the “hue per LED” base ramp and only adding hue_offset each frame.

### esp_console REPL Design

#### Console backend defaults

For device families where UART pins are frequently repurposed, default to USB Serial/JTAG:

```
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
# CONFIG_ESP_CONSOLE_UART is not set
```

Reference implementation for bringing up `esp_console` over USB Serial/JTAG:

- `0030-cardputer-console-eventbus/main/app_main.cpp` (`console_start()`).

#### Command philosophy

- Use a single top-level verb: `led`.
- Use subcommands: `led pattern ...`, `led rainbow ...`, `led ws ...`, `led status`, etc.
- Every mutating command prints `ok` or a concise error.
- Favor **idempotent commands** that can be repeated safely.

#### Proposed verb set

**Core:**

- `led status`  
  Prints: running/paused, pattern type, key pattern params, global brightness, frame ms, driver config (gpio/count/order/timings).

- `led pause` / `led resume` / `led clear`

- `led pattern set <rainbow|chase|breathing|sparkle|off>`

**Pattern configuration (examples):**

- `led rainbow set --speed 5 --sat 100 --spread 10`
- `led chase set --speed 6 --tail 5 --fg #00ffff --bg #000044 --dir forward --fade 1`
- `led breathing set --speed 3 --color #ff00ff --min 10 --max 255 --curve sine`
- `led sparkle set --speed 5 --color #ffffff --density 8 --fade 30 --mode random --bg #050505`

**Global knobs:**

- `led brightness <1..100>`
- `led frame <ms>`

**WS281x driver staging/apply:**

- `led ws status`
- `led ws set gpio <n>`
- `led ws set count <n>`
- `led ws set order <grb|rgb>`
- `led ws set timing --t0h 350 --t0l 800 --t1h 700 --t1l 600 --reset 80`
- `led ws apply`

> **Callout — Why an “apply” step?**
>
> It forces a deliberate boundary for destructive changes (GPIO, LED count, timings). This reduces accidental disruptions during interactive tuning and makes it obvious when a driver reinit occurs.

#### Parameter parsing rules (console ergonomics)

To keep the REPL pleasant:

- Integers accept decimal only (`123`).
- Colors accept either:
  - `#RRGGBB` hex (recommended), or
  - `r,g,b` decimals (optional, advanced).
- Enums accept a fixed small vocabulary:
  - `dir`: `forward|reverse|bounce`
  - `curve`: `sine|linear|ease`
  - `mode`: `fixed|random|rainbow`
  - `order`: `grb|rgb`

Hex parsing pseudocode:

```c
static bool parse_hex_rgb(const char *s, led_rgb8_t *out) {
  // Accept "#RRGGBB" or "RRGGBB"
  if (s[0] == '#') s++;
  if (strlen(s) != 6) return false;
  out->r = hexbyte(s+0);
  out->g = hexbyte(s+2);
  out->b = hexbyte(s+4);
  return true;
}
```

#### Example `led status` output

Example output format (designed for easy copy/paste and grep):

```
running=1 paused=0
pattern=rainbow frame_ms=16 brightness_pct=25
rainbow: speed=5 sat=100 spread_x10=10
ws: gpio=1 count=50 order=grb res_hz=10000000
ws_timing_ns: t0h=350 t0l=800 t1h=700 t1l=600 reset_us=80
```

#### Command handler pseudocode (argv-style)

The simplest model matches the `evt` command from `0030-cardputer-console-eventbus/main/app_main.cpp`: parse `argv[]` manually and send one queue message.

```c
static int cmd_led(int argc, char **argv) {
  if (argc < 2) return led_print_usage();

  if (!strcmp(argv[1], "status")) {
    // Optional: request snapshot via reply queue; else print cached state.
    led_print_cached_status();
    return 0;
  }

  if (!strcmp(argv[1], "pattern") && argc >= 4 && !strcmp(argv[2], "set")) {
    led_msg_t m = { .type = LED_MSG_SET_PATTERN_TYPE, .u.pattern_type = parse_pattern(argv[3]) };
    return led_send(m);
  }

  if (!strcmp(argv[1], "rainbow") && argc >= 3 && !strcmp(argv[2], "set")) {
    led_rainbow_cfg_t cfg = led_cfg_from_defaults(); // then parse flags
    parse_rainbow_flags(argc, argv, &cfg);
    led_msg_t m = { .type = LED_MSG_SET_RAINBOW, .u.rainbow = cfg };
    return led_send(m);
  }

  // ... chase/breathing/sparkle/ws subcommands ...
}
```

> **Fundamental — Keep console handlers non-blocking**
>
> Console commands should do quick argument parsing and enqueue a message. Avoid calling driver functions from the console handler directly.

#### Optional: richer parsing via argtable3

For commands with many flags, `argtable3` improves correctness and self-documentation (and is commonly used with `esp_console`). If adopted:

- Each subcommand becomes its own `esp_console_cmd_t`:
  - `led_rainbow`, `led_chase`, `led_ws`, etc.
- Each command uses an `argtable` definition for flags.

This is optional; the argv-style approach is adequate for an initial implementation.

## Design Decisions

1. **Single-owner animation task**  
   Rationale: simplifies concurrency, avoids mutexes, keeps driver/pattern state coherent.

2. **Time-based animation** (`now_ms`) rather than frame-based counters as the primary input  
   Rationale: stable perceived speed even if frame rate jitter occurs.

3. **Staged driver configuration + explicit apply**  
   Rationale: driver changes are disruptive; “apply” is a safe boundary.

4. **USB Serial/JTAG as the default console backend**  
   Rationale: avoids UART pin collisions on boards where UART pins are reused; consistent with repo guidance and existing examples.

5. **Config structs mirror existing reference code** (inspiration engine and existing 0043 driver)  
   Rationale: reuses known-good ideas and reduces design churn.

## Alternatives Considered

1. **Mutex-protected shared config/state** (console writes shared structs directly)  
   Rejected: easy to introduce race conditions (pattern state resets mid-render), harder to reason about invariants.

2. **Event-loop based control (`esp_event`) instead of a queue**  
   Possible, but rejected for now: a queue provides a simpler, explicit “single consumer” model. Events can still be integrated later if desired.

3. **Run patterns directly in `app_main()` loop** (like `0043/.../main.c`)  
   Rejected for this ticket’s goal: once a console REPL exists, separating work into a task is cleaner and keeps `app_main()` responsible for initialization only.

4. **Use the ESP-IDF `led_strip` component**  
   Not rejected universally; deferred. The repo already has a custom RMT encoder approach. If `led_strip` provides better portability across targets, we can migrate later, but that’s a separate decision.

## Implementation Plan

1. **Extract driver into a reusable module**
   - Create `led_ws281x.[ch]` around the existing RMT+encoder code from `0043/...`.
   - Expose: `init(cfg)`, `deinit()`, `show(buf,len)`.

2. **Implement pattern engine module**
   - Create `led_patterns.[ch]` (adapt the inspiration engine’s pattern/state split).
   - Ensure each pattern uses `now_ms` to derive motion where appropriate.

3. **Add animation task + queue**
   - Create `led_task.[ch]` with `led_msg_t` protocol.
   - Ensure the task owns RNG seeding and state reset semantics.

4. **Add esp_console REPL commands**
   - Create `led_console.[ch]` and register `led` command(s).
   - Default backend: USB Serial/JTAG (`sdkconfig.defaults`).

5. **Add status reporting + basic validation**
   - Implement `led status` output.
   - Add a simple “smoke test” playbook doc: wiring + expected visuals per pattern.

6. **(Optional) Persist config in NVS**
   - Driver cfg staging could be persisted; pattern cfg could be persisted for power cycle recovery.

## Open Questions

1. **Which driver fields must be runtime-mutable?**  
   For safety, consider only allowing runtime changes to:
   - global brightness
   - frame period
   - pattern parameters
   and leave driver timings/GPIO/count as menuconfig-only (or require an explicit “apply + reboot”).

2. **Color order defaults (GRB vs RGB)**  
   Many WS2812B strips are GRB; make defaults match known-good `0043` behavior (GRB), but keep a runtime override.

3. **Do we need a reply channel for `status`?**  
   Option A: console prints cached last-known status (simple).  
   Option B: console sends a request with a reply queue and prints a snapshot (more accurate; slightly more code).

4. **How many LEDs do we expect on ESP32-C6 targets?**  
   Larger counts raise concerns about memory and frame time; might require optimizing conversions and potentially leveraging RMT queueing.

## References

- Imported simulator: `ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/sources/local/patterns.jsx`
- Prior working rainbow + RMT driver baseline:
  - `0043-xiao-esp32c6-ws2811-4led-d1/main/main.c`
  - `0043-xiao-esp32c6-ws2811-4led-d1/main/ws281x_encoder.c`
  - `0043-xiao-esp32c6-ws2811-4led-d1/main/Kconfig.projbuild`
- Pattern-engine inspiration:
  - `0043-xiao-esp32c6-ws2811-4led-d1/inspiration/led_patterns.h`
  - `0043-xiao-esp32c6-ws2811-4led-d1/inspiration/led_patterns.c`
- esp_console example (USB Serial/JTAG backend selection):
  - `0030-cardputer-console-eventbus/main/app_main.cpp`
