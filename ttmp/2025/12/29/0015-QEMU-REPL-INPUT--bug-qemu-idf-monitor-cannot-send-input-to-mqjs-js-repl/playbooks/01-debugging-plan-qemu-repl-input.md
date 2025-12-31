---
Title: 'Debugging plan: QEMU REPL input (ESP32-S3, UART RX)'
Ticket: 0015-QEMU-REPL-INPUT
Status: active
Topics:
    - esp32s3
    - esp-idf
    - qemu
    - uart
    - console
    - serial
    - microquickjs
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/main.c
      Note: The REPL loop blocks in uart_read_bytes(UART_NUM_0, ..., portMAX_DELAY)
    - Path: imports/test_storage_repl.py
      Note: Raw TCP client to localhost:5555 to bypass idf_monitor input path
    - Path: imports/qemu_storage_repl.txt
      Note: Golden boot log showing js> prompt (TX path)
    - Path: ttmp/2025/12/29/0015-QEMU-REPL-INPUT--bug-qemu-idf-monitor-cannot-send-input-to-mqjs-js-repl/analysis/01-bug-report-qemu-monitor-input-not-delivered-to-uart-repl.md
      Note: Bug report + initial hypotheses
ExternalSources:
    - https://nuttx.apache.org/docs/latest/platforms/xtensa/esp32s3/index.html
Summary: ""
LastUpdated: 2025-12-29T00:00:00Z
WhatFor: ""
WhenToUse: ""
---

# Debugging plan: QEMU REPL input (ESP32-S3, UART RX)

## Purpose

