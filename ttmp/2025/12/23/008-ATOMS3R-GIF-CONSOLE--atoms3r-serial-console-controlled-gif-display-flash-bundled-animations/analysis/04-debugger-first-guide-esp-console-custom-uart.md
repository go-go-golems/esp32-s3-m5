---
Title: "Debugger-first guide: untangling esp_console + custom UART stdio (AtomS3R GROVE GPIO1/GPIO2)"
Ticket: 008-ATOMS3R-GIF-CONSOLE
Status: active
Topics:
  - esp32s3
  - esp-idf
  - esp-console
  - serial
  - debugging
  - gpio
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "A debugger-first workflow to isolate electrical/pinmux/UART-driver issues from esp_console/VFS/linenoise behavior when using a custom UART for stdio and REPL."
LastUpdated: 2025-12-25T00:00:00.000000000+00:00
WhatFor: "Use this when TX works but RX seems dead, or when console behavior changes depending on bootloader/app flash and you suspect esp_console/stdin/stdout interactions."
WhenToUse: "When you can attach a JTAG debugger (USB Serial/JTAG or external JTAG) and want to prove where bytes stop flowing: wire → pad → GPIO matrix → UART FIFO → UART driver → VFS stdin → linenoise → command dispatch."
---

## Executive summary

When `esp_console` is bound to a custom UART, it’s tempting to treat “I see the prompt” as “UART is working.” In practice, the system is layered, and **TX success does not imply RX success**. With a debugger, the fastest path is to **pick a single byte** and prove how far it travels.

This guide describes exactly how I would debug the “TX works, RX doesn’t” class of issues on AtomS3R GROVE pins (**GPIO1=TX**, **GPIO2=RX**) when using ESP-IDF 5.4.1. It’s written to avoid folklore: every stage has a concrete “proof” step, a place to set a breakpoint, and a place to read state.

## Mental model: where `esp_console` actually reads from

The key to avoiding confusion is to separate:

- **Console output channel (ESP-IDF “Channel for console output”)**
  - Controls where ROM/newlib logging goes by default (`printf`, `ESP_LOG*`, early boot output).
  - In ESP-IDF, early startup binds ROM output in `components/esp_system/port/cpu_start.c` via `esp_rom_output_set_as_console(...)`.

- **`esp_console` REPL transport**
  - The REPL is a task that ultimately does line-editing reads from **`stdin`** and writes echoes/prompt to **`stdout`**.
  - For UART REPL, ESP-IDF’s helper `esp_console_new_repl_uart()` configures UART, installs the UART driver, then tells VFS “use this UART for stdin/stdout”.

- **Secondary console output**
  - `CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG=y` means “mirror output to another place”.
  - It is **output-only**. It does not explain missing RX.

### The real data path (RX)

```text
Peer TX wire
  -> Atom pad (physical pin)
     -> GPIO input enable / pull resistors
        -> GPIO matrix input select
           -> UART1 RX signal
              -> UART1 RX FIFO
                 -> UART driver ISR + ring buffer
                    -> VFS read() on stdin
                       -> linenoise line editor
                          -> esp_console_run()
                             -> command handler
```

When RX “doesn’t work,” the only productive question is: **which arrow is broken?**

## Ground truth from ESP-IDF source (why this guide is so breakpoint-friendly)

ESP-IDF’s UART REPL helper is short and very explicit:

- `components/console/esp_console_repl_chip.c`:
  - `uart_param_config(dev_config->channel, ...)`
  - `uart_set_pin(dev_config->channel, dev_config->tx_gpio_num, dev_config->rx_gpio_num, -1, -1)`
  - `uart_driver_install(dev_config->channel, 256, 0, ...)`
  - `uart_vfs_dev_use_driver(dev_config->channel)`

That means a debugger can answer, with high confidence:

- Did we actually call `uart_set_pin(1, tx=1, rx=2, ...)`?
- Did the driver install succeed?
- Is the UART ISR running when the peer transmits?
- Are bytes in the UART FIFO?

## Debugger setup (what I’d do first)

This section assumes you have JTAG access (USB Serial/JTAG on ESP32-S3 or external JTAG), and you can run OpenOCD + GDB.

- **Official docs to keep nearby**
  - ESP-IDF JTAG debugging guide: `https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/jtag-debugging/index.html`
  - ESP-IDF stdio/console routing: `https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/stdio.html`
  - USB Serial/JTAG console notes: `https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/usb-serial-jtag-console.html`

### My “first 60 seconds” checklist

- **Build the exact ELF you flashed** (symbols matter).
- **Attach debugger and halt on reset** (so we can catch pin remaps early).
- **Set breakpoints** (see below), then continue.

## Step-by-step debugging workflow (with breakpoints and “proofs”)

### Step 1: Prove whether the RX problem is electrical or software (the fastest fork)

Before blaming `esp_console`, I want a yes/no answer to:

