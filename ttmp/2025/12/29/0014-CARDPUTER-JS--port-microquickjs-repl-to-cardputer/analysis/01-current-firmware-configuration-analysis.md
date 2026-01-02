---
Title: Legacy Firmware Configuration Analysis
Ticket: 0014-CARDPUTER-JS
Status: active
Topics:
    - esp32s3
    - esp-idf
    - cardputer
    - javascript
    - microquickjs
    - qemu
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-29T13:30:00.000000000-05:00
WhatFor: ""
WhenToUse: ""
---

# Legacy Firmware Configuration Analysis

## Overview

Analysis of the imported (pre-refactor) MicroQuickJS REPL firmware to understand its original configuration, structure, and device target before porting to Cardputer. The current firmware entrypoint is `imports/esp32-mqjs-repl/mqjs-repl/main/app_main.cpp`; the pre-split monolith was later deleted after being captured in git history.

## Firmware Structure

**Project Location:** `imports/esp32-mqjs-repl/mqjs-repl/`

**ESP-IDF Version:** v5.4.1 (sdkconfig shows v5.4.1, though delivery docs mention v5.5.2)

**Components:**
- `main/` - Main application with REPL implementation
- `components/mquickjs/` - MicroQuickJS JavaScript engine as ESP-IDF component

## Current Device Configuration

### Target Hardware
- **Device:** Generic ESP32-S3 (Xtensa LX7 dual-core)
- **Flash:** 2MB configured (`CONFIG_ESPTOOLPY_FLASHSIZE="2MB"`)
- **Memory:** 64KB JavaScript heap, ~300KB free RAM reported

### Console Interface
- **Type:** UART_NUM_0 (hardware UART)
- **GPIO Pins:** GPIO 44 (TX), GPIO 43 (RX)
- **Baud Rate:** 115200
- **Implementation:** Raw UART driver (`uart_read_bytes`, `uart_write_bytes`)

### Storage
- **File System:** SPIFFS
- **Partition:** `storage` (data, spiffs)
- **Offset:** 0x110000
- **Size:** 960KB (0xF0000)
- **Base Path:** `/spiffs`
- **Autoload Directory:** `/spiffs/autoload/`

### Partition Table

```csv
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x100000,
storage,  data, spiffs,  0x110000,0xF0000,
```

**Layout:**
- Bootloader: 0x0000 - 0x8000
- Partition Table: 0x8000 - 0x9000
- NVS: 0x9000 - 0xF000 (24KB)
- PHY Init: 0xF000 - 0x10000 (4KB)
- Factory App: 0x10000 - 0x110000 (1MB)
- SPIFFS Storage: 0x110000 - 0x200000 (960KB)

## Main Application Features

### JavaScript Engine
- **Engine:** MicroQuickJS (ES5.1 compliant)
- **Heap Size:** 64KB (`JS_MEM_SIZE = 64 * 1024`)
- **Memory Pool:** Pre-allocated static buffer (`uint8_t js_mem_buf[JS_MEM_SIZE]`)

### REPL Implementation
- **Input:** Line-based from UART (512 byte buffer)
- **Output:** Direct to UART via `printf`
- **Features:**
  - Backspace/DEL support for line editing
  - Error handling and display
  - Prompt: `js> `

### Library Loading
- **Automatic Loading:** On startup, loads all `.js` files from `/spiffs/autoload/`
- **Example Libraries:** `math.js`, `string.js`, `array.js` (created on first boot)
- **Known Issue:** Library syntax errors due to MicroQuickJS parser strictness (documented in delivery notes)

## QEMU Execution

### Build and Run
```bash
cd imports/esp32-mqjs-repl/mqjs-repl
idf.py build
idf.py qemu monitor
```

### QEMU Configuration
- **Console:** Socket connection on `localhost:5555`
- **Baud:** 115200
- **Test Script:** `test_storage_repl.py` demonstrates socket-based testing

### Boot Sequence (from logs)
1. SPIFFS initialization and formatting (if needed)
2. JavaScript engine initialization (64KB heap)
3. Example library creation on first boot
4. Library loading from `/spiffs/autoload/`
5. REPL prompt display

## Code Structure

### Legacy Main Entry Point (historical)

**Key Functions:**
- `app_main()` - Initialization and setup
- `init_spiffs()` - SPIFFS mount and initialization
- `init_js_engine()` - JavaScript context creation
- `load_autoload_libraries()` - Library loading from SPIFFS
- `repl_task()` - FreeRTOS task for REPL loop
- `process_char()` - Character-by-character input processing
- `eval_and_print()` - JavaScript evaluation and output

**UART Configuration:**
```c
#define UART_NUM UART_NUM_0
#define BUF_SIZE (1024)

uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
};
ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
```

## Memory Usage

**Reported Free Heap:** ~300KB (from boot logs)

**Allocations:**
- JavaScript heap: 64KB (static)
- UART buffers: 2KB (BUF_SIZE * 2)
- SPIFFS buffers: Variable (depends on file operations)
- FreeRTOS overhead: Variable (tasks, stacks)

## Known Limitations

1. **JavaScript Syntax:** ES5.1 only, no ES6+ features
2. **Standard Library:** Minimal stdlib (no Math, Date, RegExp, JSON by default)
3. **REPL Input:** Basic line editing only (backspace), no history or multi-line
4. **Library Loading:** Example libraries have syntax issues (storage mechanism works)

## References

- Legacy main implementation: (deleted; see git history)
- Partition table: `imports/esp32-mqjs-repl/mqjs-repl/partitions.csv`
- Configuration: `imports/esp32-mqjs-repl/mqjs-repl/sdkconfig`
- QEMU logs: `imports/qemu_storage_repl.txt`
- Test script: `imports/test_storage_repl.py`
