---
Title: 'Research Diary: Cardputer Serial Terminal Dual Backend'
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - esp32
    - firmware
    - documentation
    - cardputer
    - uart
    - usb-serial
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0015-cardputer-serial-terminal/main/hello_world_main.cpp
      Note: Primary implementation of serial terminal with dual backend
    - Path: esp32-s3-m5/0015-cardputer-serial-terminal/main/Kconfig.projbuild
      Note: Menuconfig options for backend selection and terminal behavior
    - Path: esp32-s3-m5/0038-cardputer-adv-serial-terminal/main/console_repl.cpp
      Note: Shows REPL/backend conflict handling pattern
    - Path: esp32-s3-m5/ttmp/2025/12/24/001-SERIAL-CARDPUTER-TERMINAL--serial-cardputer-terminal-keyboard-echo-uart-usb-output/reference/01-diary.md
      Note: Original implementation diary with design decisions
ExternalSources: []
Summary: 'Personal research diary documenting the investigation of the Cardputer serial terminal dual-backend architecture.'
LastUpdated: 2026-01-12T10:00:00-05:00
WhatFor: 'Capture research trail for the Cardputer serial terminal guide.'
WhenToUse: 'Reference when writing or updating the serial terminal documentation.'
---

# Research Diary: Cardputer Serial Terminal Dual Backend

## Goal

Document the research process for writing the Cardputer serial terminal dual-backend guide. This diary captures what I read, what I learned, and the key insights that should inform the final documentation.

## Step 1: Orientation — Reading the playbook and locating source material

Started by reading the playbook guide at `ttmp/2026/01/11/MO-02-ESP32-DOCS--esp32-firmware-diary-synthesis/playbook/06-guide-cardputer-serial-terminal-dual-backend.md`. The playbook identifies four key source materials:

1. `0015-cardputer-serial-terminal/README.md` — the basic serial terminal
2. `0038-cardputer-adv-serial-terminal/README.md` — the advanced version with REPL
3. `0016-atoms3r-grove-gpio-signal-tester/README.md` — Grove/UART pin usage context
4. Original implementation diary from `ttmp/2025/12/24/001-SERIAL-CARDPUTER-TERMINAL--serial-cardputer-terminal-keyboard-echo-uart-usb-output/reference/01-diary.md`

### What I did
- Read all four READMEs to understand the high-level feature set
- Located the implementation diary for historical context

### What I learned
- Tutorial 0015 is the "vanilla" serial terminal: keyboard → screen + TX to USB/UART
- Tutorial 0038 adds a TCA8418 I²C keyboard (Cardputer-ADV) + fan relay control + `esp_console` REPL
- The key architectural insight: 0038 **defaults to UART backend** so USB Serial/JTAG stays free for `esp_console`

## Step 2: Deep dive into 0015 source code

Read the main implementation file `0015-cardputer-serial-terminal/main/hello_world_main.cpp` (735 lines) and the Kconfig file.

### What I did
- Traced the data flow: `on_keypress → buffer + screen + backend_write`
- Identified the backend abstraction: `serial_backend_t` with `backend_init()`, `backend_write()`, `backend_read()`
- Noted the USB-Serial-JTAG driver installation fix (Step 4 in original diary)

### Key findings

**Backend selection is compile-time via Kconfig:**
```c
#if CONFIG_TUTORIAL_0015_BACKEND_UART
    b.kind = BACKEND_UART;
    // ... uart_driver_install, uart_param_config, uart_set_pin
#else
    b.kind = BACKEND_USB_SERIAL_JTAG;
    // ... usb_serial_jtag_driver_install if not already installed
#endif
```

**Grove UART defaults:**
- RX: G1 (GPIO1)
- TX: G2 (GPIO2)
- Baud: 115200
- UART controller: UART1

**USB-Serial-JTAG gotcha:**
If the driver isn't already installed (e.g., by the console layer), the firmware installs it explicitly to prevent host write timeouts:
```c
if (!usb_serial_jtag_is_driver_installed()) {
    usb_serial_jtag_driver_config_t cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    cfg.rx_buffer_size = 1024;
    cfg.tx_buffer_size = 1024;
    usb_serial_jtag_driver_install(&cfg);
}
```

### Terminal behavior options (Kconfig)
| Option | Default | Effect |
|--------|---------|--------|
| `LOCAL_ECHO` | y | Show typed keys on screen immediately |
| `SHOW_RX` | y | Append received bytes to screen buffer |
| `NEWLINE_CRLF` | y | Enter sends `\r\n` (not just `\n`) |
| `BS_SEND_DEL` | y | Backspace sends 0x7F (DEL), not 0x08 (BS) |

## Step 3: Understanding the REPL conflict (0038 analysis)

Read `0038-cardputer-adv-serial-terminal/main/console_repl.cpp` to understand how the firmware handles the USB Serial/JTAG vs REPL conflict.

### What I did
- Traced the `console_start()` function
- Found the explicit conflict avoidance logic

### Key finding

The 0038 firmware explicitly refuses to start `esp_console` when USB backend is selected:

```c
void console_start(void) {
#if !CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    ESP_LOGW(TAG, "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG is disabled; no REPL started");
    return;
#else
#if defined(CONFIG_TUTORIAL_0038_BACKEND_USB_SERIAL_JTAG)
    // Avoid fighting over the same transport.
    ESP_LOGW(TAG, "USB backend selected; skipping esp_console to avoid USB RX conflicts");
    return;
#endif
    // ... start REPL normally
#endif
}
```

