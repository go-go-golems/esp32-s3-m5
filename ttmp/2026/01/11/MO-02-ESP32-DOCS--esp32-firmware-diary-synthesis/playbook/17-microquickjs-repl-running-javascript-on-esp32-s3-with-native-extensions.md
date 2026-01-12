---
Title: 'MicroQuickJS REPL: Running JavaScript on ESP32-S3 with Native Extensions'
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - esp32
    - firmware
    - documentation
    - javascript
    - repl
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/app_main.cpp
      Note: Baseline REPL entry point
    - Path: esp32-s3-m5/0039-cardputer-adv-js-gpio-exercizer/main/app_main.cpp
      Note: Extended REPL with GPIO/I2C bindings
    - Path: esp32-s3-m5/0039-cardputer-adv-js-gpio-exercizer/main/esp32_stdlib_runtime.c
      Note: Native binding implementations
ExternalSources:
    - URL: https://bellard.org/quickjs/
      Note: QuickJS reference (MicroQuickJS is a lightweight derivative)
Summary: 'Complete guide to running a JavaScript REPL on ESP32-S3 and extending it with native C bindings for hardware control.'
LastUpdated: 2026-01-12T15:00:00-05:00
WhatFor: 'Teach developers to run JavaScript on ESP32-S3 and add native extensions.'
WhenToUse: 'Use when building JS-scriptable embedded applications.'
---

# MicroQuickJS REPL: Running JavaScript on ESP32-S3 with Native Extensions

## Introduction

JavaScript on embedded devices enables rapid prototyping, interactive debugging, and user-scriptable behavior. This guide teaches you how to run a JavaScript REPL on ESP32-S3 using MicroQuickJS, and how to extend it with native C bindings for hardware control.

MicroQuickJS is a lightweight JavaScript engine derived from QuickJS, optimized for constrained environments. Combined with a REPL (Read-Eval-Print Loop), it provides an interactive shell where you can type JavaScript and see results immediately—directly on the device.

By the end of this guide, you will:
- Understand the REPL architecture and its components
- Run the baseline JS REPL on your device
- Load JavaScript files from SPIFFS storage
- Add custom native bindings for GPIO, I2C, or any hardware

---

## What is MicroQuickJS?

MicroQuickJS is a table-driven JavaScript engine designed for embedded systems:

- **Small footprint** — Runs in a 64 KiB heap arena
- **No dynamic allocation during parsing** — Uses pre-generated atom and stdlib tables
- **ES2020 subset** — Core JavaScript features without browser APIs
- **C integration** — Easy to expose native functions to JS

The REPL wraps MicroQuickJS with console I/O, command parsing, and evaluator management.

---

## REPL Architecture

The REPL is deliberately simple and modular:

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         REPL Task (FreeRTOS)                           │
│                                                                         │
│  ┌──────────────┐         ┌──────────────┐         ┌──────────────┐    │
│  │   Console    │         │  LineEditor  │         │   ReplLoop   │    │
│  │  (IConsole)  │ ──────► │              │ ──────► │              │    │
│  │              │         │  - FeedByte  │         │  - HandleLine│    │
│  │  - Read()    │ bytes   │  - backspace │ line    │  - :commands │    │
│  │  - Write()   │         │  - Ctrl+C/U  │         │  - eval      │    │
│  └──────────────┘         └──────────────┘         └──────┬───────┘    │
│                                                           │            │
│                                                           ▼            │
│                                              ┌──────────────────────┐  │
│                                              │ ModeSwitchingEvaluator│ │
│                                              │                      │  │
│                                              │  ┌────────────────┐  │  │
│                                              │  │ RepeatEvaluator│  │  │
│                                              │  └────────────────┘  │  │
│                                              │  ┌────────────────┐  │  │
│                                              │  │  JsEvaluator   │  │  │
│                                              │  └────────────────┘  │  │
│                                              └──────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

| Component | Purpose | Key Methods |
|-----------|---------|-------------|
| **IConsole** | Transport abstraction | `Read()`, `Write()`, `WriteString()` |
| **LineEditor** | Byte → line conversion | `FeedByte()`, `PrintPrompt()` |
| **ReplLoop** | Command dispatch | `Run()`, `HandleLine()` |
| **IEvaluator** | Line evaluation | `EvalLine()`, `Reset()`, `Autoload()` |

This separation keeps each component testable. If JS breaks, the repeat evaluator still proves that input/output works.

---

## Console Backends

The REPL supports two console transports:

