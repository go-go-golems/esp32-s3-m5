---
Title: Diary
Ticket: 0018-QEMU-UART-ECHO
Status: active
Topics:
    - esp32s3
    - esp-idf
    - qemu
    - uart
    - serial
    - debugging
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-29T15:09:28.630658304-05:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Track building and validating the **minimal UART echo firmware** used to isolate whether ESP32-S3 QEMU can deliver **UART RX** bytes into ESP-IDF at all.

## Step 1: Create Track C ticket and capture upstream context

This step creates a new, focused ticket so a fresh debugging session can start without re-deriving the 0015 history. The core idea is to replace “complex REPL firmware + uncertain console stack” with a tiny echo loop that should work in any sane UART emulation.

### What I did
- Created ticket `0018-QEMU-UART-ECHO`.
- Wrote an analysis doc explaining why Track C is the next isolation step after Track A/Track B results in ticket `0015-QEMU-REPL-INPUT`.
- Added a concrete task list to drive the next session.

### Why
- In `0015`, manual typing appears broken.
- Bypass testing via `imports/test_storage_repl.py` depends on QEMU using a TCP serial backend; our live QEMU run was using `-serial mon:stdio`, so TCP-based bypass is blocked unless we change the backend.
- A minimal echo firmware gives a clear binary outcome: either QEMU UART RX works (then the REPL stack is the issue) or it doesn’t (then QEMU/serial config is the issue).

### What should be done next
- Create the new ESP-IDF firmware directory under `esp32-s3-m5/` next to `0016-atoms3r-grove-gpio-signal-tester/`.
- Implement UART echo loop with finite timeout + heartbeat.
- Run in QEMU and capture transcripts for both serial backends:
  - `-serial mon:stdio` (manual typing)
  - `-serial tcp::5555,server` (scriptable TCP client)

## Quick Reference

### Definition of “UART echo firmware”

Minimal firmware that:
- configures UART0 @ 115200
- reads bytes with `uart_read_bytes(..., timeout)`
- echoes received bytes back via `uart_write_bytes()`
- logs a heartbeat when no bytes are received (proves loop is alive)

### What success looks like

- When you type `abc<enter>`, you see `abc` echoed (RX + TX path validated).

## Step 2: Make `build.sh` executable and rebuild the UART echo firmware

This step removes a papercut that was blocking “use the documented commands verbatim”: `./build.sh` wasn’t executable, so trying to run the README steps failed with a permission error. After making it executable (and committing that change), building the firmware is now a single straightforward `./build.sh set-target ... && ./build.sh build` sequence.

This is intentionally boring, but it matters: the rest of Track C depends on being able to repeatedly run `./build.sh qemu ...` without mental overhead or one-off `bash ./build.sh` workarounds.

**Commit (code):** a9d186a85d7c3b1fb9782b72a73b785842eeda0d — "0018: make build.sh executable"

### What I did
- Made `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0018-qemu-uart-echo-firmware/build.sh` executable (`chmod +x`).
- Built the project using ESP-IDF 5.4.1:
  - `./build.sh set-target esp32s3`
  - `./build.sh build`

### Why
- Track C’s value is in quick iteration + reproducible transcripts. A non-executable `build.sh` is small friction, but it derails copy/paste workflows and increases chances of “works on my shell” differences.

### What worked
- `./build.sh qemu --help` and `./build.sh build` both work after the chmod change.
- The firmware builds successfully under ESP-IDF 5.4.x (using `/home/manuel/esp/esp-idf-5.4.1/export.sh`).

### What didn't work
- Before `chmod +x`, attempting to run `./build.sh ...` failed with:
  - `(eval):1: permission denied: ./build.sh`

### What I learned
- This workspace has multiple git roots; the commit for this change lives in the `esp32-s3-m5/` repo (not the workspace root).

### What was tricky to build
- N/A (mechanical change), but worth capturing because it’s an easy “waste 5 minutes” trap.

### What warrants a second pair of eyes
- Confirm we actually want `build.sh` checked in as executable across the repo (this ticket does).

### What should be done in the future
- If we add more `build.sh` helpers in other tracks/tickets, keep them executable from day one to avoid the same friction.

### Code review instructions
- Review `0018-qemu-uart-echo-firmware/build.sh` in commit `a9d186a`.

### Technical details
- Commands used:
  - `cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0018-qemu-uart-echo-firmware`
  - `chmod +x ./build.sh`
  - `./build.sh set-target esp32s3`
  - `./build.sh build`

## Step 3: Try to run under QEMU (mon:stdio) and discover an early Task WDT panic

This step attempted the first Track C validation run under QEMU using the default foreground mode (`-serial mon:stdio`). Instead of reaching `app_main` and starting the UART echo loop, the firmware consistently panicked very early with a Task Watchdog interrupt, rebooted, and repeated.

This is important because it blocks the core Track C question (UART RX): until we keep the firmware alive long enough to print the “UART echo task started” log, we can’t interpret missing RX as a QEMU UART limitation.

### What I did
- Ran `./build.sh qemu` (foreground; ESP-IDF adds `-serial mon:stdio`).
- Captured transcripts to ticket sources:
  - `ttmp/2025/12/29/0018-QEMU-UART-ECHO--track-c-minimal-uart-echo-firmware-to-isolate-qemu-uart-rx/sources/track-c-qemu-mon-stdio-piped-20251229-153152.txt`
- Decoded the backtrace using `xtensa-esp32s3-elf-addr2line` against `build/qemu_uart_echo_0018.elf`.

