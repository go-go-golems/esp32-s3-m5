---
Title: ""
Ticket: ""
Status: ""
Topics: []
DocType: ""
Intent: ""
Owners: []
RelatedFiles:
    - Path: 2026/06/11/M5STACKCHAN-BENCH--standalone-cores3-benchmark-harness-for-stackchan-firmware-performance/scripts/01-render-benchmark-charts.py
      Note: Chart renderer for benchmark article images
    - Path: build/firmware/main/bench/benchmark_main.cpp
      Note: Stack-safe benchmark implementation and first successful serial metrics
ExternalSources: []
Summary: ""
LastUpdated: 0001-01-01T00:00:00Z
WhatFor: ""
WhenToUse: ""
---



# Diary

## Goal

Create a new docmgr ticket and write an intern-ready analysis/design/implementation guide for a standalone CoreS3/M5StackChan benchmark firmware. The benchmark should measure the theoretical performance ceiling of the same device and core libraries without running the Mooncake app framework.

## Step 1: Ticket Setup, Source Evidence, and Benchmark Guide

I created a new benchmark ticket and wrote the primary guide. The guide explains why a standalone `app_main()` benchmark is different from a Mooncake benchmark app, which runtime paths should be measured, what metrics matter, and how an intern should implement the harness in phases.

### Prompt Context

**User prompt (verbatim):** "Create a new ticket to do that. Then Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable."

**Assistant interpretation:** Create a ticket for a standalone benchmark firmware design, gather source-backed evidence about the StackChan runtime, write a detailed intern-ready design/implementation guide with diagrams, pseudocode, API references, file references, then validate and upload it.

**Inferred user intent:** Preserve a clear engineering plan for measuring whether launcher animation choppiness is a hardware/display limit or a firmware/framework scheduling issue.

### What I did

- Created ticket `M5STACKCHAN-BENCH`.
- Added a design document: `design/01-standalone-benchmark-harness-analysis-design-and-implementation-guide.md`.
- Added this diary document.
- Added tasks for evidence gathering, guide writing, bookkeeping, validation, and reMarkable upload.
- Read source evidence from:
  - `main/main.cpp` — production HAL init and Mooncake loop.
  - `main/CMakeLists.txt` — source glob and build shape.
  - `main/hal/hal.cpp` — HAL init, timing, watchdog/yield, LVGL init.
  - `main/hal/hal.h` — public HAL APIs and `LvglLockGuard`.
  - `main/hal/board/stackchan_display.cc` — LVGL port task, buffer, lock/unlock.
  - `main/apps/app_launcher/app_launcher.cpp` — launcher hot path and LVGL lock scope.
  - `main/hal/hal_io_expander.cpp` — RGB LED direct HAL path.
  - `main/stackchan/stackchan.h` — `StackChan::update()` path.
  - `main/stackchan/addons/neon_light/neon_light.cpp` — neon animation update behavior.
  - `partitions.csv` — OTA/assets/coredump partition layout.

### Why

A Mooncake app benchmark would still include Mooncake scheduling and framework overhead. The requested benchmark is meant to establish a ceiling: what the same display, LVGL port, HAL, RGB LEDs, assets partition, and FreeRTOS scheduler can do when the production app framework and launcher are removed from the hot path.

### What worked

- The source evidence clearly identifies the launcher hot path: `AppLauncher::onLauncherRunning()` holds an LVGL lock while updating the launcher view and calling `GetStackChan().update()`.
- The HAL provides the required timing and hardware APIs: `millis()`, `lvglLock()`, `lvglUnlock()`, `setRgbColor()`, and `refreshRgb()`.
- The display setup exposes enough information to define meaningful benchmark questions: LVGL port task priority, task affinity, RGB565 format, DMA buffer size, and PSRAM image cache.

### What didn't work

- N/A for this documentation step. No code was built or flashed for this ticket.

### What I learned

- `Hal::feedTheDog()` is implemented as `vTaskDelay(1)`, so the production loop does yield once per iteration before calling `GetMooncake().update()`.
- The production launcher still holds the LVGL lock across `_view->update()`, `screensaver_update()`, and `GetStackChan().update()`.
- The LVGL port task priority is configured as `3` in `stackchan_display.cc`, with task affinity set on multi-core systems.
- The display buffer is `width * 20` pixels, RGB565, DMA-capable, and not PSRAM-backed.