> While the peer transmits, does the ESP32-S3 see UART RX transitions at the pad?

**Proofs (choose one):**

- **Logic analyzer / scope proof**
  - Probe the AtomS3R pin that *should* be RX (G2 → GPIO2).
  - If lows don’t reach near 0V (e.g. ~1.5V low), treat it as **contention / bias** first.

- **Debugger proof**
  - Halt in `app_main` (or just after UART is configured).
  - Call `gpio_get_level((gpio_num_t)2)` in GDB while manually toggling the peer TX.
  - If the level never changes, either the peer is not connected to that pad, or the pad isn’t actually an input (mux/enable/pulls).

If Step 1 fails, `esp_console` is not the problem yet.

### Step 2: Prove that `esp_console` configured UART1 pins the way you think it did

Set breakpoints:

- **B1**: `esp_console_new_repl_uart` in `components/console/esp_console_repl_chip.c`
- **B2**: `uart_set_pin` in `components/esp_driver_uart/src/uart.c`
- **B3**: `uart_driver_install` in `components/esp_driver_uart/src/uart.c`

Then:

- Continue until **B1** triggers.
- Inspect `dev_config`:

Pseudocode of what I’m checking:

```text
assert dev_config->channel == 1
assert dev_config->tx_gpio_num == 1
assert dev_config->rx_gpio_num == 2
assert dev_config->baud_rate == 115200
```

If these values are wrong, the bug is in your config plumbing (Kconfig, menuconfig, defaults, or runtime override).

If they’re correct, continue to **B2** and confirm the actual call uses (1,2).

### Step 3: Prove bytes are entering UART1 hardware at all

At this point, we know the driver asked for UART1 RX on GPIO2. Now we want to know if UART1 sees bytes.

**Proof options:**

- **ISR breakpoint proof**
  - Break on UART RX ISR handler (symbol depends on driver version; search for `rx_intr_handler` in `components/esp_driver_uart/src/uart.c`).
  - While halted at a safe point, resume and send bytes from the peer. If the ISR never triggers, the UART RX signal isn’t reaching the peripheral.

- **UART FIFO register proof**
  - Read UART1 RX FIFO count register (via LL/HAL helpers if available, or directly through GDB register/memory view).
  - If FIFO count never increments while the peer transmits, the failure is before the FIFO (pad / matrix / pin mux).

### Step 4: Prove the UART driver is moving bytes from FIFO → software

If FIFO count increments but the REPL never reacts:

- Confirm `uart_driver_install()` succeeded and RX buffer is configured as expected.
- Confirm the driver’s ring buffer receives bytes (inspect the UART driver object/state in memory).

This is where a debugger is decisive: you can see whether bytes are stuck in FIFO vs stuck in ring buffer vs consumed but not interpreted.

### Step 5: Prove VFS stdin is actually bound to UART1 (and isn’t being overwritten)

ESP-IDF’s helper calls:

- `uart_vfs_dev_use_driver(dev_config->channel)` (from `esp_console_repl_chip.c`)

This is the “stdio glue” step: it is what makes `read(STDIN_FILENO, ...)` pull from the UART driver.

**Breakpoints to set:**

- **B4**: `uart_vfs_dev_use_driver` in `components/esp_driver_uart/src/uart_vfs.c`
- **B5**: `uart_vfs_read` (or similarly named VFS read hook) in `components/esp_driver_uart/src/uart_vfs.c`

**What I check:**

- `uart_vfs_dev_use_driver(1)` is called (channel=1, not 0).
- `uart_vfs_read(...)` is being hit when I type on the *primary input* interface (the GROVE UART in your current design).

If `uart_vfs_read` never triggers, then either:

- stdin isn’t really bound to UART1, or
- your input is coming from somewhere else (common when output is mirrored).

### Step 6: Prove the REPL task is blocked in the place you think it is (linenoise → read())

At this stage, I want to answer a very practical question:

> Is the REPL task actually waiting on `read(stdin)` for bytes, or is it stuck/crashed elsewhere?

**Breakpoints to set:**

- **B6**: `esp_console_repl_task` in `components/console/esp_console_repl_chip.c`
- **B7**: `linenoise` (or the function it calls to read characters) in `components/console/linenoise/linenoise.c`
- **B8**: `read` syscall wrapper in `components/newlib/syscalls.c` (helpful when you want to see which VFS handles stdin)

**What I expect to see:**

- The task enters `esp_console_repl_task()`.
- It eventually reaches a `read()` path and blocks.
- When I transmit a byte on the peer TX line, execution resumes and the byte is observed at the right layer.

If the REPL task is not blocked in a `read()`-like call, then the issue is not “UART RX is dead”; it might be:

- stack overflow / reset (we already saw this class of failure once),
- a lockup before starting the REPL,
- a different task owning stdio.

### Step 7: Prove line ending expectations match your terminal (the “it works but it never submits” trap)