### USB Serial/JTAG (Default for Cardputer)

```cpp
ctx->console = std::make_unique<UsbSerialJtagConsole>();
```

- Appears as `/dev/ttyACM*` on Linux
- No external UART needed
- Works with `idf.py monitor`

### UART0 (For QEMU)

```cpp
ctx->console = std::make_unique<UartConsole>(UART_NUM_0, 115200);
```

- Standard serial at 115200 baud
- Required for QEMU testing
- Can conflict with other UART consumers

### Switching Backends

Use the helper script or Kconfig:

```bash
cd imports/esp32-mqjs-repl/mqjs-repl

# For Cardputer (USB Serial/JTAG)
./tools/set_repl_console.sh usb-serial-jtag

# For QEMU (UART0)
./tools/set_repl_console.sh uart
```

Or manually via `idf.py menuconfig`:
- `Component config → REPL Console → USB Serial/JTAG`

---

## Running the Baseline REPL

### Build and Flash

```bash
cd esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl

# Set target
./build.sh set-target esp32s3

# Build and flash
./build.sh build flash monitor
```

### First Interaction

After boot, you'll see the banner and prompt:

```
========================================
  ESP32-S3 REPL (bring-up)
========================================
repl>
```

The default mode is `repeat` (echo evaluator). Switch to JS mode:

```
repl> :mode js
mode: js
js> 1 + 2
3
js> print("hello world")
hello world
undefined
```

---

## Meta-Commands

Lines starting with `:` are handled by the REPL itself, not the evaluator:

| Command | Description |
|---------|-------------|
| `:help` | Print command help |
| `:mode` | Show current evaluator |
| `:mode repeat` | Switch to echo mode |
| `:mode js` | Switch to JavaScript mode |
| `:reset` | Reset the current evaluator (recreate JS context) |
| `:stats` | Print ESP heap stats + JS memory dump |
| `:autoload` | Mount SPIFFS and load `/spiffs/autoload/*.js` |
| `:autoload --format` | Format SPIFFS if mount fails, then load |
| `:prompt TEXT` | Change the prompt string |

### Example: Stats

```
js> :stats
heap_free=180224 heap_min_free=175000
heap_8bit_free=45000 heap_8bit_min_free=42000
heap_spiram_free=0 heap_spiram_min_free=0
memory used: 12345 bytes
```

---

## JavaScript Basics

### Built-in Functions

| Function | Description |
|----------|-------------|
| `print(...)` | Print arguments to console |
| `gc()` | Force garbage collection |
| `load(path)` | Load and evaluate a file from SPIFFS |

### Evaluation Behavior

- Non-`undefined` return values are printed
- Exceptions show stack traces
- Multi-line input is not supported (single-line REPL)

### Example Session

```
js> var x = { a: 1, b: 2 }
undefined
js> x.a + x.b
3
js> function add(a, b) { return a + b; }
undefined
js> add(10, 20)
30
js> throw new Error("oops")
Error: oops
    at <repl>:1
```

---

## SPIFFS Storage

### Overview

SPIFFS provides persistent storage for JavaScript files:

- **Partition**: `storage` at offset `0x410000` (1 MiB)
- **Mount path**: `/spiffs`
- **Autoload directory**: `/spiffs/autoload/*.js`

### Seeded Image

The build includes a seeded SPIFFS image from `spiffs_image/`:

```
spiffs_image/
└── autoload/
    └── 00-seed.js
```

Contents of `00-seed.js`:
```javascript
globalThis.AUTOLOAD_SEED = 123;
print("autoload seed loaded");
```

### Autoload Semantics

| Command | Behavior |
|---------|----------|
| `:autoload` | Mount SPIFFS (no format); error if unmountable |
| `:autoload --format` | Format if mount fails, then mount and load |

**Safe by default**: SPIFFS is never formatted implicitly. User data is preserved unless you explicitly use `--format`.

### Using load()

```
js> load("/spiffs/autoload/00-seed.js")
autoload seed loaded
js> globalThis.AUTOLOAD_SEED
123
```

**Constraints:**
- Maximum file size: 32 KiB
- Path must start with `/spiffs/`

---

## Adding JavaScript Libraries

### Path A: Ship in SPIFFS Image

1. Add a file to `spiffs_image/autoload/`:

```javascript
// spiffs_image/autoload/10-mylib.js
globalThis.MyLib = {
  add: (a, b) => a + b,
  mul: (a, b) => a * b,
};
print("MyLib loaded");
```