### What was tricky to build

The main design distinction is subtle: "standalone benchmark" should not mean a clean-room ESP-IDF display demo. It should reuse the StackChan HAL so the benchmark remains representative of the actual firmware stack. At the same time, it should avoid Mooncake, launcher, avatar, and XiaoZhi so the measured numbers can act as a ceiling.

### What warrants a second pair of eyes

- Whether the first implementation should temporarily replace `main/main.cpp` or be Kconfig-selectable from the beginning.
- Whether `esp_lvgl_port` exposes a clean display flush hook for accurate FPS measurement without patching component internals.
- Whether calling full `GetHAL().init()` initializes too much for a ceiling benchmark, or whether that is the right realistic baseline.

### What should be done in the future

- Implement Phase 1 skeleton benchmark and verify it boots.
- Add LVGL lock wait/hold metrics.
- Add pacing-mode matrix and serial summaries.
- Compare standalone results to a Mooncake benchmark app and to production launcher instrumentation.

### Code review instructions

- Start with the design doc's "Current-state architecture and evidence" section.
- Cross-check the line-referenced claims in `main.cpp`, `hal.cpp`, `stackchan_display.cc`, and `app_launcher.cpp`.
- Validate the proposed benchmark sequence before writing code.

### Technical details

- Ticket: `M5STACKCHAN-BENCH`.
- Guide: `design/01-standalone-benchmark-harness-analysis-design-and-implementation-guide.md`.
- Benchmark core API: `esp_timer_get_time()`, `GetHAL().lvglLock()`, `GetHAL().lvglUnlock()`, `GetHAL().setRgbColor()`, `GetHAL().refreshRgb()`, `vTaskDelay()`.

## Step 2: First Standalone Benchmark Implementation Attempts and Crash Analysis

This step started the transition from design-only work into a real standalone benchmark firmware. The aim was to keep the StackChan hardware initialization path intact while replacing the production Mooncake launcher loop with a purpose-built benchmark `app_main()`. That would let us measure LVGL lock timing, loop pacing, RGB LED refresh cost, asset lookup latency, and heap pressure without the Mooncake app manager or launcher view logic in the hot path.

The first attempts did build and flash, but they also uncovered important runtime hazards before any useful benchmark summaries were produced. Those failures are valuable: they show that the benchmark itself must be extremely careful not to become the source of LVGL heap churn, log lock contention, or display-task starvation.

### Prompt Context

**User prompt (verbatim):** "go ahead"

**Assistant interpretation:** Implement the standalone benchmark described in the guide, build it, flash it to the connected StackChan, and begin collecting measurements.

**Inferred user intent:** Move from benchmark design to actual hardware measurements so we can quantify the theoretical performance ceiling and compare it with the choppy launcher behavior.

### What I did

- Added a Kconfig switch to `main/Kconfig.projbuild`:
  - `CONFIG_STACKCHAN_STANDALONE_BENCHMARK`
  - Purpose: select a benchmark entry point instead of the production Mooncake launcher entry point.
- Modified `main/CMakeLists.txt` so it chooses between:
  - production `main.cpp`
  - benchmark `bench/benchmark_main.cpp`