Even when RX bytes arrive, a REPL can feel “dead” if **Enter isn’t recognized**.

ESP-IDF’s REPL helper configures UART VFS line endings in `esp_console_new_repl_uart()`:

- RX line ending expected: `ESP_LINE_ENDINGS_CR`
- TX line ending emitted: `ESP_LINE_ENDINGS_CRLF`

**The practical implication:** your terminal should send `CR` (0x0D) on Enter, or `CRLF`. If it sends only `LF` (0x0A), you may see typed characters but the command never “executes”.

**Debugger proof:** break at `uart_vfs_read` and confirm what byte arrives when you press Enter.

### Step 8: Prove nobody is accidentally driving GPIO2 (the “RX pin is not high‑Z” class)

Your earlier scope observation (~1.5V low when connected) is exactly what I expect when a line is not a clean high‑Z input. If we can attach a debugger, we can stop guessing and check the SoC’s view of that pin.

**Proofs:**

- Call `gpio_get_direction((gpio_num_t)2)` if available (or inspect the GPIO enable registers).
- Call `gpio_get_level((gpio_num_t)2)` while you physically force the line high/low.
- Set breakpoints on GPIO configuration APIs and catch the moment GPIO2’s mode changes:
  - `gpio_config`
  - `gpio_set_direction`
  - `gpio_set_pull_mode` / `gpio_pullup_en`
  - `gpio_set_intr_type`

If you ever see GPIO2 configured as an output (or have output enabled), stop: that is not an `esp_console` “stdio interaction” issue; it’s pin ownership/collision.

### Step 9: Untangle the “mirrored output” illusion (what you might be missing)

This is the most common conceptual trap when you have:

- **primary console on custom UART**, and
- **secondary output mirrored to USB Serial/JTAG**.

You can easily end up with the following UX:

- The prompt and logs appear on **both** UART and JTAG (because output is mirrored),
- but **only one place is actually providing stdin** to the REPL.

In your current `0013` approach, the REPL is started explicitly over UART (our code calls `esp_console_new_repl_uart(...)`), so **interactive input is expected on the UART**, not on JTAG.

So if you type into the JTAG monitor and expect commands to run, it will feel like “RX is broken” even though it’s not; you’re typing into the wrong input path.

**Proof (debugger):** break at `uart_vfs_read` and type on each interface. Only one of them should trigger reads.

## What I would do live (if you literally gave me debugger access)

If I can attach OpenOCD/GDB, I would do this in roughly this order:

1. Halt on reset and set breakpoints `esp_console_new_repl_uart`, `uart_set_pin`, `uart_driver_install`, `uart_vfs_dev_use_driver`.
2. Continue to the UART REPL init and confirm the pins are exactly (TX=1, RX=2).
3. Put a breakpoint in the UART RX ISR path (or poll FIFO count) and send bytes; confirm whether UART1 sees bytes at all.
4. Put a breakpoint in `uart_vfs_read` and confirm bytes reach stdin.
5. Put a breakpoint in `linenoise`’s read path and confirm the REPL consumes bytes and sees Enter (`CR`).
6. If bytes show up in hardware but not in software, I’d focus on UART driver state (interrupt enable, FIFO thresholds).
7. If nothing shows up in hardware, I’d focus on pinmux / contention (GPIO2 not high‑Z, wrong GROVE mapping, TX↔TX wiring mistakes).

## Quick symptom → likely layer mapping

This is the “don’t get stuck” table I keep in my head:

- **Prompt prints on GPIO1, but typing does nothing, and UART RX ISR never fires**
  - Electrical / pinmux / wrong pin / contention.

- **UART RX ISR fires, FIFO count increases, but `uart_vfs_read` never fires**
  - stdin not bound where you think; VFS not installed; you’re typing into the wrong interface.

- **`uart_vfs_read` fires and returns bytes, but Enter never submits**
  - Line endings mismatch (LF vs CR).

- **Everything works electrically, but the board resets shortly after prompt**
  - Task stack / watchdog / unrelated crash masking UART behavior.

## Related files (where to start reading / placing breakpoints)

- ESP-IDF startup binds ROM output: `components/esp_system/port/cpu_start.c`
- ESP-IDF REPL transport glue: `components/console/esp_console_repl_chip.c`
- UART VFS glue: `components/esp_driver_uart/src/uart_vfs.c`
- UART driver: `components/esp_driver_uart/src/uart.c`
- Linenoise input loop: `components/console/linenoise/linenoise.c`
- AtomS3R tutorial REPL start: `esp32-s3-m5/0013-atoms3r-gif-console/main/console_repl.cpp`

## What I’d do next (without changing code)

If you want one actionable “next step” before any more refactors:

- Use the debugger to prove whether UART1 RX FIFO count increments when the peer transmits.
  - If it doesn’t: the next work is electrical/pin mapping.
  - If it does: the next work is VFS/stdio/line endings and “which input is primary”.



