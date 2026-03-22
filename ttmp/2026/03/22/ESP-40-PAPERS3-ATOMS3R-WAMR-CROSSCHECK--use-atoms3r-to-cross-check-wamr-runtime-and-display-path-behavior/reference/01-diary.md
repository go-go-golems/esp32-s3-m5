---
Title: Diary
Ticket: ESP-40-PAPERS3-ATOMS3R-WAMR-CROSSCHECK
Status: active
Topics:
    - papers3
    - atoms3r
    - wasm
    - firmware
    - esp-idf
    - debugging
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0081-atoms3r-wamr-probe-console/CMakeLists.txt
      Note: Project-level dependency boundary that caused the first build failure
    - Path: 0081-atoms3r-wamr-probe-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_thread.c
      Note: Local WAMR task identity override needed for esp_console execution
    - Path: 0081-atoms3r-wamr-probe-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/shared_platform.cmake
      Note: Local WAMR dual-bus override needed to recover AtomS3R execution
    - Path: ttmp/2026/03/22/ESP-40-PAPERS3-ATOMS3R-WAMR-CROSSCHECK--use-atoms3r-to-cross-check-wamr-runtime-and-display-path-behavior/scripts/flash_and_probe_atoms3r.sh
      Note: Stable USB Serial/JTAG flash and probe sequence for AtomS3R
    - Path: ttmp/2026/03/22/ESP-40-PAPERS3-ATOMS3R-WAMR-CROSSCHECK--use-atoms3r-to-cross-check-wamr-runtime-and-display-path-behavior/scripts/serial_send_and_capture.py
      Note: Command capture helper used for every hardware probe
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-22T16:19:17.113850028-04:00
WhatFor: ""
WhenToUse: ""
---


# Diary

## Goal

This diary captures the AtomS3R cross-device debugging experiment for the WAMR work. It exists so a later reader can reconstruct not just what eventually worked, but what we believed at each stage, what turned out to be wrong, and what exact evidence moved the investigation forward.

## Step 1: Create The Cross-Device Control Ticket

The first step in this ticket was to define the AtomS3R work as a control experiment rather than as a new product feature. That distinction matters because it changes the success criteria: the goal is not to build a polished second demo, but to isolate whether the remaining WAMR instability follows the runtime across boards or stays attached to the PaperS3 display stack.

The practical outcome of this step was a new ticket, a design guide that explains the experiment in system terms, and a concrete task list that forces the work to proceed in stages. That provides enough structure to compare results later without mentally reconstructing the whole debugging context from shell history.

### Prompt Context

**User prompt (verbatim):** "Ok, let's continue debugging, creating a new docmgr ticket. Then create a new separate project for the atoms3r, and use that to further your analysis of the WAMR issues. I connected the ATOMS3R on serial."

**Assistant interpretation:** Create a new ticket for the next debugging branch, stand up a dedicated AtomS3R-based comparison project, and use it to test whether the remaining WAMR problems survive outside the PaperS3 hardware path.

**Inferred user intent:** Use a second ESP32-S3 device as a control board so the WAMR investigation stops depending on a single complicated display target.

### What I did

- Created ticket `ESP-40-PAPERS3-ATOMS3R-WAMR-CROSSCHECK`.
- Added the initial design document and diary document to the ticket.
- Inspected the existing AtomS3R projects in the repository to choose a practical base project.
- Identified `0013-atoms3r-gif-console` as the best starting point because it already has known-good AtomS3R display bring-up, backlight sequencing, and `esp_console`.
- Inspected the current PaperS3 WAMR files in `0079` to determine the minimum subset worth porting.

### Why

- The PaperS3 ticket already isolated the remaining failure to a narrow part of the system, but not enough to prove whether the unresolved problem is general to WAMR on ESP32-S3 or specific to the PaperS3 display path.
- AtomS3R provides a cleaner control board than continuing to speculate from the same hardware target.

### What worked

- The repository already contained a good AtomS3R display/console baseline, so this did not need to start from a blank project.
- The `0079` Wasm runtime surface is modular enough to port in pieces.

### What didn't work