- Created the first benchmark implementation at:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/main/bench/benchmark_main.cpp`
- Enabled the benchmark via local config:
  - `sdkconfig.defaults.local`
  - content: `CONFIG_STACKCHAN_STANDALONE_BENCHMARK=y`
- Ran reconfigure and build through the existing `stackchan` tmux session.
- Flashed the benchmark firmware to `/dev/ttyACM0`.
- Started `idf.py monitor` and captured logs to `/tmp/stackchan-bench-monitor.log`, `/tmp/stackchan-bench-monitor2.log`, and later attempted `/tmp/stackchan-bench-monitor3.log`.

### Why

A Mooncake app would still include Mooncake scheduling and launcher behavior, so it would not answer the first-order question: what can the same hardware and HAL do when the framework is not in the way? The standalone benchmark is intended to reuse `GetHAL().init()`, the same LVGL port, the same display driver, the same RGB LED path, and the same assets partition, but bypass normal app installation and launcher updates.

### What worked

- The Kconfig option was accepted by ESP-IDF configuration.
- The CMake switch successfully selected `bench/benchmark_main.cpp` instead of `main.cpp`.
- The benchmark build completed successfully.
- The benchmark binary was significantly smaller than the production firmware:
  - first successful benchmark build: `stack-chan.bin binary size 0x2effd0 bytes`, `0x200030 bytes (41%) free`
  - later safer rebuild: `stack-chan.bin binary size 0x2effb0 bytes`, `0x200050 bytes (41%) free`
  - static-buffer rebuild: `stack-chan.bin binary size 0x2f0080 bytes`, `0x1fff80 bytes (41%) free`
- Flashing succeeded repeatedly:
  - `Hash of data verified.`
  - `Leaving...`
  - `Hard resetting via RTS pin...`
  - `Done`
- The benchmark booted far enough to print:
  - `BENCH_BOOT standalone benchmark starting`
  - `BENCH_START target=esp32s3 duration_ms=10000`
- The HAL initialization path succeeded up through display/LVGL setup before benchmark loop execution.

### What didn't work

#### Failure 1: Interrupt watchdog timeout on CPU0

The first benchmark version started with an aggressive `busy` mode and mutated LVGL on every loop iteration. The device repeatedly rebooted before any `BENCH_SUMMARY` line was emitted.

Observed monitor evidence from `/tmp/stackchan-bench-monitor.log`:

```text
I (691) BENCH: BENCH_BOOT standalone benchmark starting
I (2291) BENCH: BENCH_START target=esp32s3 duration_ms=10000
Guru Meditation Error: Core  0 panic'ed (Interrupt wdt timeout on CPU0).
```

Backtrace excerpt:

```text
0x420328d1: run_mode(BenchMode)
    at .../main/bench/benchmark_main.cpp:310
0x42033081: app_main
    at .../main/bench/benchmark_main.cpp:392
```

The backtrace also showed CPU1 in LVGL drawing code, for example:

```text
0x420d1f8a: lv_draw_add_task
    at .../managed_components/lvgl__lvgl/src/draw/lv_draw.c:106
```

Interpretation: the benchmark's first mode was too aggressive. It entered measurement immediately after UI creation, logged at mode start, and updated LVGL at a frequency high enough to contend with the LVGL render task. This turned the benchmark into a stress/crash generator instead of a stable measurement harness.

#### Failure 2: LVGL label text heap/free assertion

After removing the initial `busy` mode and throttling LVGL updates to every 100 ms, the benchmark still crashed. The new crash was different and more specific:

```text
assert failed: heap_caps_free heap_caps_base.c:80 (heap != NULL && "free() target pointer is outside heap areas")
```

Backtrace excerpt from `/tmp/stackchan-bench-monitor2.log`:

```text
0x40379f89: heap_caps_free
    at .../components/heap/heap_caps_base.c:80
0x4038dd59: free
    at .../components/newlib/src/heap.c:34
0x420daf09: lv_free_core
    at .../managed_components/lvgl__lvgl/src/stdlib/clib/lv_mem_core_clib.c:74
0x420db095: lv_free
    at .../managed_components/lvgl__lvgl/src/stdlib/lv_mem.c:136
0x420e340b: set_text_internal
    at .../managed_components/lvgl__lvgl/src/widgets/label/lv_label.c:1011
0x42032af0: run_mode(BenchMode)
    at .../main/bench/benchmark_main.cpp:202
0x420330a5: app_main
    at .../main/bench/benchmark_main.cpp:397
