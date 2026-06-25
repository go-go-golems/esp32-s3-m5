---
Title: Investigation diary
Ticket: ATOMS3R-M12-NATIVE-QUICKJS
Status: active
Topics:
    - atoms3r
    - esp32s3
    - quickjs
    - javascript
    - firmware
    - psram
    - repl
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0103-atoms3r-m12-native-quickjs/main/app_main.cpp
      Note: AtomS3R M12 QuickJS service startup and PSRAM baseline logging
    - Path: 0103-atoms3r-m12-native-quickjs/sdkconfig.defaults
      Note: ESP32-S3 USB Serial/JTAG and PSRAM configuration
    - Path: ttmp/2026/06/25/ATOMS3R-M12-NATIVE-QUICKJS--atoms3r-m12-native-quickjs-firmware-with-psram/design-doc/01-atoms3r-m12-native-quickjs-analysis-design-and-implementation-guide.md
      Note: Primary intern-facing design guide
ExternalSources: []
Summary: Chronological diary for the AtomS3R M12 native QuickJS with PSRAM ticket.
LastUpdated: 2026-06-25T22:30:00-07:00
WhatFor: Use to resume AtomS3R M12 QuickJS bring-up and hardware validation.
WhenToUse: Read before flashing 0103, changing serial target, changing PSRAM settings, or extending QuickJS APIs on AtomS3R.
---

# Diary

## Goal

Capture the design, implementation, validation, failures, and follow-up work for running native upstream QuickJS on the AtomS3R M12 / ESP32-S3 with PSRAM. The first target is a USB Serial/JTAG console firmware that proves the QuickJS service and memory profile before adding WiFi, storage, or display APIs.

## Step 1: Create the AtomS3R M12 QuickJS ticket and build the first firmware scaffold

Started a new board-specific QuickJS port for the AtomS3R M12. The workstation also has an ESP32-P4 attached, so the first implementation decision was to identify the serial devices by USB identity and avoid numeric `/dev/ttyACM*` assumptions. The AtomS3R M12 is the Espressif USB Serial/JTAG device; the ESP32-P4 is the CH343 bridge.

The initial firmware target, `0103-atoms3r-m12-native-quickjs`, reuses the proven native QuickJS components from the ESP32-P4 work. It starts `qjs_service` with a conservative 1 MiB QuickJS memory cap, logs PSRAM/heap baselines before and after service startup, and exposes the same `js status`, `js eval`, `js reset`, `js gc`, and `js bench` console commands through USB Serial/JTAG.

### Prompt Context

**User prompt (verbatim):** "I connected a atoms3r M12 (there's also a ESP32P4 connected, so don't mix them up and use device-id to find the serial interface.

I want you to stasrt a new ticket and a new firmware to build and run quickjs on our S3r M12 using PSRAM.

Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable."

**Assistant interpretation:** Create a new docmgr ticket and new ESP-IDF firmware target for native QuickJS on AtomS3R M12 / ESP32-S3 with PSRAM, using by-id serial identification to avoid flashing the connected ESP32-P4, and write/upload a detailed intern-facing design guide.

**Inferred user intent:** Validate that the smaller ESP32-S3R8-class AtomS3R M12 can run the native QuickJS service with PSRAM headroom, then use that as a foundation for later WiFi/storage/device API work.

**Commit (code):** 3bfd6cc8670cb9ae756e706dbc065467c56e4d04 — "0103: add AtomS3R M12 native QuickJS scaffold"

### What I did

- Identified connected serial devices:
  - AtomS3R M12: `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_B4:3A:45:BE:16:80-if00 -> /dev/ttyACM1`.
  - ESP32-P4: `/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00 -> /dev/ttyACM0`.
- Created docmgr ticket `ATOMS3R-M12-NATIVE-QUICKJS` with topics `atoms3r`, `esp32s3`, `quickjs`, `javascript`, `firmware`, `psram`, and `repl`.
- Added the design guide `design-doc/01-atoms3r-m12-native-quickjs-analysis-design-and-implementation-guide.md`.
- Replaced `tasks.md` with phased implementation tasks.
- Created firmware directory `0103-atoms3r-m12-native-quickjs` with:
  - `CMakeLists.txt`
  - `README.md`
  - `sdkconfig.defaults`
  - `partitions.csv`
  - `main/CMakeLists.txt`
  - `main/app_main.cpp`
  - `main/js_command.cpp`
  - `main/js_command.h`
