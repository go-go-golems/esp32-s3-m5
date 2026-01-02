---
Title: Cardputer Port Requirements
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

# Cardputer Port Requirements

## Overview

Analysis of differences between the current generic ESP32-S3 firmware configuration and M5Stack Cardputer requirements, identifying necessary changes for porting.

## Device Comparison

### Hardware Specifications

| Feature | Current (Generic ESP32-S3) | Cardputer |
|---------|---------------------------|-----------|
| **SoC** | ESP32-S3 (Xtensa LX7 dual-core) | ESP32-S3 (Xtensa LX7 dual-core) |
| **Flash** | 2MB configured | 8MB typical |
| **SRAM** | Generic (assumed standard) | 512KB only (no PSRAM) |
| **Console** | UART_NUM_0 (GPIO 44/43) | USB Serial JTAG (native USB) |
| **Display** | None | ST7789 SPI (240×135, RGB565) |
| **Keyboard** | None | 3×7 GPIO matrix |
| **Speaker** | None | I2S (GPIO 42/41/43) |
| **SD Card** | None | Yes (via SPI) |

## Required Changes

### 1. Console Interface Migration

**Current:** UART driver (`uart_read_bytes`, `uart_write_bytes`)

**Required:** USB Serial JTAG driver (`usb_serial_jtag_read_bytes`, `usb_serial_jtag_write_bytes`)

**Reference Implementation:** `0015-cardputer-serial-terminal/main/hello_world_main.cpp`

**Changes Needed:**
- Replace UART driver initialization with USB Serial JTAG driver
- Update read/write calls in `repl_task()` and `process_char()`
- Ensure driver is installed before use

**Code Pattern:**
```cpp
usb_serial_jtag_driver_config_t cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
cfg.rx_buffer_size = 1024;
cfg.tx_buffer_size = 1024;
esp_err_t err = usb_serial_jtag_driver_install(&cfg);

// Read: usb_serial_jtag_read_bytes(data, max_len, 0)
// Write: usb_serial_jtag_write_bytes(data, len, 0)
```

### 2. Flash Size Configuration

**Current:** 2MB (`CONFIG_ESPTOOLPY_FLASHSIZE="2MB"`)

**Required:** 8MB (`CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y`)

**Location:** `sdkconfig.defaults` or `sdkconfig`

**Cardputer Pattern (from tutorials):**
```kconfig
CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y
CONFIG_ESPTOOLPY_FLASHSIZE="8MB"
```

### 3. Partition Table Update

**Current Partition Table:**
```csv
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x100000,  # 1MB app
storage,  data, spiffs,  0x110000,0xF0000,   # 960KB SPIFFS
```

**Cardputer Typical Partition Table:**
```csv
nvs,      data, nvs,     0x9000,  0x5000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 4M,        # 4MB app
storage,  data, fat,     ,        1M,         # 1MB storage (FAT or SPIFFS)
```

**Changes Needed:**
- Increase app partition from 1MB to 4MB
- Increase storage partition from 960KB to 1MB
- Consider FAT vs SPIFFS (Cardputer tutorials use FAT, but SPIFFS should work)

### 4. Memory Constraints

**Cardputer Memory Budget:**
- Total SRAM: 512KB
- Display buffers: ~64KB (if using display)
- FreeRTOS overhead: Variable
- Application code/data: Variable
- **Available for JS heap + buffers:** ~448KB (without display) or ~384KB (with display)

**Current Usage:**
- JavaScript heap: 64KB
- UART buffers: 2KB
- SPIFFS buffers: Variable
- FreeRTOS: Variable

**Assessment:** Current 64KB JS heap should fit, but need to verify total memory usage doesn't exceed Cardputer's constraints.

### 5. CPU Frequency

**Current:** Not explicitly set (defaults to 160MHz or 240MHz)

**Cardputer Pattern:** 240MHz (`CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y`)

**Location:** `sdkconfig.defaults`

### 6. Main Task Stack Size

**Current:** Default (likely 4096 bytes)

**Cardputer Pattern:** 8000 bytes (`CONFIG_ESP_MAIN_TASK_STACK_SIZE=8000`)

**Reason:** C++ applications (like M5GFX) can be stack-hungry

## Optional Enhancements

### Keyboard Input Integration
- Cardputer has built-in 3×7 GPIO matrix keyboard
- Could integrate keyboard input as alternative to USB Serial JTAG
- Reference: `0012-cardputer-typewriter` for keyboard scanning

### Display Output Integration
- Cardputer has ST7789 SPI display (240×135)
- Could show REPL output on screen
- Reference: `0011-cardputer-m5gfx-plasma-animation` for M5GFX usage

### Speaker Integration
- Cardputer has I2S speaker
- Could add audio feedback for REPL actions
- Reference: `007-CARDPUTER-SYNTH` for I2S speaker usage

## Configuration File Changes

### sdkconfig.defaults (Create New)

```kconfig
# Cardputer defaults (ESP-IDF 5.4.1)

# Flash size: Cardputer is commonly 8MB.
CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y
CONFIG_ESPTOOLPY_FLASHSIZE="8MB"

# Partition table: project-local partitions.csv
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
CONFIG_PARTITION_TABLE_FILENAME="partitions.csv"

# CPU frequency: Cardputer demos usually run at 240MHz.
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ=240

# Main task stack: C++ can be stack-hungry.
CONFIG_ESP_MAIN_TASK_STACK_SIZE=8000
```

### partitions.csv (Update)

```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 4M,
storage,  data, spiffs,  ,        1M,
```

## Code Changes Summary

1. **legacy/main.c (pre-split reference):**
   - Replace UART driver with USB Serial JTAG driver
   - Update `repl_task()` to use `usb_serial_jtag_read_bytes`
   - Update `printf` calls (should work via VFS, but verify)

2. **CMakeLists.txt:**
   - No changes needed (component structure should work)

3. **sdkconfig.defaults:**
   - Create new file with Cardputer defaults

4. **partitions.csv:**
   - Update partition sizes for 8MB flash

## Testing Considerations

### QEMU Testing
- QEMU may not accurately reflect Cardputer's memory constraints
- USB Serial JTAG may not work in QEMU (verify)
- May need to keep UART option for QEMU testing

### Hardware Testing
- Verify memory usage on real Cardputer hardware
- Test USB Serial JTAG driver installation and operation
- Verify SPIFFS partition mounting and file operations
- Test JavaScript engine initialization and REPL operation

## References

- Cardputer serial terminal: `0015-cardputer-serial-terminal/`
- Cardputer keyboard: `0012-cardputer-typewriter/`
- Cardputer display: `0011-cardputer-m5gfx-plasma-animation/`
- Cardputer audio: `007-CARDPUTER-SYNTH` (ticket analysis)
- USB Serial JTAG: `esp32-s3-m5/ttmp/2025/12/23/008-ATOMS3R-GIF-CONSOLE/playbooks/01-esp-console-repl-usb-serial-jtag-quickstart.md`