2. Rebuild (regenerates `build/storage.bin`):

```bash
./build.sh build
```

3. Flash:

```bash
./build.sh flash
```

4. Use in REPL:

```
js> :autoload --format
autoload: /spiffs/autoload/00-seed.js
autoload: /spiffs/autoload/10-mylib.js
MyLib loaded
js> MyLib.add(2, 3)
5
```

### Path B: Upload at Runtime

(Future work: implement file upload via serial protocol or HTTP)

---

## Adding Native Extensions

To expose hardware functionality to JavaScript, you add native C functions and register them in the stdlib.

### Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          Your Application                               │
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │  esp32_stdlib_runtime.c                                          │  │
│  │                                                                  │  │
│  │  static JSValue js_my_function(JSContext* ctx, JSValue* this_val,│  │
│  │                                 int argc, JSValue* argv) {       │  │
│  │      // Parse arguments                                          │  │
│  │      // Do hardware work                                         │  │
│  │      // Return result                                            │  │
│  │  }                                                               │  │
│  │                                                                  │  │
│  │  #include "esp32_stdlib.h"  // Generated; references js_my_func  │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │  esp32_stdlib.h (generated)                                      │  │
│  │                                                                  │  │
│  │  // Registers js_my_function as global "myFunction"             │  │
│  └──────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────┘
```

### Example: GPIO Control (from 0039)

The GPIO exercizer adds these JS functions:

| JS Function | Native Implementation |
|-------------|----------------------|
| `gpio.high(pin)` | `js_gpio_high()` |
| `gpio.low(pin)` | `js_gpio_low()` |
| `gpio.square(pin, hz)` | `js_gpio_square()` |
| `gpio.pulse(pin, width_us, period_ms)` | `js_gpio_pulse()` |
| `gpio.stop([pin])` | `js_gpio_stop()` |

### Native Function Pattern

```c
static JSValue js_gpio_high(JSContext* ctx, JSValue* this_val, 
                             int argc, JSValue* argv) {
    // 1. Validate arguments
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "gpio.high(pin) requires 1 argument");
    }
    
    // 2. Parse arguments
    int pin = 0;
    if (JS_ToInt32(ctx, &pin, argv[0])) {
        return JS_EXCEPTION;  // Parsing failed
    }
    
    // 3. Do the work (send to control plane)
    exercizer_gpio_config_t cfg = {
        .pin = (uint8_t)pin,
        .mode = EXERCIZER_GPIO_MODE_HIGH,
    };
    exercizer_ctrl_event_t ev = {
        .type = EXERCIZER_CTRL_GPIO_SET,
        .data_len = sizeof(cfg),
    };
    memcpy(ev.data, &cfg, sizeof(cfg));
    
    if (!exercizer_control_send(&ev)) {
        return JS_ThrowTypeError(ctx, "gpio.high: control plane not ready");
    }
    
    // 4. Return result
    return JS_UNDEFINED;
}
```

### Control Plane Pattern

The 0039 example uses a control plane to decouple JS from hardware:

```cpp
// In app_main.cpp
static ControlPlane s_ctrl;
static GpioEngine s_gpio;
static I2cEngine s_i2c;

void app_main(void) {
    // Start control plane
    s_ctrl.Start();
    
    // Register as default for JS bindings
    exercizer_js_bind(&s_ctrl);
    
    // Start control task
    xTaskCreate(&control_task, "ctrl", 4096, &ctx, 5, nullptr);
    
    // Start REPL task
    xTaskCreate(repl_task, "repl", 8192, ctx.release(), 5, nullptr);
}

void control_task(void *arg) {
    while (true) {
        exercizer_ctrl_event_t ev;
        if (!s_ctrl.Receive(&ev, portMAX_DELAY)) continue;
        
        switch (ev.type) {
        case EXERCIZER_CTRL_GPIO_SET:
        case EXERCIZER_CTRL_GPIO_STOP:
            s_gpio.HandleEvent(ev);
            break;
        case EXERCIZER_CTRL_I2C_CONFIG:
        case EXERCIZER_CTRL_I2C_TXN:
            s_i2c.HandleEvent(ev);
            break;
        }
    }
}
```

This pattern:
- Keeps the REPL task responsive
- Allows hardware operations on a dedicated task
- Uses a queue for thread-safe communication

---

## Running the GPIO Exercizer

### Build and Flash

```bash
cd esp32-s3-m5/0039-cardputer-adv-js-gpio-exercizer
idf.py set-target esp32s3
idf.py build flash monitor
```

### Example Session

```
========================================
  Cardputer-ADV JS GPIO Exercizer