- The connected AtomS3R did not show up as a normal `/dev/ttyACM*` or `/dev/ttyUSB*` node when first inspected.
- `lsusb` only showed `303a:8000 Espressif ESP UVC Device`, which suggests the currently connected hardware is present on USB but not in the console mode expected for the planned REPL/flash loop.

### What I learned

- It is easy to say "use another board" in the abstract, but the real work starts with picking the right base project. `0013` is useful because it already solved multiple AtomS3R board-specific details that would otherwise muddy the experiment.
- Hardware visibility on the host is part of the debugging problem. If the board is currently enumerating as a different USB class, that changes how we should flash and monitor it.

### What was tricky to build

- The tricky part was resisting the temptation to immediately clone `0079` onto AtomS3R wholesale. Doing that would preserve a lot of irrelevant complexity and make later conclusions weaker. The safer approach is to combine only the known-good AtomS3R display path from `0013` with the minimum WAMR surface from `0079`.

### What warrants a second pair of eyes

- The USB identity `303a:8000 Espressif ESP UVC Device` may indicate the connected hardware is in a camera-oriented firmware or different USB mode than the planned debug path. That interpretation should be validated before drawing conclusions from the lack of tty devices.

### What should be done in the future

- Create a minimal AtomS3R probe project instead of continuing to reason from existing projects.
- Build the project before trying to untangle the USB mode further, so we have firmware ready once the board path is confirmed.

### Code review instructions

- Start with the design guide for the intended experiment boundaries.
- Confirm that `0013-atoms3r-gif-console` is in fact the right base and that no simpler AtomS3R project in the repo would make a better control.
- Validate the host-side USB enumeration observations before relying on them.

### Technical details

- Candidate base project: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0013-atoms3r-gif-console`
- Wasm source project: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console`
- Host commands used:
  - `find /dev -maxdepth 1 \( -name 'ttyACM*' -o -name 'ttyUSB*' -o -name 'cu.usb*' -o -name 'tty.usb*' \) -print | sort`
  - `find /dev/serial -maxdepth 2 -type l -print 2>/dev/null | sort`
  - `lsusb`

## Step 2: Build A Dedicated AtomS3R Probe Project

The next step was to turn the design intent into a real control-board firmware. I created `0081-atoms3r-wamr-probe-console` as a deliberately narrow project: AtomS3R display bring-up and backlight control come from `0013`, while the runtime service, host API, module registry, and console commands come from `0079`. The first success criterion here was simply "build cleanly under ESP-IDF 5.3.4 without inheriting unrelated repo baggage."

That goal mostly worked, but the first build exposed a useful integration mistake. The copied `CMakeLists.txt` still pointed at the repo-wide `components` directory, which dragged in an unrelated `mqjs_service` dependency chain requiring `mquickjs`. That failure had nothing to do with WAMR or AtomS3R and was fixed by shrinking the component search path so the project only sees the components it actually needs.

### Prompt Context

**User prompt (verbatim):** (same as Step 1)

**Assistant interpretation:** Build a separate AtomS3R project that can exercise the same WAMR lifecycle as PaperS3 while using a different display backend.

**Inferred user intent:** Produce a clean control target whose behavior can be compared directly against the existing PaperS3 runtime findings.

### What I did

- Created `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0081-atoms3r-wamr-probe-console`.
- Copied the known-good AtomS3R display and backlight files from `0013`.
- Copied the minimal Wasm console/runtime files and embedded probe assets from `0079`.
- Added new project-local files:
  - `main/app_main.cpp`
  - `main/atoms3r_canvas.cpp`
  - `main/atoms3r_canvas.h`
  - `main/CMakeLists.txt`
  - `README.md`
- Switched the new project to USB Serial/JTAG console defaults.
- Reduced the embedded Wasm asset set to `return-42`, `log-only`, and `hello-frame`.
- Fixed the first build issue by removing the repo-wide extra component directory from `CMakeLists.txt`.
- Built successfully with:
  - `unset IDF_PYTHON_ENV_PATH IDF_PATH && source /home/manuel/esp/esp-idf-5.3.4/export.sh >/dev/null && idf.py -C 0081-atoms3r-wamr-probe-console reconfigure build`

