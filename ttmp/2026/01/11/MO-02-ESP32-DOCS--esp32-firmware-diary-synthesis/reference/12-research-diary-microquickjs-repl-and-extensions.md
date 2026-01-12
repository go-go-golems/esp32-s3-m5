---
Title: 'Research Diary: MicroQuickJS REPL and Extensions'
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - esp32
    - firmware
    - documentation
    - javascript
    - repl
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/app_main.cpp
      Note: Baseline REPL entry point
    - Path: esp32-s3-m5/0039-cardputer-adv-js-gpio-exercizer/main/app_main.cpp
      Note: Extended REPL with GPIO/I2C bindings
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/docs/repl.md
      Note: REPL component reference
ExternalSources: []
Summary: 'Research diary for the MicroQuickJS REPL and native extensions guide.'
LastUpdated: 2026-01-12T15:00:00-05:00
WhatFor: 'Document the research trail for the MicroQuickJS REPL guide.'
WhenToUse: 'Reference when updating or extending REPL documentation.'
---

# Research Diary: MicroQuickJS REPL and Extensions

## Goal

Investigate the MicroQuickJS REPL architecture and how to extend it with native bindings for hardware control.

---

## Step 1: Understand the REPL Architecture

The REPL is split into three responsibilities:

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         REPL Task (FreeRTOS)                           │
│                                                                         │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐              │
│  │   Console    │    │  LineEditor  │    │   ReplLoop   │              │
│  │  (IConsole)  │────│              │────│              │              │
│  │              │    │  - prompt    │    │  - dispatch  │              │
│  │  - Read()    │    │  - FeedByte  │    │  - :commands │              │
│  │  - Write()   │    │  - backspace │    │  - eval      │              │
│  └──────────────┘    └──────────────┘    └──────────────┘              │
│                                                 │                       │
│                                                 ▼                       │
│                                          ┌──────────────┐              │
│                                          │  Evaluator   │              │
│                                          │ (IEvaluator) │              │
│                                          │              │              │
│                                          │  - repeat    │              │
│                                          │  - js        │              │
│                                          └──────────────┘              │
└─────────────────────────────────────────────────────────────────────────┘
```

### Key Components

1. **IConsole** — Transport abstraction (`Read`, `Write`, `WriteString`)
   - `UartConsole` — For QEMU (UART0)
   - `UsbSerialJtagConsole` — For Cardputer (`/dev/ttyACM*`)

2. **LineEditor** — Minimal line editing
   - Backspace handling
   - Ctrl+C (cancel), Ctrl+U (kill), Ctrl+L (clear)
   - ANSI escapes ignored (no arrow key history)

3. **ReplLoop** — Command dispatch
   - Meta-commands start with `:` (`:help`, `:mode`, etc.)
   - All other lines forwarded to evaluator

4. **IEvaluator** — Line evaluation
   - `RepeatEvaluator` — Echo back (for testing)
   - `JsEvaluator` — MicroQuickJS evaluation

### Entry Point

```cpp
extern "C" void app_main(void) {
    auto ctx = std::make_unique<ReplTaskContext>();
    
#if CONFIG_MQJS_REPL_CONSOLE_USB_SERIAL_JTAG
    ctx->console = std::make_unique<UsbSerialJtagConsole>();
#else
    ctx->console = std::make_unique<UartConsole>(UART_NUM_0, 115200);
#endif

    ctx->evaluator = std::make_unique<ModeSwitchingEvaluator>();
    ctx->editor = std::make_unique<LineEditor>(ctx->evaluator->Prompt());
    ctx->repl = std::make_unique<ReplLoop>();

    xTaskCreate(repl_task, "repl_task", 8192, ctx.release(), 5, nullptr);
}
```

---

## Step 2: Analyze the Mode Switching Evaluator

The `ModeSwitchingEvaluator` wraps multiple evaluators:

```cpp
class ModeSwitchingEvaluator : public IEvaluator {
public:
    bool SetMode(std::string_view mode, std::string* error);
    EvalResult EvalLine(std::string_view line);
    
private:
    RepeatEvaluator repeat_;
    std::optional<JsEvaluator> js_;  // Lazy-constructed
    IEvaluator* current_ = &repeat_;
};
```

**Key insight**: The JS evaluator is lazily constructed when first needed, saving memory until JS mode is explicitly activated.

### Mode Switching

```cpp
bool ModeSwitchingEvaluator::SetMode(std::string_view mode, std::string* error) {
    mode = trim(mode);
    if (mode == "repeat") {
        current_ = &repeat_;
        return true;
    }
    if (mode == "js") {
        if (!js_.has_value()) {
            js_.emplace();  // Construct JS context now
        }
        current_ = &js_.value();
        return true;
    }
    *error = "unknown mode (expected: repeat|js)";
    return false;
}
```

---

## Step 3: Analyze the REPL Loop

The `ReplLoop` reads bytes and dispatches lines:

```cpp
void ReplLoop::Run(IConsole& console, LineEditor& editor, IEvaluator& evaluator) {
    uint8_t buffer[128];

    while (true) {
        const int len = console.Read(buffer, sizeof(buffer), portMAX_DELAY);
        if (len <= 0) continue;

        for (int i = 0; i < len; i++) {
            const auto completed = editor.FeedByte(buffer[i], console);
            if (completed.has_value()) {
                HandleLine(console, editor, evaluator, completed.value());
                editor.PrintPrompt(console);
            }
        }
    }
}
```

### Meta-Commands

```cpp
void ReplLoop::HandleLine(...) {
    const std::string stripped = trim(line);
    
    if (stripped == ":help") { /* print help */ return; }
    if (stripped == ":mode" || stripped.rfind(":mode ", 0) == 0) { /* switch mode */ return; }
    if (stripped == ":reset") { /* reset evaluator */ return; }
    if (stripped == ":stats") { /* print heap + JS stats */ return; }
    if (stripped == ":autoload" || stripped.rfind(":autoload ", 0) == 0) { /* load SPIFFS scripts */ return; }
    if (stripped.rfind(":prompt ", 0) == 0) { /* set prompt */ return; }

    // Not a meta-command: forward to evaluator
    const EvalResult result = evaluator.EvalLine(stripped);
    console.Write(result.output.data(), result.output.size());
}
```

---

## Step 4: Analyze JS Evaluator

The `JsEvaluator` manages the MicroQuickJS context:

```cpp
class JsEvaluator final : public IEvaluator {
public:
    JsEvaluator();
    EvalResult EvalLine(std::string_view line) override;
    bool Reset(std::string* error) override;
    bool GetStats(std::string* out) override;
    bool Autoload(bool format, std::string* out, std::string* error) override;

private:
    static constexpr size_t kJsMemSize = 64 * 1024;
    static uint8_t js_mem_buf_[kJsMemSize];
    JSContext* ctx_ = nullptr;
};
```

**Memory model**: Fixed 64 KiB arena for JS heap.

### Evaluation

```cpp
EvalResult JsEvaluator::EvalLine(std::string_view line) {
    EvalResult result;
    
    // Evaluate with REPL flags
    JSValue val = JS_Eval(ctx_, line.data(), line.size(), 
                          "<repl>", JS_EVAL_REPL | JS_EVAL_RETVAL);
    
    if (JS_IsException(ctx_, val)) {
        // Format exception
        JSValue exc = JS_GetException(ctx_);
        JS_PrintValueF(ctx_, exc, JS_DUMP_LONG);
        result.ok = false;
    } else if (!JS_IsUndefined(val)) {
        // Print return value
        JS_PrintValueF(ctx_, val, JS_DUMP_LONG);
        result.ok = true;
    }
    
    return result;
}
```

---

## Step 5: Analyze SPIFFS Autoload

Autoload scans `/spiffs/autoload/*.js` and evaluates each file:

```cpp
bool JsEvaluator::Autoload(bool format, std::string* out, std::string* error) {
    // Mount SPIFFS
    esp_err_t err = SpiffsEnsureMounted(format);
    if (err != ESP_OK) {
        *error = "SPIFFS mount failed";
        return false;
    }
    
    // Scan directory
    DIR* dir = opendir("/spiffs/autoload");
    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL) {
        if (!ends_with(ent->d_name, ".js")) continue;
        
        // Read and evaluate file
        char path[64];
        snprintf(path, sizeof(path), "/spiffs/autoload/%s", ent->d_name);
        
        uint8_t* buf;
        size_t len;
        SpiffsReadFile(path, &buf, &len);
        
        JSValue val = JS_Eval(ctx_, (char*)buf, len, path, 0);
        free(buf);
        
        *out += "autoload: ";
        *out += path;
        *out += "\n";
    }
    closedir(dir);
    
    return true;
}
```

### Seeded Script

The build includes a seed file at `spiffs_image/autoload/00-seed.js`:

```javascript
globalThis.AUTOLOAD_SEED = 123;
print("autoload seed loaded");
```

This proves autoload is working when tested.

---

## Step 6: Analyze Native Extensions (0039)

The GPIO exercizer extends the REPL with hardware control bindings.

### Extension Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                            app_main.cpp                                │
│                                                                         │
│  ControlPlane.Start()                                                  │
│        │                                                               │
│        ▼                                                               │
│  exercizer_js_bind(&s_ctrl)  ──────────────────────────────────────┐   │
│        │                                                           │   │
│        │                                                           ▼   │
│        │     ┌────────────────────────────────────────────────────────┐│
│        │     │  esp32_stdlib_runtime.c                                ││
│        │     │                                                        ││
│        │     │  js_gpio_high(ctx, ...)  ───┐                          ││
│        │     │  js_gpio_low(ctx, ...)   ───┼───► exercizer_control_   ││
│        │     │  js_gpio_square(ctx, ...) ──┤     send(&event)         ││
│        │     │  js_gpio_pulse(ctx, ...)  ──┘                          ││
│        │     │                                                        ││
│        │     │  js_i2c_config(ctx, ...)  ──┬───► exercizer_control_   ││
│        │     │  js_i2c_tx(ctx, ...)      ──┘     send(&event)         ││
│        │     └────────────────────────────────────────────────────────┘│
│        ▼                                                               │
│  ┌────────────────┐          ┌────────────────┐                        │
│  │ Control Task   │◄─────────│ Control Queue  │◄─── events             │
│  │                │          └────────────────┘                        │
│  │ GpioEngine     │                                                    │
│  │ I2cEngine      │                                                    │
│  └────────────────┘                                                    │
└─────────────────────────────────────────────────────────────────────────┘
```

### Binding Registration

```cpp
// In app_main.cpp
static ControlPlane s_ctrl;

void app_main(void) {
    s_ctrl.Start();
    exercizer_js_bind(&s_ctrl);  // Set as default control plane
    // ... start REPL
}

// In js_bindings.cpp
void exercizer_js_bind(ControlPlane *ctrl) {
    exercizer_control_set_default(static_cast<void *>(ctrl));
}
```

### Native JS Function Example

```c
static JSValue js_gpio_high(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
    if (argc < 1) {
        return js_throw_ctrl(ctx, "gpio.high(pin) requires 1 argument");
    }
    
    int pin = 0;
    if (JS_ToInt32(ctx, &pin, argv[0])) {
        return JS_EXCEPTION;
    }

    // Build control event
    exercizer_gpio_config_t cfg = {
        .pin = (uint8_t)pin,
        .mode = EXERCIZER_GPIO_MODE_HIGH,
    };
    exercizer_ctrl_event_t ev = {
        .type = EXERCIZER_CTRL_GPIO_SET,
        .data_len = sizeof(cfg),
    };
    memcpy(ev.data, &cfg, sizeof(cfg));
    
    // Send to control plane
    if (!exercizer_control_send(&ev)) {
        return js_throw_ctrl(ctx, "gpio.high: control plane not ready");
    }
    
    return JS_UNDEFINED;
}
```

### Stdlib Integration

Native functions are referenced in the generated `esp32_stdlib.h` and included at the end of `esp32_stdlib_runtime.c`:

```c
// At the end of esp32_stdlib_runtime.c
#include "esp32_stdlib.h"
```

The `esp32_stdlib.h` contains the table that registers functions like `gpio.high`, `gpio.low`, `i2c.config`, etc. into the JS global object.

---

## Quick Reference

### REPL Meta-Commands

| Command | Description |
|---------|-------------|
| `:help` | Show command help |
| `:mode` | Show current evaluator |
| `:mode repeat\|js` | Switch evaluator |
| `:reset` | Reset current evaluator |
| `:stats` | Print heap + JS memory stats |
| `:autoload` | Load `/spiffs/autoload/*.js` |
| `:autoload --format` | Format SPIFFS if needed, then load |
| `:prompt TEXT` | Set prompt string |

### JS Built-in Functions

| Function | Description |
|----------|-------------|
| `print(...)` | Print to console |
| `gc()` | Force garbage collection |
| `load(path)` | Load and eval a SPIFFS file |

### GPIO Extension Functions (0039)

| Function | Description |
|----------|-------------|
| `gpio.high(pin)` | Set pin high |
| `gpio.low(pin)` | Set pin low |
| `gpio.square(pin, hz)` | Generate square wave |
| `gpio.pulse(pin, width_us, period_ms)` | Generate periodic pulses |
| `gpio.stop([pin])` | Stop waveform (all or specific pin) |
| `gpio.setMany([...])` | Configure multiple pins |

### I2C Extension Functions (0039)

| Function | Description |
|----------|-------------|
| `i2c.config(port, sda, scl, addr, hz)` | Configure I2C |
| `i2c.tx(bytes)` | Send bytes |
| `i2c.txrx(bytes, readLen)` | Send then receive |

### Console Backend Selection

```bash
# USB Serial/JTAG (for Cardputer)
cd imports/esp32-mqjs-repl/mqjs-repl
./tools/set_repl_console.sh usb-serial-jtag

# UART0 (for QEMU)
./tools/set_repl_console.sh uart
```