========================================
js> gpio.high(1)
undefined
js> gpio.low(1)
undefined
js> gpio.square(2, 1000)
undefined
js> gpio.pulse(3, 500, 1000)
undefined
js> gpio.stop()
undefined
```

### I2C Example

```
js> i2c.config(0, 21, 22, 0x50, 100000)
undefined
js> i2c.tx([0x00, 0x01, 0x02])
undefined
js> i2c.txrx([0x00], 4)
undefined
```

---

## Line Editor Behavior

The line editor is intentionally minimal for embedded reliability:

| Key | Action |
|-----|--------|
| Backspace | Delete last character |
| Ctrl+C | Cancel current line |
| Ctrl+U | Kill entire line |
| Ctrl+L | Clear screen |
| Enter | Submit line |

**Not supported**: Arrow keys, history, multiline input. These would add complexity and potential QEMU/device input regressions.

---

## Memory Model

### JS Heap

```cpp
static constexpr size_t kJsMemSize = 64 * 1024;  // 64 KiB
static uint8_t js_mem_buf_[kJsMemSize];
```

The JS context uses a fixed arena. `:stats` shows usage:

```
js> :stats
memory used: 12345 bytes
```

### SPIFFS Buffers

`load()` reads files into heap-allocated buffers (max 32 KiB per file). Large scripts should be split.

---

## Troubleshooting

### No Input in Monitor

| Symptom | Cause | Solution |
|---------|-------|----------|
| No echo when typing | Wrong console backend | Check `CONFIG_MQJS_REPL_CONSOLE_*` |
| `idf.py monitor` doesn't send | USB enumeration | Use `--port /dev/serial/by-id/...` |
| QEMU hangs on input | Using USB backend | Switch to UART with `set_repl_console.sh uart` |

### SPIFFS Errors

| Symptom | Cause | Solution |
|---------|-------|----------|
| `:autoload` fails | Not formatted | Use `:autoload --format` once |
| `load: file too large` | >32 KiB file | Split the file |
| Files not found | Stale SPIFFS image | Rebuild and reflash |

### JS Errors

| Symptom | Cause | Solution |
|---------|-------|----------|
| `var` not recognized | Atom table mismatch | Regenerate stdlib |
| `OutOfMemory` | 64 KiB exceeded | Use `:reset` and simplify |
| `setTimeout` not supported | Not implemented | Use polling loops |

### Extension Errors

| Symptom | Cause | Solution |
|---------|-------|----------|
| `control plane not ready` | Not initialized | Call `ControlPlane.Start()` |
| Function not found | Not in stdlib | Regenerate `esp32_stdlib.h` |

---

## Validation Scripts

### QEMU Smoke Test

```bash
cd imports/esp32-mqjs-repl/mqjs-repl
./tools/test_js_repl_qemu_uart_stdio.sh --timeout 120
```

### Device Smoke Test

```bash
cd imports/esp32-mqjs-repl/mqjs-repl
./tools/set_repl_console.sh usb-serial-jtag
./build.sh build
./tools/flash_device_usb_jtag.sh --port auto
python3 ./tools/test_js_repl_device_uart_raw.py --port auto --timeout 90
```

---

## Summary

The MicroQuickJS REPL provides a powerful interactive JavaScript environment for ESP32-S3:

1. **Modular architecture** — Console, editor, loop, and evaluator are independent
2. **Multiple backends** — USB Serial/JTAG for hardware, UART for QEMU
3. **Persistent storage** — SPIFFS for shipping and loading JS libraries
4. **Safe autoload** — Never formats implicitly; `--format` is explicit
5. **Native extensions** — C functions callable from JS via stdlib tables
6. **Control plane pattern** — Decouples JS from hardware for responsiveness

Use this pattern to build JS-scriptable embedded applications, interactive debugging tools, and user-programmable devices.

---

## References

- [QuickJS (original)](https://bellard.org/quickjs/)
- `imports/esp32-mqjs-repl/mqjs-repl/docs/repl.md` — REPL component reference
- `imports/esp32-mqjs-repl/mqjs-repl/docs/js.md` — JS component reference
- `imports/esp32-mqjs-repl/mqjs-repl/docs/spiffs.md` — SPIFFS storage reference
- `0039-cardputer-adv-js-gpio-exercizer/` — GPIO/I2C extension example
