---
Title: "JS Sequencer Cookbook (Timers + GPIO)"
Ticket: 0066-cardputer-ledchain-gfx-sim
Status: draft
Topics:
  - microquickjs
  - gpio
  - timers
  - sequencing
DocType: reference
Intent: how-to
Owners: []
RelatedFiles:
  - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/js_service.cpp
    Note: Defines `every(...)`, `cancel(...)`, `gpio.write(...)`, `gpio.toggle(...)`
  - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/esp32_stdlib_runtime.c
    Note: Native `setTimeout/clearTimeout` and `gpio.high/low`
Summary: "Practical, copy-pasteable JS snippets for sequencing GPIO (G3/G4) and simulator patterns."
LastUpdated: 2026-01-23
---

# JS sequencer cookbook (0066)

This document is the “copy/paste” layer on top of the 0066 JS primitives:

- timers: `setTimeout`, `clearTimeout`, `every`, `cancel`
- GPIO: `gpio.write`, `gpio.toggle` (userland), and `gpio.high/low` (native)
- simulator: `sim.*`

The overall goal is to let you write *small deterministic sequences* without adding a large native “sequencer engine” prematurely.

## 0) Fundamentals (mental model)

### Where your JS runs

All JS runs on the `mqjs_service` VM-owner task. Timer expiry and future events never execute JS directly; they post jobs into the service.

Implication:

- JS is single-threaded (good).
- Timer callbacks are best-effort: if you schedule too much work, the service queue can saturate and you can lose callbacks.

### What “blocking” means

For control APIs (e.g., `sim.setPattern(...)`) the practical meaning of “blocking” is: *your call returns once the request is accepted*, not once a frame is rendered.

### About GPIO labels

Use board labels `"G3"` and `"G4"`. They map to ESP32-S3 GPIO numbers via Kconfig:

- `CONFIG_TUTORIAL_0066_G3_GPIO` (default 3)
- `CONFIG_TUTORIAL_0066_G4_GPIO` (default 4)

## 1) Common idioms

### 1.1 One-shot delay

```js
setTimeout(function () {
  print("hello from the future");
}, 250);
```

### 1.2 Periodic loop + cancellation

```js
var n = 0;
var h = every(100, function () {
  n++;
  print("tick", n);
  if (n >= 10) cancel(h);
});
```

### 1.3 Safe periodic loop (stop on error)

`every(...)` already cancels itself if the callback throws.

```js
var h = every(50, function () {
  // throw new Error("stop");
});
```

## 2) GPIO recipes

### 2.1 Pulse N times (fixed period)

“Pulse” here means: toggle high, then low after `on_ms`, repeat every `period_ms`.

```js
function pulseN(label, n, period_ms, on_ms) {
  var i = 0;
  gpio.write(label, 0);
  var h = every(period_ms, function () {
    if (i >= n) { cancel(h); gpio.write(label, 0); return; }
    i++;
    gpio.write(label, 1);
    setTimeout(function () { gpio.write(label, 0); }, on_ms);
  });
  return h;
}

// Example:
pulseN("G3", 10, 200, 30);
```

### 2.2 Alternating G3 / G4 (metronome)

```js
gpio.write("G3", 0);
gpio.write("G4", 0);

var h = every(100, function () {
  gpio.toggle("G3");
  gpio.toggle("G4");
});

// Later:
// cancel(h); gpio.write("G3",0); gpio.write("G4",0);
```

### 2.3 “Bit-bang” a short pattern (explicit schedule)

This is useful when you want exact relative timing and a finite sequence.

```js
function schedule(label, steps) {
  // steps: [{ at_ms, v }]
  for (var i = 0; i < steps.length; i++) {
    (function (s) {
      setTimeout(function () { gpio.write(label, s.v); }, s.at_ms);
    })(steps[i]);
  }
}

gpio.write("G3", 0);
schedule("G3", [
  { at_ms: 0,   v: 1 },
  { at_ms: 20,  v: 0 },
  { at_ms: 40,  v: 1 },
  { at_ms: 60,  v: 0 },
]);
```

### 2.4 Two-line “protocol”: G3 clock + G4 data

```js
function sendBits(bits, bit_ms) {
  gpio.write("G3", 0);
  gpio.write("G4", 0);
  var i = 0;
  var h = every(bit_ms, function () {
    if (i >= bits.length) { cancel(h); gpio.write("G3",0); gpio.write("G4",0); return; }
    gpio.write("G4", bits[i] ? 1 : 0);
    gpio.toggle("G3"); // rising edge every tick
    i++;
  });
  return h;
}

sendBits([1,0,1,1,0,0,1,0], 25);
```

## 3) Sequencing simulator patterns (no GPIO)

### 3.1 “Playlist” of patterns

```js
function playlist(items) {
  var i = 0;
  function next() {
    if (i >= items.length) return;
    var it = items[i++];
    it.fn();
    setTimeout(next, it.ms);
  }
  next();
}

playlist([
  { ms: 2000, fn: function () { sim.setBrightness(100); sim.setPattern("rainbow"); } },
  { ms: 2000, fn: function () { sim.setPattern("chase"); sim.setChase(30,5,10,1,"#FFFFFF","#000000",0,1,100); } },
  { ms: 2000, fn: function () { sim.setPattern("breathing"); sim.setBreathing(8,"#00AAFF",5,255,0); } },
  { ms: 2000, fn: function () { sim.setPattern("sparkle"); sim.setSparkle(10,"#FFFFFF",20,64,0,"#000000"); } },
  { ms: 2000, fn: function () { sim.setPattern("off"); } },
]);
```

## 4) Combined: sync pattern changes with GPIO edges

### 4.1 Pattern change on each G3 toggle

```js
var i = 0;
var patterns = ["rainbow", "chase", "breathing", "sparkle", "off"];

var h = every(1000, function () {
  gpio.toggle("G3");
  var p = patterns[i++ % patterns.length];
  sim.setPattern(p);
});
```

## 5) Debugging and safety

### 5.1 Always keep a “stop handle”

Store handles in globals so you can stop a runaway loop:

```js
var H = {};
H.blink = every(100, function () { gpio.toggle("G3"); });
// cancel(H.blink);
```

### 5.2 If the console gets sluggish

You can saturate the JS service if callbacks run too fast or do too much work.

Typical mitigation:

- Increase periods (`every(50, ...)` → `every(200, ...)`).
- Keep callbacks short.
- Prefer “explicit schedule” (`setTimeout` steps) for finite sequences.

