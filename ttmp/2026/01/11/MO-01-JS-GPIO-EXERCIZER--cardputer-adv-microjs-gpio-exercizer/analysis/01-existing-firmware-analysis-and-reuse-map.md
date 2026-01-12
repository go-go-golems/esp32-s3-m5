---
Title: Existing Firmware Analysis and Reuse Map
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
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0004-i2c-rolling-average/main/hello_world_main.c
      Note: I2C master + timer queue pattern
    - Path: esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/gpio_tx.cpp
      Note: Timer-based GPIO square/pulse generation
    - Path: esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/manual_repl.cpp
      Note: Deterministic USB Serial/JTAG control plane
    - Path: esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/uart_tester.cpp
      Note: UART TX/RX worker tasks and buffer pattern
    - Path: esp32-s3-m5/0037-cardputer-adv-fan-control-console/main/fan_console.c
      Note: Pattern scheduling loop for on/off sequences
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp
      Note: MicroQuickJS context and eval behavior
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/repl/ReplLoop.cpp
      Note: REPL loop mechanics and command handling
ExternalSources: []
Summary: Survey of ESP32-S3 firmware examples to reuse for a MicroQuickJS GPIO/protocol exercizer.
LastUpdated: 2026-01-11T18:35:22.006735529-05:00
WhatFor: Identify reusable firmware patterns for JS-driven GPIO, UART, and I2C signal generation on Cardputer-ADV.
WhenToUse: Use before designing the firmware architecture or JS API surface for the GPIO exercizer.
---


# Existing Firmware Analysis and Reuse Map

## Goal

We need a Cardputer-ADV firmware that accepts JS over REPL and generates repeatable GPIO and protocol signals (PWM-like toggles, UART frames, I2C traffic, etc). This document maps existing ESP32-S3 examples in `esp32-s3-m5/` so we can reuse proven patterns for REPL I/O, scheduling, and signal generation.

## Constraints and guiding facts

- MicroQuickJS runs in a single task with a fixed memory pool; other tasks must not call the JS API directly.
- High precision waveforms should be delegated to hardware (RMT/LEDC) or timer-driven tasks; JS itself should only configure and enqueue work.
- USB Serial/JTAG is the preferred REPL transport for Cardputer-class boards in this repo.

## Reuse Map (fast scan)

| Firmware | Files / Symbols | Reuse value |
| --- | --- | --- |
| `imports/esp32-mqjs-repl/mqjs-repl` | `app_main.cpp`, `repl/ReplLoop.cpp`, `eval/JsEvaluator.cpp`, `console/UsbSerialJtagConsole.cpp`, `storage/Spiffs.cpp` | Working MicroQuickJS REPL, console abstraction, JS eval, SPIFFS autoload. |
| `0016-atoms3r-grove-gpio-signal-tester` | `manual_repl.cpp`, `control_plane.cpp`, `signal_state.cpp`, `gpio_tx.cpp`, `gpio_rx.cpp`, `uart_tester.cpp` | GPIO TX/RX patterns, UART tester, deterministic manual REPL with control queue. |
| `0004-i2c-rolling-average` | `hello_world_main.c` | I2C master bus config + polling with esp_timer and queues. |
| `0037-cardputer-adv-fan-control-console` | `fan_console.c` | Pattern scheduling loop (blink, tick, burst, preset sequences). |
| `0038-cardputer-adv-serial-terminal` | `console_repl.cpp` | `esp_console` REPL setup over USB Serial/JTAG, command registration. |
| `0003-gpio-isr-queue-demo` | `hello_world_main.c` | ISR -> queue event pipeline for GPIO edge capture. |

## Deep dives + reuse details

### 1) MicroQuickJS REPL base (imports/esp32-mqjs-repl)

This is the strongest base for the JS REPL. It already encapsulates USB Serial/JTAG, line editing, and JS evaluation in a dedicated task. The ModeSwitchingEvaluator allows bootstrapping with a simple evaluator before enabling JS.

**Key files + symbols**

- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/app_main.cpp`
  - `ReplTaskContext`, `repl_task`, `app_main`
- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/repl/ReplLoop.cpp`
  - `ReplLoop::Run`, `ReplLoop::HandleLine`
- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp`
  - `JsEvaluator::EvalLine`, `JsEvaluator::Reset`, `JsEvaluator::Autoload`
- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/console/UsbSerialJtagConsole.cpp`
  - `UsbSerialJtagConsole::Read`, `UsbSerialJtagConsole::Write`
- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/storage/Spiffs.cpp`
  - `SpiffsEnsureMounted`, `SpiffsReadFile`

**Pseudocode (REPL loop)**

```text
app_main:
  console = UsbSerialJtagConsole()
  evaluator = ModeSwitchingEvaluator()
  editor = LineEditor(prompt)
  repl = ReplLoop()
  spawn repl_task(console, evaluator, editor, repl)

ReplLoop::Run:
  while true:
    n = console.Read(buf)
    for each byte:
      line = editor.FeedByte(byte)
      if line complete:
        HandleLine(line)
        editor.PrintPrompt()