### What worked
- We captured the exact QEMU command line emitted by ESP-IDF 5.4.1, including the default serial backend:
  - `... -nographic -serial mon:stdio`
- Backtrace decoding confirmed the crash is coming from Task WDT initialization/ISR paths, not from our UART code.

### What didn't work
- The run did not reach `app_main`. It panicked in a loop:
  - `Guru Meditation Error: Core  0 panic'ed (LoadProhibited).`
  - PC mapped to `task_wdt_isr` (ESP-IDF `components/esp_system/task_wdt/task_wdt.c:476`)

### What I learned
- ESP-IDF’s QEMU runner tries to disable watchdogs via a QEMU `-global ...wdt_disable...` knob, but our QEMU binary warns:
  - `qemu-system-xtensa: warning: global timer.esp32s3.timg.wdt_disable has invalid class name`
  This strongly suggests the WDT disable hook is not taking effect for esp32s3 in this environment, causing Task WDT to fire immediately under emulation.

### What was tricky to build
- The failure mode is subtle: nothing in our app is “wrong”, but the emulation environment diverges enough that the watchdog trips before the app logic runs.

### What warrants a second pair of eyes
- Confirm the root cause is truly “QEMU WDT disable hook doesn’t work in our esp32s3 QEMU build”, vs. some other early-interrupt bug. (The `invalid class name` warning is a big clue, but it’s worth sanity-checking.)

### What should be done in the future
- If we need watchdog coverage in QEMU later, we should treat this as an environment/tooling contract: either upgrade the QEMU/ESP-IDF combo where `wdt_disable` works, or keep watchdogs off for emulator-only runs and document the rationale.

### Code review instructions
- Start with the transcript in:
  - `.../sources/track-c-qemu-mon-stdio-piped-20251229-153152.txt`
- Then inspect ESP-IDF QEMU runner logic in `tools/idf_py_actions/qemu_ext.py` (outside repo, but relevant context).

## Step 4: Compare against the `imports/esp32-mqjs-repl` QEMU firmware config

This step cross-checks our failing QEMU run against the “known TX working” import (`imports/esp32-mqjs-repl`) to avoid chasing a self-inflicted config mismatch. The import’s transcript shows it reaches `app_main` and prints the `js>` prompt under QEMU, so its environment/config combination is at least viable for UART TX.

The key takeaway: the import’s success likely depends on a different ESP-IDF/QEMU combination (the captured transcript is ESP-IDF v5.5.2), while this workspace currently only has ESP-IDF 5.4.1 installed. That explains why “same general approach” can succeed there but fail here.

### What I did
- Located the import QEMU transcript:
  - `esp32-s3-m5/imports/qemu_storage_repl.txt` (shows `idf_monitor -p socket://localhost:5555` and reaches `js>`).
- Compared key `sdkconfig` settings between:
  - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/sdkconfig`
  - `esp32-s3-m5/0018-qemu-uart-echo-firmware/sdkconfig.defaults`

### What I learned
- The import project has watchdogs enabled in `sdkconfig` (Task WDT + Int WDT), yet its QEMU run reaches `app_main`.
- The import transcript explicitly reports `ESP-IDF: v5.5.2`, but this machine currently has only `esp-idf-5.4.1` installed under `/home/manuel/esp/`.
- Therefore, “copy the sdkconfig settings” is not sufficient; we need a QEMU-stable setup for **our** ESP-IDF/QEMU version.

### Decision
- For Track C on this environment, disable watchdogs via config so the echo loop can run long enough to test UART RX.
- Implementation detail: `sdkconfig.defaults` changes do **not** automatically override an existing generated `sdkconfig`; we will need an `idf.py fullclean` (via `./build.sh fullclean`) to force regeneration and ensure WDT settings take effect.

### What should be done next
- Run `./build.sh fullclean && ./build.sh set-target esp32s3 && ./build.sh build`
- Re-run the Track C QEMU transcripts (mon:stdio and tcp serial) once the firmware stays alive.

## Step 5: Force-regenerate `sdkconfig` so QEMU watchdog disable actually takes effect

This step fixes a subtle but critical configuration trap: changing `sdkconfig.defaults` does **not** automatically override an already-generated `sdkconfig`. Our first attempt to “disable watchdogs for QEMU” changed defaults but left the effective `sdkconfig` unchanged, so QEMU continued to panic in a Task WDT loop.

After running a fullclean, the regenerated `sdkconfig` now reflects the intended watchdog state (`# CONFIG_ESP_INT_WDT is not set`, `# CONFIG_ESP_TASK_WDT_EN is not set`, `# CONFIG_INT_WDT is not set`). This unblocks the actual purpose of Track C: validating UART RX.

### What I did
- Regenerated the project configuration and rebuilt:
  - `./build.sh fullclean`
  - `./build.sh set-target esp32s3`
  - `./build.sh build`
- Verified effective config in `sdkconfig`:
  - `# CONFIG_ESP_INT_WDT is not set`
  - `# CONFIG_ESP_TASK_WDT_EN is not set`
  - `# CONFIG_INT_WDT is not set`

### Why
- Without this, we’d be “testing UART RX” but actually just repeatedly rebooting before `app_main`, which would produce misleading “no RX” results.

### What warrants a second pair of eyes
- Confirm we’re not accidentally masking another early-boot QEMU bug by disabling watchdogs. (For Track C isolation, this is acceptable; later work may want watchdogs back on real hardware.)