```

The failing call path was `lv_label_set_text()` inside the benchmark's LVGL update path. This indicates that repeatedly assigning dynamically copied label strings caused LVGL to free text memory that was not allocated from the expected heap region, or otherwise exposed a mismatch in label text ownership/allocator assumptions under this port/configuration.

Interpretation: the benchmark should not allocate/free label text in the hot path. It should use persistent buffers and `lv_label_set_text_static()` for frequently updated labels.

### What I learned

- The benchmark must be gentler than a normal UI demo. It is trying to measure scheduling and LVGL behavior, so if it mutates LVGL too often, it changes the system under test.
- Mode-start logging can itself appear in crash backtraces because ESP-IDF logging takes locks and may interact badly with watchdog/interrupt timing when the system is already under pressure.
- `lv_label_set_text()` is not a zero-cost operation. It can allocate and free internal label text. In this benchmark, that allocation/free path caused a fatal heap assertion.
- `lv_label_set_text_static()` is a better fit for benchmark hot paths because it avoids LVGL text allocation churn, as long as the backing buffer remains valid for the lifetime of the label.
- The correct first benchmark should probably start with only stable paced modes (`delay_1_tick`, `target_60_fps`, `target_30_fps`) and add `yield`/`busy` modes only after the stable path works.
- The benchmark can still reveal real findings even before summary metrics: the LVGL text-update path and overly aggressive main-loop pressure are themselves risks for smooth UI animation.

### What was tricky to build

The tricky part was that each failure looked at first like a problem with the device or a generic "benchmark crash," but the backtraces pointed to distinct mechanisms:

1. **Interrupt WDT timeout:** the benchmark entered a mode that starved the system or contended too aggressively with LVGL/display work.
2. **Heap assertion in `lv_label_set_text()`:** the benchmark's text update method caused dynamic memory churn/ownership trouble inside LVGL.

The first solution was to reduce how often the benchmark touches LVGL and remove the initial busy mode. The second solution was to remove dynamic label text allocation from the hot path by switching to persistent global character buffers and `lv_label_set_text_static()`.

### What warrants a second pair of eyes

- Whether `lv_label_set_text()` crashing implies a deeper LVGL allocator/configuration issue or just unsafe use in this benchmark context.
- Whether the benchmark should avoid `assets::get_image()` in the first stable run, because asset lookup may add unrelated SPIFFS/cache complexity.
- Whether `GetHAL().init()` starts background tasks (head touch, servo feedback, display task) that make "standalone" still less isolated than expected.
- Whether the benchmark should temporarily disable or defer specific HAL subsystems for a second, more synthetic display-only ceiling.

### What should be done in the future

- Continue from the static-buffer benchmark build that was just flashed.
- Monitor whether it produces `BENCH_SUMMARY` lines without crashing.
- If it still crashes, reduce scope further:
  1. No RGB sampling.
  2. No asset lookup.
  3. No label text mutation after setup.
  4. Only move a dot and collect lock wait/hold metrics.
- Once stable, reintroduce one workload at a time.
- Record a "minimum stable benchmark" baseline before measuring expensive peripherals.

### Code review instructions

Review the current benchmark implementation here:

```text
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/main/bench/benchmark_main.cpp
```

Focus on:

- static label buffers:
  - `g_title_text`
  - `g_mode_text`
  - `g_stats_text`
- `lv_label_set_text_static()` usage
- `measured_lvgl_update()` frequency
- mode order in `app_main()`
- whether RGB/asset sampling should be gated behind compile-time switches

Build/flash logs:

```text
/tmp/stackchan-bench-reconfigure.log
/tmp/stackchan-bench-build.log
/tmp/stackchan-bench-flash.log
/tmp/stackchan-bench-build2.log
/tmp/stackchan-bench-flash2.log
/tmp/stackchan-bench-build3.log
/tmp/stackchan-bench-flash3.log
```

Monitor logs:

```text
/tmp/stackchan-bench-monitor.log
/tmp/stackchan-bench-monitor2.log
```

The attempted third monitor command was interrupted before results were captured because we paused to write this diary.

### Technical details

Files modified for the benchmark implementation:

```text
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/main/Kconfig.projbuild
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/main/CMakeLists.txt
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/main/bench/benchmark_main.cpp
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/sdkconfig.defaults.local
```

Important exact changes:

- `main/Kconfig.projbuild` now has `CONFIG_STACKCHAN_STANDALONE_BENCHMARK`.
- `main/CMakeLists.txt` selects `bench/benchmark_main.cpp` when that config is enabled.
- `sdkconfig.defaults.local` currently contains:

```text
CONFIG_STACKCHAN_STANDALONE_BENCHMARK=y
```

Current benchmark state at pause:

- Static-buffer version has built successfully.
- Static-buffer version has flashed successfully.
- It has **not yet been monitored to a conclusion** because the user asked to pause and write this extended diary before continuing.

## Step 3: Pause and Write Extended Diary Before Continuing

The user paused the implementation flow and asked for a detailed diary before continuing. This step records that pause explicitly so the later writeup can distinguish implementation work from documentation/recovery work.

### Prompt Context

**User prompt (verbatim):** "before we continue. write a detailed extended diary and all the steps we went through so that we can later on write a detailed diary writeup of all our research. Then we can continue"

**Assistant interpretation:** Stop active benchmarking, do not continue running/monitoring yet, and preserve the implementation attempt history in enough detail for a future research diary or report.

**Inferred user intent:** Avoid losing context about the failed benchmark iterations, including what was changed, what crashed, and what we learned from each crash.

### What I did

- Stopped continuing the benchmark execution work.
- Wrote this extended diary entry documenting:
  - Kconfig integration.
  - CMake entry-point selection.
  - benchmark implementation shape.
  - build/flash outcomes.
  - interrupt watchdog crash.
  - LVGL label text heap assertion.
  - static-buffer fix.
  - current state before the next monitor run.

### Why

The benchmark effort has already generated non-obvious knowledge about the firmware runtime. If we continue without writing it down, the later report would likely miss the distinction between the first WDT failure, the later LVGL heap assertion, and the static-buffer mitigation.

### What worked

- The diary now captures the implementation history with exact error strings and log locations.
- The current state is explicit: static-buffer build flashed; monitoring not yet completed.

### What didn't work

- N/A. This was a documentation pause, not a code execution step.

### What I learned

- The most valuable diary entries are written immediately after the failure, before the working memory of line numbers and error messages is lost.

### What was tricky to build

The main challenge was separating the implementation chronology from the crash analysis. The failures were close together in time but technically different, and the diary needs to preserve them as separate learning points.

### What warrants a second pair of eyes

- Confirm the interpretations of the WDT and heap assertion after reviewing the full logs.
- Confirm whether the static-buffer fix is sufficient or whether it only masks a deeper LVGL text allocator issue.

### What should be done in the future

- Resume from the static-buffer flashed benchmark.
- Run monitor and check for `BENCH_SUMMARY` lines.
- If stable, mark build/flash/capture tasks accordingly.
- If unstable, simplify the benchmark further and document the next failure mode.

### Code review instructions

- Read Step 2 first for implementation and crash details.
- Then inspect `benchmark_main.cpp` before running the next monitor session.
- Do not reintroduce busy mode until the paced modes produce stable summaries.

### Technical details

This diary was updated before any further benchmark execution after the third flash.

## Step 4: Stack-Safe Metrics Storage and First Successful Hardware Benchmark Run

After resuming the benchmark, the static-label version still rebooted before any summary lines. The new failure was different from the earlier LVGL label heap assertion: FreeRTOS reported a stack overflow in the `main` task. This made the next root cause clear: the benchmark was putting too much metric storage on the ESP-IDF `main` task stack.

The fix was to move benchmark metric storage to static/global memory, reduce the sample reservoir from 2048 to 512 values per metric, and remove a large percentile-copy operation from `MetricRecorder::summarize()`. With those changes, the benchmark completed a full four-mode cycle and printed `BENCH_SUMMARY` lines for `delay_1_tick`, `target_60_fps`, `target_30_fps`, and `yield` without crashes.

### Prompt Context

**User prompt (verbatim):** "continue"

**Assistant interpretation:** Resume the paused benchmark work from the already-flashed static-buffer version, diagnose any remaining failures, stabilize the harness, and capture first measurements.

**Inferred user intent:** Get the Phase 1 standalone benchmark to a working state and preserve concrete serial metrics for later performance analysis.

### What I did

- Inspected `/tmp/stackchan-bench-monitor3.log` after the static-label version kept rebooting.
- Found repeated benchmark starts with no summaries:
  - `BENCH_BOOT standalone benchmark starting`
  - `BENCH_START target=esp32s3 duration_ms=10000`
- Found the new primary failure:

```text
***ERROR*** A stack overflow in task main has been detected.
```

- Also saw secondary FreeRTOS assertions, likely caused by stack corruption after the overflow:

```text
assert failed: xTaskRemoveFromEventList tasks.c:3894 (pxUnblockedTCB)
```

- Identified the benchmark bug:
  - `BenchMetrics metrics;` was local inside `run_mode()`.
  - `BenchMetrics` contains four `MetricRecorder` objects.
  - Each `MetricRecorder` contained `std::array<uint32_t, 2048>`.
  - That means roughly 32 KiB of sample arrays were allocated on the ESP-IDF `main` task stack, before considering other locals/call frames.
- Fixed the benchmark implementation:
  - reduced `SAMPLE_CAP` from `2048` to `512`;
  - changed `MetricRecorder::summarize()` so it sorts the stored sample buffer in place instead of doing `auto copy = _samples`, which also copied a large array onto the stack;
  - added static/global `BenchMetrics g_metrics`;
  - changed `run_mode()` to use `BenchMetrics& metrics = g_metrics;` instead of a local stack object;
  - changed `update_summary_screen()` to accept non-const `BenchMetrics&` because `summarize()` now sorts in place.
- Rebuilt successfully:
  - log: `/tmp/stackchan-bench-build4.log`
  - binary: `stack-chan.bin binary size 0x2f1f40 bytes`
  - app partition free: `0x1fe0c0 bytes (40%) free`
- Flashed successfully:
  - log: `/tmp/stackchan-bench-flash4.log`
  - `Hash of data verified.`
  - `Hard resetting via RTS pin...`
  - `Done`
- Captured a bounded tmux monitor run:
  - log: `/tmp/stackchan-bench-monitor4.log`
- Verified there were no crash/error lines during the successful run.

### Why

The goal was to get a stable Phase 1 baseline. The benchmark is only useful if it can run longer than a single 10-second mode without corrupting the runtime. Moving sample storage out of the task stack keeps the measurement machinery from destabilizing the system under test.

### What worked

The stack-safe build completed one full benchmark cycle and emitted four summaries:

```text
BENCH_SUMMARY mode=delay_1_tick duration_ms=10009 loop_count=974 loop_hz=97 heap_internal_free=209727 heap_internal_min=209015 psram_free=8059436 lvgl_wait_count=100 lvgl_wait_min_us=27 lvgl_wait_avg_us=1917 lvgl_wait_p95_us=17729 lvgl_wait_max_us=29682 lvgl_hold_count=100 lvgl_hold_min_us=753 lvgl_hold_avg_us=814 lvgl_hold_p95_us=827 lvgl_hold_max_us=1097 rgb_count=51 rgb_min_us=6170 rgb_avg_us=6912 rgb_p95_us=6749 rgb_max_us=15988 asset_count=11 asset_min_us=50 asset_avg_us=12660 asset_p95_us=138756 asset_max_us=138756
BENCH_SUMMARY mode=target_60_fps duration_ms=10009 loop_count=495 loop_hz=49 heap_internal_free=209515 heap_internal_min=209015 psram_free=8059436 lvgl_wait_count=100 lvgl_wait_min_us=25 lvgl_wait_avg_us=30 lvgl_wait_p95_us=33 lvgl_wait_max_us=42 lvgl_hold_count=100 lvgl_hold_min_us=755 lvgl_hold_avg_us=810 lvgl_hold_p95_us=830 lvgl_hold_max_us=945 rgb_count=50 rgb_min_us=6096 rgb_avg_us=6460 rgb_p95_us=6513 rgb_max_us=6530 asset_count=10 asset_min_us=53 asset_avg_us=58 asset_p95_us=82 asset_max_us=82
BENCH_SUMMARY mode=target_30_fps duration_ms=10029 loop_count=251 loop_hz=25 heap_internal_free=209727 heap_internal_min=209015 psram_free=8059436 lvgl_wait_count=84 lvgl_wait_min_us=25 lvgl_wait_avg_us=279 lvgl_wait_p95_us=31 lvgl_wait_max_us=7114 lvgl_hold_count=84 lvgl_hold_min_us=767 lvgl_hold_avg_us=805 lvgl_hold_p95_us=831 lvgl_hold_max_us=884 rgb_count=50 rgb_min_us=6048 rgb_avg_us=6162 rgb_p95_us=6525 rgb_max_us=6525 asset_count=10 asset_min_us=53 asset_avg_us=62 asset_p95_us=66 asset_max_us=66
BENCH_SUMMARY mode=yield duration_ms=10025 loop_count=1304294 loop_hz=130104 heap_internal_free=209727 heap_internal_min=209015 psram_free=8059436 lvgl_wait_count=101 lvgl_wait_min_us=26 lvgl_wait_avg_us=28 lvgl_wait_p95_us=30 lvgl_wait_max_us=34 lvgl_hold_count=101 lvgl_hold_min_us=749 lvgl_hold_avg_us=813 lvgl_hold_p95_us=834 lvgl_hold_max_us=839 rgb_count=51 rgb_min_us=6092 rgb_avg_us=6429 rgb_p95_us=6492 rgb_max_us=6495 asset_count=11 asset_min_us=52 asset_avg_us=56 asset_p95_us=103 asset_max_us=103
```

Key first-run observations:

- Stable internal heap after initialization was about 209 KiB free.
- Minimum internal heap stayed at 209,015 bytes during the run.
- PSRAM free stayed around 8,059,436 bytes.
- LVGL lock hold time was consistently around 0.8 ms for the simple label/dot update.
- Direct RGB refresh cost was around 6.1-6.9 ms on average for all 12 LEDs.
- Asset lookup was normally very cheap after cache/warmup, but the first mode included a 138 ms outlier, likely first-use assets partition/checksum/cache work.
- `target_60_fps` only achieved about 49 loop Hz because the pacing loop uses `vTaskDelay(1)` and the benchmark workload includes RGB refresh and periodic UI work.
- `yield` mode achieved a very high loop count, but because the expensive work is throttled, this mostly measures how fast the outer loop can spin/yield between periodic workloads.

### What didn't work

The static-label version still crashed before this fix. Exact failure:

```text
***ERROR*** A stack overflow in task main has been detected.
```

Representative backtrace:

```text
Backtrace: 0x40386035:0x3fcad750 0x40385ffd:0x3fcad770 0x40386f66:0x3fcad790 0x403884e2:0x3fcad810 0x4038706c:0x3fcad830 0x40387062:0x00000000 |<-CORRUPTED
--- 0x40386f66: vApplicationStackOverflowHook
    at /home/manuel/esp/esp-idf-5.5.4/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:563