This playbook provides a systematic debugging plan for investigating why interactive input doesn't reach the MicroQuickJS REPL when running on ESP32-S3 under QEMU emulation. The firmware boots successfully and displays the `js>` prompt (proving UART TX works), but typed characters don't echo or evaluate (suggesting UART RX isn't reaching the application).

This is a common pain point when developing REPL-based firmware for embedded systems: the serial path works in one direction (device → host), but fails in the other (host → device). Under QEMU, this becomes even trickier because there are more layers in the stack (emulated hardware, virtual serial ports, monitor tools) and less visibility into what's happening at each stage.

**Who should use this**: Developers debugging UART-based interactive applications on ESP32-S3 under QEMU, or anyone hitting "REPL prompt appears but input doesn't work" issues.

## Background: Why This Is Tricky

When you run `idf.py qemu monitor`, several things happen:

1. **QEMU launches** with `-serial tcp::5555,server`, which maps the emulated ESP32-S3's UART0 to a TCP server on your host.
2. **idf_monitor connects** to `socket://localhost:5555` and acts as a "smart terminal" that can decode stack traces, apply filters, and handle special commands (like Ctrl+T for the menu).
3. **Your firmware** uses `uart_read_bytes(UART_NUM_0, ...)` to read from the UART driver, expecting bytes to arrive whenever you type.

This creates a multi-layer pipeline where bytes can get lost or transformed at any stage:

- **Terminal/tmux layer**: Are keystrokes actually being sent?
- **idf_monitor layer**: Is it forwarding normal keystrokes, or only hotkeys?
- **TCP layer**: Are bytes reaching the TCP socket?
- **QEMU UART emulation**: Does QEMU's ESP32-S3 UART model properly implement RX?
- **ESP-IDF UART driver**: Is the driver configured correctly?
- **Application layer**: Is our code actually reading from the right source?

This playbook provides a structured approach to isolate which layer is failing.

## Goal

Determine **where input bytes are getting lost** in the `idf.py qemu monitor` flow:

```
Host keyboard 
  → idf_monitor 
  → TCP stream (socket://localhost:5555) 
  → QEMU (-serial tcp::5555,server) 
  → ESP32-S3 UART peripheral emulation 
  → ESP-IDF UART driver 
  → uart_read_bytes() in our app
```

And produce either a "known-good" workflow for **interactive RX** or a documented limitation/workaround.

## What We Already Know (Evidence)

Before diving into debugging, it's important to understand what's working and what isn't. This helps us narrow down where the problem lies.

### Working: UART TX and Monitor Connection

The firmware boots successfully under QEMU and prints a complete banner followed by the `js>` prompt. This proves several things:
- **QEMU UART TX emulation works**: The emulated UART can send bytes out.
- **TCP transport works**: Bytes are flowing from QEMU to `idf_monitor` over `localhost:5555`.
- **idf_monitor display works**: The monitor correctly decodes and displays the output.
- **Firmware reaches the REPL loop**: The application has successfully initialized (SPIFFS, JavaScript engine) and entered the interactive loop.

### Working: idf_monitor Control Path

When we press `Ctrl+T` followed by `Ctrl+H`, the monitor prints its help menu. This proves:
- **Keystrokes reach idf_monitor**: Whether typed manually or sent via tmux, some input is getting through.
- **Monitor hotkey processing works**: The monitor is actively processing certain input sequences.

### Not Working: Normal Character Input to Application

When we type regular characters (like `1+2` followed by Enter), nothing happens. The firmware doesn't echo characters and doesn't print any evaluation results. Looking at the code in `imports/esp32-mqjs-repl/mqjs-repl/main/main.c`, the REPL loop is:

```c
while (1) {
    int len = uart_read_bytes(UART_NUM, data, BUF_SIZE, portMAX_DELAY);
    if (len > 0) {
        for (int i = 0; i < len; i++) {
            process_char(data[i]);  // echoes via putchar()
        }
    }
}
```

The `portMAX_DELAY` timeout means this call blocks forever waiting for bytes. The fact that we see no echo or output suggests `uart_read_bytes()` **never returns with data > 0**.

### Critical Gap: Manus "QEMU Tested" Claims

The Manus deliverables (in `imports/`) claim "Working REPL" and "QEMU Compatible" with test verification. However, the only concrete artifact—`imports/qemu_storage_repl.txt`—shows boot logs and the `js>` prompt appearing, but **no interactive evaluation transcript** (no `js> 1+2` followed by `3`).

This means we should treat the claim of "interactive RX works" as **unverified** until we capture a working transcript. It's possible:
- They only validated TX (seeing the prompt), or
- They tested on hardware but not QEMU, or
- Something changed between their build and ours (ESP-IDF version, sdkconfig, build environment).

### Key Configuration Details

Looking at the firmware's `sdkconfig`, several settings are relevant to our investigation:

**Console configuration** (lines 1158-1169):
```
CONFIG_ESP_CONSOLE_UART_DEFAULT=y           # Primary console on UART
CONFIG_ESP_CONSOLE_UART_NUM=0                # Using UART0
CONFIG_ESP_CONSOLE_UART_BAUDRATE=115200      # Matches our REPL baud rate
CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG=y  # Secondary console enabled
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED=y
```

The **secondary USB-Serial-JTAG console** is particularly interesting. On real hardware, the ESP32-S3 has a built-in USB-JTAG interface that can act as a second console output. Under QEMU, this hardware doesn't exist, so having it configured might cause issues. This is our **prime suspect** for Track E.

**UART driver configuration** (lines 835-838):
```
# ESP-Driver:UART Configurations
# CONFIG_UART_ISR_IN_IRAM is not set
```

The UART ISR is *not* in IRAM, which is fine for QEMU (where flash access is fast) but means UART interrupt handling goes through normal flash-cached code paths.

**Line ending configuration** (lines 1823-1828):
```
CONFIG_NEWLIB_STDOUT_LINE_ENDING_CRLF=y      # Outputs \r\n
CONFIG_NEWLIB_STDIN_LINE_ENDING_CR=y         # Expects \r on input
```

This is **critical**: the firmware is configured to expect `\r` (carriage return) as the line ending for stdin. Most modern terminals send `\n` (line feed) when you press Enter. This mismatch could explain why pressing Enter doesn't trigger evaluation—the firmware might be waiting for `\r` while we're sending `\n`.

However, our REPL code explicitly checks for both:
```c
if (c == '\n' || c == '\r') {  // Handles both
```

So this *should* work, but it's worth verifying what the terminal is actually sending.

## Existing Utilities in This Repo

This repository already has several tools and reference materials that make debugging more efficient. Here's what you can leverage:

### Test Scripts

**`imports/test_storage_repl.py`** - Direct TCP REPL Client

This Python script bypasses `idf_monitor` entirely and connects directly to the QEMU serial TCP port (`localhost:5555`). It sends commands, waits for responses, and prints the output. This is invaluable for isolating whether the problem is in the monitor tool or deeper in the stack. The script includes a suite of JavaScript test cases (arithmetic, variables, functions, arrays) that you can run to validate end-to-end functionality.

### Reference Logs

**`imports/qemu_storage_repl.txt`** - Golden Boot Log

This is a captured transcript showing successful QEMU boot with the REPL firmware. It demonstrates the expected boot sequence: SPIFFS initialization, JavaScript engine startup, banner display, and the `js>` prompt. Use this as a baseline to compare against—any deviations might indicate what went wrong. However, note that this log shows only TX (device → host output), not RX (host → device input).

### Build Wrapper

**`imports/esp32-mqjs-repl/mqjs-repl/build.sh`** - Reproducible Build Script

This wrapper script handles ESP-IDF environment setup automatically (sources `export.sh` only when needed) and then runs `idf.py` with your chosen arguments. It's useful for ensuring consistent builds across different shells and environments. For debugging, you can use `./build.sh qemu monitor` to launch QEMU + monitor in a single command.

### Related Playbooks

**Ticket 0017: QEMU Bring-Up Playbook**

Located at `ttmp/2025/12/29/0017-QEMU-NET-GFX.../playbook/01-bring-up-playbook...`, this document covers the fundamentals of running ESP-IDF firmware in QEMU, including common pitfalls like:
- "qemu-system-xtensa not installed" even after installation (environment drift)
- Partition table mismatches causing runtime failures
- Monitor input issues (the problem we're debugging now)

It's worth skimming this before starting, as it provides context on the QEMU setup we're using.

## Investigation Tracks (Decision Tree)

The debugging strategy uses a "divide and conquer" approach, with each track isolating a different layer of the input stack. Start with Track A (quickest validation) and proceed through the tracks based on results. Each track is designed to answer a specific question and either rule out or confirm a hypothesis.

The tracks are ordered by diagnostic value and execution speed—we start with the simplest tests that require no code changes, then move to experiments that require building modified firmware.

### Track A: Confirm Baseline Manual Input (No Automation)

**Question this track answers**: Is the problem specific to our automation (tmux `send-keys`) or does it affect manual typing too?

**Why this matters**: When debugging issues reported from automated testing, it's critical to verify that the issue reproduces with real human interaction. tmux `send-keys` can be tricky—it successfully triggers `idf_monitor` hotkeys (like `Ctrl+T Ctrl+H`), which proves *some* input reaches the monitor, but it might not send regular printable characters the way you'd expect. Terminal mode, line discipline, and buffering can all interfere with automation in subtle ways.

**How to execute**:

1. Start QEMU + monitor using the standard command (see "Commands" section below)
2. Wait for the firmware to boot and display the `js>` prompt
3. **Without using tmux send-keys**, manually type directly into the terminal window:
   ```
   1+2
   ```
   Then press Enter
4. Observe carefully

**What to record**:
- Do you see characters echoing as you type? (The firmware calls `putchar(c)` for each printable character)
- Does the firmware print a result after you press Enter? (Should print `3` followed by a new `js>` prompt)
- Does anything appear in the terminal at all, or is it completely silent?

**How to interpret the results**:

- **If it works manually** (you see `1` `+` `2` echoing, then `3` and a new prompt):
  - The problem is **automation-specific** (tmux, terminal settings, or how we're sending keystrokes)
  - Next step: investigate terminal modes, tmux paste bracketing, or switch to a different automation approach
  - This is actually good news—the core UART path works!

- **If it still fails manually** (no echo, no result):
  - The problem is **fundamental**, not automation-related
  - Proceed immediately to Track B (bypass the monitor entirely) to isolate whether `idf_monitor` or QEMU itself is the problem

### Track B: Bypass `idf_monitor` and Send Raw Bytes to TCP:5555

**Question this track answers**: Is the problem in `idf_monitor`'s input handling, or is it deeper in QEMU's UART emulation?

**Why this matters**: `idf_monitor` is a sophisticated tool that does more than just pass bytes through—it decodes stack traces, colorizes output, filters logs, and handles special commands. This complexity means it could be intercepting, transforming, or dropping input in ways that aren't obvious. By connecting directly to QEMU's TCP serial port, we completely bypass the monitor and send raw bytes straight to the emulated UART.

This test gives us a definitive answer about whether the UART RX path works at all under QEMU. If it works here but not through `idf_monitor`, we know the problem is in the monitor's input handling. If it fails here too, we know the problem is either in QEMU's UART emulation or in our firmware's UART configuration.

**How to execute**:

1. Ensure QEMU + monitor is running (from Track A, or start it fresh)
2. Open a **second terminal** (don't interrupt the monitor)
3. Run the bypass test script:
   ```bash
   python3 /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/test_storage_repl.py
   ```

The script will:
- Connect directly to `localhost:5555` (the TCP port QEMU is listening on)
- Read any initial output (should see the boot log and `js>` prompt)
- Send a series of JavaScript test commands (`1+2+3`, `var x = 10`, `x * 5`, etc.)
- Print responses from the firmware
- Report success or failure for each test

**What to record**:
- Capture the **complete output** from the script (save it to `sources/bypass-test-YYYY-MM-DD-HHMMSS.txt`)
- Pay attention to the "Initial Output" section—did we successfully read the boot log and prompt?
- For each `>>> command` sent, did we get a response back?
- Look for any error messages or exceptions from the Python script itself

**How to interpret the results**:

- **If bypass works** (you see commands echoing and results like `6`, `10`, `50`):
  - **Conclusion**: QEMU's UART RX path is functional!
  - **Root cause**: The problem is in `idf_monitor`'s input forwarding logic, terminal mode incompatibility, or line ending translation
  - **Next steps**: 
    - Check `idf_monitor` source code for input handling
    - Try alternative monitor tools (minicom, screen, socat)
    - Investigate terminal modes and line discipline settings

- **If bypass also fails** (no echo, no results, or connection timeout):
  - **Conclusion**: The problem is **not** in `idf_monitor`
  - **Possible causes**:
    - QEMU's ESP32-S3 UART RX emulation may be incomplete or buggy
    - Our firmware might not be reading from the UART we think it is
    - There may be a configuration mismatch (UART pins, console settings)
  - **Next steps**: Proceed to Track C (minimal echo firmware) to isolate firmware from QEMU issues

### Track C: Minimal UART Echo Firmware (No JS, No SPIFFS)

**Question this track answers**: Is this a fundamental QEMU UART RX limitation, or is it specific to our REPL firmware's complexity?

**Why this matters**: The current firmware does a lot: it initializes SPIFFS, creates/loads JavaScript files, sets up a JavaScript engine with 64KB of memory, starts a FreeRTOS task, and implements a line-editing REPL. Any of these components could be interfering with UART RX in subtle ways—perhaps stdio buffering, console subsystem conflicts, task priority issues, or memory pressure. 

By creating a minimal "echo" firmware that does *nothing* except UART I/O, we can definitively answer: "Does QEMU's ESP32-S3 UART RX emulation work at all?" If the minimal firmware works but the REPL doesn't, we know the problem is in the application layer. If the minimal firmware also fails, we know it's a QEMU or UART driver issue.

**Implementation approach**:

Create a new ESP-IDF project (or a branch) with `main.c` that:

```c
#include "driver/uart.h"
#include "esp_log.h"

void app_main(void) {
    // Configure UART0
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, 2048, 0, 0, NULL, 0);
    
    printf("UART Echo Test - ready\n");
    
    uint8_t data[128];
    int alive_counter = 0;
    
    while (1) {
        // Read with 1 second timeout (not infinite)
        int len = uart_read_bytes(UART_NUM_0, data, sizeof(data), pdMS_TO_TICKS(1000));
        
        if (len > 0) {
            printf("RX: got %d bytes\n", len);
            uart_write_bytes(UART_NUM_0, (const char*)data, len);  // Echo back
        } else {
            // No bytes received in this interval
            if (++alive_counter % 5 == 0) {
                printf("Still alive (no RX for 5 seconds)\n");
            }
        }
    }
}
```

Key design choices:
- **Finite timeout**: Using 1-second timeout instead of `portMAX_DELAY` lets us log "still alive" messages, proving the loop is running
- **Direct UART write**: Using `uart_write_bytes()` instead of `printf()` avoids stdio buffering and console subsystem interactions
- **Logging**: Regular heartbeat messages prove the firmware is running and the TX path works

**What to test**:
1. Build and run: `idf.py build && idf.py qemu monitor`
2. Type `abc` and press Enter
3. Observe the output

**Expected behaviors**:

If UART RX works:
```
UART Echo Test - ready
Still alive (no RX for 5 seconds)
RX: got 4 bytes
abc
Still alive (no RX for 5 seconds)
```

If UART RX doesn't work:
```
UART Echo Test - ready
Still alive (no RX for 5 seconds)
Still alive (no RX for 5 seconds)
Still alive (no RX for 5 seconds)
[continues indefinitely with no "RX: got" messages]
```

**How to interpret the results**:

- **If echo firmware works** (you see "RX: got N bytes" and characters echoed):
  - **Conclusion**: QEMU UART RX fundamentally works
  - **Root cause**: The problem is specific to the REPL firmware—likely interactions between stdio/console subsystems, FreeRTOS task scheduling, line-ending handling, or the JavaScript engine overhead
  - **Next steps**: Focus on Track D (SPIFFS regression) and Track E (console conflicts)

- **If echo firmware fails** (never sees any RX bytes):
  - **Conclusion**: QEMU's ESP32-S3 UART RX emulation is broken, incomplete, or requires special configuration
  - **Root cause**: This is a QEMU emulation limitation, not an application bug
  - **Next steps**: 
    - Check if there's a newer QEMU version with better ESP32-S3 support
    - Search for QEMU esp32s3 UART RX bug reports
    - Consider alternative development workflows (use hardware, or accept QEMU as TX-only for now)

### Track D: "Storage Broke It" A/B Test (SPIFFS vs No SPIFFS)

**Question this track answers**: Did integrating SPIFFS/autoload introduce a regression that broke UART RX?

**Why this matters**: The firmware has evolved through several stages: first a basic REPL, then adding SPIFFS support, then adding automatic library loading from flash. It's possible that one of these additions introduced a subtle bug. For example:

- **SPIFFS initialization** takes significant time (mounting, formatting on first boot) and might interact poorly with UART driver timing
- **File I/O operations** during autoload could be blocking the REPL task or interfering with UART interrupts
- **Memory pressure** from allocating file buffers could be causing heap fragmentation issues
- **Task scheduling** changes (SPIFFS has background tasks) might be affecting REPL task priority

The Manus deliverables don't include a transcript showing that storage+REPL works together interactively—they show that each feature works in isolation (SPIFFS mounts, REPL prompt appears) but not that you can *use* the REPL after storage is loaded.

**Implementation approach**:

Build two firmware variants and test them side-by-side:

**Variant D1 (current - with storage)**:
- Uses the existing `main.c` with full SPIFFS + autoload + REPL

**Variant D2 (no storage)**:
- Comment out or `#ifdef` the SPIFFS initialization in `main.c`:
  ```c
  void app_main(void) {
      // ESP_LOGI(TAG, "Starting ESP32-S3 MicroQuickJS REPL with Storage");
      
      // Skip SPIFFS entirely
      // if (init_spiffs() != ESP_OK) { ... }
      // create_example_libraries();
      // load_autoload_libraries();
      
      // Configure UART (keep this)
      uart_config_t uart_config = { ... };
      ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
      ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
      
      // Initialize JavaScript engine (keep this)
      init_js_engine();
      
      // Start REPL task (keep this)
      xTaskCreate(repl_task, "repl_task", 8192, NULL, 5, NULL);
  }
  ```

**What to test**:
1. Build and run D2: `idf.py build && idf.py qemu monitor`
2. Try typing `1+2` Enter
3. Compare behavior with D1 (the current firmware)

**How to interpret the results**:

- **If D2 works (no storage = RX works) but D1 fails (storage = RX broken)**:
  - **Conclusion**: Storage integration introduced a regression
  - **Suspects**:
    - SPIFFS initialization timing (the ~1.2s format delay might be breaking UART setup)
    - File I/O blocking the REPL task during critical UART events
    - Memory fragmentation from SPIFFS allocations
    - Heap exhaustion when loading libraries (check free heap after autoload vs before)
  - **Next steps**: Bisect the storage initialization (try SPIFFS mount without format, try mount without autoload, try autoload one file at a time)

- **If both D1 and D2 fail**:
  - **Conclusion**: Storage is not the root cause
  - **Implication**: The problem was present before storage was added, or is fundamental to the UART/REPL/QEMU interaction
  - **Next steps**: Focus on Tracks B, C, and E (bypass monitor, minimal echo, console conflicts)

### Track E: Console/UART Ownership Conflicts

**Question this track answers**: Is ESP-IDF's console subsystem interfering with our direct UART driver usage?

**Why this matters**: ESP-IDF has a console component that, by default, uses UART0 for stdout/stderr logging. Our firmware *also* installs a UART0 driver and reads directly from it using `uart_read_bytes()`. On real hardware, this usually works fine—the console writes to UART TX, and we read from UART RX, and they coexist peacefully.

However, under QEMU emulation, there might be subtle conflicts:
- QEMU's UART model might only support one "consumer" of RX data
- The ESP-IDF console might be registering VFS handlers that intercept stdin reads
- There could be initialization ordering issues where the console configures UART0 one way, then we reconfigure it
- The secondary USB-Serial-JTAG console configuration might be interacting with UART0 in unexpected ways

Looking at `imports/esp32-mqjs-repl/mqjs-repl/sdkconfig`, we can see:
```
CONFIG_ESP_CONSOLE_UART_DEFAULT=y
CONFIG_ESP_CONSOLE_UART_NUM=0
CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG=y
```

This means ESP-IDF is configured to use **UART0 as primary console** and **USB-Serial-JTAG as secondary**. The secondary console setting is particularly interesting—on real hardware, USB-Serial-JTAG works, but under QEMU it might not be properly emulated, potentially causing configuration conflicts.

**How to investigate**:

1. **Check what the console subsystem is doing**:
   - Add logging right before and after `uart_driver_install()` to see if the console is fighting us
   - Check if `uart_is_driver_installed(UART_NUM_0)` returns true before we try to install

2. **Try disabling secondary console**:
   - Run `idf.py menuconfig`
   - Navigate to: Component config → ESP System Settings → Channel for console secondary output
   - Select "No secondary console"
   - Rebuild and test

3. **Experiment with VFS/stdin approach** (optional, for learning):
   - Instead of `uart_read_bytes()`, try reading from stdin using standard C I/O:
     ```c
     char buf[128];
     if (fgets(buf, sizeof(buf), stdin)) {
         // process buf
     }
     ```
   - This lets ESP-IDF's VFS layer handle UART reads
   - If this works, it suggests our direct UART driver usage is conflicting with something

**How to interpret the results**:

- **If disabling secondary console fixes it**:
  - **Conclusion**: USB-Serial-JTAG configuration under QEMU was interfering with UART0 RX
  - **Fix**: Disable secondary console for QEMU builds, or switch secondary to "None"

- **If VFS/stdin approach works but uart_read_bytes() doesn't**:
  - **Conclusion**: There's a conflict between direct UART driver usage and ESP-IDF console initialization
  - **Options**: 
    - Use the VFS approach for REPL input (less control but more compatible)
    - Investigate console subsystem initialization order
    - Try `esp_vfs_dev_uart_use_driver()` to properly bridge VFS and UART driver

- **If nothing changes**:
  - **Conclusion**: Console configuration isn't the problem
  - Focus on other tracks

## Quick Start: Recommended Order

If you're new to this issue, follow these tracks in order. Each one builds on the previous and takes 5-15 minutes:

1. **Start with Track A** (manual typing) - 5 min
   - Quickest way to rule out automation issues
   - Requires no code changes or scripts

2. **If Track A fails, run Track B** (bypass script) - 5 min
   - Uses existing `test_storage_repl.py`
   - Definitively answers "does UART RX work at all?"

3. **Based on Track B results**:
   - If Track B works → investigate `idf_monitor` input handling
   - If Track B fails → run Track C (minimal echo) to confirm QEMU limitation

4. **In parallel, try Track E** (disable secondary console) - 10 min
   - Quick config change via menuconfig
   - Often fixes UART conflicts

5. **If still stuck, try Track D** (no-SPIFFS build) - 15 min
   - Requires code modification
   - Tests storage regression hypothesis

## Commands (Copy/Paste Ready)

These commands assume you're starting from the repository root. Adjust paths if needed.

### Command 1: Run QEMU + Monitor (Baseline)

This starts the current firmware under QEMU with `idf_monitor` attached. You'll see boot logs, then the `js>` prompt.

```bash
# Navigate to firmware directory
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl

# Clear environment variable that might skip export.sh
unset ESP_IDF_VERSION

# Run QEMU with monitor (wrapper handles ESP-IDF env setup)
./build.sh qemu monitor
```

**Expected output**:
```
Running qemu on socket://localhost:5555
--- esp-idf-monitor 1.8.0 on socket://localhost:5555 115200
--- Quit: Ctrl+] | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H
[... boot logs ...]
js>
```

**To test input**: Just start typing in this terminal window.

**To exit**: Press `Ctrl+]` to quit the monitor (QEMU will stop automatically).

### Command 2: Bypass idf_monitor (Raw TCP Test)

This runs the Python test script that connects directly to QEMU's TCP serial port, bypassing the monitor entirely.

**Prerequisites**: QEMU must be running from Command 1 (in a separate terminal or tmux session).

```bash
# Run the bypass test script
python3 /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/test_storage_repl.py
```

**Expected output if RX works**:
```
Connecting to QEMU on localhost:5555...
Connected!

Initial Output:
[... boot logs and js> prompt ...]

Testing JavaScript Execution
--- Test: Basic arithmetic ---
>>> 1+2+3
6
js>

--- Test: Variable declaration ---
>>> var x = 10
js>
[... more tests ...]
```

**Expected output if RX doesn't work**:
```
Connecting to QEMU on localhost:5555...
Connected!

Initial Output:
[... boot logs and js> prompt ...]

Testing JavaScript Execution
--- Test: Basic arithmetic ---
>>> 1+2+3
[timeout, no response]
```

### Command 3: Check QEMU Process and Port

If the bypass script can't connect, verify QEMU is actually running and listening on the right port.

```bash
# Check if QEMU process is running
ps aux | grep qemu-system-xtensa

# Check if port 5555 is open
netstat -tlnp | grep 5555
# or
ss -tlnp | grep 5555
```

### Command 4: Run in tmux (For Long Sessions)

If you want to run QEMU in the background and attach/detach freely:

```bash
# Start QEMU in a named tmux session
tmux new-session -s mqjs-qemu \
  -c /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl \
  "unset ESP_IDF_VERSION; ./build.sh qemu monitor"

# Detach: Ctrl+B then D
# Reattach later:
tmux attach -t mqjs-qemu

# Kill the session when done:
tmux kill-session -t mqjs-qemu
```

## Firmware Instrumentation Ideas (If We Need More Visibility)

If the investigation tracks above don't give a clear answer, we may need to add diagnostic logging to the firmware to see exactly what's happening inside the UART driver. These modifications should be done on a branch and kept minimal—we want to add visibility without changing behavior.

**⚠️ Important**: Make these changes on a `debug/uart-visibility` branch, not on main. We want to be able to easily revert them.

### Change 1: Finite Timeout with Heartbeat Logging

**Current code**:
```c
int len = uart_read_bytes(UART_NUM, data, BUF_SIZE, portMAX_DELAY);
```

**Modified code**:
```c
int len = uart_read_bytes(UART_NUM, data, BUF_SIZE, pdMS_TO_TICKS(200));
if (len > 0) {
    ESP_LOGI(TAG, "UART RX: received %d bytes", len);
    for (int i = 0; i < len; i++) {
        process_char(data[i]);
    }
} else if (len == 0) {
    // Periodic heartbeat so we know loop is alive
    static int heartbeat_counter = 0;
    if (++heartbeat_counter % 25 == 0) {  // Every 5 seconds
        ESP_LOGI(TAG, "UART RX: waiting (5s since last byte)");
    }
}
```

**What this reveals**:
- Proves the REPL task is running (not stuck elsewhere)
- Shows if we're getting *any* RX bytes at all
- Gives us timing information (how often we check)

### Change 2: Log UART Driver State After Installation

**Add after `uart_driver_install()`**:
```c
ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));

// Diagnostic logging
size_t buffered = 0;
esp_err_t err = uart_get_buffered_data_len(UART_NUM, &buffered);
ESP_LOGI(TAG, "UART driver installed: buffered_data=%zu, err=%s", 
         buffered, esp_err_to_name(err));

uart_port_t uart_num_check = UART_NUM;
ESP_LOGI(TAG, "Using UART_NUM_%d", uart_num_check);
```

**What this reveals**:
- Confirms the driver actually installed (would catch API failures)
- Shows which UART number we're using (catches wrong-constant bugs)
- Baseline buffered data count (should be 0 initially)

### Change 3: Direct UART Write for Echo (Bypass stdio)

**Current code**:
```c
putchar(c);
fflush(stdout);
```

**Alternative for diagnostics**:
```c
uart_write_bytes(UART_NUM, &c, 1);  // Direct write, no stdio buffering
```

**What this reveals**:
- Whether stdio buffering/console subsystem is interfering with echo
- Confirms our UART driver handle is valid (write would fail if not)

**When to use these**: Only if Tracks A-E don't give clear results. Start with minimal changes (just the heartbeat logging) and add more only if needed.

## Success Criteria: What "Done" Looks Like

This issue will be considered resolved when we can demonstrate reliable interactive input to the REPL, with evidence captured for future reference.

### Minimum Success Criteria

**Functional requirement**:
- Type `1+2` followed by Enter
- See the characters echo as you type: `1` `+` `2`
- See the evaluation result: `3`
- See a new prompt: `js>`

**Documentation requirement**:
- Capture a complete transcript showing multiple interactive commands working
- Store it in `sources/qemu-repl-input-working-YYYY-MM-DD.txt`
- Document the exact steps/configuration needed to reproduce

### Ideal Success Scenario

Beyond the minimum, we ideally want:

1. **Reproducible workflow**: Clear, copy-paste commands that any developer can run to get interactive REPL working
2. **Understanding of root cause**: Know *why* it wasn't working and what fixed it
3. **Regression prevention**: Document any configuration gotchas or prerequisites
4. **Test automation**: Either confirm `test_storage_repl.py` works, or create an equivalent test that validates RX

### Acceptable "Done" States

Not all outcomes mean "success" in the traditional sense, but they can still be valuable conclusions:

**Outcome 1: It works, and we document the workflow**
- Best case: we find the issue, fix it, and document the working configuration
- Deliverable: Updated playbook with verified steps

**Outcome 2: It works with bypass script but not with idf_monitor**
- Acceptable: we have a working development loop, just using the Python script instead of the monitor
- Deliverable: Document the workaround and file an upstream bug report about idf_monitor

**Outcome 3: QEMU RX doesn't work (fundamental limitation)**
- Valid conclusion: we document that QEMU ESP32-S3 UART RX is incomplete/broken
- Deliverable: Update playbooks to note "QEMU is TX-only, use hardware for interactive testing"
- Impact: We'd need to test REPL interactivity on actual ESP32-S3 hardware instead

**Outcome 4: Firmware needs modification (console API instead of raw UART)**
- Acceptable: we switch to using ESP-IDF's console component APIs
- Deliverable: Updated firmware that uses supported console APIs
- Trade-off: Might lose some REPL features (custom line editing) but gain compatibility

### What Gets Archived

Regardless of outcome, we should capture:
- Complete transcripts of all tests (working or not)
- The exact firmware version (git hash) tested
- The ESP-IDF version and QEMU version used
- Any configuration changes made
- The decision made and why

## Decision Matrix: What to Do Based on Results

This matrix helps you decide which track to pursue next based on your results so far.

| Track A Result | Track B Result | Next Action | Likely Root Cause |
|---|---|---|---|
| ✅ Manual works | n/a | Stop - it works! Document workflow | Automation issue (tmux) |
| ❌ Manual fails | ✅ Bypass works | Investigate idf_monitor | Monitor input forwarding bug |
| ❌ Manual fails | ❌ Bypass fails | Run Track C (minimal echo) | QEMU or firmware issue |
| n/a | n/a | ✅ Echo works | REPL/stdio/console conflict |
| n/a | n/a | ❌ Echo fails | QEMU UART RX limitation |

**Quick routing guide**:
- **Both A and B fail** → QEMU or UART driver issue → Track C (echo)
- **A fails, B works** → Monitor issue → Check idf_monitor docs/alternatives
- **Echo works, REPL doesn't** → App layer issue → Tracks D and E
- **Echo fails** → QEMU limitation → Document, test on hardware

## Common Pitfalls and Known Issues

Based on web research and prior debugging in this repo, here are issues that commonly cause similar symptoms:

### Pitfall 1: USB-Serial-JTAG Secondary Console Conflicts

**Symptom**: UART RX doesn't work even though TX works fine.

**Cause**: ESP-IDF config has `CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG=y`, which enables a secondary console on USB-Serial-JTAG. Under QEMU, this peripheral isn't properly emulated, and its presence can interfere with UART0 RX.

**Solution**: Disable secondary console via `idf.py menuconfig`:
```
Component config → ESP System Settings → Channel for console secondary output → None
```

Reference: [GitHub Issue #9369](https://github.com/espressif/esp-idf/issues/9369) describes Console REPL issues with USB-Serial-JTAG.

### Pitfall 2: Flash Mode/Frequency Incompatibility with QEMU

**Symptom**: Firmware boots but behaves unexpectedly, or boot logs show warnings.

**Cause**: QEMU's flash emulation is more limited than real hardware. Some configurations (like QIO mode or 120 MHz frequency) aren't properly emulated.

**Solution**: Use conservative flash settings known to work with QEMU:
- Flash mode: DIO (not QIO)
- Flash frequency: 80 MHz (not 120 MHz)

Check your `sdkconfig`:
```
CONFIG_ESPTOOLPY_FLASHMODE_DIO=y
CONFIG_ESPTOOLPY_FLASHFREQ_80M=y
```

Reference: [Bug Advisory AR2024-010](https://documentation.espressif.com/ar2024-010_en.html) about HPM-DC flash issues.

### Pitfall 3: Environment Drift (ESP_IDF_VERSION Already Set)

**Symptom**: `qemu-system-xtensa is not installed` even after you installed it.

**Cause**: If `ESP_IDF_VERSION` is already set in your shell, the build wrapper skips sourcing `export.sh`, which means newly-installed tools aren't in your PATH.

**Solution**: Run in a fresh shell, or explicitly unset before using wrapper scripts:
```bash
unset ESP_IDF_VERSION
./build.sh qemu monitor
```

### Pitfall 4: Wrong UART Number

**Symptom**: No RX bytes, but everything else seems configured correctly.

**Cause**: Firmware might be reading from UART1 or UART2 while QEMU maps `-serial` to UART0.

**Solution**: Verify in your firmware:
```c
#define UART_NUM UART_NUM_0  // Must be UART0 for default QEMU setup
```

And check what QEMU is actually mapping with verbose flags (advanced).

## External Resources

These resources provide additional context and debugging approaches:

### Official Documentation

- **[ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/)** - Official ESP-IDF documentation
- **[ESP32-S3 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)** - Hardware-level UART details
- **[ESP-IDF UART Driver API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/uart.html)** - UART driver documentation

### QEMU and Emulation

- **[NuttX ESP32-S3 QEMU Documentation](https://nuttx.apache.org/docs/latest/platforms/xtensa/esp32s3/index.html)** - Covers QEMU setup and limitations for ESP32-S3
- **[esp32-qemu-sim GitHub](https://github.com/tobozo/esp32-qemu-sim)** - Community QEMU scripts and examples

### Related Issues

- **[ESP-IDF Issue #9369](https://github.com/espressif/esp-idf/issues/9369)** - Console REPL not working with USB-Serial-JTAG
- **[Bug Advisory AR2024-010](https://documentation.espressif.com/ar2024-010_en.html)** - Flash frequency configuration issues

### Internal References

- **[Ticket 0017 QEMU Bring-Up Playbook](../../../0017-QEMU-NET-GFX--qemu-esp32-s3-exploration-networking-graphics-project/playbook/01-bring-up-playbook-run-esp-idf-esp32-s3-firmware-in-qemu-monitor-logs-common-pitfalls.md)** - General QEMU setup and common pitfalls
- **[Bug Report Analysis](../analysis/01-bug-report-qemu-monitor-input-not-delivered-to-uart-repl.md)** - Initial investigation and hypotheses
- **[Investigation Diary](../reference/01-diary.md)** - Step-by-step progress log