**Why this matters:**
- USB Serial/JTAG is a single transport — only one consumer can read from it
- If `esp_console` is reading for REPL commands, the terminal can't receive typed input
- If the terminal is reading for serial data, REPL commands get eaten

**The 0038 solution:**
- Default to UART backend (`CONFIG_TUTORIAL_0038_BACKEND_UART=y`)
- Reserve USB Serial/JTAG for `esp_console`
- User can switch to USB backend if they don't need REPL

### What I learned
This is a fundamental constraint that must be documented clearly:

> **You cannot have both an interactive REPL and raw serial terminal on USB Serial/JTAG simultaneously.** Choose one:
> - UART backend + USB REPL (recommended for development)
> - USB backend + no REPL (raw serial passthrough)

## Step 4: Grove pin mapping and wiring

Read the Kconfig help text and GPIO definitions to document wiring correctly.

### Grove defaults on Cardputer

| Signal | GPIO | Kconfig Symbol |
|--------|------|----------------|
| RX (into Cardputer) | G1 (GPIO1) | `CONFIG_TUTORIAL_0015_UART_RX_GPIO` |
| TX (out of Cardputer) | G2 (GPIO2) | `CONFIG_TUTORIAL_0015_UART_TX_GPIO` |

### Wiring rule (must document)

When connecting an external USB↔UART adapter:
- **Adapter TX → Cardputer RX (G1)**
- **Adapter RX → Cardputer TX (G2)**

This is the standard "cross TX/RX" rule for UART, but worth emphasizing because it's a common wiring mistake.

### GPIO conflict with keyboard autodetect

The 0015 firmware has a keyboard autodetect feature that probes GPIO1/GPIO2 as alternate IN0/IN1 pins. If UART backend uses these pins, the autodetect is disabled:

```c
#if CONFIG_TUTORIAL_0015_BACKEND_UART
    if (CONFIG_TUTORIAL_0015_UART_RX_GPIO == CONFIG_TUTORIAL_0015_KB_ALT_IN0_GPIO ||
        CONFIG_TUTORIAL_0015_UART_RX_GPIO == CONFIG_TUTORIAL_0015_KB_ALT_IN1_GPIO ||
        ...) {
        s_allow_autodetect_in01 = false;
        ESP_LOGW(TAG, "keyboard autodetect disabled: UART pins conflict with alt IN0/IN1");
    }
#endif
```

This is an internal detail that doesn't need user documentation, but it explains why the firmware works correctly despite the apparent GPIO overlap.

## Step 5: Terminal buffer and echo behavior

Analyzed the terminal buffer implementation to understand echo modes.

### Data flow

```
Keyboard press
  → token_to_tx_bytes() → tx[] bytes
  → backend_write(tx)  [always]
  → if LOCAL_ECHO: term_apply_byte(tx) → buffer → dirty=true

Serial RX (when SHOW_RX enabled)
  → backend_read() → rx[]
  → term_apply_byte(rx) → buffer → dirty=true

Render loop (when dirty)
  → canvas.fillScreen(BLACK)
  → print header lines
  → print visible lines from buffer
  → draw cursor
  → pushSprite()
```

### Echo scenarios to document

| Scenario | LOCAL_ECHO | SHOW_RX | Result |
|----------|------------|---------|--------|
| Talk to echo-enabled device | on | on | Duplicated characters (local + remote echo) |
| Talk to echo-enabled device | off | on | Single echo from remote device |
| Talk to non-echoing device | on | on | Local echo + remote response visible |
| Dumb terminal (output only) | off | on | Only see what remote sends |

**Recommendation for docs:** Suggest disabling LOCAL_ECHO when talking to devices that echo input back.

## Step 6: Flash tip — stable device path

From the 0015 README:

> If `/dev/ttyACM0` disappears/re-enumerates during flashing, use the stable by-id path

```bash
idf.py -p '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00' flash monitor
```

This is a practical tip that should go in the troubleshooting section.

## Step 7: Synthesis — what the final doc needs

Based on this research, the final guide should include:

1. **What the terminal does** — single-paragraph overview
2. **Backend comparison table** — USB vs UART with pros/cons
3. **The USB/JTAG vs REPL conflict** — clear explanation with the constraint
4. **Wiring diagram for Grove UART** — including TX/RX crossing
5. **Menuconfig walkthrough** — key options with recommendations
6. **Echo mode explanation** — when to use LOCAL_ECHO
7. **Troubleshooting checklist** — device path, wiring, echo duplicates

## Quick Reference

### Menuconfig path
`Tutorial 0015: Cardputer Serial Terminal`

### Build commands
```bash
cd esp32-s3-m5/0015-cardputer-serial-terminal
idf.py set-target esp32s3
idf.py build flash monitor
```

### Key files
| File | Purpose |
|------|---------|
| `main/hello_world_main.cpp` | Terminal implementation |
| `main/Kconfig.projbuild` | Menuconfig options |
| `sdkconfig.defaults` | Default configuration |
| `partitions.csv` | Partition table (4MB factory + 1MB storage) |

## Related

- Original diary: `ttmp/2025/12/24/001-SERIAL-CARDPUTER-TERMINAL--serial-cardputer-terminal-keyboard-echo-uart-usb-output/reference/01-diary.md`
- Keyboard guide: link to keyboard documentation when available
- Grove GPIO tester: `0016-atoms3r-grove-gpio-signal-tester/` for signal testing context
