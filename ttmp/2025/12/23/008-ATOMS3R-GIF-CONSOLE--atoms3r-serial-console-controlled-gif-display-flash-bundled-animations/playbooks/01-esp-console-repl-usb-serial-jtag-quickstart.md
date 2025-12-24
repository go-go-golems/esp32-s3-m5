---
Title: 'Playbook: esp_console REPL quickstart (USB Serial/JTAG + ESP-IDF 5.x)'
Ticket: 008-ATOMS3R-GIF-CONSOLE
Status: active
Topics:
    - esp32s3
    - esp-idf
    - console
    - esp_console
    - usb-serial-jtag
    - repl
    - uart
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: echo-base--openai-realtime-embedded-sdk/components/esp-protocols/components/console_simple_init/console_simple_init.c
      Note: Repo helper showing recommended `esp_console_new_repl_*` usage
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/main/hello_world_main.cpp
      Note: Real example of USB Serial/JTAG REPL + command registration + common init pitfalls
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-23T00:00:00.000000000-05:00
WhatFor: "Copy/paste playbook for adding an esp_console REPL to an ESP-IDF project"
WhenToUse: "When creating a new ESP-IDF project that needs a serial console command interface"
---

# Playbook: esp_console REPL quickstart (USB Serial/JTAG + ESP-IDF 5.x)

## Goal

Set up an **interactive command-line REPL** on an ESP-IDF project using `esp_console`, with an emphasis on **USB Serial/JTAG** (great for ESP32-S3 dev workflows). The end state is:

- a prompt (e.g. `app> `)
- commands like `help`, `list`, `play 2`
- no custom line-editing code

## Context (what this is and why it matters)

`esp_console` is an ESP-IDF facility that gives you a small REPL framework: command registration, help/usage output, argument parsing hooks, and line editing (via linenoise). It’s a great fit for embedded “control plane” work because it keeps the serial UX predictable and minimizes custom parsing code.

The two important architectural decisions are:

- **Transport choice**: USB Serial/JTAG vs UART vs USB CDC
- **Init ownership**: in ESP-IDF 5.x, the `esp_console_new_repl_*` helpers perform more initialization than you might expect. If you double-init the console, you can hit `ESP_ERR_INVALID_STATE` and crash.

This playbook is written to avoid those traps and produce a minimal, reliable setup.

## Architecture overview (mental model)

It helps to think of the console stack as three layers:

- **Transport / device driver**: where bytes come from (UART / USB CDC / USB Serial-JTAG).
- **VFS binding**: how `stdin/stdout` are backed (UART VFS, `usb_serial_jtag_vfs_*`).
- **REPL task**: the `esp_console` line editor + command dispatcher running in a FreeRTOS task.

The `esp_console_new_repl_*` convenience functions wire these layers together for a given transport and then spawn the REPL task.

## Quick Reference

### Recommended approach (ESP-IDF 5.x)

- Enable the transport in `sdkconfig` (or `sdkconfig.defaults`)
- Call:
  - `esp_console_new_repl_usb_serial_jtag(...)` (or `_uart`, `_usb_cdc`)
  - register commands
  - `esp_console_start_repl(repl)`

**Important**: On ESP-IDF 5.4.1, **do not call `esp_console_init()` before** `esp_console_new_repl_usb_serial_jtag()`. The helper already does console initialization internally; calling init twice can return `ESP_ERR_INVALID_STATE`.

### Minimal `menuconfig` settings (USB Serial/JTAG)

- `Component config → Console`
  - Enable: `USB Serial/JTAG` console (`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`)
  - Disable UART default if you want USB-only (`CONFIG_ESP_CONSOLE_UART_DEFAULT=n`)

### AtomS3R: run the REPL on the GROVE port (UART on G1/G2)

`esp_console` is **not** “USB-only”. On ESP-IDF 5.x, the REPL can run over:
- USB Serial/JTAG (`esp_console_new_repl_usb_serial_jtag`)
- UART (`esp_console_new_repl_uart`)
- USB CDC (`esp_console_new_repl_usb_cdc`)

For AtomS3R specifically, you can run the REPL on the external GROVE connector by using the UART transport.

#### AtomS3R GROVE (Port.A) pin mapping (M5Stack standard)

Per the M5Stack AtomS3 docs, the 4-pin HY2.0/GROVE port exposes:

- **GND**: black wire
- **5V**: red wire
- **G2**: yellow wire → `GPIO2`
- **G1**: white wire → `GPIO1`