- Reused existing components:
  - `components/quickjs_native`
  - `components/qjs_service`
- Configured the first AtomS3R build for:
  - ESP32-S3 target.
  - USB Serial/JTAG console.
  - 8 MB flash.
  - Octal PSRAM at 80 MHz.
  - `CONFIG_SPIRAM_USE_MALLOC=y`.
  - 4 MiB app partition plus 3 MiB reserved data partition.
  - 1 MiB QuickJS memory limit.
  - 64 KiB QuickJS stack limit.
  - 32 KiB-class QuickJS owner task stack.
- Built the firmware successfully with ESP-IDF 5.4.2:

```text
0103-atoms3r-m12-native-quickjs.bin binary size 0xb4d00 bytes. Smallest app partition is 0x400000 bytes. 0x34b300 bytes (82%) free.
```

### Why

- The ESP32-P4 native QuickJS firmware proves the service architecture, but the AtomS3R M12 has a different memory envelope and console path.
- A separate `0103` target keeps the S3R port isolated from the active ESP32-P4 visual REPL work and from the colleague's separate ticket/firmware.
- Starting console-only avoids mixing QuickJS memory bring-up with display, WiFi, or filesystem variables.

### What worked

- Device identity was unambiguous via `/dev/serial/by-id`.
- The new ESP32-S3 firmware scaffold compiled successfully.
- The reusable QuickJS components did not require source changes for the ESP32-S3 build.
- The app partition has substantial headroom: 82% free in the 4 MiB factory partition.

### What didn't work

- No hardware flash or console smoke has been run yet. The firmware is built but not validated on the AtomS3R M12 device.
- The QuickJS component still emits known upstream pointer-type warnings. They are already locally tolerated in `components/quickjs_native` and did not fail the build.

### What I learned

- The existing `quickjs_native` and `qjs_service` boundaries are portable enough to compile for ESP32-S3 without changing the engine or service code.
- The largest immediate risk is not compile-time portability; it is validating the exact PSRAM mode and memory headroom on the AtomS3R M12 hardware.
- By-id serial paths are mandatory in this session because both the AtomS3R and ESP32-P4 expose `/dev/ttyACM*` devices.

### What was tricky to build

- The serial target had to be treated as part of the design, not as an afterthought. Accidentally flashing `/dev/ttyACM0` would hit the ESP32-P4 CH343 bridge, not the AtomS3R.
- The PSRAM configuration needs to be conservative for first boot. The design uses Octal 80 MHz before attempting faster modes.
- The firmware reserves storage partition space now but avoids mounting or using it until QuickJS memory behavior is characterized.

### What warrants a second pair of eyes

- Review `sdkconfig.defaults` against the exact AtomS3R M12 PSRAM electrical configuration; if the board differs from the expected ESP32-S3R8/Octal profile, boot may fail or PSRAM may not initialize.
- Review whether `REQUIRES` pulls in more ESP-IDF networking components than necessary for milestone 1; the binary still fits comfortably, but dependency reduction may help later.
- Review `cfg.task_stack_words = 32768`; it follows the proven P4 service but should be measured on S3 after hardware smoke.

### What should be done in the future

- Flash and monitor from tmux using only the AtomS3R by-id path.
- Capture boot logs to `/tmp/0103-atoms3r-m12-native-quickjs-smoke.log`.
- Run `js status`, `js eval "print(1+2)"`, exception, completion-value, reset, and bench tests.
- Record PSRAM size, internal heap, 8-bit heap, PSRAM free, QuickJS memory used, atom count, and benchmark timings.

### Code review instructions

- Start in `0103-atoms3r-m12-native-quickjs/sdkconfig.defaults` for board configuration.
- Review `0103-atoms3r-m12-native-quickjs/main/app_main.cpp` for service startup, memory logging, and USB Serial/JTAG console creation.
- Review `0103-atoms3r-m12-native-quickjs/main/js_command.cpp` for command parity with the proven ESP32-P4 native console.
- Validate build with:

```bash
source /home/manuel/esp/esp-idf-5.4.2/export.sh
cd 0103-atoms3r-m12-native-quickjs
idf.py build
```

- Validate hardware with a tmux monitor session using:

```bash
PORT=/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_B4:3A:45:BE:16:80-if00
idf.py -p "$PORT" flash monitor
```

### Technical details

- ESP32-P4 by-id path to avoid: `/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00`.
- AtomS3R M12 by-id path to use: `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_B4:3A:45:BE:16:80-if00`.
- Firmware target: `0103-atoms3r-m12-native-quickjs`.
- Build artifact: `0103-atoms3r-m12-native-quickjs/build/0103-atoms3r-m12-native-quickjs.bin`.
- Initial binary size: `0xb4d00`.
- App partition: `4 MiB`.
- Free in app partition after first build: `0x34b300` / 82%.

## Step 2: Flash AtomS3R M12 and validate native QuickJS on hardware

The first hardware validation flashed `0103-atoms3r-m12-native-quickjs` to the AtomS3R M12 using the Espressif USB Serial/JTAG by-id path. The boot logs confirmed the expected target: `ESP32-S3-PICO-1`, embedded 8 MB flash, embedded 8 MB Octal PSRAM, USB Serial/JTAG mode, and the AtomS3R MAC address `b4:3a:45:be:16:80`. QuickJS initialized successfully through the reused `qjs_service` owner task.

The console smoke passed. `js status` reported a ready runtime with a 1 MiB memory cap, `print(1+2)` produced `3`, an exception produced `Error: boom`, a completion value produced `42`, `js reset` cleared the global environment, and `js bench` completed 10k loop, 100k loop, and `fib(20)` without timeout or stack failure.

### Prompt Context

**User prompt (verbatim):** "btw, when you get to that point, you can run tmux idf.py btw, and just reflash with Ctrl-A when done. that way you can read th eoutput easily and don't have to worry about crash log filling up your context window, etc..."

**Assistant interpretation:** Use tmux for the hardware flash/monitor phase so output can be captured to a file and the session can be reused for reflashing instead of dumping long monitor logs into the chat.

**Inferred user intent:** Keep serial validation controlled, readable, and recoverable while continuing the task-by-task implementation workflow.

**Commit (code):** 3bfd6cc8670cb9ae756e706dbc065467c56e4d04 — "0103: add AtomS3R M12 native QuickJS scaffold"

### What I did

- Confirmed the AtomS3R target path still resolved to `/dev/ttyACM1`:

```text
/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_B4:3A:45:BE:16:80-if00 -> /dev/ttyACM1
```

- Confirmed no process was holding the AtomS3R by-id serial path before flashing.
- Started `idf.py -p <AtomS3R-by-id> flash monitor` through tmux with output mirrored to:

```text
/tmp/0103-atoms3r-m12-native-quickjs-flash-monitor.log
```

- Found that the existing/default tmux server was in a mount namespace where this workspace appeared read-only. A simple `touch` inside that tmux server failed with:

```text
touch: cannot touch '.../0103-atoms3r-m12-native-quickjs/build/tmux-write-test': Read-only file system
```

- Started a fresh tmux server socket with `tmux -L qjs0103`, which inherited the current writable namespace. The same write test passed.
- Flashed and monitored from the fresh tmux server.
- Captured smoke output to:

```text
/tmp/0103-atoms3r-m12-native-quickjs-smoke.log
```

- Released the serial port after validation by killing the dedicated `tmux -L qjs0103` server.

### Why

- The project has two connected `/dev/ttyACM*` boards. Using the AtomS3R by-id path prevents accidentally flashing the ESP32-P4.
- tmux keeps monitor output inspectable without flooding the agent context.
- The fresh tmux socket was necessary because the already-running tmux server had a stale/read-only mount view of the workspace.

### What worked

- Flash succeeded through the AtomS3R by-id serial path.
- esptool identified the board correctly:

```text
Chip is ESP32-S3-PICO-1 (LGA56) (revision v0.2)
Features: WiFi, BLE, Embedded Flash 8MB (GD), Embedded PSRAM 8MB (AP_3v3)
USB mode: USB-Serial/JTAG
MAC: b4:3a:45:be:16:80
```

- PSRAM initialized successfully:

```text
I (177) esp_psram: Found 8MB PSRAM device
I (178) esp_psram: Speed: 80MHz
I (604) esp_psram: SPI SRAM memory test OK
I (616) esp_psram: Adding pool of 8192K of PSRAM memory to heap allocator
```

- QuickJS initialized successfully:

```text
I (628) qjs_service: task start name=qjs0103 prio=8 core=0
I (638) qjs_service: runtime init status=ESP_OK elapsed=9 ms
```

- Memory baselines were captured:

```text
before_qjs: psram_initialized=1 psram_size=8388608 internal_free=370735 8bit_free=8756923 psram_free=8386188
after_qjs:  psram_initialized=1 psram_size=8388608 internal_free=186331 8bit_free=8572519 psram_free=8386188
```

- `js status` baseline:

```text
ready=1 busy=0 evals=0 resets=0 last_eval_ms=0
limits: memory=1048576 stack=65536
quickjs: used=49760 malloc=360 atoms=518
esp_heap: internal=184715 8bit=8570439 psram=8385724
```

- Eval smoke passed:

```text
js eval "print(1+2)"
[atoms3r-eval] ok=1 timed_out=0 elapsed=2ms
3
```

- Exception smoke passed after correcting the host-side quote escaping:

```text
js eval "throw new Error('boom')"
[atoms3r-eval] ok=0 timed_out=0 elapsed=2ms
error: Error: boom
```

- Completion value smoke passed:

```text
js eval "let x=41; x+1"
[atoms3r-eval] ok=1 timed_out=0 elapsed=2ms
42
```

- Reset smoke passed:

```text
js reset
reset: ESP_OK
js eval "typeof x"
[atoms3r-eval] ok=1 timed_out=0 elapsed=1ms
undefined
```

- Benchmark smoke passed:

```text
[bench-10k] ok=1 timed_out=0 elapsed=25ms
sum10k=20,s=49995000
[bench-100k] ok=1 timed_out=0 elapsed=223ms
sum100k=217,s=4999950000
[bench-fib20] ok=1 timed_out=0 elapsed=60ms
fib20=6765,ms=55
```

### What didn't work

- The default existing tmux server could not write to the workspace and caused CMake/idf.py failures such as:

```text
file failed to open for writing (Read-only file system):
.../0103-atoms3r-m12-native-quickjs/build/local_components_list.temp.yml
```

- The first exception command I sent through shell/tmux escaping reached the device as `throw new Error(\x27boom\x27)`, which QuickJS reported as a syntax error. Sending the literal single-quoted JavaScript string fixed the test.

### What I learned

- Native QuickJS on AtomS3R M12 is viable with the 1 MiB cap. The idle QuickJS memory use is about 50 KiB, consistent with the ESP32-P4 baseline.
- The AtomS3R M12 PSRAM configuration is correct for the first milestone: Octal PSRAM at 80 MHz initialized and passed the ESP-IDF memory test.
- A dedicated tmux server socket is safer than reusing an old tmux server when the old server may have inherited a stale mount namespace.

### What was tricky to build

- The confusing part was that normal shell writes to the build directory worked, but writes from the existing tmux server failed. That meant the failure was not file permissions or an actual read-only root filesystem; it was the tmux server's inherited environment/mount view.
- The flash runbook should explicitly use `tmux -L qjs0103` or another fresh server socket if similar read-only errors appear.
- Serial single ownership remained important: the monitor was killed after the smoke to release `/dev/ttyACM1`.

### What warrants a second pair of eyes

- Review whether the 1 MiB QuickJS cap should remain the default after adding WiFi/storage, or whether it should become a Kconfig option.
- Review the internal heap drop from `before_qjs` to `after_qjs`; some of that is owner task stack, queue/semaphore allocation, and runtime initialization, but future WiFi/TLS work will need enough internal headroom.
- Review why `qjs_service` allocations did not visibly reduce `psram_free`; if we want QuickJS heap pressure to favor PSRAM, we may need a custom QuickJS allocator or ESP-IDF heap configuration changes later.