### Why

- A clean comparison project is more trustworthy than patching more conditions into `0079`.
- The AtomS3R display path is simple enough that a failure there is easier to attribute than on PaperS3.

### What worked

- The mixed-source scaffold compiled successfully under `ESP-IDF 5.3.4`.
- The minimal architecture was small enough that the first build issue was obvious and easy to isolate.

### What didn't work

- The first configure/build attempt failed with:
  - `Failed to resolve component 'mquickjs'`
- Root cause:
  - the copied top-level `CMakeLists.txt` still included the repo-wide `../components` directory, which pulled in unrelated components not needed for this project.

### What I learned

- Cross-project copying is safest when the component graph is kept aggressively small. Otherwise the first failure is often some unrelated dependency bleed-through.
- The AtomS3R probe can stay much leaner than the earlier AtomS3R GIF application.

### What was tricky to build

- The tricky part was not the display or WAMR code. It was trimming the project identity down so that only the intended dependencies remained. If too much of `0013` had been carried across, this ticket would have turned into a generic AtomS3R app port instead of a controlled runtime experiment.

### What warrants a second pair of eyes

- The new `atoms3r_canvas` abstraction is intentionally simple and direct. That is desirable for debugging, but it means future feature work should not assume it is a polished rendering layer.

### What should be done in the future

- Keep the project narrow until the comparison results are fully exploited.
- Resist adding more demo features until the PaperS3-vs-AtomS3R conclusion is stable.

### Code review instructions

- Start with:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0081-atoms3r-wamr-probe-console/CMakeLists.txt`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0081-atoms3r-wamr-probe-console/main/CMakeLists.txt`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0081-atoms3r-wamr-probe-console/main/app_main.cpp`
- Confirm the project only carries the minimum code needed for the probe matrix.

### Technical details

- Initial failing command:
  - `idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0081-atoms3r-wamr-probe-console reconfigure build`
- Initial failure:
  - `Failed to resolve component 'mquickjs'`

## Step 3: Stabilize USB Serial/JTAG Flashing On AtomS3R

Once the build existed, the next blocker was host-side flashing behavior. A naive `idf.py flash` worked in the sense that the image was written, but it left the board in ROM download mode afterward. That made every follow-up probe ambiguous because the board was not actually running the application firmware after flash unless it was explicitly rebooted another way.

The fix was to stop using the default post-flash reset behavior for the scripted path. I copied the known stable USB Serial/JTAG approach already documented elsewhere in this repo: flash with `--before usb_reset --after no_reset`, then issue a second `esptool` call with `--after watchdog_reset` to reboot into the application. I saved this as a ticket-local script so the exact procedure is preserved with the ticket.

### Prompt Context

**User prompt (verbatim):** (same as Step 1)

**Assistant interpretation:** Make the AtomS3R probe practically usable on hardware, not just buildable.

**Inferred user intent:** Avoid ad hoc flashing steps and capture the real transport quirks as part of the investigation.

### What I did

- Added ticket scripts:
  - `scripts/list_usb_candidates.sh`
  - `scripts/serial_send_and_capture.py`
  - `scripts/flash_and_probe_atoms3r.sh`
- Used `list_usb_candidates.sh` to identify the connected board as:
  - `/dev/ttyACM0`
  - `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_B4:3A:45:BE:16:80-if00`
  - USB VID/PID `303a:1001`
- Flashed once with the initial `idf.py`-based script and observed:
  - `rst:0x15 (USB_UART_CHIP_RESET),boot:0x10 (DOWNLOAD(USB/UART0))`
  - `waiting for download`
- Replaced the flashing logic with the more stable `esptool` sequence:
  - flash with `--before usb_reset --after no_reset`
  - reboot with `read_mac` and `--after watchdog_reset`

### Why

- A control-board experiment is only useful if the flash/boot/probe loop itself is reliable.
- USB Serial/JTAG transport behavior is part of the real system under test.

### What worked

- The board was positively identified as `ESP32-S3-PICO-1` with embedded `8MB Flash` and `8MB PSRAM`.
- The revised flash path reliably boots into the application.

### What didn't work

- The original scripted flow using `idf.py -p <port> build flash` followed by immediate serial probing left the board in ROM download mode.

### What I learned

- On this hardware path, the difference between `hard_reset` and `no_reset` plus an explicit watchdog reboot is not cosmetic. It decides whether the next step is talking to the app or talking to the ROM bootloader.
- Ticket-local scripts are especially valuable for transport issues because these details are easy to forget and hard to rediscover quickly.

### What was tricky to build

- The tricky part was that the failure looked superficially like "serial probe not working," but the real issue was earlier: the board was not booting the app after flash. Without keeping the ROM output, this would have been easy to misdiagnose as a prompt timing or serial read problem.

### What warrants a second pair of eyes

- The serial capture helper appears to trigger a fresh `USB_UART_CHIP_RESET` when it opens the USB Serial/JTAG device. That does not block the current scripted use because the app reboots quickly, but it is worth understanding if we later want more interactive long-running sessions.

### What should be done in the future

- If the serial helper reset behavior becomes a problem, investigate whether a different pyserial open mode or monitor path avoids the reset.

### Code review instructions

- Review:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-40-PAPERS3-ATOMS3R-WAMR-CROSSCHECK--use-atoms3r-to-cross-check-wamr-runtime-and-display-path-behavior/scripts/list_usb_candidates.sh`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-40-PAPERS3-ATOMS3R-WAMR-CROSSCHECK--use-atoms3r-to-cross-check-wamr-runtime-and-display-path-behavior/scripts/serial_send_and_capture.py`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-40-PAPERS3-ATOMS3R-WAMR-CROSSCHECK--use-atoms3r-to-cross-check-wamr-runtime-and-display-path-behavior/scripts/flash_and_probe_atoms3r.sh`
- Validate the flashing sequence on the same hardware before refactoring it.

