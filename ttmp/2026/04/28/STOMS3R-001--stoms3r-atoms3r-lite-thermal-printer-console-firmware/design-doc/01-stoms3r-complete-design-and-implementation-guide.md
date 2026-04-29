---
title: "SToMS3R Complete Design and Implementation Guide"
tags:
  - design-doc
  - esp32s3
  - atoms3r
  - thermal-printer
  - console
  - wifi
  - esp-idf
  - firmware
  - provisioning
  - escpos
created: 2026-04-28
status: active
intent: long-term
---

# SToMS3R — AtomS3R Lite Thermal Printer Console Firmware

## Complete Design & Implementation Guide

> **Audience:** A new intern joining the embedded firmware team. You should be
> comfortable reading C code and have basic familiarity with serial terminals.
> This guide explains every layer of the system — from the hardware on your desk
> to the last line of firmware — so you can build, flash, and extend the firmware
> with confidence.

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [What Is This Project?](#2-what-is-this-project)
3. [Hardware — The AtomS3R Lite Board](#3-hardware--the-atoms3r-lite-board)
4. [Hardware — The M5Stack Thermal Printer (K118)](#4-hardware--the-m5stack-thermal-printer-k118)
5. [Wiring Diagram: AtomS3R Lite ↔ Thermal Printer](#5-wiring-diagram-atoms3r-lite---thermal-printer)
6. [Software Stack Overview](#6-software-stack-overview)
7. [ESP-IDF Build System Primer](#7-esp-idf-build-system-primer)
8. [Console Subsystem (esp_console)](#8-console-subsystem-esp_console)
9. [WiFi Console Commands](#9-wifi-console-commands)
10. [Thermal Printer Console Commands](#10-thermal-printer-console-commands)
11. [ESC/POS Protocol Deep Dive](#11-escpos-protocol-deep-dive)
12. [NVS Key-Value Storage](#12-nvs-key-value-storage)
13. [Project Directory Structure](#13-project-directory-structure)
14. [File-by-File Implementation Guide](#14-file-by-file-implementation-guide)
15. [sdkconfig.defaults Explained](#15-sdkconfigdefaults-explained)
16. [Build, Flash, and Monitor Workflow](#16-build-flash-and-monitor-workflow)
17. [Testing Plan](#17-testing-plan)
18. [Troubleshooting Guide](#18-troubleshooting-guide)
19. [Future Extensions](#19-future-extensions)
20. [Appendix A — ESC/POS Command Quick Reference](#appendix-a--escpos-command-quick-reference)
21. [Appendix B — AtomS3R Lite Pin Map](#appendix-b--atoms3r-lite-pin-map)
22. [Appendix C — Related Tickets and References](#appendix-c--related-tickets-and-references)

---

## 1. Executive Summary

SToMS3R ("Screw This, On My S3R") is a firmware project that turns an M5Stack
AtomS3R Lite — a tiny ESP32-S3 board — into a networked thermal printer
controller. You connect the board to an M5Stack K118 thermal printer kit over
UART, flash the firmware, and then interact with everything through a text
console over USB.

The firmware provides two groups of console commands:

- **WiFi commands** — scan for networks, join an access point, check connection
  status, and disconnect. Credentials are saved to NVS flash so they survive
  power cycles.
- **Printer commands** — print plain text, feed paper, print barcodes, print QR
  codes, and print monochrome bitmaps, all using the industry-standard ESC/POS
  protocol spoken over a serial UART link.

There is no display, no web UI, and no mobile app needed. Everything is driven
from a USB serial terminal (minicom, `idf.py monitor`, screen, PuTTY, etc.).
This keeps the firmware small, debuggable, and extensible.

### Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| ESP-IDF (not Arduino) | First-class RTOS, better UART driver, NVS integration, `esp_console` built-in |
| USB Serial/JTAG console | UART pins are free for the printer; no pin conflict |
| `esp_console` REPL | Line editing, history, tab completion, argument parsing for free |
| NVS for WiFi credentials | Persistent storage without a filesystem |
| Modular C files | One module per subsystem (WiFi, printer, console registration) |

---

## 2. What Is This Project?

### The Short Version

You have two pieces of hardware on your desk:

1. **An M5Stack AtomS3R Lite** — a 24 mm × 24 mm development board built around
   the ESP32-S3-PICO-1-N8R8 chip. It has Wi-Fi, Bluetooth, 8 MB of flash, and
   8 MB of PSRAM. It exposes a USB-C port for power, programming, and serial
   communication, plus a row of GPIO pins along the bottom edge.

2. **An M5Stack ATOM Thermal Printer Kit (K118)** — a 58 mm thermal printer
   mechanism (the kind used in receipt printers) on a carrier board. The carrier
   board connects to a controller board via a 4-wire UART cable at 9600 baud.

The firmware we are building runs on the AtomS3R Lite and drives the printer
through that UART link.

### The Longer Version

The M5Stack ecosystem originally shipped an **ATOM Lite** (based on the older
ESP32-PICO-D4) as the controller for the K118 printer kit. The original firmware
was written in Arduino C++ and provided a web-based UI and MQTT integration.
That firmware worked, but it had several limitations:

- The Arduino framework hides a lot of the ESP32's capabilities.
- The web UI requires a browser and adds complexity.
- The ESP32-PICO-D4 has only 520 KB of SRAM and no PSRAM, limiting bitmap
  printing.
- UART pins on the ATOM Lite overlapped with the USB serial console, requiring
  careful workarounds.

We are upgrading to the **AtomS3R Lite** specifically because:

- The **ESP32-S3** has a built-in **USB Serial/JTAG** peripheral, which gives us
  a dedicated console path that is completely separate from any UART pins.
- 8 MB of **PSRAM** means we can buffer full-page bitmaps for printing.
- The ESP-IDF framework gives us direct access to the UART driver, NVS, and the
  `esp_console` library without Arduino abstraction layers.

The name **SToMS3R** is a backronym: the project was originally attempted on the
ATOM Lite and it did not go smoothly. This is the "do it right this time" version,
on the S3R.

### What You Will Learn

By the end of this guide, you will understand:

- How the ESP32-S3 boots and runs your firmware.
- How UART communication works between the ESP32 and the thermal printer.
- How the ESC/POS protocol encodes text, barcodes, QR codes, and bitmaps.
- How `esp_console` provides an interactive command-line interface.
- How NVS (Non-Volatile Storage) persists WiFi credentials across reboots.
- How the ESP-IDF build system (CMake + Kconfig) compiles and links everything.

---

## 3. Hardware — The AtomS3R Lite Board

### What It Is

The AtomS3R Lite is a member of the M5Stack ATOM family — ultra-compact
(24 mm × 24 mm) development boards. The "Lite" variant removes the onboard
0.85-inch IPS display found on the full AtomS3R, leaving more GPIO pins free
and reducing power consumption. For our use case this is ideal because we do
not need a screen — all interaction happens through the USB serial console.

### Key Specifications

| Parameter | Value |
|-----------|-------|
| SoC | ESP32-S3-PICO-1-N8R8 |
| CPU | Xtensa dual-core LX7, up to 240 MHz |
| Flash | 8 MB (embedded) |
| PSRAM | 8 MB (embedded, octal SPI) |
| Wi-Fi | 802.11 b/g/n (2.4 GHz) |
| Bluetooth | BLE 5.0 |
| USB | USB-C (USB OTG + USB Serial/JTAG) |
| Onboard LED | RGB WS2812 (NeoPixel) on GPIO35 |
| Button | Tactile button on GPIO41 (BOOT button) |
| Bottom GPIO header | G5, G6, G7, G8, G38, G39, 5V, 3.3V, GND |
| Expansion port | HY2.0-4P (G5, G6, 5V, GND) |

### Why PSRAM Matters

PSRAM is external RAM that sits alongside the internal SRAM. The ESP32-S3 has
about 512 KB of internal SRAM shared between your code and the RTOS. When you
want to buffer a full 384-pixel-wide × 1000-line bitmap for the printer, that
is (384/8) × 1000 = 48,000 bytes — nearly 10% of your total internal RAM.

With 8 MB of PSRAM, you can allocate large buffers without worrying about
running out of heap. The ESP-IDF malloc() allocator can be configured to
automatically place large allocations in PSRAM.

```c
// In sdkconfig.defaults:
CONFIG_SPIRAM_USE_CAPS_ALLOC=y   // Use heap_caps_malloc() for PSRAM
CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=16384  // Keep small allocs in internal RAM

// In code:
uint8_t *bitmap = heap_caps_malloc(48000, MALLOC_CAP_SPIRAM);  // Goes to PSRAM
```

### USB Serial/JTAG — The Console Lifeline

The ESP32-S3 has a dedicated hardware peripheral called **USB Serial/JTAG**.
This is different from the older ESP32's USB-UART bridge (an external chip like
CP2102 or CH340). The USB Serial/JTAG is built into the silicon itself:

- It appears as a `/dev/ttyACM0` (Linux) or COM port (Windows) when you plug
  in the USB-C cable.
- It provides both a serial console (for `printf`, `esp_log`, `esp_console`)
  and JTAG debugging through the same cable.
- It uses **no GPIO pins** — it is routed internally through the USB pins.

This is critically important for our project because it means **every GPIO pin
on the AtomS3R Lite is available for the printer UART and future expansions**.
On the older ATOM Lite (ESP32-PICO-D4), the USB-UART bridge consumed GPIO1/GPIO3
for TX/RX, which sometimes conflicted with the printer's UART2 pins.

### GPIO Pin Availability

The bottom header exposes six GPIO pins. Here is what they can do:

| Pin | Capabilities | Notes |
|-----|-------------|-------|
| G5 | GPIO, I2C SDA, SPI CLK | HY2.0-4P port pin 1 |
| G6 | GPIO, I2C SCL, SPI MISO | HY2.0-4P port pin 2 |
| G7 | GPIO, SPI MOSI | Free on header |
| G8 | GPIO, SPI CS | Free on header |
| G38 | GPIO, input-only (no pull-up) | Bottom header only |
| G39 | GPIO, input-only (no pull-up) | Bottom header only |

For the printer UART, we need two pins: one for TX (transmit to printer) and
one for RX (receive from printer). G5 and G6 are good candidates because they
support full GPIO output and are also reachable through the HY2.0-4P connector.

---

## 4. Hardware — The M5Stack Thermal Printer (K118)

### What It Is

The M5Stack ATOM Thermal Printer Kit (SKU K118) pairs a tiny 58 mm thermal
printer mechanism with a carrier board. The carrier board provides the power
circuitry, a connector cable, and a mounting bracket. You pair it with any
compatible controller board — originally the ATOM Lite, but in our case the
AtomS3R Lite.

The printer mechanism itself is a generic 58 mm thermal receipt printer head,
manufactured by one of several Chinese OEMs (commonly marked as "FTP-628" or
similar). It uses a **thermal line-dot method**: a heating element array with
384 tiny resistive dots across the 58 mm paper width. When a dot is activated,
it heats the thermally-sensitive paper and creates a black mark.

### Key Specifications

| Parameter | Value |
|-----------|-------|
| Paper width | 58 mm (2.28 inches) |
| Print width | 48 mm (384 dots) |
| Resolution | 203 dpi (8 dots/mm) |
| Print speed | ~60 mm/s |
| Dots per line | 384 |
| Bytes per line | 48 (384 dots / 8 bits) |
| Communication | UART TTL, 9600 baud, 8N1 |
| Power | DC 12V, 2.5A (external power supply required) |
| Interface voltage | 3.3V TTL logic (compatible with ESP32 GPIO) |
| Paper roll diameter | ~30 mm max |

### How Thermal Printing Works

Thermal printing is fundamentally different from inkjet or laser printing:

1. There is **no ink**. The paper is coated with a chemical that turns black
   when heated above ~70°C.
2. The printer head has a **single row** of 384 heating dots. It prints one
   horizontal line at a time.
3. To print a line, the firmware sends a 48-byte bitmap (384 bits = 384 dots).
   Each bit set to 1 means "heat this dot"; 0 means "skip."
4. The printer's internal controller handles motor advancement, timing, and
   dot heating based on the received data.

This means printing a full-page receipt is really just sending hundreds of
48-byte scan lines, one after another, separated by line-feed commands.

### The Communication Protocol

The printer speaks a dialect of **ESC/POS** — a standard command set developed
by Epson for point-of-sale printers. The protocol is byte-oriented and sent
over a simple UART serial link:

- **Baud rate**: 9600
- **Data bits**: 8
- **Parity**: None
- **Stop bits**: 1
- **Flow control**: None (some models support CTS hardware flow control)

All commands start with the **ESC** byte (`0x1B`) or **GS** byte (`0x1D`),
followed by a command byte and optional parameters. Text data is sent as plain
ASCII bytes.

The protocol is half-duplex in practice: you send commands and the printer
executes them. The printer can send back status bytes, but most simple
implementations (including ours) ignore them and operate in "fire and forget"
mode.

### Power Requirements

**Important:** The printer mechanism requires 12V at up to 2.5A. The USB-C port
on the AtomS3R Lite provides only 5V. You must use the external 12V power supply
that comes with the K118 kit. The carrier board regulates the 12V down to 3.3V
for the logic-level interface.

When the printer is actively printing, it draws significant current. If the
power supply is undersized, you will see:
- Faded or incomplete lines
- Paper jams
- In severe cases, the ESP32 may brown-out and reset

Always use the recommended 12V/2.5A supply.

---

## 5. Wiring Diagram: AtomS3R Lite ↔ Thermal Printer

### Physical Connection

The K118 kit includes a 4-wire cable with a HY2.0-4P connector on each end.
You plug one end into the printer carrier board and the other end into the
AtomS3R Lite's HY2.0-4P port (which maps to GPIO5, GPIO6, 5V, and GND).

### Pin Mapping

```
AtomS3R Lite              K118 Printer Carrier Board
┌───────────┐              ┌──────────────────┐
│           │              │                  │
│  GPIO5 ───┼──────────────┼─── RX  (printer │
│    (TX)   │   UART TX    │    receives)     │
│           │              │                  │
│  GPIO6 ───┼──────────────┼─── TX  (printer │
│    (RX)   │   UART RX    │    sends)        │
│           │              │                  │
│  GND   ───┼──────────────┼─── GND          │
│           │              │                  │
│  5V    ───┼──────────────┼─── VCC (logic)   │
│           │              │                  │
└───────────┘              └──────────────────┘
                           (External 12V power
                            supply connected to
                            carrier board barrel jack)
```

### Important Wiring Notes

- **TX ↔ RX crossover**: The ESP32's TX pin (GPIO5) connects to the printer's
  RX pin. The printer's TX pin connects to the ESP32's RX pin (GPIO6). This is
  standard serial crossover wiring — one side's transmit is the other side's
  receive.

- **Logic level**: Both the ESP32-S3 and the printer carrier board operate at
  3.3V logic. No level shifters are needed.

- **USB is independent**: The USB-C cable connects to your computer for
  flashing and the serial console. It uses the built-in USB Serial/JTAG
  peripheral — completely separate from GPIO5/GPIO6.

- **Ground must be shared**: The GND connection between the AtomS3R Lite and
  the printer carrier board is essential. Without it, the UART signal levels
  will float and you will get garbage data.

### UART Configuration in Code

```c
#define PRINTER_UART_NUM   UART_NUM_1
#define PRINTER_TX_GPIO    5    // AtomS3R Lite bottom header / HY2.0-4P
#define PRINTER_RX_GPIO    6    // AtomS3R Lite bottom header / HY2.0-4P
#define PRINTER_BAUD       9600
```

We use **UART_NUM_1** (not UART_NUM_0, which is often reserved for the
USB-UART bridge on older ESP32 boards). On the ESP32-S3 with USB Serial/JTAG,
UART_NUM_0 is free, but we still use UART_NUM_1 as a convention to keep the
console and printer UARTs clearly separated.

---

## 6. Software Stack Overview

### Layer Diagram

```
┌──────────────────────────────────────────────────────────────┐
│                     User Terminal                            │
│            (minicom / idf.py monitor / screen)               │
│            Connected via USB-C to AtomS3R Lite               │
└──────────────────────┬───────────────────────────────────────┘
                       │ USB Serial/JTAG (virtual COM port)
                       │ /dev/ttyACM0
┌──────────────────────▼───────────────────────────────────────┐
│                    esp_console                                │
│         Line editor + command dispatcher + argtable3          │
│         Built into ESP-IDF, runs on USB Serial/JTAG           │
├──────────┬────────────────────┬───────────────────────────────┤
│ wifi_cmd │ printer_cmd        │ system_cmd                    │
│ module   │ module             │ module (help, reboot, etc.)   │
├──────────┴────────────────────┴───────────────────────────────┤
│                        Application Layer                      │
├──────────┬────────────────────┬───────────────────────────────┤
│ wifi_mgr │ printer_drv        │ nvs_store                     │
│          │                    │                               │
│ Connect  │ Send ESC/POS       │ Read/write WiFi               │
│ /scan    │ bytes to UART1     │ SSID/password to              │
│ /status  │                    │ non-volatile flash            │
├──────────┴────────────────────┴───────────────────────────────┤
│                     ESP-IDF Layer                             │
│  esp_wifi  |  driver/uart  |  nvs_flash  |  esp_event       │
│  lwip      |  esp_netif    |  freertos   |  esp_timer       │
├───────────────────────────────────────────────────────────────┤
│                     FreeRTOS (RTOS)                           │
│           Tasks, queues, semaphores, timers                   │
├───────────────────────────────────────────────────────────────┤
│                     ESP32-S3 Hardware                         │
│     CPU cores  |  Wi-Fi radio  |  UART1  |  USB Serial/JTAG  │
│     NVS flash  |  PSRAM        |  GPIO   |  RGB LED          │
└───────────────────────────────────────────────────────────────┘
```

### What Each Layer Does

**User Terminal** — The program on your computer that you type into. The
firmware does not know or care what terminal program you use; it just reads
lines from the USB serial port and writes responses back.

**esp_console** — A library built into ESP-IDF that provides:
- A **line editor** with cursor movement, backspace, and history (up/down arrows).
- A **command registration system** where each module registers its commands.
- **Argument parsing** via the argtable3 library (POSIX-style `--flag value`).
- A built-in `help` command that lists all registered commands.

**Command modules** (wifi_cmd, printer_cmd, system_cmd) — Each module is a C
file that:
- Registers one or more commands with `esp_console_cmd_register()`.
- Provides a handler function that runs when the user types the command.
- Uses argtable3 to parse arguments.

**Application modules** (wifi_mgr, printer_drv, nvs_store) — The "business logic"
that the command handlers call:
- `wifi_mgr` wraps the ESP-IDF Wi-Fi APIs into simple connect/scan/disconnect
  functions.
- `printer_drv` wraps the UART driver and builds ESC/POS command packets.
- `nvs_store` wraps the NVS (Non-Volatile Storage) API for saving WiFi
  credentials.

**ESP-IDF Layer** — The SDK provided by Espressif. It contains drivers for all
the hardware peripherals, the Wi-Fi stack, the TCP/IP stack (lwIP), and the
event loop.

**FreeRTOS** — The real-time operating system kernel. ESP-IDF runs on top of
FreeRTOS. Your `app_main()` function runs in a FreeRTOS task. The Wi-Fi stack
and other drivers run in their own tasks internally.

---

## 7. ESP-IDF Build System Primer

### What Is ESP-IDF?

ESP-IDF (Espressif IoT Development Framework) is the official SDK for ESP32
chips. It provides:

- **Header files and libraries** for all hardware peripherals (UART, SPI, I2C,
  Wi-Fi, Bluetooth, etc.)
- **FreeRTOS** as the underlying RTOS
- **Build tools** based on CMake and Ninja
- **Configuration system** (Kconfig / menuconfig) for enabling/disabling features
- **Flashing and monitoring tools** (`idf.py flash monitor`)

### Project Structure

Every ESP-IDF project follows this layout:

```
stoms3r/
├── CMakeLists.txt           # Top-level: declares project name and targets
├── sdkconfig.defaults        # Default Kconfig values (committed to git)
├── sdkconfig                 # Generated full config (do NOT commit)
├── partitions.csv            # Flash partition layout
├── main/                     # Your application code
│   ├── CMakeLists.txt        # Tells the build system to compile these files
│   ├── app_main.c            # Entry point (the "main()" of ESP-IDF)
│   ├── wifi_mgr.c / .h       # Wi-Fi management module
│   ├── wifi_cmd.c / .h       # Wi-Fi console commands
│   ├── printer_drv.c / .h    # Thermal printer UART driver
│   ├── printer_cmd.c / .h    # Printer console commands
│   └── nvs_store.c / .h      # NVS helper functions
├── components/               # Optional: reusable libraries (empty for now)
└── build/                    # Generated build artifacts (gitignored)
```

### CMakeLists.txt — The Build Recipe

The top-level `CMakeLists.txt` is minimal:

```cmake
# CMakeLists.txt (project root)
cmake_minimum_required(VERSION 3.16)

# This must match the directory name or be explicitly set:
set(PROJECT_NAME "stoms3r")

# Pull in ESP-IDF's build system:
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(${PROJECT_NAME})
```

The `main/CMakeLists.txt` tells the build system which source files to compile:

```cmake
# main/CMakeLists.txt
idf_component_register(
    SRCS
        "app_main.c"
        "wifi_mgr.c"
        "wifi_cmd.c"
        "printer_drv.c"
        "printer_cmd.c"
        "nvs_store.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_netif nvs_flash driver esp_console esp_https_ota
)
```

The `REQUIRES` line tells the build system which ESP-IDF components our code
needs. The linker will only pull in the libraries we actually depend on, keeping
the firmware binary small.

### sdkconfig.defaults — Your Configuration Baseline

ESP-IDF has hundreds of configuration options (similar to Linux's `make
menuconfig`). Instead of running `idf.py menuconfig` every time, we commit a
`sdkconfig.defaults` file with our preferred settings:

```ini
# sdkconfig.defaults for SToMS3R (AtomS3R Lite, ESP32-S3)

# Target chip
CONFIG_IDF_TARGET="esp32s3"

# Flash size: AtomS3R Lite has 8 MB
CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y
CONFIG_ESPTOOLPY_FLASHSIZE="8MB"

# Use USB Serial/JTAG for the interactive console
# This frees all UART pins for the printer
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y

# Enable PSRAM (8 MB on AtomS3R Lite)
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_USE_CAPS_ALLOC=y

# Wi-Fi
CONFIG_ESP_WIFI_ENABLED=y

# Console history and line editing
CONFIG_ESP_CONSOLE_HISTORY_LEN=50
```

### The Build Commands

```bash
# One-time: set up ESP-IDF environment variables
source $HOME/esp/v5.4.x/esp-idf/export.sh

# Configure the target chip (run once after cloning)
idf.py set-target esp32s3

# Build the firmware
idf.py build

# Flash to the board and open the serial monitor
idf.py -p /dev/ttyACM0 flash monitor

# Exit monitor: Ctrl+]
```

---

## 8. Console Subsystem (esp_console)

### What Is esp_console?

`esp_console` is an ESP-IDF component that gives you an interactive command-line
interface (a REPL — Read-Eval-Print Loop) over a serial connection. Think of it
as a tiny embedded shell, similar to what you get when you open a terminal on
Linux, but running inside the ESP32.

It provides these features out of the box:

- **Line editing**: Backspace, cursor movement (left/right arrows), Home/End.
- **Command history**: Up/Down arrows cycle through previously typed commands.
- **Tab completion**: Press Tab to auto-complete command names.
- **Argument parsing**: The `argtable3` library parses `--flag value` style args.
- **Built-in `help` command**: Lists all registered commands with their help text.

### How Commands Are Registered

Each command is described by an `esp_console_cmd_t` struct:

```c
// From esp_console.h
typedef struct {
    const char *command;       // Command name, e.g. "wifi_scan"
    const char *help;          // One-line help text shown by `help`
    const char *hint;          // Hint string for argument completion
    esp_console_cmd_func_t func; // The function that runs when command is typed
    argtable3 struct pointers;   // Parsed via the .argtable field (see below)
} esp_console_cmd_t;
```

Here is a minimal example of registering a command:

```c
#include "esp_console.h"
#include "argtable3/argtable3.h"

// Step 1: Define argument structure using argtable3
static struct {
    arg_lit_t *verbose;   // --verbose flag (no value)
    arg_end_t *end;       // Marks end of arg table (required)
} wifi_scan_args;

// Step 2: The command handler function
static int do_wifi_scan(int argc, char **argv)
{
    // Parse the arguments
    int nerrors = arg_parse(argc, argv, (void **) &wifi_scan_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, wifi_scan_args.end, argv[0]);
        return 1;
    }

    if (wifi_scan_args.verbose->count > 0) {
        printf("Verbose scan enabled\n");
    }

    // Call the Wi-Fi manager to scan
    wifi_mgr_scan();
    return 0;
}

// Step 3: Register the command
void wifi_cmd_register(void)
{
    // Initialize argtable entries
    wifi_scan_args.verbose = arg_lit0("v", "verbose", "show detailed info");
    wifi_scan_args.end = arg_end(1);

    // Fill in the command struct
    const esp_console_cmd_t cmd = {
        .command = "wifi_scan",
        .help = "Scan for nearby Wi-Fi access points",
        .hint = NULL,
        .func = &do_wifi_scan,
        .argtable = &wifi_scan_args,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
```

### How the Console Loop Works

The console loop is started by calling `esp_console_start_repl()`:

```c
// In app_main.c
void app_main(void)
{
    // Initialize NVS
    nvs_flash_init();

    // Initialize the console
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.max_cmdline_length = 256;
    repl_config.max_cmdline_args = 8;
    esp_console_dev_usb_serial_jtag_config_t hw_config =
        ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));

    // Register all command modules
    wifi_cmd_register();
    printer_cmd_register();

    // Start the REPL (this call does not return)
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
```

The sequence is:
1. **Initialize the console** with the USB Serial/JTAG transport.
2. **Register commands** from each module.
3. **Start the REPL** — this creates a FreeRTOS task that reads lines from the
   serial port, parses them, and dispatches to the matching handler.

### argtable3 Quick Reference

The `argtable3` library is how we parse command-line arguments. Common types:

| argtable3 type | Meaning | Example |
|---------------|---------|--------|
| `arg_lit0/1` | Boolean flag (--verbose) | `arg_lit0("v", "verbose", "help text")` |
| `arg_str0/1` | String value (--ssid MyWiFi) | `arg_str1(NULL, NULL, "<ssid>", "SSID name")` |
| `arg_int0/1` | Integer value (--count 3) | `arg_int0("c", "count", "<n>", "repeat count")` |
| `arg_dbl0/1` | Double value | `arg_dbl0("t", "timeout", "<s>", "seconds")` |
| `arg_end` | End marker (required) | `arg_end(2)` — 2 = max expected errors |

The `0`/`1` suffix means: minimum required count. `arg_str1` = exactly 1 required.
`arg_str0` = 0 or more (optional). `arg_lit0` = 0 or 1 (flag is optional).

---

## 9. WiFi Console Commands

### Overview

The Wi-Fi subsystem provides these console commands:

| Command | What It Does |
|---------|-------------|
| `wifi_scan` | Scan for nearby access points and print a table |
| `wifi_connect --ssid <name> --pass <password>` | Join a Wi-Fi network |
| `wifi_status` | Show current connection state and IP address |
| `wifi_disconnect` | Disconnect from the current network |
| `wifi_forget` | Erase saved credentials from NVS |

### Command Flow: wifi_scan

When the user types `wifi_scan`, the following happens:

```
User types: wifi_scan
       │
       ▼
esp_console parses line, finds handler for "wifi_scan"
       │
       ▼
do_wifi_scan() called
       │
       ▼
wifi_mgr_scan() called
       │
       ▼
esp_wifi_scan_start() — asks ESP32 Wi-Fi radio to scan
       │
       ▼
esp_wifi_scan_get_ap_num() — count found APs
       │
       ▼
esp_wifi_scan_get_ap_records() — get AP records
       │
       ▼
Print formatted table to console:
  #  SSID              RSSI  Channel  Auth
  1  MyHomeWiFi        -42   6        WPA2
  2  Neighbor_5G       -71   11       WPA2
  3  CoffeeShop        -85   1        OPEN
```

### Command Flow: wifi_connect

```
User types: wifi_connect --ssid MyHomeWiFi --pass s3cret123
       │
       ▼
argtable3 parses --ssid and --pass
       │
       ▼
wifi_mgr_connect(ssid, password) called
       │
       ├── esp_wifi_set_mode(WIFI_MODE_STA)
       ├── esp_wifi_set_config(WIFI_IF_STA, &cfg)
       ├── esp_wifi_start()
       ├── esp_wifi_connect()
       │
       ▼
Event handler receives IP_EVENT_STA_GOT_IP
       │
       ▼
nvs_store_save_wifi(ssid, password)  ← persist for next boot
       │
       ▼
printf("Connected! IP: 192.168.1.42\n")
```

### Auto-Connect on Boot

On startup, the firmware checks NVS for saved credentials:

```c
// Pseudocode for app_main.c
void app_main(void) {
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // Try to load saved credentials
    char ssid[64] = {0};
    char password[64] = {0};
    if (nvs_store_load_wifi(ssid, sizeof(ssid), password, sizeof(password)) == ESP_OK) {
        printf("Found saved WiFi: %s, connecting...\n", ssid);
        wifi_mgr_connect(ssid, password);  // non-blocking
    }

    // Start console (always, regardless of WiFi state)
    start_console();
}
```

### wifi_mgr Module Pseudocode

```c
// wifi_mgr.c — simplified pseudocode

static bool s_connected = false;
static esp_netif_ip_info_t s_ip_info;

// Event handler — called by the ESP event loop
static void wifi_event_handler(void *arg, esp_event_base_t base,
                                int32_t id, void *event_data)
{
    if (base == WIFI_EVENT) {
        switch (id) {
            case WIFI_EVENT_STA_CONNECTED:
                printf("WiFi: associated with AP\n");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                s_connected = false;
                printf("WiFi: disconnected\n");
                // Optional: auto-reconnect
                esp_wifi_connect();
                break;
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        s_ip_info = event->ip_info;
        s_connected = true;
        printf("WiFi: got IP " IPSTR "\n", IP2STR(&s_ip_info.ip));
    }
}

void wifi_mgr_init(void) {
    // Register event handlers
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                         &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                         &wifi_event_handler, NULL, NULL);
}

void wifi_mgr_scan(void) {
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    wifi_scan_config_t scan_config = { .ssid = NULL, .bssid = NULL,
                                        .channel = 0, .show_hidden = false };
    esp_wifi_scan_start(&scan_config, true);  // blocking

    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);

    wifi_ap_record_t *aps = malloc(ap_count * sizeof(wifi_ap_record_t));
    esp_wifi_scan_get_ap_records(&ap_count, aps);

    // Print table header
    printf("%4s %-32s %5s %7s %s\n", "#", "SSID", "RSSI", "Ch", "Auth");
    for (int i = 0; i < ap_count; i++) {
        printf("%2d  %-32s %4d  %3d     %s\n",
               i + 1,
               aps[i].ssid,
               aps[i].rssi,
               aps[i].primary,
               auth_mode_str(aps[i].authmode));
    }
    free(aps);
    esp_wifi_stop();
}
```

---

## 10. Thermal Printer Console Commands

### Overview

The printer subsystem provides these console commands:

| Command | What It Does |
|---------|-------------|
| `printer_init` | Reset the printer to a known state |
| `printer_text <string>` | Print a line of text |
| `printer_feed [lines]` | Feed paper (default 3 lines) |
| `printer_bold <on|off>` | Enable/disable bold text |
| `printer_size <n>` | Set font size (0–7, where 0 is smallest) |
| `printer_barcode <type> <data>` | Print a barcode (CODE128, EAN13, etc.) |
| `printer_qr <text>` | Print a QR code |
| `printer_bitmap_test` | Print a test bitmap pattern |
| `printer_status` | Query printer status byte |

### Command Flow: printer_text

```
User types: printer_text "Hello World"
       │
       ▼
do_printer_text() called
       │
       ▼
printer_drv_print_text("Hello World")
       │
       ▼
Send ESC/POS bytes over UART1 to the printer:
   0x48 0x65 0x6C 0x6C 0x6F 0x20 0x57 0x6F 0x72 0x6C 0x64  ("Hello World")
   0x0A  (line feed / newline)
       │
       ▼
Printer mechanism heats dots and advances paper
```

### Command Flow: printer_qr

```
User types: printer_qr "https://m5stack.com"
       │
       ▼
do_printer_qr() called
       │
       ▼
printer_drv_print_qr("https://m5stack.com")
       │
       ▼
Send ESC/POS QR commands over UART1:
   1. Set QR error correction level: GS ( k pL pH cn=49 fn=49 [n=48..51]
      0x1D 0x28 0x6B 0x03 0x00 0x31 0x45 0x31  (Level M)
   2. Store QR data: GS ( k pL pH cn=49 fn=80 m=48 d1..dk
      0x1D 0x28 0x6B <len_lo> <len_hi> 0x31 0x50 0x30 <data_bytes>
   3. Print QR: GS ( k pL pH cn=49 fn=81 m=48
      0x1D 0x28 0x6B 0x03 0x00 0x31 0x51 0x30
       │
       ▼
Printer renders and prints the QR code
```

### printer_drv Module Pseudocode

```c
// printer_drv.c — simplified pseudocode

#include "driver/uart.h"

#define PRINTER_UART    UART_NUM_1
#define PRINTER_TX_GPIO 5
#define PRINTER_RX_GPIO 6
#define PRINTER_BAUD    9600
#define BUF_SIZE        1024

static QueueHandle_t uart_queue;

esp_err_t printer_drv_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = PRINTER_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(PRINTER_UART, BUF_SIZE, BUF_SIZE,
                                         10, &uart_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(PRINTER_UART, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(PRINTER_UART,
                                  PRINTER_TX_GPIO, PRINTER_RX_GPIO,
                                  UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    return ESP_OK;
}

// Low-level: send raw bytes
static esp_err_t send_bytes(const uint8_t *data, size_t len)
{
    int written = uart_write_bytes(PRINTER_UART, data, len);
    if (written < 0 || (size_t)written != len) {
        return ESP_FAIL;
    }
    // Wait for UART TX FIFO to drain
    uart_wait_tx_done(PRINTER_UART, pdMS_TO_TICKS(100));
    return ESP_OK;
}

// Reset printer: ESC @
esp_err_t printer_drv_reset(void)
{
    uint8_t cmd[] = { 0x1B, 0x40 };
    return send_bytes(cmd, sizeof(cmd));
}

// Print plain text
esp_err_t printer_drv_print_text(const char *text)
{
    send_bytes((const uint8_t *)text, strlen(text));
    uint8_t lf = 0x0A;  // line feed
    return send_bytes(&lf, 1);
}

// Set font size (0–7)
esp_err_t printer_drv_set_font_size(uint8_t size)
{
    if (size > 7) size = 7;
    uint8_t cmd[] = { 0x1D, 0x21, (uint8_t)((size << 4) | size) };
    return send_bytes(cmd, sizeof(cmd));
}

// Enable/disable bold
esp_err_t printer_drv_set_bold(bool on)
{
    uint8_t cmd[] = { 0x1B, 0x45, on ? 1 : 0 };
    return send_bytes(cmd, sizeof(cmd));
}

// Feed N lines
esp_err_t printer_drv_feed(uint8_t lines)
{
    uint8_t cmd[] = { 0x1B, 0x64, lines };
    return send_bytes(cmd, sizeof(cmd));
}

// Print QR code
esp_err_t printer_drv_print_qr(const char *data)
{
    size_t len = strlen(data);

    // Step 1: Set error correction level to M
    uint8_t ecl_cmd[] = { 0x1D, 0x28, 0x6B, 0x03, 0x00, 0x31, 0x45, 0x31 };
    send_bytes(ecl_cmd, sizeof(ecl_cmd));

    // Step 2: Store QR data
    // Header: GS ( k pL pH cn=49 fn=80 m=48
    // pL = len + 3, pH = 0
    uint8_t store_header[] = { 0x1D, 0x28, 0x6B,
                                (uint8_t)(len + 3), 0x00,
                                0x31, 0x50, 0x30 };
    send_bytes(store_header, sizeof(store_header));
    send_bytes((const uint8_t *)data, len);

    // Step 3: Print the QR code
    uint8_t print_cmd[] = { 0x1D, 0x28, 0x6B, 0x03, 0x00, 0x31, 0x51, 0x30 };
    return send_bytes(print_cmd, sizeof(print_cmd));
}

// Print barcode
esp_err_t printer_drv_print_barcode(uint8_t type, const char *data)
{
    size_t len = strlen(data);
    // GS k m n d1..dn
    uint8_t header[] = { 0x1D, 0x6B, type, (uint8_t)len };
    send_bytes(header, sizeof(header));
    return send_bytes((const uint8_t *)data, len);
}

// Print raster bitmap (monochrome, MSB-first)
esp_err_t printer_drv_print_bitmap(uint16_t width, uint16_t height,
                                     const uint8_t *pixels)
{
    uint16_t bytes_per_row = width / 8;
    // GS v 0 m xL xH yL yH d1..dk
    uint8_t header[] = {
        0x1D, 0x76, 0x30, 0x00,
        (uint8_t)(bytes_per_row & 0xFF), (uint8_t)(bytes_per_row >> 8),
        (uint8_t)(height & 0xFF), (uint8_t)(height >> 8)
    };
    send_bytes(header, sizeof(header));
    return send_bytes(pixels, bytes_per_row * height);
}
```

---

## 11. ESC/POS Protocol Deep Dive

### What Is ESC/POS?

ESC/POS is a command protocol created by Epson for controlling receipt printers.
"ESC" stands for the Escape character (byte `0x1B`), which prefixes many commands.
"POS" stands for Point of Sale. Despite the name, the protocol is used by almost
all thermal receipt printers worldwide — not just Epson brands.

The M5Stack K118 printer implements a subset of ESC/POS. It does not support
the full specification (e.g., no color printing, no cash drawer control), but
it covers everything we need: text formatting, barcodes, QR codes, and bitmaps.

### How Commands Are Structured

Every ESC/POS command follows this pattern:

```
[Prefix byte(s)] [Command byte] [Parameters...]
```

The prefix is one of:
- **ESC** (`0x1B`) — the most common prefix
- **GS** (`0x1D`) — "Group Separator", used for advanced commands
- **FS** (`0x1C`) — used for some extended commands

Here is a decoding table of the commands we use:

### Text Commands

| Command | Bytes | Description |
|---------|-------|-------------|
| Initialize printer | `1B 40` | Reset to power-on defaults |
| Print + line feed | `0A` | Move paper up one line |
| Feed N lines | `1B 64 n` | Advance paper by n lines |
| Bold on | `1B 45 01` | Enable emphasized printing |
| Bold off | `1B 45 00` | Disable emphasized printing |
| Underline on | `1B 2D 01` | Single-line underline |
| Underline off | `1B 2D 00` | No underline |
| Font size | `1D 21 n` | Set size: n = (height << 4) \| width, 0–7 each nibble |
| Justify left | `1B 61 00` | Left alignment |
| Justify center | `1B 61 01` | Center alignment |
| Justify right | `1B 61 02` | Right alignment |
| Set print position | `1B 24 nL nH` | Absolute horizontal position in dots |

### Barcode Commands

| Command | Bytes | Description |
|---------|-------|-------------|
| Print barcode | `1D 6B m n d1..dn` | m=type, n=data length |
| Set HRI position | `1D 48 n` | 0=not printed, 1=above, 2=below, 3=both |
| Set barcode width | `1D 77 n` | Module width in dots (2–6) |
| Set barcode height | `1D 68 n` | Height in dots (1–255) |

Barcode types (the `m` byte):

| Value | Type | Data Length |
|-------|------|------------|
| 0x41 | UPC-A | 11–12 digits |
| 0x42 | UPC-E | 11–12 digits |
| 0x43 | JAN13/EAN13 | 12–13 digits |
| 0x44 | JAN8/EAN8 | 7–8 digits |
| 0x45 | CODE39 | Variable |
| 0x46 | ITF | Variable (even count) |
| 0x47 | CODABAR | Variable |
| 0x48 | CODE93 | Variable |
| 0x49 | CODE128 | Variable |

### QR Code Commands

QR codes use a multi-step command sequence under the GS ( k group:

```
Step 1: Set error correction level
  1D 28 6B 03 00 31 45 n
  n = 48 (L), 49 (M), 50 (Q), 51 (H)

Step 2: Store QR data
  1D 28 6B pL pH 31 50 30 d1..dk
  pL = k+3 (low byte), pH = 0 (high byte)
  d1..dk = QR data bytes

Step 3: Print QR code
  1D 28 6B 03 00 31 51 30
```

Error correction levels trade data capacity for resilience:

| Level | Recovery % | Use Case |
|-------|-----------|----------|
| L (48) | ~7% | High-capacity, clean environment |
| M (49) | ~15% | Good default balance |
| Q (50) | ~25% | Moderate damage expected |
| H (51) | ~30% | Harsh environment, maximum resilience |

### Bitmap (Raster Image) Command

```
1D 76 30 m xL xH yL yH d1..dk

m:  mode byte (0=normal, 1=double-width, 2=double-height, 3=both)
xL xH: bytes per line = width_pixels / 8 (little-endian 16-bit)
yL yH: number of lines = height in pixels (little-endian 16-bit)
d1..dk: bitmap data, k = bytes_per_line * height
```

The bitmap data is **1-bit monochrome, MSB first**: the most significant bit of
each byte represents the leftmost pixel in that group of 8. A 1 bit means "print
a dot" (black); a 0 bit means "leave blank."

For the K118 printer (384 dots wide):
- Bytes per line = 384 / 8 = **48 bytes**
- A full-page image of 800 lines = 48 × 800 = **38,400 bytes** (~37.5 KB)

### Sending a Bitmap — Step by Step

```c
// Pseudocode: print a simple test pattern (alternating lines)
void print_test_pattern(void)
{
    const int width_dots = 384;    // Full printer width
    const int bytes_per_row = 48;  // 384 / 8
    const int height = 100;        // 100 scan lines

    // 1. Send bitmap header
    uint8_t header[] = {
        0x1D, 0x76, 0x30, 0x00,            // GS v 0 mode=normal
        (uint8_t)(bytes_per_row & 0xFF),    // xL = 48
        (uint8_t)(bytes_per_row >> 8),      // xH = 0
        (uint8_t)(height & 0xFF),           // yL = 100
        (uint8_t)(height >> 8)              // yH = 0
    };
    uart_write_bytes(PRINTER_UART, header, sizeof(header));

    // 2. Send bitmap data line by line
    for (int row = 0; row < height; row++) {
        uint8_t line[bytes_per_row];

        if (row % 2 == 0) {
            // Even lines: all black (0xFF)
            memset(line, 0xFF, bytes_per_row);
        } else {
            // Odd lines: all white (0x00)
            memset(line, 0x00, bytes_per_row);
        }

        uart_write_bytes(PRINTER_UART, line, bytes_per_row);
    }
}
```

---

## 12. NVS Key-Value Storage

### What Is NVS?

NVS (Non-Volatile Storage) is an ESP-IDF component that stores small key-value
pairs in a dedicated region of the flash memory. It behaves like a persistent
dictionary: you store a string, integer, or blob under a key name, and it
survives power cycles and firmware reboots.

NVS is perfect for storing:
- WiFi SSID and password
- User preferences (default font size, printer settings)
- Device configuration flags

### How NVS Is Organized

NVS uses a **namespace** concept to avoid key collisions between different
modules:

```
NVS Flash Partition
├── Namespace: "wifi"
│   ├── Key: "ssid"     → "MyHomeWiFi"
│   └── Key: "password" → "s3cret123"
├── Namespace: "printer"
│   ├── Key: "font_size" → 2
│   └── Key: "bold"      → 0
└── Namespace: "system"
    └── Key: "boot_count" → 42
```

### nvs_store Module API

```c
// nvs_store.h

// WiFi credentials
esp_err_t nvs_store_save_wifi(const char *ssid, const char *password);
esp_err_t nvs_store_load_wifi(char *ssid, size_t ssid_len,
                                char *password, size_t password_len);
esp_err_t nvs_store_erase_wifi(void);

// Generic helpers
esp_err_t nvs_store_save_str(const char *ns, const char *key, const char *value);
esp_err_t nvs_store_load_str(const char *ns, const char *key,
                               char *buf, size_t buf_len);
```

### nvs_store Implementation Pseudocode

```c
// nvs_store.c
#include "nvs_flash.h"
#include "nvs.h"

esp_err_t nvs_store_save_wifi(const char *ssid, const char *password)
{
    nvs_handle_t handle;
    ESP_ERROR_CHECK(nvs_open("wifi", NVS_READWRITE, &handle));

    ESP_ERROR_CHECK(nvs_set_str(handle, "ssid", ssid));
    ESP_ERROR_CHECK(nvs_set_str(handle, "password", password));

    // IMPORTANT: changes are not committed until you call nvs_commit()
    ESP_ERROR_CHECK(nvs_commit(handle));
    nvs_close(handle);

    return ESP_OK;
}

esp_err_t nvs_store_load_wifi(char *ssid, size_t ssid_len,
                                char *password, size_t password_len)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("wifi", NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return err;  // Namespace doesn't exist yet — no saved WiFi
    }

    err = nvs_get_str(handle, "ssid", ssid, &ssid_len);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }

    err = nvs_get_str(handle, "password", password, &password_len);
    nvs_close(handle);
    return err;
}

esp_err_t nvs_store_erase_wifi(void)
{
    nvs_handle_t handle;
    ESP_ERROR_CHECK(nvs_open("wifi", NVS_READWRITE, &handle));
    nvs_erase_key(handle, "ssid");
    nvs_erase_key(handle, "password");
    nvs_commit(handle);
    nvs_close(handle);
    return ESP_OK;
}
```

### Important NVS Gotchas

- **Always call `nvs_commit()`** after writing. Without it, your changes may
  be lost if the device loses power before the next automatic commit.
- **NVS keys must be ≤ 15 characters**. Use short, descriptive names.
- **String values have a maximum length** of 4000 bytes, but in practice we
  keep them short (WiFi SSIDs are ≤ 32 bytes).
- **Initialize NVS early** in `app_main()` with `nvs_flash_init()`. If it
  returns `ESP_ERR_NVS_NO_FREE_PAGES`, call `nvs_flash_erase()` and then
  `nvs_flash_init()` again (this means the partition was corrupt or new).

---

## 13. Project Directory Structure

### Complete File Tree

```
stoms3r/
├── CMakeLists.txt                # Top-level CMake (project name)
├── sdkconfig.defaults            # Default ESP-IDF configuration
├── partitions.csv                # Flash partition layout
├── build.sh                      # Build helper script
│
└── main/
    ├── CMakeLists.txt            # Source file registration
    │
    ├── app_main.c               # Entry point: init NVS, WiFi, printer, start console
    │
    ├── wifi_mgr.h               # WiFi management API
    ├── wifi_mgr.c               # WiFi scan/connect/disconnect/event handling
    ├── wifi_cmd.h               # WiFi console commands registration
    ├── wifi_cmd.c               # wifi_scan, wifi_connect, wifi_status, wifi_disconnect
    │
    ├── printer_drv.h           # Thermal printer driver API
    ├── printer_drv.c           # UART init, ESC/POS command builders
    ├── printer_cmd.h           # Printer console commands registration
    ├── printer_cmd.c           # printer_text, printer_feed, printer_qr, etc.
    │
    ├── nvs_store.h             # NVS helper API
    └── nvs_store.c             # Save/load WiFi credentials, settings
```

### Dependency Graph

```
app_main.c
├── wifi_mgr.h     → wifi_mgr.c      → esp_wifi, esp_event, esp_netif
├── printer_drv.h  → printer_drv.c   → driver/uart
├── nvs_store.h    → nvs_store.c     → nvs_flash
├── wifi_cmd.h     → wifi_cmd.c      → wifi_mgr.h, argtable3, esp_console
└── printer_cmd.h  → printer_cmd.c   → printer_drv.h, argtable3, esp_console
```

Each `.c` file includes its own `.h` header and the headers of the modules it
depends on. The `app_main.c` file is the orchestrator: it initializes all
subsystems and starts the console.

### partitions.csv

```csv
# Name,    Type, SubType, Offset,  Size
nvs,       data, nvs,     ,        0x6000
phy_init,  data, phy,     ,        0x1000
factory,   app,  factory, ,        4M
storage,   data, fat,     ,        1M
```

- **nvs** (24 KB): Stores WiFi credentials and settings.
- **phy_init** (4 KB): RF calibration data (managed by ESP-IDF).
- **factory** (4 MB): The firmware binary (more than enough room).
- **storage** (1 MB): FAT filesystem partition for future use (storing
  bitmaps on flash, etc.).

---

## 14. File-by-File Implementation Guide

This section walks through each source file in the order you should implement
them. Start with the lowest-level modules and build up.

### Implementation Order

```
1. nvs_store.c / .h          ← no dependencies, pure NVS wrappers
2. printer_drv.c / .h        ← depends only on UART driver
3. wifi_mgr.c / .h           ← depends on esp_wifi, nvs_store
4. printer_cmd.c / .h        ← depends on printer_drv, esp_console
5. wifi_cmd.c / .h           ← depends on wifi_mgr, nvs_store, esp_console
6. app_main.c                ← wires everything together
```

### File 1: nvs_store.c / nvs_store.h

**Purpose:** Thin wrapper around the NVS API for storing and retrieving
WiFi credentials.

**Key Functions:**
- `nvs_store_init()` — Call `nvs_flash_init()`, handle the "no free pages" case.
- `nvs_store_save_wifi(ssid, password)` — Write to namespace `"wifi"`.
- `nvs_store_load_wifi(ssid_buf, ssid_len, pass_buf, pass_len)` — Read from namespace `"wifi"`. Returns `ESP_ERR_NVS_NOT_FOUND` if no credentials saved.
- `nvs_store_erase_wifi()` — Delete the keys.

**Error handling pattern:**
```c
esp_err_t nvs_store_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    return ret;
}
```

### File 2: printer_drv.c / printer_drv.h

**Purpose:** Initialize the UART that talks to the thermal printer and provide
functions that construct and send ESC/POS command packets.

**Key Functions:**
- `printer_drv_init()` — Install UART driver, configure 9600/8N1, set TX/RX pins.
- `printer_drv_reset()` — Send `ESC @` (0x1B 0x40).
- `printer_drv_print_text(text)` — Send ASCII text + line feed.
- `printer_drv_feed(lines)` — Send `ESC d n`.
- `printer_drv_set_font_size(size)` — Send `GS ! n`.
- `printer_drv_set_bold(on)` — Send `ESC E n`.
- `printer_drv_set_align(align)` — Send `ESC a n` (0=left, 1=center, 2=right).
- `printer_drv_print_barcode(type, data)` — Send `GS k m n d1..dn`.
- `printer_drv_print_qr(data)` — Send the 3-step QR sequence.
- `printer_drv_print_bitmap(w, h, pixels)` — Send `GS v 0` raster bitmap.

**Internal helper:**
```c
static esp_err_t send_bytes(const uint8_t *data, size_t len) {
    int written = uart_write_bytes(PRINTER_UART, data, len);
    if (written < 0 || (size_t)written != len) return ESP_FAIL;
    uart_wait_tx_done(PRINTER_UART, pdMS_TO_TICKS(200));
    return ESP_OK;
}
```

**UART initialization detail:**
```c
esp_err_t printer_drv_init(void) {
    // 1. Install driver with RX buffer + event queue
    uart_driver_install(PRINTER_UART, 1024, 1024, 10, &uart_queue, 0);

    // 2. Configure parameters
    uart_config_t cfg = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(PRINTER_UART, &cfg);

    // 3. Assign pins
    uart_set_pin(PRINTER_UART, 5 /*TX*/, 6 /*RX*/,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    return ESP_OK;
}
```

### File 3: wifi_mgr.c / wifi_mgr.h

**Purpose:** Manage the Wi-Fi lifecycle — initialization, scanning, connection,
and event handling.

**Key Functions:**
- `wifi_mgr_init()` — Init netif, create default STA, register event handlers.
- `wifi_mgr_scan()` — Perform a blocking scan and return results.
- `wifi_mgr_connect(ssid, password)` — Start STA mode and connect.
- `wifi_mgr_disconnect()` — Disconnect and stop.
- `wifi_mgr_is_connected()` — Check current state.
- `wifi_mgr_get_ip(buf, len)` — Copy current IP to string buffer.

**Event handler pattern:**
```c
static void event_handler(void *arg, esp_event_base_t base,
                           int32_t id, void *data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        s_connected = false;
        ESP_LOGI(TAG, "WiFi disconnected, reconnecting...");
        esp_wifi_connect();  // auto-reconnect
    }
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *evt = data;
        s_connected = true;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&evt->ip_info.ip));
    }
}
```

### File 4: printer_cmd.c / printer_cmd.h

**Purpose:** Register printer-related console commands that the user can type.

**Key Functions:**
- `printer_cmd_register()` — Register all printer commands.

**Registered commands:**

| Command Name | Handler | argtable Args |
|-------------|---------|---------------|
| `printer_init` | `do_printer_init` | none |
| `printer_text` | `do_printer_text` | `<string>` (required) |
| `printer_feed` | `do_printer_feed` | `<lines>` (optional, default 3) |
| `printer_bold` | `do_printer_bold` | `<on\|off>` (required) |
| `printer_size` | `do_printer_size` | `<n>` (required, 0–7) |
| `printer_barcode` | `do_printer_barcode` | `<type>` `<data>` |
| `printer_qr` | `do_printer_qr` | `<text>` (required) |
| `printer_bitmap_test` | `do_printer_bitmap_test` | none |

**Example command registration:**
```c
static int do_printer_text(int argc, char **argv) {
    struct {
        arg_str1_t *text;
        arg_end_t *end;
    } args;

    args.text = arg_str1(NULL, NULL, "<text>", "Text to print");
    args.end = arg_end(1);

    int nerrors = arg_parse(argc, argv, (void **)&args);
    if (nerrors != 0) {
        arg_print_errors(stderr, args.end, argv[0]);
        return 1;
    }

    printer_drv_print_text(args.text->sval[0]);
    return 0;
}
```

### File 5: wifi_cmd.c / wifi_cmd.h

**Purpose:** Register WiFi-related console commands.

**Registered commands:**

| Command Name | Handler | argtable Args |
|-------------|---------|---------------|
| `wifi_scan` | `do_wifi_scan` | none |
| `wifi_connect` | `do_wifi_connect` | `--ssid <s> --pass <p>` |
| `wifi_status` | `do_wifi_status` | none |
| `wifi_disconnect` | `do_wifi_disconnect` | none |
| `wifi_forget` | `do_wifi_forget` | none |

### File 6: app_main.c

**Purpose:** The entry point. Initialize all subsystems and start the console.

**Pseudocode:**
```c
void app_main(void) {
    ESP_LOGI(TAG, "SToMS3R starting...");

    // 1. Initialize NVS
    nvs_store_init();

    // 2. Initialize network stack
    esp_netif_init();
    esp_event_loop_create_default();

    // 3. Initialize Wi-Fi manager
    wifi_mgr_init();

    // 4. Initialize printer UART
    printer_drv_init();
    printer_drv_reset();  // Send ESC @ to reset printer

    // 5. Try auto-connect from saved credentials
    char ssid[64], password[64];
    if (nvs_store_load_wifi(ssid, sizeof(ssid),
                             password, sizeof(password)) == ESP_OK) {
        ESP_LOGI(TAG, "Auto-connecting to %s...", ssid);
        wifi_mgr_connect(ssid, password);
    }

    // 6. Start the interactive console
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.max_cmdline_length = 256;
    esp_console_dev_usb_serial_jtag_config_t hw_cfg =
        ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(
        esp_console_new_repl_usb_serial_jtag(&hw_cfg, &repl_cfg, &repl));

    // 7. Register all command groups
    wifi_cmd_register();
    printer_cmd_register();

    // 8. Start REPL (does not return)
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
```

---

## 15. sdkconfig.defaults Explained

This is the complete `sdkconfig.defaults` for the project, with every line
explained:

```ini
# ==============================================================================
# SToMS3R sdkconfig.defaults
# Target: M5Stack AtomS3R Lite (ESP32-S3-PICO-1-N8R8)
# ==============================================================================

# Chip target — tells the build system to compile for ESP32-S3
CONFIG_IDF_TARGET="esp32s3"

# Flash size: AtomS3R Lite has 8 MB of embedded flash
CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y
CONFIG_ESPTOOLPY_FLASHSIZE="8MB"

# ==============================================================================
# Console: USB Serial/JTAG
# This is the single most important setting for this project.
# It routes esp_console to the built-in USB Serial/JTAG peripheral,
# freeing ALL UART pins for the thermal printer.
# ==============================================================================
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
# Explicitly disable UART console (prevents pin conflicts)
# CONFIG_ESP_CONSOLE_UART_DEFAULT is not set

# ==============================================================================
# PSRAM: 8 MB octal SPI RAM on AtomS3R Lite
# Allows large bitmap buffers without exhausting internal SRAM.
# ==============================================================================
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_USE_CAPS_ALLOC=y
# Keep allocations < 16KB in internal RAM for speed
CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=16384

# ==============================================================================
# Wi-Fi
# ==============================================================================
CONFIG_ESP_WIFI_ENABLED=y
CONFIG_ESP_WIFI_STA_CONNECTED_SCAN=y

# ==============================================================================
# Console enhancements
# ==============================================================================
CONFIG_ESP_CONSOLE_HISTORY_LEN=50
CONFIG_ESP_SYSTEM_EVENT_TASK_STACK_SIZE=4096

# ==============================================================================
# Partition table: use our custom partitions.csv
# ==============================================================================
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
CONFIG_PARTITION_TABLE_FILENAME="partitions.csv"

# CPU frequency: 240 MHz (maximum, for snappy console + fast printing)
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ=240
```

---

## 16. Build, Flash, and Monitor Workflow

### One-Time Setup

```bash
# 1. Clone the repo and enter the project directory
cd stoms3r/

# 2. Source ESP-IDF (adjust path to your installation)
source ~/esp/v5.4.x/esp-idf/export.sh

# 3. Set the target chip (only needed once)
idf.py set-target esp32s3

# 4. Build
idf.py build
```

### Daily Workflow

```bash
# Source ESP-IDF (every new terminal)
source ~/esp/v5.4.x/esp-idf/export.sh

# Build + flash + monitor in one command
idf.py -p /dev/ttyACM0 flash monitor

# Exit the monitor: Ctrl+]
```

### Monitoring Tips

- **The monitor shows all `ESP_LOGI` output** plus the console prompt.
- You can type console commands directly into the monitor window.
- If the output looks garbled, check that your terminal is set to **115200 baud**
  (the default for USB Serial/JTAG).

### Common Build Errors

| Error | Cause | Fix |
|-------|-------|-----|
| `Unknown config name: SPIRAM` | Target not set to ESP32-S3 | Run `idf.py set-target esp32s3` |
| `uart_driver_install failed` | UART already installed | Call `uart_driver_delete()` first, or check for double-init |
| `nvs_flash_init failed` | Corrupt NVS partition | Erase flash: `idf.py erase-flash` then reflash |
| Port not found | Wrong device path | Check `ls /dev/ttyACM*` or `ls /dev/ttyUSB*` |

### Permissions

If you get "Permission denied" on `/dev/ttyACM0`:

```bash
sudo usermod -aG dialout $USER
# Log out and back in for group changes to take effect
```

---

## 17. Testing Plan

### Smoke Test Checklist

Flash the firmware and walk through these steps in order:

#### Step 1: Console Starts
- [ ] Plug in USB-C to AtomS3R Lite + 12V to printer carrier board
- [ ] Open serial monitor: `idf.py -p /dev/ttyACM0 monitor`
- [ ] See boot log with `SToMS3R starting...`
- [ ] See console prompt
- [ ] Type `help` — should list all registered commands

#### Step 2: Printer Communication
- [ ] Type `printer_init` — should return silently (no error)
- [ ] Type `printer_text "Hello from SToMS3R"` — paper should advance with text
- [ ] Type `printer_feed 5` — should feed 5 blank lines
- [ ] Type `printer_size 3` then `printer_text "BIG TEXT"` — should print larger text
- [ ] Type `printer_bold on` then `printer_text "Bold!"` then `printer_bold off`

#### Step 3: Barcodes and QR
- [ ] Type `printer_barcode CODE128 "12345678"` — should print a barcode
- [ ] Type `printer_qr "https://m5stack.com"` — should print a scannable QR code

#### Step 4: Bitmap Test
- [ ] Type `printer_bitmap_test` — should print a test pattern (alternating lines)

#### Step 5: WiFi
- [ ] Type `wifi_scan` — should list nearby access points
- [ ] Type `wifi_connect --ssid MyWiFi --pass MyPassword` — should connect
- [ ] Type `wifi_status` — should show IP address
- [ ] Power cycle the board — should auto-connect to saved WiFi
- [ ] Type `wifi_forget` then reboot — should NOT auto-connect

### Automated Testing Ideas (Future)

- **UART loopback test**: Connect TX to RX with a jumper, send known bytes,
  verify they come back. This tests the UART driver without a printer.
- **Console command tests**: Write a Python script that sends commands over
  serial and checks the response text.
- **WiFi mock**: Use `esp_wifi_set_mode(WIFI_MODE_NULL)` to test WiFi manager
  logic without actually scanning.

---

## 18. Troubleshooting Guide

### Printer Does Not Print Anything

| Symptom | Check | Fix |
|---------|-------|-----|
| No sound from printer motor | 12V power supply | Verify 12V supply is connected and powered on |
| Motor runs but no text | UART wiring | Verify TX↔RX crossover; check GND is shared |
| Garbled characters | Baud rate mismatch | Confirm both sides are at 9600 |
| Printer prints but paper is blank | Paper orientation | Thermal paper only prints on one side — flip the roll |

### Console Does Not Respond

| Symptom | Check | Fix |
|---------|-------|-----|
| No output in terminal | USB connection | Try a different USB-C cable; some are charge-only |
| Boot log shows but no prompt | Console config | Verify `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` |
| Garbled output | Baud rate | Set terminal to 115200 baud |
| Type but no echo | Line ending | Set terminal line endings to CR+LF or LF |

### WiFi Will Not Connect

| Symptom | Check | Fix |
|---------|-------|-----|
| Scan returns empty | Antenna | AtomS3R Lite has internal antenna; verify not shielded by metal |
| Connect fails | Password | Double-check password; passwords are case-sensitive |
| Connect fails silently | Auth mode | Some enterprise WPA networks are not supported |
| Connects but no IP | DHCP | Check that your router has DHCP enabled |
| Was working, now stuck | Stale state | `wifi_forget` then re-enter credentials |

### Build Failures

| Symptom | Check | Fix |
|---------|-------|-----|
| `undefined reference to esp_console_*` | REQUIRES in CMake | Add `esp_console` to REQUIRES list |
| `fatal error: argtable3.h` | Missing include | Add `argtable3` to REQUIRES |
| `spi_flash_op_failed` during flash | Connection | Hold BOOT button, press RESET, release BOOT |

---

## 19. Future Extensions

Once the core firmware is working, here are natural next steps:

### Phase 2: Enhanced Printing
- **Image download + print**: Download a PNG from a URL, dither to 1-bit, print.
- **Custom fonts**: Upload font files and render text in different typefaces.
- **Print formatting**: Columns, tables, and receipt-style layouts.

### Phase 3: Network Printing
- **HTTP print endpoint**: Run a tiny HTTP server that accepts print jobs.
- **MQTT print**: Subscribe to a topic and print incoming messages.
- **mDNS discovery**: Broadcast as `stoms3r.local` on the network.

### Phase 4: Advanced Features
- **OTA updates**: Update firmware over Wi-Fi without USB.
- **BLE provisioning**: Use Bluetooth to configure WiFi instead of serial.
- **Multi-printer support**: Drive multiple printers from one AtomS3R Lite.
- **Camera integration**: Attach a camera module and print snapshots.

---

## Appendix A — ESC/POS Command Quick Reference

### Initialization
| Bytes | Description |
|-------|-------------|
| `1B 40` | Initialize printer (reset) |

### Text Formatting
| Bytes | Description |
|-------|-------------|
| `1B 45 n` | Bold: n=1 on, n=0 off |
| `1D 21 n` | Font size: n=(height<<4)\|width, 0–7 per nibble |
| `1B 2D n` | Underline: n=0 off, n=1 single, n=2 double |
| `1B 61 n` | Align: n=0 left, n=1 center, n=2 right |
| `1B 24 nL nH` | Set horizontal position |

### Paper Control
| Bytes | Description |
|-------|-------------|
| `0A` | Print and line feed |
| `1B 64 n` | Feed n lines |
| `1D 56 n` | Cut paper (n=0 full, n=1 partial) — may not be supported on K118 |

### Barcodes
| Bytes | Description |
|-------|-------------|
| `1D 6B m n d1..dn` | Print barcode type m with n-byte data |
| `1D 48 n` | HRI position: 0=hide, 1=above, 2=below, 3=both |
| `1D 77 n` | Barcode module width (2–6) |
| `1D 68 n` | Barcode height in dots |

### QR Codes (GS ( k function group)
| Bytes | Description |
|-------|-------------|
| `1D 28 6B 03 00 31 45 n` | Set EC level: n=48(L), 49(M), 50(Q), 51(H) |
| `1D 28 6B pL pH 31 50 30 d1..dk` | Store data (pL=k+3) |
| `1D 28 6B 03 00 31 51 30` | Print QR |

### Raster Bitmap
| Bytes | Description |
|-------|-------------|
| `1D 76 30 m xL xH yL yH d1..dk` | Print raster bitmap |
| m=0 normal, m=1 double-width, m=2 double-height, m=3 both |
| xL,xH = bytes/line (LE16); yL,yH = lines (LE16); k = x×y |

---

## Appendix B — AtomS3R Lite Pin Map

```
AtomS3R Lite Bottom Header (viewed from bottom, USB-C on top):

  ┌────────────────────┐
  │      USB-C         │
  ├────────────────────┤
  │                    │
  │  [RGB LED]  [BTN]  │
  │                    │
  ├────────────────────┤
  │ G39 G38 G8  G7     │
  │  ●   ●   ●   ●    │
  │                    │
  │  ●   ●   ●   ●    │
  │ GND 3V3 5V  G6    │  ← HY2.0-4P port: G5, G6, 5V, GND
  │                    │
  ├────────────────────┤
  │     [HY2.0-4P]     │
  │  G5  G6  5V  GND   │
  └────────────────────┘
```

| Pin | Function | Notes |
|-----|----------|-------|
| G5 | UART1 TX (to printer RX) | Also on HY2.0-4P |
| G6 | UART1 RX (from printer TX) | Also on HY2.0-4P |
| G7 | Free GPIO | Bottom header only |
| G8 | Free GPIO | Bottom header only |
| G38 | Input-only GPIO | No internal pull-up |
| G39 | Input-only GPIO | No internal pull-up |
| GPIO35 | RGB LED (WS2812) | Do not use for other purposes |
| GPIO41 | BOOT button | Active-low, used for firmware download |

### Internal Peripherals

| Peripheral | Pins Used | Notes |
|-----------|-----------|-------|
| USB Serial/JTAG | Internal (USB DP/DM pins) | No GPIO pins consumed |
| Flash (SPI) | Internal (GPIO27–32) | Do not use these pins |
| PSRAM (Octal SPI) | Internal (GPIO33–37) | Do not use these pins |

---

## Appendix C — Related Tickets and References

### Previous Work

| Ticket | Description | Path |
|--------|-------------|------|
| 0090-m5printer-research | Original K118 thermal printer research | `0090-m5printer-research/` |
| 0091-m5printer-ble-provision | BLE provisioning attempt (Arduino) | `0091-m5printer-ble-provision/` |
| 0092-m5-printer-esp-idf-provision | ESP-IDF provisioning on ATOM Lite | `0092-m5-printer-esp-idf-provision/` |

### Key Reference Documents

| Document | Location |
|----------|----------|
| ESC/POS Programming Manual | `0090-m5printer-research/reference/05-escpos-manual.md` |
| ATOM-PRINTER Technical Deep Dive | `0090-m5printer-research/docs/TECHNICAL-DEEP-DIVE.md` |
| ATOM-PRINTER Developer Guide | `0090-m5printer-research/docs/DEVELOPER-GUIDE.md` |
| Bitmap Printing Guide | `0090-m5printer-research/reference/26-bitmap-printing.md` |
| ESP-IDF WiFi Provisioning API | `0091-m5printer-ble-provision/reference/12-wifi_provisioning_api.md` |
| Existing ESP-IDF Printer Driver (ATOM Lite) | `0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov/main/app_printer.c` |

### External References

| Resource | URL |
|----------|-----|
| AtomS3R Official Docs | https://docs.m5stack.com/en/core/AtomS3R |
| AtomS3R Lite Official Docs | https://docs.m5stack.com/en/core/AtomS3%20Lite |
| ESP-IDF Console API | https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/system/console.html |
| ESP-IDF UART Driver | https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/uart.html |
| ESP-IDF NVS API | https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/storage/nvs_flash.html |
| ESP-IDF WiFi STA Guide | https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/wifi.html |
| M5Stack K118 Printer Kit | https://docs.m5stack.com/en/atom/atom_printer |
| ATOM-PRINTER GitHub | https://github.com/m5stack/ATOM-PRINTER |
| argtable3 Documentation | https://www.argtable.org/ |

### ESP-IDF API Header Files

| API | Header |
|-----|--------|
| Console | `#include "esp_console.h"` |
| UART | `#include "driver/uart.h"` |
| NVS | `#include "nvs_flash.h"` and `#include "nvs.h"` |
| WiFi | `#include "esp_wifi.h"` |
| Netif | `#include "esp_netif.h"` |
| Events | `#include "esp_event.h"` |
| Arguments | `#include "argtable3/argtable3.h"` |
| Logging | `#include "esp_log.h"` |
| Error check | `#include "esp_check.h"` |
| FreeRTOS | `#include "freertos/FreeRTOS.h"` and `#include "freertos/task.h"` |
