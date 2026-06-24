---
title: Phase 1 Device Bring-Up Post-Mortem (ESP32-P4)
doc_type: design
ticket: ESP32-P4-QUICKJS-WASM
topics:
  - esp32p4
  - quickjs
  - wasm
  - wamr
  - firmware
  - debugging
status: active
---

# Phase 1 Device Bring-Up Post-Mortem — QuickJS-WASM on the ESP32-P4

**Audience:** the intern continuing this work. This document assumes you have read the design guide (`design/01-...md`) and the Phase-0 host results. It records exactly where the device port stands at the end of the first device session, the two crashes found, and the precise next steps.

**Status:** Phase 0 (host) passes. Phase 1 firmware builds, flashes, boots, and WAMR initialises on the ESP32-P4. Two crashes were found during device bring-up. The first (a store fault writing to flash) is fixed and verified. The second (a `pthread_self` assertion in WAMR's thread setup) is diagnosed but not yet fixed. JavaScript has not yet been evaluated on the device.

## 1. Executive summary

The JS-in-WASM-in-WAMR stack runs correctly on the host. Porting it to the ESP32-P4 exposed two embedding problems that the host did not, because the host environment hides them. Both are now understood.

1. **Crash A — WAMR writes to the module buffer.** `wasm_runtime_load` was handed a pointer to the embedded `quickjs.wasm`, which lives in read-only flash. WAMR's loader writes into that buffer and faulted with a store-access exception. Fix: copy the blob into writable PSRAM before loading. Verified — the load then succeeds.
2. **Crash B — WAMR's thread tracking calls `pthread_self`.** `wasm_runtime_call_wasm` sets thread info on the exec environment, which calls `pthread_self`. The call is made from `app_main`, whose task is a FreeRTOS task, not a POSIX thread, so `pthread_self` asserts. Fix: run the WAMR calls from a pthread. Not yet applied.

Neither crash is in QuickJS. JavaScript evaluation has not yet been reached because Crash B fires at the `qjs_init` call, before the engine runs.

## 2. Fundamentals the intern needs

These are the facts that make the two crashes intelligible. Read them before debugging.

### 2.1 The stack, recapped

User JavaScript is evaluated by a QuickJS engine that is itself compiled to a WebAssembly module (`quickjs.wasm`). That module is executed by WAMR, which is embedded in ESP-IDF firmware. There are two host boundaries: WAMR↔wasm (native symbols registered under the import module `"env"`) and QuickJS↔user JS (C functions registered as JS globals inside the wasm). The firmware's job is to load `quickjs.wasm`, instantiate it, call `qjs_init`, and feed JavaScript source to `qjs_eval`.

### 2.2 ESP32-P4 memory map (what is writable)

- **Internal SRAM** (`0x4FF00000` region, ~768 KB): fast, executable, the IDF heap and task stacks live here. Small.
- **Flash** (mapped to `0x40000000`-region segment 0 in this build): read-only at runtime. `EMBED_FILES` places `quickjs.wasm` here. The bootloader log shows `segment 0: vaddr=40070020 size=14064ch`, so the wasm blob occupies `0x40070020`–`0x401b066c`.
- **PSRAM** (32 MB, mapped to the `0x49000000`-ish region, 200 MHz hex): large and writable. The WAMR pool and the writable copy of the wasm both live here.

The single rule: anything WAMR or QuickJS writes to must be in SRAM or PSRAM, never in the flash-mapped segment.

### 2.3 `EMBED_FILES` produces a read-only pointer

`EMBED_FILES "quickjs.wasm"` turns the blob into `_binary_quickjs_wasm_start`/`_end` symbols that point into flash-mapped `.rodata`. The pointer is valid for reading and for passing to `wasm_runtime_load` only if the loader treats it as read-only. WAMR's loader does not; it writes into the buffer it is given. This is the root of Crash A.

### 2.4 WAMR tracks the calling thread

When `wasm_runtime_call_wasm` is invoked, WAMR records which thread owns the execution environment (`wasm_exec_env_set_thread_info` → `os_self_thread`). On ESP-IDF this resolves to `pthread_self`. `pthread_self` is only valid on a task created through ESP-IDF's pthread layer; a plain FreeRTOS task (including the `main_task` that runs `app_main`) is not a pthread, and the call asserts. This is the root of Crash B. The host did not hit this because every Linux thread is a pthread.

### 2.5 Interpreted QuickJS is slow, but not the current blocker

QuickJS runs as interpreted wasm under WAMR. Context setup (`JS_NewContext`) is expensive on a 360 MHz core. This will matter for responsiveness later, but it is not the cause of either crash: Crash B fires at the `qjs_init` call boundary, before the engine executes.

## 3. What works on the device (verified logs)

Flashing and booting succeed. From the monitor:

```
I (600) esp_psram: Found 32MB PSRAM device
I (601) esp_psram: Speed: 200MHz
I (1566) cpu_start: cpu freq: 360000000 Hz
I (1567) app_init: Project name: 0100-esp32-p4-quickjs-wasm
I (1880) 0100_qjs: WAMR ready: pool=16777216 bytes, pool_external=yes
I (1880) 0100_host: registered 3 env native symbols
I (1960) 0100_run: copied quickjs.wasm (1231348 bytes) to writable buffer 0x49000aa8
```

So: 32 MB hex PSRAM at 200 MHz is detected; the CPU runs at 360 MHz; the 16 MB WAMR pool is allocated in PSRAM (`pool_external=yes`); the three `env` native symbols register; and the wasm blob is copied to a writable PSRAM buffer. The load then proceeds. None of the firmware's setup is the problem.

## 4. Crash A — store access fault in `b_memmove_s` (fixed)

Before the PSRAM copy was added, the firmware crashed during `wasm_runtime_load`:

```
Guru Meditation Error: Core 0 panic'ed (Store access fault). Exception was unhandled.
MEPC : 0x4fc19ff6   RA : 0x4001c51c   MCAUSE : 0x00000007   MTVAL : 0x400809b9
--- 0x4001c51c: b_memmove_s at .../espressif__wasm-micro-runtime/core/shared/utils/bh_common.c:116
A0 : 0x400809b9   A1 : 0x400809ba   A2 : 0x00000003
```

`MCAUSE 0x7` is a store access fault. `MTVAL 0x400809b9` is the faulting store address. That address lies inside segment 0 (`0x40070020`–`0x401b066c`), the flash-mapped region that holds `quickjs.wasm`. The register dump shows `b_memmove_s` copying three bytes with destination `0x400809b9` and source `0x400809ba` — an in-place shift inside the module buffer. WAMR's loader was rewriting the module buffer, which is read-only flash.

The host test did not fault because `host_test.c` read the wasm from disk with `malloc` (writable) before calling `wasm_runtime_load`. The device passed the flash pointer directly.

**Fix (applied in `main/wasm_runner.cpp`, verified):** copy the embedded blob into a PSRAM buffer and pass that to `wasm_runtime_load`.

```c
if (g_wasm_copy == nullptr) {
    size_t sz = quickjs_wasm_size();
    g_wasm_copy = (uint8_t *)heap_caps_malloc(sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!g_wasm_copy) g_wasm_copy = (uint8_t *)heap_caps_malloc(sz, MALLOC_CAP_8BIT);
    memcpy(g_wasm_copy, quickjs_wasm_data(), sz);
}
g_mod = wasm_runtime_load(g_wasm_copy, quickjs_wasm_size(), err, sizeof(err));
```

After this change the log shows `copied quickjs.wasm (1231348 bytes) to writable buffer 0x49000aa8` and the load completes. Crash A is closed.

## 5. Crash B — `pthread_self` assertion in `wasm_exec_env_set_thread_info` (open)

With the load fixed, the firmware proceeds to `qjs_init` (a `wasm_runtime_call_wasm` with zero arguments) and immediately asserts:

```
assert failed: pthread_self pthread...
#0  panic_abort  (details="assert failed: pthread_self pthre...")
   at esp_system/panic.c:483
#3  pthread_self ()
#4  os_self_thread ()                       (WAMR)
#5  wasm_exec_env_set_thread_info ()
#6  wasm_call_function (argc=0, argv=0x0)
#7  wasm_runtime_call_wasm ()
#8  wasm_runner_init ()                    ← our qjs_init call
#9  app_main ()
```

`wasm_runtime_call_wasm` calls `wasm_exec_env_set_thread_info`, which calls `os_self_thread`, which calls `pthread_self`. `pthread_self` asserts because the caller — `app_main` running on the IDF `main_task` — is a FreeRTOS task, not a POSIX thread.

This did not happen on the host because Linux threads are pthreads. It did not happen in project `0079` on the ESP32-S3 either, which means `0079` either calls `wasm_runtime_call_wasm` from a context that is a pthread, or configures WAMR differently. That difference is the thing to find.

### Fix directions, in order of likelihood

1. **Run WAMR calls from a pthread.** Create the WAMR session (`load`/`instantiate`/`qjs_init`) and the `qjs_eval` calls inside a task launched with ESP-IDF's pthread API (`pthread_create` with `esp_pthread_set_cfg`), not from `app_main`. Feed JavaScript to that pthread from the console command via a queue. This makes `pthread_self` valid.
2. **Lazy-init from the console task.** Do not call `wasm_runner_init` from `app_main`. Instead, initialise on the first `js eval`, which runs in the `esp_console` task. Verify whether that task is a pthread; if it is not, fall back to direction 1.
3. **Compare with `0079`.** Read `0079/main/wasm_module_runner.cpp` and `wasm_command.cpp` to see exactly which task invokes `wasm_runtime_call_wasm` and what `0079`'s `sdkconfig` WAMR options are. `0079` works on the S3, so its pattern is a proven reference; the goal is to find what makes its calling task a pthread (or what WAMR option skips `os_self_thread`).

The third direction is the highest-value first step, because it replaces guessing with a known-working reference.

## 6. Current state and what remains

- Phase 0 host smoke test: passes (`print(1+2)`→`3`, loops, exceptions).
- Phase 1 firmware: builds for `esp32p4`, flashes, boots, WAMR inits, natives register, wasm loads (after the Crash A fix).
- Crash A: fixed and verified on hardware.
- Crash B: diagnosed, fix not yet applied. This is the single blocker to running JavaScript on the device.
- Not yet reached: `qjs_init` completion, `js eval` on the device, `js status`, performance measurement.

## 7. Next steps (prioritized)

1. Resolve Crash B (Section 5). Start by reading `0079`'s runner and `sdkconfig`; then either run WAMR calls from a pthread or lazy-init from the console task.
2. Once `qjs_init` completes, run `js eval "print(1+2)"` on the device and confirm `3`. Then `js status` (check `wamr.heap_*` high-water).
3. Characterise `qjs_init` latency (interpreted QuickJS context setup on a 360 MHz core) and `js eval` latency. If context setup is too slow for a boot-time init, defer it to the first `js eval` (lazy).
4. Re-enable the deferred Phase 1/2 items: `js repl`, `js reset`, real `gpio_write`/`millis` wiring (the `host_gpio_write` native currently calls `gpio_set_level` without `gpio_config`), `js bench`.

## 8. How to resume tomorrow

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0100-esp32-p4-quickjs-wasm
source /home/manuel/esp/esp-idf-5.4.2/export.sh
# device is on /dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00 (-> /dev/ttyACM0)
idf.py build && idf.py -p /dev/ttyACM0 flash
# drive the monitor in tmux so you can send commands and capture output:
tmux new-session -d -s qjs0100 -c "$PWD" \
  "bash -lc 'source /home/manuel/esp/esp-idf-5.4.2/export.sh >/dev/null 2>&1; idf.py -p /dev/ttyACM0 monitor'"
sleep 6
tmux capture-pane -t qjs0100 -p | tail -40
# send a command:
tmux send-keys -t qjs0100 'js eval "print(1+2)"' Enter
# stop:
tmux kill-session -t qjs0100
```

Serial ownership: keep one monitor per port; kill the tmux session before flashing (the flash and the monitor cannot share `/dev/ttyACM0`). See `AGENTS.md` and `docs/01-playbook-esp-idf-build-and-dev-environment.md`.

## 9. Reproduce / review

- Firmware: `0100-esp32-p4-quickjs-wasm/main/{wasm_runner.cpp,wasm_host_api.cpp,wasm_runtime_service.cpp,js_command.cpp,app_main.cpp,quickjs_embed.h}`.
- Crash A fix: `wasm_runner.cpp` `wasm_runner_init` (the PSRAM copy before `wasm_runtime_load`).
- Crash B site: `wasm_runner.cpp` `wasm_runner_init`, the `wasm_runtime_call_wasm(env, finit, 0, nullptr)` `qjs_init` call.
- Diary: `reference/01-investigation-diary.md` Steps 9–11.
