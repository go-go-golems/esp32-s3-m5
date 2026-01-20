---
Title: Speed Scaling, Brightness, Power, and Operator Scripts
Ticket: MO-032-ESP32C6-LED-PATTERNS-CONSOLE
Status: active
Topics:
    - esp32
    - animation
    - console
    - serial
    - esp-idf
    - usb-serial-jtag
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/led_console.c
      Note: Operator REPL parsing/help for speed and train controls
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/led_patterns.c
      Note: Pattern timing math and updated speed mappings
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/led_patterns.h
      Note: Pattern configuration structs and speed ranges
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/led_task.c
      Note: Frame cadence and queue semantics
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/led_ws281x.c
      Note: Brightness scaling and pixel packing
    - Path: ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/playbook/01-0044-build-flash-led-console-smoke-test.md
      Note: How to validate on hardware
    - Path: ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/scripts/led_smoke_commands.txt
      Note: Operator smoke-test command list
    - Path: ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/scripts/tmux_flash_monitor.sh
      Note: Optional tmux flash/monitor helper
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-20T16:47:43.402894496-05:00
WhatFor: ""
WhenToUse: ""
---


# Speed Scaling, Brightness, Power, and Operator Scripts

This document answers the pattern-tuning questions raised during MO-032, with a focus on:

- how each pattern’s `speed` value is mapped to actual time behavior (ms → motion),
- why some speeds “saturated” or felt too fast,
- how (and why) brightness is scaled in firmware,
- what that implies for **LED power/current**, and
- what the operator scripts do (and where they live).

All references are to the current firmware implementation in `0044-xiao-esp32c6-ws281x-patterns-console/` and the MO-032 ticket assets under `ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/`.

## Fundamentals: how animations run in this firmware

### One owner task, one frame cadence

The LED engine is a single-owner task that:

1. drains a queue of control messages (pattern set/config updates, driver cfg updates),
2. renders the active pattern into the WS281x pixel buffer,
3. transmits the buffer via RMT,
4. sleeps until the next frame period.

Reference: `0044-xiao-esp32c6-ws281x-patterns-console/main/led_task.c`

> **Callout — Why speed felt “capped” previously**
>
> If the pattern only advances “one step per frame”, then once the internal step interval becomes smaller than `frame_ms`, increasing speed further can’t create more visible motion because you are still limited to one update per frame.
>
> Fixing this generally means making the pattern time-based (integrate over `dt_ms`) rather than frame-step-based.

### Where “speed” lives

Pattern parameters live in `led_pattern_cfg_t` and are applied via `LED_MSG_SET_*` messages:

- `0044-xiao-esp32c6-ws281x-patterns-console/main/led_patterns.h`
- `0044-xiao-esp32c6-ws281x-patterns-console/main/led_task.h`
- `0044-xiao-esp32c6-ws281x-patterns-console/main/led_task.c`

The console parses operator commands and sends queue messages; it does not mutate pattern state directly:

- `0044-xiao-esp32c6-ws281x-patterns-console/main/led_console.c`

## Answer: Rainbow speed range 0..20 (no need faster)

### What we changed

Rainbow `speed` is now bounded to `0..20` and interpreted as **rotations per minute (RPM)**:

- `speed = 0` → static rainbow (no hue rotation over time)
- `speed = 20` → 20 rotations/minute → one full 360° rotation every 3 seconds

Reference: `0044-xiao-esp32c6-ws281x-patterns-console/main/led_patterns.c` (`render_rainbow`)

### Why RPM is a good operator knob

- RPM is stable across different `frame_ms` values.
- It maps linearly to time: doubling speed doubles angular velocity.
- It keeps the “useful” range small and intuitive.

### The mapping (math)

The pattern computes a hue offset in degrees:

```c
// speed_rpm: 0..20
hue_offset_deg = (now_ms * speed_rpm * 360) / 60000;
```

Then each LED gets an additional per-LED hue ramp based on `spread_x10`.

## Answer: Chase speed saturating above ~30, and “train spacing / multiple trains”

### Why speed “didn’t change much” above ~30 before

Previously, chase speed behaved like “how often to advance one LED”, implemented as:

- `step_ms = 200 / speed`
- update at most once per render call

Once `step_ms` became smaller than `frame_ms`, the chase head effectively advanced by ~1 LED every frame, and higher speeds could not advance more than that (so it felt capped).