### What should be done in the future

- Run Phase 2 stress scripts against the 1 MiB cap.
- Measure after repeated eval/reset cycles.
- Decide whether to keep the 1 MiB cap, make it configurable, or test a 2 MiB cap.
- Only after memory characterization, add the first read-only `system` JavaScript namespace.

### Code review instructions

- Review hardware validation against `/tmp/0103-atoms3r-m12-native-quickjs-smoke.log`.
- Confirm `0103-atoms3r-m12-native-quickjs/main/app_main.cpp` logs enough memory information for future regressions.
- Confirm `0103-atoms3r-m12-native-quickjs/main/js_command.cpp` command behavior matches `0101` where intended.
- Re-run flash/monitor with:

```bash
PORT=/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_B4:3A:45:BE:16:80-if00
tmux -L qjs0103 new-session -d -s qjs0103 "bash -lc 'source /home/manuel/esp/esp-idf-5.4.2/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs && idf.py --no-hints -p $PORT flash monitor 2>&1 | tee /tmp/0103-atoms3r-m12-native-quickjs-flash-monitor.log'"
```

### Technical details

- Flash/monitor log: `/tmp/0103-atoms3r-m12-native-quickjs-flash-monitor.log`.
- Smoke log: `/tmp/0103-atoms3r-m12-native-quickjs-smoke.log`.
- AtomS3R by-id path: `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_B4:3A:45:BE:16:80-if00`.
- Firmware binary: `0xb4d00` bytes.
- QuickJS runtime init: `9 ms`.
- QuickJS memory limit: `1,048,576 bytes`.
- QuickJS status baseline: `used=49,760`, `atoms=518`.

## Step 3: Characterize the 1 MiB QuickJS memory cap

The second hardware pass kept the same flashed firmware and attached a fresh tmux monitor to run controlled memory-pressure scripts. This pass tested whether the initial 1 MiB QuickJS heap cap fails safely and whether the runtime remains usable after an allocation failure.

The result supports keeping the default cap at 1 MiB for the next milestone. A bounded 20,000-element numeric array completed successfully. A deliberately oversized allocation failed as a JavaScript-visible `InternalError: out of memory`, not as a firmware crash, watchdog reset, or serial disconnect. A later `js status` and `js reset` confirmed the service remained responsive.

### Prompt Context

**User prompt (verbatim):** (same tmux/hardware-validation guidance as Step 2)

**Assistant interpretation:** Continue the AtomS3R hardware validation through tmux, capture bounded stress evidence, and update the ticket before moving to feature work.

**Inferred user intent:** Validate runtime limits and stability before adding higher-level JavaScript APIs or storage/WiFi features.

**Commit (code):** 3bfd6cc8670cb9ae756e706dbc065467c56e4d04 — "0103: add AtomS3R M12 native QuickJS scaffold"

### What I did

- Attached a new monitor session through `tmux -L qjs0103`.
- Captured the run to:

```text
/tmp/0103-atoms3r-m12-native-quickjs-memory-stress.log
```

- Ran a clean reset and baseline status.
- Ran a bounded numeric-array stress script:

```text
js eval "(()=>{let a=[]; for(let i=0;i<20000;i++) a.push(i); print(\"array20k\",a.length);})()"
```

- Ran an oversized string-array stress script that was expected to hit the 1 MiB cap:

```text
js eval "try{let a=[]; for(let i=0;i<200000;i++) a.push(\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"); print(\"made\",a.length)}catch(e){print(\"caught\",e)}"
```

- Ran `js status`, `js reset`, and a final `js status` after the allocation failure.
- Released the serial port after the run.

### Why

- Phase 1 showed the runtime boots and evaluates simple programs. Phase 2 needed evidence that the memory cap is a safe operational boundary.
- The firmware should reject excessive JavaScript allocations without destabilizing the ESP32-S3 process.
- The cap decision affects future WiFi, TLS, storage, and JavaScript API work.

### What worked

- Baseline after reset:

```text
ready=1 busy=0 evals=0 resets=1 last_eval_ms=0
limits: memory=1048576 stack=65536
quickjs: used=49760 malloc=360 atoms=518
esp_heap: internal=184695 8bit=8570419 psram=8385724
```

- Bounded numeric array succeeded:

```text
[atoms3r-eval] ok=1 timed_out=0 elapsed=94ms
array20k 20000
```

- Status after the bounded script returned close to baseline:

```text
ready=1 busy=0 evals=1 resets=1 last_eval_ms=94
limits: memory=1048576 stack=65536
quickjs: used=49816 malloc=360 atoms=518
esp_heap: internal=184579 8bit=8570303 psram=8385724
```

- Oversized allocation failed cleanly inside QuickJS:

```text
[atoms3r-eval] ok=1 timed_out=0 elapsed=457ms
caught InternalError: out of memory
```

- Status after the out-of-memory condition showed the service remained ready:

```text
ready=1 busy=0 evals=2 resets=1 last_eval_ms=457
limits: memory=1048576 stack=65536
quickjs: used=49872 malloc=360 atoms=518
esp_heap: internal=184407 8bit=8570131 psram=8385724
```

- Reset restored the QuickJS baseline:

```text
reset: ESP_OK
ready=1 busy=0 evals=2 resets=2 last_eval_ms=457
limits: memory=1048576 stack=65536
quickjs: used=49760 malloc=360 atoms=518
esp_heap: internal=184379 8bit=8570103 psram=8385724
```

### What didn't work

- Attaching `idf.py monitor` reset the board through USB/JTAG control-line behavior. This was acceptable for the stress run but should be remembered when trying to preserve an in-memory JavaScript session across monitor attaches.
- Long console commands wrap visually in `esp_console`/monitor output. The command still executed correctly, but logs include wrapped/duplicated prompt fragments.

### What I learned

- The 1 MiB QuickJS cap is a usable first default on AtomS3R M12.
- Out-of-memory behavior is recoverable at the JavaScript/service level when the script catches the exception.
- The firmware does not need a cap increase before the first API namespace work.

### What was tricky to build

- The main trickiness was separating JavaScript memory pressure from ESP-IDF heap pressure. `js status` is the useful command because it reports both QuickJS allocator statistics and ESP heap baselines.
- The out-of-memory script had to catch the exception so the console command could return `ok=1` while still proving the cap was reached.
- Monitor attach reset behavior means each stress run should explicitly start from `js reset` and capture a fresh baseline.

### What warrants a second pair of eyes

- Review whether the `ok=1` result for a caught `InternalError: out of memory` is the desired console semantics. It is correct JavaScript semantics because the script catches the exception, but operators may want a dedicated stress command later.
- Review whether we should add a built-in `js stress` command for repeatable measurements instead of relying on long one-line eval strings.
- Review the small internal heap drift after eval/reset cycles to decide whether longer soak tests are needed.

### What should be done in the future

- Add repeated eval/reset soak testing if the firmware begins to run long-lived scripts.
- Keep `QJS_SERVICE_DEFAULT_MEMORY_LIMIT_BYTES` at 1 MiB until WiFi/TLS/storage are integrated and measured.
- Consider a Kconfig knob for the cap once multiple firmware roles need different JavaScript budgets.

### Code review instructions

- Review `/tmp/0103-atoms3r-m12-native-quickjs-memory-stress.log` for the exact command sequence and outputs.
- Compare baseline, post-array, post-OOM, and post-reset `js status` blocks.
- Confirm the task checklist marks Phase 2 as complete only because the cap decision is conservative: keep 1 MiB, do not raise to 2 MiB yet.

### Technical details

- Monitor log: `/tmp/0103-atoms3r-m12-native-quickjs-memory-stress.log`.
- Cap under test: `1,048,576 bytes`.
- Stack cap: `65,536 bytes`.
- Successful stress: 20,000 numeric array entries, `94 ms`.
- Clean failure stress: 200,000 string pushes, `InternalError: out of memory`, `457 ms`.
- Decision: keep 1 MiB default until WiFi/TLS/storage memory pressure is measured.