```

**Notes**

- This REPL already defines meta-commands like `:mode`, `:reset`, `:autoload` and prints memory stats.
- `JsEvaluator` uses a fixed-size memory buffer and reads JS from SPIFFS for autoload.

### 2) GPIO signal generator + manual REPL (0016-atoms3r-grove-gpio-signal-tester)

This firmware already does exactly the kind of signal generation we want, just not JS-driven. It uses a deterministic REPL over USB Serial/JTAG and a queue-based control plane to drive GPIO TX, GPIO RX, and UART testing.

**Key files + symbols**

- `esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/hello_world_main.cpp`
  - `app_main` loop consumes `CtrlEvent` queue
- `esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/manual_repl.cpp`
  - `manual_repl_start`, `handle_tokens`, `ctrl_send`
- `esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/control_plane.cpp`
  - `ctrl_init`, `ctrl_send`
- `esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/signal_state.cpp`
  - `tester_state_apply_event` (routes control plane events to TX/RX/UART)
- `esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/gpio_tx.cpp`
  - `gpio_tx_apply`, `gpio_tx_stop` (esp_timer-based square + pulse)
- `esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/gpio_rx.cpp`
  - `gpio_rx_configure`, `gpio_rx_snapshot`
- `esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/uart_tester.cpp`
  - `uart_tester_start_tx`, `uart_tester_start_rx`

**Pseudocode (control plane + GPIO TX)**

```text
manual_repl:
  parse command -> CtrlEvent
  ctrl_send(event)

app_main:
  ctrl_q = ctrl_init()
  while true:
    if xQueueReceive(ctrl_q, ev):
      tester_state_apply_event(ev)

gpio_tx_apply(pin, cfg):
  configure GPIO output
  if cfg.mode == Square:
    esp_timer_start_periodic(half_period_us)
  if cfg.mode == Pulse:
    esp_timer_start_periodic(period_ms)
    esp_timer_start_once(width_us)
```

**Notes**

- `gpio_tx` shows how to generate square waves and pulse trains without a busy loop.
- `uart_tester` shows a pair of FreeRTOS tasks with shared state and a stream buffer for RX.

### 3) I2C polling + timer queue pattern (0004-i2c-rolling-average)

This sample demonstrates how to structure I2C traffic with a timer callback and a worker task to keep the ISR/timer short.

**Key files + symbols**

- `esp32-s3-m5/0004-i2c-rolling-average/main/hello_world_main.c`
  - `i2c_init`, `poll_timer_cb`, `i2c_poll_task`, `esp_timer_start_periodic`

**Pseudocode (I2C worker)**

```text
poll_timer_cb:
  enqueue poll_evt

i2c_poll_task:
  while true:
    evt = xQueueReceive(poll_queue)
    i2c_master_transmit(...)
    i2c_master_receive(...)
```

### 4) Pattern scheduling loops (0037-cardputer-adv-fan-control-console)

This shows how to encode multi-step patterns using a FreeRTOS task with dynamic parameters and presets.

**Key files + symbols**

- `esp32-s3-m5/0037-cardputer-adv-fan-control-console/main/fan_console.c`
  - `anim_task`, `anim_start`, `anim_stop`, preset steps arrays

**Notes**

- The preset table pattern is a good fit for future GPIO waveform templates.
- Task-based sequencing avoids blocking the REPL loop.

### 5) `esp_console` REPL setup (0038-cardputer-adv-serial-terminal)

This is a usable reference for `esp_console` setup over USB Serial/JTAG, especially the guard against USB transport conflicts.

**Key files + symbols**

- `esp32-s3-m5/0038-cardputer-adv-serial-terminal/main/console_repl.cpp`
  - `console_start`, `esp_console_new_repl_usb_serial_jtag`, `esp_console_cmd_register`

### 6) GPIO ISR queue pattern (0003-gpio-isr-queue-demo)

If we need hardware edge capture beyond the 0016 counter, this ISR -> queue pipeline is a clean pattern.

**Key files + symbols**

- `esp32-s3-m5/0003-gpio-isr-queue-demo/main/hello_world_main.c`
  - `gpio_isr_handler`, `gpio_event_task`

## Candidate architecture diagram (reuse-driven)

```text
USB Serial/JTAG
     |
     v
[REPL + MicroQuickJS]  <-- from imports/esp32-mqjs-repl
     |
     v
[Command Queue / Control Plane]  <-- from 0016 control_plane
     |
     +--> [GPIO TX Engine]  <-- 0016 gpio_tx (esp_timer) or RMT
     |
     +--> [GPIO RX Engine]  <-- 0016 gpio_rx + 0003 ISR queue
     |
     +--> [UART Engine]     <-- 0016 uart_tester
     |
     +--> [I2C Engine]      <-- 0004 i2c_poll_task
```

## Reuse recommendations

- Start from `imports/esp32-mqjs-repl/mqjs-repl` for the REPL and JS evaluator; this avoids re-deriving the USB Serial/JTAG transport and JS context lifecycle.
- Reuse `0016` control-plane queue pattern to decouple JS input from hardware tasks.
- Prefer `esp_timer` or RMT/LEDC for accurate waveforms; JS should enqueue parameterized patterns, not toggle pins directly.

## Open gaps (need new work)

- RMT/LEDC waveform generation for higher-frequency and protocol-accurate waveforms (not covered in these samples).
- JS API bindings for GPIO, UART, and I2C (C wrappers + JS glue) that sit above the control plane.
- Protocol simulators (I2C master scripts, UART framing, PWM sweeps) with adjustable timing precision.