### Technical details

- Initial bad post-flash state:
  - `rst:0x15 (USB_UART_CHIP_RESET),boot:0x10 (DOWNLOAD(USB/UART0))`
  - `waiting for download`
- Stable replacement sequence:
  - `python -m esptool --chip esp32s3 -p <port> -b 460800 --before usb_reset --after no_reset write_flash ...`
  - `python -m esptool --chip esp32s3 -p <port> --before no_reset --after watchdog_reset read_mac`

## Step 4: Reproduce And Then Recover The WAMR Instantiate Failure On AtomS3R

The decisive debugging step was running the actual AtomS3R probe matrix. Before carrying over any local WAMR platform fixes, `wasm status` and `wasm list` worked, but `wasm run-preflush return-42` crashed before guest execution. Decoding the trace showed the exact same failure boundary previously seen in the early `ESP-39` migration stage: `os_mmap()` during linear-memory instantiation inside Espressif WAMR.

That result was important because it proved the fresh AtomS3R project had recreated the stock Espressif baseline rather than some new board-specific problem. After applying the same two local platform fixes already required in `0079` (`WASM_MEM_DUAL_BUS_MIRROR=0` and `xTaskGetCurrentTaskHandle()` instead of `pthread_self()`), the AtomS3R runtime recovered completely for the baseline probe set. `return-42`, `log-only`, replay-only `hello-frame`, and full Wasm-driven `hello-frame` all succeeded on real hardware.

### Prompt Context

**User prompt (verbatim):** (same as Step 1)

**Assistant interpretation:** Use AtomS3R hardware results to sharpen the WAMR diagnosis rather than just speculating from code.

**Inferred user intent:** Determine whether the live PaperS3 bug is truly generic to WAMR on ESP32-S3 or whether it survives only on the PaperS3 path.

### What I did

- Ran the initial AtomS3R probe matrix after successful flash.
- Confirmed:
  - `wasm status` succeeded
  - `wasm list` succeeded
- Reproduced the failure on stock Espressif WAMR:
  - `wasm run-preflush return-42`
- Decoded the AtomS3R backtrace with:
  - `xtensa-esp32s3-elf-addr2line -pfiaC -e build/atoms3r_wamr_probe_console.elf ...`