## Step 4: Add and validate the read-only `system` namespace

The first JavaScript-facing AtomS3R API is intentionally small: a read-only `system` metadata object. It exposes firmware identity, board identity, target, ticket, PSRAM size, flash size, and the QuickJS memory/stack caps. The namespace is implemented outside `qjs_service.cpp` so board-specific bindings do not crowd the reusable runtime service.

The namespace is installed through `qjs_service_run()`, which keeps all `JSContext*` mutation on the QuickJS owner task. It is installed once after service startup and again after `js reset`, because reset recreates the QuickJS runtime and context.

### Prompt Context

**User prompt (verbatim):** (continuation of the AtomS3R M12 native QuickJS ticket work; see Steps 2 and 3 for the tmux/hardware-validation guidance)

**Assistant interpretation:** Continue the phased AtomS3R firmware work by adding the first safe JavaScript API namespace after hardware and memory validation.

**Inferred user intent:** Move from raw eval smoke toward a reusable embedded JavaScript platform while preserving runtime ownership, bounded behavior, and clear documentation.

**Commit (code):** 690972ce701a493f23b35c843c1c84772edff12c — "0103: add read-only QuickJS system namespace"

### What I did

- Added `0103-atoms3r-m12-native-quickjs/main/system_namespace.h`.
- Added `0103-atoms3r-m12-native-quickjs/main/system_namespace.cpp`.
- Updated `0103-atoms3r-m12-native-quickjs/main/CMakeLists.txt` to compile the namespace installer and depend on `spi_flash`.
- Updated `0103-atoms3r-m12-native-quickjs/main/app_main.cpp` to install the namespace after `qjs_service_start()`.
- Updated `0103-atoms3r-m12-native-quickjs/main/js_command.cpp` to reinstall the namespace after `js reset`.
- Updated `0103-atoms3r-m12-native-quickjs/README.md` with the `system` contract and validated memory posture.
- Updated the design guide and task checklist.
- Built the firmware with:

```bash
source /home/manuel/esp/esp-idf-5.4.2/export.sh
cd 0103-atoms3r-m12-native-quickjs
idf.py --no-hints build
```

- Flashed and monitored through the AtomS3R by-id path in a fresh `tmux -L qjs0103` session.
- Captured initial validation output to:

```text
/tmp/0103-atoms3r-m12-native-quickjs-system-namespace.log
```

- Fixed a QuickJS value ownership edge in the final `system` global definition path after checking `JS_DefinePropertyValueStr()` ownership semantics.
- Rebuilt and reflashed the final build, then captured final validation output to:

```text
/tmp/0103-atoms3r-m12-native-quickjs-system-namespace-final.log
```

### Why

- The first namespace should be read-only and bounded because it exercises the native binding path without introducing blocking I/O or mutable shared state.
- Installing through `qjs_service_run()` preserves the existing owner-task invariant for QuickJS.
- Reinstalling after reset keeps `js reset` useful: it clears script globals while restoring firmware-provided APIs.

### What worked

- Build passed:

```text
0103-atoms3r-m12-native-quickjs.bin binary size 0xb5270 bytes. Smallest app partition is 0x400000 bytes. 0x34ad90 bytes (82%) free.
```

- Flash and boot passed on the AtomS3R by-id path.
- `system.board` returned the expected board name:

```text
js eval "system.board"
[atoms3r-eval] ok=1 timed_out=0 elapsed=1ms
AtomS3R M12
```

- `JSON.stringify(system)` returned the expected metadata:

```text
{"firmware":"0103-atoms3r-m12-native-quickjs","board":"AtomS3R M12","target":"esp32s3","ticket":"ATOMS3R-M12-NATIVE-QUICKJS","psramInitialized":true,"psramBytes":8388608,"flashBytes":8388608,"quickjsMemoryLimitBytes":1048576,"quickjsStackLimitBytes":65536}
```

- The object is non-extensible:

```text
js eval "Object.isExtensible(system)"
[atoms3r-eval] ok=1 timed_out=0 elapsed=2ms
false
```

- Strict writes fail as read-only property errors:

```text
js eval "(()=>{'use strict'; system.board='Other'})()"
[atoms3r-eval] ok=0 timed_out=0 elapsed=3ms
error: TypeError: 'board' is read-only
```

- The original value remains unchanged after the failed write:

```text
js eval "system.board"
[atoms3r-eval] ok=1 timed_out=0 elapsed=1ms
AtomS3R M12
```

- The namespace survives `js reset` because reset reinstalls it:

```text
js reset
reset: ESP_OK
js eval "system.board"
[atoms3r-eval] ok=1 timed_out=0 elapsed=1ms
AtomS3R M12
js eval "system.psramBytes"
[atoms3r-eval] ok=1 timed_out=0 elapsed=1ms
8388608
```

### What didn't work

- My first strict-write test was malformed by host-side shell escaping and reached QuickJS as `\x27use strict\x27`, which produced `SyntaxError: invalid number literal`. Re-sending the literal single-quoted JavaScript fixed the validation.
- `clang-format` is not installed in this shell, so the attempted formatting command failed with:

```text
/bin/bash: line 35: clang-format: command not found
```

The subsequent ESP-IDF build still passed.

### What I learned

- A 0103-local installer keeps board metadata out of the reusable `qjs_service` component while still enforcing the owner-task rule.
- Reinstall-after-reset is required for firmware APIs that live inside the QuickJS global object.
- A read-only object with `JS_DefinePropertyValueStr(..., JS_PROP_ENUMERABLE)` properties plus `JS_PreventExtensions()` gives the expected JavaScript behavior: enumerable metadata, no new properties, and strict write failures.

### What was tricky to build

- The main design choice was where to put board-specific bindings. Putting `system` into `qjs_service.cpp` would have made the reusable component know about AtomS3R. The better boundary is a board-local installer invoked through `qjs_service_run()`.
- Reset semantics were another sharp edge. `qjs_service_reset()` destroys and recreates the runtime, so startup-only installation would disappear after reset. The console reset path now reinstalls `system` immediately after a successful reset.
- Quote escaping in tmux-driven eval commands remains easy to get wrong; validation logs should distinguish malformed host commands from firmware behavior.

### What warrants a second pair of eyes

- Review `system_namespace.cpp` for QuickJS value ownership around `JS_DefinePropertyValueStr()` and `JS_FreeValue()`; one failure-path double-free risk was found and fixed before the final build/flash.
- Review whether the namespace should expose static metadata only, as implemented, or whether future heap/status access should be functions to avoid stale values.
- Review whether reset-reinstall should eventually be a reusable `qjs_service` post-reset hook if more firmware targets need namespace restoration.

### What should be done in the future

- Design WiFi and storage namespaces before implementing them.
- Keep blocking operations out of the QuickJS owner task.
- Consider adding a common binding installer pattern if multiple firmware targets start to define their own namespace sets.

### Code review instructions

- Start with `0103-atoms3r-m12-native-quickjs/main/system_namespace.cpp`.
- Check ownership and failure paths in the helper functions that define properties.
- Check the two call sites: startup in `app_main.cpp` and reset in `js_command.cpp`.
- Validate with:

```bash
source /home/manuel/esp/esp-idf-5.4.2/export.sh
cd 0103-atoms3r-m12-native-quickjs
idf.py --no-hints build
PORT=/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_B4:3A:45:BE:16:80-if00
idf.py --no-hints -p "$PORT" flash monitor
```

- Then run:

```text
js eval "system.board"
js eval "JSON.stringify(system)"
js eval "Object.isExtensible(system)"
js eval "(()=>{'use strict'; system.board='Other'})()"
js reset
js eval "system.board"
```

### Technical details

- Namespace installer file: `0103-atoms3r-m12-native-quickjs/main/system_namespace.cpp`.
- Header: `0103-atoms3r-m12-native-quickjs/main/system_namespace.h`.
- Initial validation log: `/tmp/0103-atoms3r-m12-native-quickjs-system-namespace.log`.
- Final validation log: `/tmp/0103-atoms3r-m12-native-quickjs-system-namespace-final.log`.
- Binary size after adding the namespace: `0xb5270` bytes.
- App partition free space: `0x34ad90` bytes, 82%.