```

The corrupted backtrace was expected once the task stack had already overflowed. The important clue was the task name: `main`.

### What I learned

- On ESP-IDF, large C++ objects in `app_main()` / callees can silently exceed the `main` task stack even when the code looks harmless on a desktop.
- Percentile calculations are a hidden stack risk if they copy fixed-size arrays locally.
- For firmware benchmarks, storage for sample reservoirs should be static/global, heap-allocated with explicit checks, or drastically smaller.
- Once benchmark overhead is controlled, the StackChan HAL/LVGL/display path can run a simple benchmark loop stably.

### What was tricky to build

The tricky part was that the benchmark had three consecutive failure modes, each one revealed only after fixing the previous one:

1. Aggressive busy/LVGL pressure caused watchdog resets.
2. Dynamic LVGL label text updates caused a heap/free assertion.
3. The metric reservoir caused `main` task stack overflow.

The third issue was especially easy to miss because the code used ordinary C++ value objects. On desktop, four arrays of 2048 samples would not be remarkable. On the ESP-IDF `main` task stack, it is fatal.

### What warrants a second pair of eyes

- The current percentile method sorts the sample buffer in place. That is acceptable for an approximate benchmark reservoir, but a reviewer should confirm we do not need insertion-order preservation.
- `target_60_fps` achieving 49 Hz may be a pacing artifact from `vTaskDelay(1)`, not a hardware ceiling.
- The first asset lookup outlier should be separated from warm-cache asset measurements in future runs.
- The benchmark still includes HAL background subsystems such as MCP/head touch/servo initialization; this is realistic but not minimal.

### What should be done in the future

- Add an optional CSV/NDJSON summary output mode for easier comparison across repeated runs.
- Add a display-only mode with RGB and asset lookup disabled.
- Add a warmup phase before recording asset lookup metrics.
- Add a production launcher instrumentation pass to compare against this standalone baseline.
- Consider a custom benchmark task with explicit stack size if future benchmark code needs larger local working sets.

### Code review instructions

Start with:

```text
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/main/bench/benchmark_main.cpp
```

Review:

- `SAMPLE_CAP = 512`
- `MetricRecorder::summarize()` sorting in place
- global `BenchMetrics g_metrics`
- `run_mode()` using `BenchMetrics& metrics = g_metrics`
- `lv_label_set_text_static()` usage
- throttled UI/RGB/asset update periods

Validation commands/logs:

```text
/tmp/stackchan-bench-build4.log
/tmp/stackchan-bench-flash4.log
/tmp/stackchan-bench-monitor4.log
```

Successful validation evidence:

```text
I (48053) BENCH: BENCH_CYCLE_DONE repeating in 3s
```

### Technical details

Current successful benchmark source changes are in:

```text
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/main/bench/benchmark_main.cpp
```

Current local benchmark enablement is still:

```text
CONFIG_STACKCHAN_STANDALONE_BENCHMARK=y
```

in:

```text
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5StackChan/build/firmware/sdkconfig.defaults.local
```
