---
Title: 'Concrete Implementation Plan: JS GPIO Exercizer (GPIO + I2C)'
Ticket: MO-01-JS-GPIO-EXERCIZER
Status: active
Topics:
    - esp32s3
    - cardputer
    - gpio
    - serial
    - console
    - esp-idf
    - tooling
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0004-i2c-rolling-average/main/hello_world_main.c
      Note: I2C master setup and worker task pattern
    - Path: esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/control_plane.cpp
      Note: Queue-based control plane
    - Path: esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/gpio_tx.cpp
      Note: Timer-driven GPIO waveform logic
    - Path: esp32-s3-m5/0039-cardputer-adv-js-gpio-exercizer/main/app_main.cpp
      Note: Firmware wiring
    - Path: esp32-s3-m5/0039-cardputer-adv-js-gpio-exercizer/main/esp32_stdlib_runtime.c
      Note: JS bindings runtime
    - Path: esp32-s3-m5/components/exercizer_control/src/ControlPlane.cpp
      Note: Implemented control-plane component
    - Path: esp32-s3-m5/components/exercizer_gpio/src/GpioEngine.cpp
      Note: Implemented GPIO engine
    - Path: esp32-s3-m5/components/exercizer_i2c/src/I2cEngine.cpp
      Note: Implemented I2C engine
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/app_main.cpp
      Note: REPL bring-up template
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp
      Note: JS evaluation and binding patterns
ExternalSources: []
Summary: Concrete implementation plan for a MicroQuickJS-driven GPIO exercizer with GPIO and I2C engines.
LastUpdated: 2026-01-11T19:11:19.684950635-05:00
WhatFor: Provide a step-by-step code plan and reuse map for implementing the JS GPIO exercizer.
WhenToUse: Use when implementing the GPIO and I2C engines and wiring JS bindings.
---



# Concrete Implementation Plan: JS GPIO Exercizer (GPIO + I2C)

## Executive Summary

This document specifies the concrete code layout and implementation steps to build a MicroQuickJS-based GPIO exercizer with two engines: GPIO waveform output and I2C transaction execution. The plan reuses the existing mqjs-repl REPL/evaluator scaffolding and the GPIO/I2C patterns from `0016` and `0004`, and defines a clear separation between JS control-plane commands and real-time engine tasks.

## Problem Statement

We need a Cardputer-ADV firmware that can accept JS over REPL and produce deterministic GPIO and I2C signals. JS cannot be used for precise timing, so the design must keep JS as a configuration/control layer while engines (timer-driven tasks) perform the real-time work.

## Proposed Solution

- Create C++ components for the control plane and engines, similar to `LineEditor`/`ReplLoop`.
- Create a new firmware target (e.g. `0039-cardputer-adv-js-gpio-exercizer`) that composes:
  - MicroQuickJS REPL (from `imports/esp32-mqjs-repl`)
  - Control plane component (queue + event definitions)
  - GPIO engine component (based on `0016` gpio_tx + control_plane style)
  - I2C engine component (based on `0004` i2c polling task pattern)
- Add JS bindings that enqueue engine commands instead of doing timing in JS.

### Architecture overview

```text
USB Serial/JTAG
     |
     v
[MicroQuickJS REPL Task]
     |
     v
[JS bindings -> ControlPlane component]
     |                     |
     v                     v
GpioEngine component   I2cEngine component
(timer task)           (I2C worker task)
```

## Concrete code layout

Proposed project layout (components + firmware):

```
esp32-s3-m5/components/
  exercizer_control/
    include/exercizer/ControlPlane.h
    src/ControlPlane.cpp
  exercizer_gpio/
    include/exercizer/GpioEngine.h
    src/GpioEngine.cpp
  exercizer_i2c/
    include/exercizer/I2cEngine.h
    src/I2cEngine.cpp

esp32-s3-m5/0039-cardputer-adv-js-gpio-exercizer/
  main/
    app_main.cpp
    js_bindings.cpp
    js_bindings.h
```

Reuse references (existing sources):

- REPL + JS engine:
  - `imports/esp32-mqjs-repl/mqjs-repl/main/app_main.cpp`
  - `imports/esp32-mqjs-repl/mqjs-repl/main/repl/ReplLoop.cpp`
  - `imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp`
- GPIO timing pattern:
  - `0016-atoms3r-grove-gpio-signal-tester/main/gpio_tx.cpp`
  - `0016-atoms3r-grove-gpio-signal-tester/main/control_plane.cpp`
- I2C worker pattern:
  - `0004-i2c-rolling-average/main/hello_world_main.c`

## Detailed implementation plan (pseudocode + reuse)

### 1) Control plane component (reuse 0016 pattern)

**Component**: `components/exercizer_control`

```cpp
// ControlPlane.h
enum class CtrlType : uint8_t {
  GpioConfig,
  GpioStart,
  GpioStop,
  I2cConfig,
  I2cTxn,
  Status,
};

struct CtrlEvent {
  CtrlType type;
  int32_t arg0;
  int32_t arg1;
  uint8_t data[64]; // small payload (GPIO config, I2C bytes)
};

class ControlPlane {
 public:
  void Start();
  bool Send(const CtrlEvent &ev);
  bool Receive(CtrlEvent *out, TickType_t timeout);
};
```