### What we changed: speed is now LEDs per second

Chase `speed` is now interpreted as **LEDs per second** (`LED/s`) and integrated over time:

- `speed = 0` → static chase head
- `speed = 30` → head moves 30 LEDs per second (fractional movement accumulates)

Reference: `0044-xiao-esp32c6-ws281x-patterns-console/main/led_patterns.c` (`chase_update_pos`)

#### The mapping (math)

We maintain head position as Q16.16 “LED units”:

```c
delta_q16 = speed_leds_per_sec * dt_ms * 65536 / 1000;
pos_q16 += delta_q16;
head_index = pos_q16 >> 16;
```

This keeps speed meaningful even when `frame_ms` is large, because the head can advance multiple LEDs per frame (or fractional LEDs across frames).

### Train spacing: `gap_len`

The “interval between trains” is now `gap_len`, defined as background LEDs between the tail and the next train head.

The effective spacing is:

```
train_period = tail_len + gap_len
```

Reference: `0044-xiao-esp32c6-ws281x-patterns-console/main/led_patterns.c` (`chase_train_period`)

### Multiple trains: `trains`

The chase renderer supports multiple trains:

- `trains = 1` → classic single train
- `trains = N` → N trains spaced by `train_period`

If `trains * train_period > led_count`, the implementation clamps trains so trains don’t overlap beyond the ring’s capacity.

Reference: `0044-xiao-esp32c6-ws281x-patterns-console/main/led_patterns.c` (`render_chase`)

### Example commands

Single train, medium speed:

```text
led chase set --speed 30 --tail 6 --gap 10 --trains 1 --fg #00ffff --bg #000010 --dir forward --fade 1
```

Three trains:

```text
led chase set --speed 30 --tail 6 --gap 10 --trains 3 --fg #00ffff --bg #000010 --dir forward --fade 1
```

> **Callout — `bounce` mode**
>
> Bounce mode uses a single moving head that reflects at the ends. Multi-train is currently treated as `trains=1` in bounce mode to avoid ambiguous “multiple reflections” semantics.

## Answer: Breathing is too fast, and why the brightness jumps near the top

### Why it was too fast

The original mapping allowed very short periods at higher speed values.

### What we changed: speed is breaths per minute (BPM)

Breathing `speed` is now bounded to `0..20` and interpreted as **breaths per minute**:

- `speed = 0` → static at max intensity
- `speed = 1` → 1 breath per minute → 60s period
- `speed = 20` → 20 breaths per minute → 3s period

Reference: `0044-xiao-esp32c6-ws281x-patterns-console/main/led_patterns.c` (`render_breathing`)

### Why brightness changes were “drastic” at high values

Two separate effects can cause the “top end” to feel jumpy:

1) **Curve generation bug (actual discontinuity)**

The previous sine lookup implementation contained discontinuities that caused visible jumps in intensity.
This was not a deliberate perception adjustment; it was simply a bad lookup function.

The sine implementation was replaced with a full 256-sample sine wave table, removing the discontinuity.

Reference: `0044-xiao-esp32c6-ws281x-patterns-console/main/led_patterns.c` (`ease_sine_u8`)

2) **Perception vs physical brightness (gamma)**

Even with a perfect mathematical sine, humans do not perceive brightness linearly. A linear PWM/intensity ramp looks “too steep” near the high end.

> **Callout — Perception correction (gamma)**
>
> If we want breathing to look like a perceptually smooth sine, we typically:
> 1) choose the desired brightness curve in *perceived space*, then
> 2) apply a gamma encoding (often ~2.0–2.4) to convert perceived intensity into LED PWM intensity.
>
> This is not currently applied globally in MO-032; current scaling is linear. The breathing improvements here focused on (a) correct sine and (b) slower speed range.

## Answer: Sparkle speed is way too high; need “few sparkles per second”

### Why sparkle was too fast before

The original implementation treated `density_pct` as a per-LED probability per frame. Even “small” values become huge event rates:

Example:

- 50 LEDs
- `density_pct = 8`
- `frame_ms = 25` → 40 frames/sec

Expected spawns per frame:

```
E[spawns/frame] ≈ led_count * density_pct/100 = 50 * 0.08 = 4
```

Expected spawns per second:

```
E[spawns/sec] ≈ 4 * 40 = 160 spawns/sec
```

That will always look “way too high” if the goal is “a few sparkles per second”.

### What we changed: time-based spawn + concurrent cap

Sparkle now has:

- `speed 0..20` mapped to a time-based **spawn rate** (0.0..4.0 sparkles/sec)
- `density_pct` interpreted as **max concurrent sparkles %**, not a spawn probability
- a Q16.16 spawn accumulator so fractional sparkles/sec (slower than 1/sec) works naturally

Reference: `0044-xiao-esp32c6-ws281x-patterns-console/main/led_patterns.c` (`render_sparkle`)

#### The mapping (math)

```c
rate_x10 = speed * 2;              // 0..40 => 0.0..4.0 sparkles/sec
accum_q16 += rate_x10 * dt_ms * 65536 / 10000;
spawn_n = accum_q16 >> 16;
```

Then we randomly pick `spawn_n` LEDs that are currently off, bounded by:

```
max_active = led_count * density_pct / 100
```

### Practical tuning advice

For “rare sparkles” on a 50 LED strip:

```text
led sparkle set --speed 1 --density 2 --fade 20 --mode random --bg #050505
```

For “a few sparkles per second”:

```text
led sparkle set --speed 5 --density 8 --fade 30 --mode random --bg #050505
```

## Brightness: how it’s mapped, and what it means for power

### What the firmware does (current behavior)

There are two layers of brightness scaling:

1) **Per-pattern intensity scaling** (pattern-specific)
   - e.g. breathing computes an intensity `0..255` and scales RGB by that
2) **Global brightness percent** (`1..100`)
   - applied last when packing/writing pixels

Reference:

- `0044-xiao-esp32c6-ws281x-patterns-console/main/led_patterns.c`
- `0044-xiao-esp32c6-ws281x-patterns-console/main/led_ws281x.c` (`led_ws281x_set_pixel_rgb`)

The global brightness scale is linear:

```c
out = (in * brightness_pct) / 100;
```

### What that implies for current draw (rule-of-thumb)

Most WS2812/WS2811-family LEDs behave roughly like constant-current drivers per color channel.
Typical worst-case is ~20 mA per channel at “full on” (varies by part), so:

- full white (R+G+B) ≈ 60 mA per LED (worst-case rule-of-thumb)

An approximate current model for one LED:

```
I_led ≈ I_chan_max * (R/255 + G/255 + B/255)
```

Then global brightness scales those channel values down, so (roughly):

```
I_led_with_brightness_pct ≈ I_led * brightness_pct/100
```

Total strip current:

```
I_total ≈ Σ I_led(i)  (over all LEDs)
```

> **Callout — Perceived brightness is not proportional to power**
>
> A “50% brightness” setting (linear scaling) often looks like much more than 50% to humans (depending on ambient conditions), but electrically it is roughly half the LED current for the same colors. If we later add gamma correction for perceptual linearity, the relationship between a human-friendly knob and power becomes non-linear by design.

### Practical power sizing and safety notes

- Always size the 5V supply for the worst-case pattern you might run (especially bright white scenes).
- Use a common ground between the ESP32 board and the LED power supply.
- Consider a level shifter for the WS281x data line if the strip expects 5V logic.

## Operator scripts (ticket-local)

Per request, all scripts live inside the ticket in `scripts/`:

- `ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/scripts/tmux_flash_monitor.sh`
  - Starts a tmux session with:
    - left pane: `idf.py -p <PORT> flash monitor`
    - right pane: prints the smoke command list for copy/paste
  - This is optional; you can flash/test without tmux.

- `ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/scripts/led_smoke_commands.txt`
  - A curated list of commands to paste into the monitor session to exercise:
    - help/status
    - each pattern’s config knobs
    - `led log on|off`
    - WS cfg staging/apply boundary

Reference playbook:

- `ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/playbook/01-0044-build-flash-led-console-smoke-test.md`

## Next steps / follow-ups

- If you want breathing to be perceptually linear (smooth in human space), add a gamma LUT (or a perceptual curve mode) and document that the brightness knob becomes non-linear w.r.t. power.
- If you want even slower sparkles than `speed=1` (≈ 0.2 sparkles/sec), add a separate `--rate` argument in 0.01–0.1 sparkles/sec units while keeping `--speed` as a coarse knob.