- Confirmed the failing boundary:
  - `os_mmap`
  - `wasm_mremap_linear_memory`
  - `wasm_allocate_linear_memory`
  - `wasm_runtime_instantiate`
- Diffed the AtomS3R managed component files against the already-recovered `0079` copies.
- Applied the same two local platform fixes into `0081` managed WAMR files.
- Rebuilt and reflashed.
- Reran the baseline probes:
  - `wasm run-preflush return-42`
  - `wasm run-preflush log-only`
  - `wasm replay hello-frame`
  - `wasm run-preflush hello-frame`

### Why

- This was the step that could actually answer the ticket’s main question.
- The important comparison is not just "does AtomS3R crash?" but "does AtomS3R fail at the same boundary as the stock Espressif runtime, and does it recover with the same fixes?"

### What worked

- The stock failure reproduced exactly where expected on AtomS3R.
- The same two local WAMR platform fixes recovered the runtime on AtomS3R.
- After those fixes, all baseline probes succeeded.

### What didn't work

- Before the local WAMR platform fixes, `wasm run-preflush return-42` panicked with:
  - `Guru Meditation Error: Core / panic'ed (Cache disabled but cached memory region accessed).`
  - `Write back error occurred while dcache tries to write back to flash`
- Decoded boundary:
  - `os_mmap()` in `espidf_memmap.c:76`
  - during `wasm_runtime_instantiate()`

### What I learned

- The current PaperS3 live failure is not well explained by "Espressif WAMR simply does not work on ESP32-S3." AtomS3R disproves that after the known local runtime-platform fixes are applied.
- The earlier `ESP-39` recovery changes were not PaperS3 quirks only; they are broader ESP32-S3 runtime integration fixes for our current usage pattern.
- Once those are in place, the AtomS3R display path is stable enough to execute the same Wasm-driven `hello-frame` pattern that still causes trouble on PaperS3.

### What was tricky to build

- The tricky part was separating two different stories that could easily be conflated:
  - "stock Espressif WAMR has a real instantiate problem in this project shape"
  - "after the already-known platform fixes, the AtomS3R board path is healthy"
- Without keeping both the failing and recovered states in the diary, the final result would look simpler than it really was.

### What warrants a second pair of eyes

- The local managed-component patches are still overrides against a generated dependency tree. That is good enough for debugging, but it should not be treated as the final packaging strategy.
- The repeated backlight I2C failures are not blocking the current probe matrix, but the board-specific backlight assumptions likely need cleanup if this project becomes something more permanent.

### What should be done in the future

- Feed this result back into the PaperS3 investigation: the remaining bug should now be treated as PaperS3 display-path specific unless new counter-evidence appears.
- Consider extracting the known-good WAMR ESP32-S3 platform overrides into a more deliberate patching or wrapper strategy rather than hand-editing managed components in multiple projects.

### Code review instructions

- Start with the AtomS3R project files:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0081-atoms3r-wamr-probe-console/main/app_main.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0081-atoms3r-wamr-probe-console/main/atoms3r_canvas.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0081-atoms3r-wamr-probe-console/main/wasm_module_runner.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0081-atoms3r-wamr-probe-console/main/wasm_host_api.cpp`
- Then review the local WAMR platform overrides:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0081-atoms3r-wamr-probe-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/shared_platform.cmake`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0081-atoms3r-wamr-probe-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_thread.c`
- Validate by rerunning the same probe matrix on the same board.

### Technical details

- Decoded failing backtrace before recovery:
  - `0x42019cae: os_mmap at .../espidf_memmap.c:76`
  - `0x42017379: wasm_mremap_linear_memory at .../wasm_memory.c:1504`
  - `0x42017b0a: wasm_runtime_instantiate at .../wasm_runtime_common.c:1652`
  - `0x4200bdf0: RunEmbeddedWasmModuleOnCurrentThread(...) at .../wasm_module_runner.cpp:138`
- Successful recovered probes:
  - `wasm run-preflush return-42`
  - `wasm run-preflush log-only`
  - `wasm replay hello-frame`
  - `wasm run-preflush hello-frame`