**Reuse**: `0016` `control_plane.cpp` for queue creation and send behavior.

### 2) GPIO engine component (reuse gpio_tx, timer-driven)

**Component**: `components/exercizer_gpio`

- Move the square/pulse/high/low logic from `0016` `gpio_tx.cpp` into a reusable engine.
- Engine should be driven by control events, not by JS loops.

**Pseudocode**:

```cpp
class GpioEngine {
 public:
  void Start(ControlPlane &ctrl);
  void ApplyConfig(const GpioConfig &cfg);
};

void GpioEngine::ApplyConfig(const GpioConfig &cfg) {
  // reuse logic from gpio_tx.cpp
  switch (cfg.mode) {
    case High: set_level(1); break;
    case Low: set_level(0); break;
    case Square: start_periodic_timer(cfg.hz); break;
    case Pulse: start_pulse_timer(cfg.width_us, cfg.period_ms); break;
  }
}
```

**Reuse**: `0016` `gpio_tx.cpp` (esp_timer, square/pulse callbacks).

### 3) I2C engine component (reuse queue + worker pattern)

**Component**: `components/exercizer_i2c`

- Configure bus and device handles using `i2c_master_*` APIs.
- Accept control events that describe transactions (addr, write bytes, read length).
- Run a worker task that dequeues transactions and executes them.

**Pseudocode**:

```cpp
class I2cEngine {
 public:
  void Start(ControlPlane &ctrl);
  void HandleTxn(const I2cTxn &txn);
};

void I2cEngine::HandleTxn(const I2cTxn &txn) {
  for (;;) {
    I2cTxn txn = queue_receive();
    if (txn.write_len) i2c_master_transmit(...);
    if (txn.read_len) i2c_master_receive(...);
  }
}
```

**Reuse**: `0004` `hello_world_main.c` for bus setup and worker task pattern.

### 4) JS bindings (new)

**File**: `main/js_bindings.{h,cpp}`

- Export a small API into JS that emits control events.
- JS functions are thin wrappers around `ControlPlane::Send`.

**Pseudocode**:

```cpp
// JS: gpio.square(pin, hz)
static JSValue js_gpio_square(JSContext *ctx, JSValueConst this_val,
                              int argc, JSValueConst *argv) {
  CtrlEvent ev = {};
  ev.type = CtrlType::GpioStart;
  // pack pin + hz in ev.data
  ctrl.Send(ev);
  return JS_UNDEFINED;
}
```

**Reuse**: Use `JsEvaluator` pattern for setting JS globals; bindings should not do timing.

### 5) app_main wiring

**File**: `main/app_main.cpp`

- Mirror `imports/esp32-mqjs-repl` REPL bring-up, but inject custom bindings.
- Start control-plane queue and the engine tasks before launching REPL.

**Pseudocode**:

```cpp
void app_main() {
  ControlPlane ctrl;
  ctrl.Start();

  GpioEngine gpio;
  I2cEngine i2c;
  gpio.Start(ctrl);
  i2c.Start(ctrl);

  auto console = std::make_unique<UsbSerialJtagConsole>();
  auto evaluator = std::make_unique<ModeSwitchingEvaluator>();
  register_js_bindings(evaluator->Context(), ctrl);
  auto editor = std::make_unique<LineEditor>("js> ");
  auto repl = std::make_unique<ReplLoop>();

  repl->Run(*console, *editor, *evaluator);
}
```

## Design Decisions

- **Queue-based control plane**: keeps JS out of real-time timing loops.
- **Reuse existing timers**: leverage `gpio_tx.cpp` logic to avoid re-deriving timer behavior.
- **Single JS task**: MicroQuickJS remains isolated to the REPL task.
- **I2C worker task**: explicit queue decoupling avoids JS-side blocking I2C calls.

## Alternatives Considered

- **JS-driven timing**: rejected due to jitter and non-deterministic scheduling.
- **RMT-only GPIO**: deferred for now; timer-based is simpler and sufficient for initial tests.
- **Synchronous I2C calls in JS**: rejected to avoid blocking REPL and to keep I2C state centralized.

## Implementation Plan (tasks)

1) Create new C++ components for ControlPlane, GpioEngine, and I2cEngine.
2) Create new firmware target directory and copy scaffolding from `imports/esp32-mqjs-repl`.
3) Implement control plane component and GPIO engine (based on `0016`).
4) Implement I2C engine component (based on `0004`).
5) Add JS bindings to enqueue GPIO/I2C commands via ControlPlane.
6) Wire app_main and validate basic REPL + GPIO + I2C flows.

## Open Questions

- Which pins will be the default GPIO/I2C pins on Cardputer-ADV?
- Do we want immediate SPIFFS autoload support in v1?
- When do we introduce the high-frequency VM backend (RMT)?