Important: **G1/G2 are just GPIOs** (they’re *not inherently* RX vs TX). You decide which one is TX/RX via `menuconfig` / `sdkconfig`.

#### Minimal `sdkconfig.defaults` for GROVE UART console

This is the “most copy/paste” configuration (UART1, 115200 baud, TX on G1/GPIO1, RX on G2/GPIO2):

```ini
CONFIG_ESP_CONSOLE_UART_CUSTOM=y
CONFIG_ESP_CONSOLE_UART_CUSTOM_NUM_1=y
CONFIG_ESP_CONSOLE_UART_NUM=1
CONFIG_ESP_CONSOLE_UART_TX_GPIO=1
CONFIG_ESP_CONSOLE_UART_RX_GPIO=2
CONFIG_ESP_CONSOLE_UART_BAUDRATE=115200
```

If you prefer to use a different UART controller (e.g. UART2), set `CONFIG_ESP_CONSOLE_UART_NUM=2` and select the matching UART option in `menuconfig` (ESP-IDF will generate the corresponding `CONFIG_ESP_CONSOLE_UART_CUSTOM_NUM_*` flag for you).

#### Wiring a USB↔UART adapter to AtomS3R GROVE

- **Adapter GND → Atom GND** (black)
- **Adapter RX → Atom TX** (whatever pin you set as `CONFIG_ESP_CONSOLE_UART_TX_GPIO`)
- **Adapter TX → Atom RX** (whatever pin you set as `CONFIG_ESP_CONSOLE_UART_RX_GPIO`)

Make sure the adapter is **3.3V TTL** (do not drive GPIOs with 5V).

#### Minimal code pattern (UART)

With the Kconfig above, you typically don’t need custom UART init code; this uses the Kconfig-selected UART+pins:

```c
#include "esp_console.h"

static void start_console_uart(void) {
#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
    esp_console_repl_t *repl = NULL;

    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.prompt = "app> ";

    esp_console_dev_uart_config_t hw_cfg = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_cfg, &repl_cfg, &repl));

    esp_console_register_help_command();
    register_commands();

    ESP_ERROR_CHECK(esp_console_start_repl(repl));
#else
    ESP_LOGW("console", "UART console disabled in sdkconfig");
#endif
}
```

#### Operational note: logs vs REPL transport

The REPL uses `stdin/stdout` under the hood, so **your prompt + `printf()` output + (often) `ESP_LOG*` output will share the same transport**. If you switch the console to GROVE UART, expect most output to go there too.

### CMake component dependency

In your `main/CMakeLists.txt`, ensure you depend on `console`:

```cmake
idf_component_register(
  SRCS "main.c"
  PRIV_REQUIRES console
)
```

(You can use `REQUIRES console` too; `PRIV_REQUIRES` is usually fine for `main`.)

## Step-by-step procedure

### Step 1: Enable a console transport (sdkconfig.defaults)

For USB Serial/JTAG:

```ini
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
CONFIG_ESP_CONSOLE_UART_DEFAULT=n
```

For UART instead:

```ini
CONFIG_ESP_CONSOLE_UART_DEFAULT=y
```

### Step 1.1: Decide which transport you actually want (trade-offs)

Before you commit to a transport, decide based on how you’ll run and debug the device:

- **USB Serial/JTAG**:
  - Pros: one cable; common on ESP32-S3; works well with `idf.py monitor`
  - Cons: behavior depends on host “connected” state; some boards have bootloader quirks
- **UART**:
  - Pros: very stable; works on any ESP32; easiest to reason about electrically
  - Cons: needs a USB-UART bridge (or external adapter) and extra wiring on some boards

For AtomS3R, USB Serial/JTAG is usually the most convenient during development, but you should be aware that some vendor firmwares modify USB Serial/JTAG behavior at boot.

### Step 2: Register your commands

Create simple command functions:

```c
static int cmd_hello(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("hello\n");
    return 0;
}
```

Register them:

```c
static void register_commands(void) {
    const esp_console_cmd_t hello_cmd = {
        .command = "hello",
        .help = "Print hello",
        .hint = NULL,
        .func = &cmd_hello,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&hello_cmd));
}
```

### Step 3: Create and start the REPL (USB Serial/JTAG)

Minimal pattern:

```c
#include "esp_console.h"

static void start_console_usb_serial_jtag(void) {
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    esp_console_repl_t *repl = NULL;

    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.prompt = "app> ";

    esp_console_dev_usb_serial_jtag_config_t hw_cfg =
        ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();

    // IMPORTANT (ESP-IDF 5.x):
    // Do NOT call esp_console_init() before this for USB Serial/JTAG.
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_cfg, &repl_cfg, &repl));

    esp_console_register_help_command();
    register_commands();

    ESP_ERROR_CHECK(esp_console_start_repl(repl));
#else
    ESP_LOGW("console", "USB Serial/JTAG console disabled in sdkconfig");
#endif
}
```

### Step 3.1: “Be robust” startup pattern (don’t crash the whole app on console failure)

For demos and UI loops, it’s often better to log and continue if the REPL fails to start (e.g. misconfigured transport). Pattern:

- create REPL
- if it fails: log error and continue without console

This is the approach used in our `0013` AtomS3R mock-GIF console chapter.

### Step 4: Verify

- Flash + monitor the device
- You should see a prompt `app> `
- Type `help` and verify `hello` appears
- Type `hello`

## Usage Examples

### Example: “list/play/stop” skeleton (no argtable)

```c
static int cmd_list(int argc, char **argv) { (void)argc; (void)argv; printf("0: red\n1: blue\n"); return 0; }
static int cmd_play(int argc, char **argv) { if (argc < 2) { printf("usage: play <id>\n"); return 1; } printf("play %s\n", argv[1]); return 0; }
static int cmd_stop(int argc, char **argv) { (void)argc; (void)argv; printf("stop\n"); return 0; }
```

### Example: enforce argument parsing policy (“strict int or name”)

When your command interface is part of a demo, it helps to be strict and predictable: reject invalid inputs and print usage.

- Accept either a numeric index (`play 2`) or a name (`play bounce`)
- Print `usage:` on error

This reduces time wasted on “why didn’t it do anything” ambiguity.

## Common pitfalls (and how to avoid them)

### Pitfall: `ESP_ERR_INVALID_STATE` from `esp_console_new_repl_usb_serial_jtag`

Common cause: calling `esp_console_init()` manually, then calling `esp_console_new_repl_usb_serial_jtag()` (double init).

Fix: let the `esp_console_new_repl_*` helper own the init for that transport, then register commands and start the repl.

### Pitfall: Wrong header names / version drift

On ESP-IDF 5.4.1, REPL APIs are available via `esp_console.h`. Avoid copy-pasting older examples that include headers like `esp_console_repl.h` (not present).

### Pitfall: “No prompt appears”

Common causes:

- Wrong transport enabled in `sdkconfig` (UART enabled but you expected USB Serial/JTAG).
- Bootloader / board firmware interfering with USB Serial/JTAG.
- You started the REPL but didn’t keep the main task alive (app exits).

Debug moves:

- Log which transport is compiled in (print `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG` etc.).
- Print “starting repl…” before `esp_console_new_repl_*` and log `esp_err_to_name(err)` on failure.

### Pitfall: “button press doesn’t work” when pairing console + button

Not console-specific, but common in “console-controlled demo” projects:

- confirm the GPIO number matches the board revision
- log `gpio_get_level(pin)` at boot to validate pin mapping
- use ISR → queue, and debounce in the consumer (not in ISR)

## Related

- Repo helper component:
  - `echo-base--openai-realtime-embedded-sdk/components/esp-protocols/components/console_simple_init/console_simple_init.c`
- Parent ticket architecture notes:
  - `008-ATOMS3R-GIF-CONSOLE/analysis/01-brainstorm-architecture-serial-controlled-animation-playback-on-atoms3r.md`

## External resources

- ESP-IDF `esp_console` component source (local, authoritative for your IDF version):
  - `$IDF_PATH/components/console/esp_console.h`
  - `$IDF_PATH/components/console/esp_console_repl_chip.c`
- M5Stack AtomS3 documentation:
  - https://docs.m5stack.com/en/core/AtomS3 — AtomS3/AtomS3R GROVE Port.A pin mapping (G1=GPIO1, G2=GPIO2) and wire colors
- ESP-IDF console documentation:
  - https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/console.html — esp_console REPL APIs (esp_console_new_repl_uart / esp_console_new_repl_usb_serial_jtag)
  - https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/stdio.html — Standard I/O + console routing and UART console Kconfig options (pins/baud)
  - https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/usb-serial-jtag-console.html — USB Serial/JTAG console details on ESP32-S3
