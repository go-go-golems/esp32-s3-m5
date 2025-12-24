---
Title: Diary
Ticket: 002-M5-ESPS32-TUTORIAL
Status: active
Topics:
    - esp32
    - m5stack
    - freertos
    - esp-idf
    - tutorial
    - grove
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-21T08:58:33.012710629-05:00
WhatFor: "Step-by-step implementation diary for creating an ESP32-S3 Cardputer tutorial project (ESP-IDF 5.4.1) and testing M5Cardputer-UserDemo compatibility with IDF 5.4.1."
WhenToUse: "Use while implementing or reviewing work under ticket 002; contains commands, failures, and decisions in chronological order."
---

# Diary

## Goal

Capture the exact steps (commands, outcomes, failures) taken to:

- Create a new **Cardputer tutorial project** under `esp32-s3-m5/0XXX`, starting from scratch with `idf.py`, and make it compile with **ESP-IDF 5.4.1**.
- Attempt to build **`M5Cardputer-UserDemo`** with **ESP-IDF 5.4.1** (even though it historically uses **ESP-IDF 4.4.6**), and document any compatibility issues.

## Step 1: Setup + confirm ESP-IDF installations (4.4.6 and 5.4.1)

This step verified that the workspace has the expected ESP-IDF installations available, so we can run a clean build attempt for both the new tutorial project and `M5Cardputer-UserDemo` under ESP-IDF 5.4.1.

**Commit (code):** N/A — environment check only

### What I did
- Created this diary document under ticket `002-M5-ESPS32-TUTORIAL`.
- Verified ESP-IDF installations exist at:
  - `/home/manuel/esp/esp-idf-4.4.6`
  - `/home/manuel/esp/esp-idf-5.4.1`

### Why
- We need ESP-IDF 5.4.1 to build the new tutorial project and to test whether `M5Cardputer-UserDemo` can be built on a newer ESP-IDF than the repo currently uses.

### What worked
- Both ESP-IDF trees are present locally (confirmed by directory listing).

### What didn't work
- N/A

### What I learned
- This workspace is set up for side-by-side ESP-IDF version testing, so we can do an A/B comparison without installing anything new.

### What was tricky to build
- N/A (no build yet)

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- N/A

### Code review instructions
- N/A

### Technical details
- Commands run (verbatim):

```bash
ls -la /home/manuel/esp && ls -la /home/manuel/esp/esp-idf-5.4.1 && ls -la /home/manuel/esp/esp-idf-4.4.6
```

## Related
- `../playbook/01-research-guide-esp32-s3-m5stack-development-tutorial.md`: Research guide and example-project spec for this effort.
- `../sources/research-results-tutorial-1.md`: Intern research results to incorporate into tutorial prose.

## Step 2: Create a fresh Cardputer tutorial project and compile it (ESP-IDF 5.4.1)

This step created a new project under `esp32-s3-m5/0001-cardputer-hello_world` by copying the upstream ESP-IDF `hello_world` example and building it using ESP-IDF 5.4.1 for the `esp32s3` target. The goal here is a **known-good baseline** project that compiles immediately, before we start adding Cardputer/M5-specific components.

**Commit (code):** N/A — new project created, no modifications yet

### What I did
- Created `esp32-s3-m5/` and copied the ESP-IDF 5.4.1 example `examples/get-started/hello_world` into `esp32-s3-m5/0001-cardputer-hello_world`.
- Set the target to `esp32s3`.
- Built the project.

### Why
- We want a tutorial project that starts from a clean, upstream ESP-IDF baseline so it’s easy to reproduce and debug.
- A successful `hello_world` compile proves: toolchain + CMake + Python env + IDF install are all correct.

### What worked
- `idf.py build` completed successfully and produced `build/hello_world.bin`.

### What didn't work
- N/A

### What I learned
- ESP-IDF 5.4.1 is functional in this environment and can build an ESP32-S3 target project end-to-end.

### What was tricky to build
- N/A (baseline example)

### What warrants a second pair of eyes
- Confirm the tutorial folder naming (`0001-cardputer-hello_world`) matches your preferred numbering scheme for the `esp32-s3-m5/0XXX` series.

### What should be done in the future
- Add a Cardputer-specific “Hello” variant (likely C++ + M5Unified/M5GFX), but only after we confirm the minimal baseline is stable.

### Code review instructions
- Start at `esp32-s3-m5/0001-cardputer-hello_world/` and run the build commands below.

### Technical details
- Commands run (verbatim):

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation && source /home/manuel/esp/esp-idf-5.4.1/export.sh && install -d /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5 && cp -a /home/manuel/esp/esp-idf-5.4.1/examples/get-started/hello_world /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0001-cardputer-hello_world && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0001-cardputer-hello_world && idf.py set-target esp32s3 && idf.py build
```

- Key build result lines:
  - `Project build complete.`
  - `Generated .../build/hello_world.bin`

## Step 3: Double-check Cardputer `sdkconfig` assumptions and align tutorial defaults

This step validated the “sdkconfig facts” we’re relying on for the Cardputer tutorial against the **actual** Cardputer project configuration. The key discovery is that Cardputer’s historical config (ESP-IDF v4.4.6) uses **8MB flash + custom partition table + 160 MHz CPU**, while a freshly generated ESP-IDF v5.4.1 `sdkconfig` may default to **2MB flash + single-app partitions** if you regenerate it (e.g., via `idf.py set-target`). Based on that, we updated our tutorial baseline project to include `sdkconfig.defaults` and a Cardputer-style `partitions.csv`, and rebuilt to confirm everything still compiles.

**Commit (code):** N/A — documentation + config defaults + rebuild validation

### What I did
- Extracted the relevant settings from `M5Cardputer-UserDemo/sdkconfig.old` (the pre-IDF-5 regeneration baseline).
- Added to the tutorial baseline project:
  - `esp32-s3-m5/0001-cardputer-hello_world/sdkconfig.defaults`
  - `esp32-s3-m5/0001-cardputer-hello_world/partitions.csv`
  - Updated `esp32-s3-m5/0001-cardputer-hello_world/README.md` with the “why” and implications.
- Rebuilt tutorial 0001 from scratch using `sdkconfig.defaults` (deleted generated `sdkconfig` and `build/` before rebuild).
- Corrected the “CPU frequency” section in the config comparison doc used as reference input.

### Why
- The tutorial must not accidentally teach “2MB flash + default partitions” when Cardputer needs a larger flash layout (4M app + 1M storage) and historically uses 8MB flash config.
- We want `sdkconfig.defaults` to be the tutorial’s source of truth (reproducible build intent from the analysis).

### What worked
- Tutorial 0001 still builds cleanly under ESP-IDF 5.4.1 with:
  - `--flash_size 8MB`
  - a 4MB factory app partition (from `partitions.csv`)

### What didn't work
- N/A (this step was a correction/realignment)

### What I learned
- **Cardputer baseline config (ESP-IDF 4.4.6) key facts**:
  - Flash size: `CONFIG_ESPTOOLPY_FLASHSIZE="8MB"`
  - Partition table: `CONFIG_PARTITION_TABLE_CUSTOM=y` with `partitions.csv`
  - CPU frequency: `CONFIG_ESP32S3_DEFAULT_CPU_FREQ_MHZ=160` (note: IDF 4 uses `ESP32S3_*` prefix)
  - Main task stack: `CONFIG_ESP_MAIN_TASK_STACK_SIZE=8000`
  - PSRAM: not enabled (`# CONFIG_SPIRAM is not set`)
- **IDF 5 vs IDF 4 naming**: IDF 5 uses `CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_*` while IDF 4 used `CONFIG_ESP32S3_DEFAULT_CPU_FREQ_*`.

### What was tricky to build
- It’s easy to accidentally “invalidate” the baseline `sdkconfig` by regenerating it under a different ESP-IDF version. We should treat the *original* config as authoritative when doing comparisons, and prefer `sdkconfig.defaults` for tutorial reproducibility.

### What warrants a second pair of eyes
- Confirm that “8MB flash” is correct for the Cardputer variant we care about (it matches the original repo config, but hardware variants exist).
- Confirm the partition layout (`4M` app + `1M` storage) matches the tutorial’s intended use cases (it’s Cardputer-like, but we may later decide to add OTA partitions).

### What should be done in the future
- Add a short “Config drift” note to the tutorial: `sdkconfig` is generated; treat `sdkconfig.defaults` as the contract.

### Code review instructions
- Review:
  - `esp32-s3-m5/0001-cardputer-hello_world/sdkconfig.defaults`
  - `esp32-s3-m5/0001-cardputer-hello_world/partitions.csv`
  - `esp32-s3-m5/0001-cardputer-hello_world/README.md`
- Rebuild:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0001-cardputer-hello_world && rm -f sdkconfig && rm -rf build && idf.py set-target esp32s3 && idf.py build
```

### Technical details
- Cardputer baseline settings source:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Cardputer-UserDemo/sdkconfig.old`
- Tutorial baseline build output confirms `--flash_size 8MB` and `Smallest app partition is 0x400000 bytes` after the change.

## Step 4: Try building `M5Cardputer-UserDemo` on ESP-IDF 5.4.1 (TinyUSB component naming)

This step starts the actual compatibility test: can `M5Cardputer-UserDemo` build under ESP-IDF 5.4.1? The first failure was during CMake configure: the project’s `main/CMakeLists.txt` hard-coded a TinyUSB component name that only exists in the ESP-IDF 4.4.x dependency branch. We patched `main/CMakeLists.txt` to select the correct TinyUSB component for ESP-IDF 5.x and re-ran the build (now running in the background with logs captured to a file to avoid tool timeouts).

**Commit (code):** N/A — local build + one compatibility patch

### What I did
- Attempted to build `M5Cardputer-UserDemo` using ESP-IDF 5.4.1.
- Observed CMake failure: missing component `leeebo__tinyusb_src`.
- Patched `M5Cardputer-UserDemo/main/CMakeLists.txt`:
  - If `IDF_VERSION_MAJOR >= 5`, link TinyUSB via `espressif__tinyusb`
  - Else, keep using `leeebo__tinyusb_src`
- Restarted the build under ESP-IDF 5.4.1 with output redirected to a log file.

### Why
- `M5Cardputer-UserDemo/main/idf_component.yml` intentionally switches dependencies:
  - IDF >= 5: use `espressif/tinyusb` (component name becomes `espressif__tinyusb`)
  - IDF < 5: use `leeebo/tinyusb_src` (component name becomes `leeebo__tinyusb_src`)
- The CMake logic needed to match that dependency switch.

### What worked
- The re-run build reaches Ninja compilation and resolves `espressif__tinyusb` (no longer fails during CMake configure).

### What didn't work
- Initial attempt failed with:

```text
Failed to resolve component 'leeebo__tinyusb_src'
... component 'leeebo__tinyusb_src' could not be found ...
```

### What I learned
- When using ESP-IDF component manager, the CMake-visible component name is `<namespace>__<component>` (double-underscore), so dependency switches across IDF versions must be reflected in any `idf_component_get_property(... COMPONENT_LIB)` calls.

### What was tricky to build
- The failure looks like a “dependency resolution” problem, but the dependency was actually present (`espressif__tinyusb`) — the CMake file was referencing the wrong component name.

### What warrants a second pair of eyes
- Confirm `target_link_libraries(${tusb_lib} PRIVATE ${COMPONENT_LIB})` is the correct intent here for IDF 5.x TinyUSB. (It is what the project did before; we preserved behavior and only fixed selection.)

### What should be done in the future
- If the IDF 5 build later fails on binary size checks, we’ll need to ensure Cardputer’s `partitions.csv` (4M app) is actually selected under IDF 5 as well (likely via `sdkconfig.defaults`). N/A for now until we see the build outcome.

### Code review instructions
- Start at `M5Cardputer-UserDemo/main/CMakeLists.txt` and verify the `IDF_VERSION_MAJOR` conditional.
- Re-run build (IDF 5.4.1):

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Cardputer-UserDemo && rm -rf build && idf.py build
```

### Technical details
- Build command (first attempt, failed at CMake configure):

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Cardputer-UserDemo && rm -rf build && idf.py --version && idf.py set-target esp32s3 && idf.py build
```

- Current re-run build is logging to:
  - `/home/manuel/.cursor/projects/home-manuel-workspaces-2025-12-21-echo-base-documentation/agent-tools/m5cardputer-idf5.4.1-build.log`

## Step 5: Second IDF 5.4.1 build attempt still fails (PikaPython type mismatch)

This step tried to unblock the ESP-IDF 5.4.1 build by making the previously-fatal fmt/spdlog warning non-fatal for the `main` component. That helped: the build proceeds further, and `-Wdangling-reference` is downgraded to a warning. However, the build still fails later in the **PikaPython** sources with incompatible pointer-type errors (treated as errors).

**Commit (code):** N/A — one additional build-system change + build attempt

### What I did
- Added `-Wno-error=dangling-reference` to the `main` component compile options.
- Rebuilt under ESP-IDF 5.4.1 and captured logs.
- Located the actual failing errors in the build stdout log.

### Why
- ESP-IDF 5.4.1 uses a newer GCC toolchain (GCC 13+; our environment shows GCC 14) which triggers additional diagnostics in vendored headers/libraries.

### What worked
- The original `-Werror=dangling-reference` blocker from fmt/spdlog is no longer fatal for translation units compiled under the `main` component.

### What didn't work
- Build still fails due to PikaPython C compile errors:

```text
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Cardputer-UserDemo/main/apps/app_repl/pikapython/pikascript-core/PikaVM.c:1790:58: error: passing argument 4 of 'byteCodeFrame_findInsForward' from incompatible pointer type [-Wincompatible-pointer-types]
/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Cardputer-UserDemo/main/apps/app_repl/pikapython/pikascript-core/PikaVM.c:1792:67: error: passing argument 4 of 'byteCodeFrame_findInsUnitBackward' from incompatible pointer type [-Wincompatible-pointer-types]
```

### What I learned
- Even after addressing the “new GCC warnings” problem, there are still **real API/type mismatches** in `pikapython` code that block an ESP-IDF 5.x + newer-GCC toolchain build.

### What was tricky to build
- The build output tail can look like it “failed on warnings” (spdlog/fmt), but the actual hard failure is later and unrelated (PikaPython).

### What warrants a second pair of eyes
- Confirm whether we should:
  - patch PikaPython in-tree for GCC 13+/IDF5 compatibility, or
  - treat PikaPython as optional and gate it behind a config flag so the rest of the firmware builds under IDF5.

### What should be done in the future
- Ask the intern to research known PikaPython build issues with newer GCC / ESP-IDF 5.x and report the minimal patch.

### Code review instructions
- Reproduce with ESP-IDF 5.4.1:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Cardputer-UserDemo && rm -rf build && idf.py build
```

### Technical details
- Build log used to find the failing errors:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Cardputer-UserDemo/build/log/idf_py_stdout_output_4033133`

## Step 6: Create tutorial 0002 (FreeRTOS tasks + queue) and make it compile

This step intentionally “leaves Cardputer behind” and starts the next tutorial in the `esp32-s3-m5/0XXX` series as a pure ESP-IDF example: a FreeRTOS **producer/consumer** setup using a **queue**, with logging via `ESP_LOGI`. We copied the upstream `hello_world` project as a base (so build wiring is known-good), replaced `main/hello_world_main.c` with the queue demo, and confirmed it compiles on ESP-IDF 5.4.1 for `esp32s3`.

**Commit (code):** N/A — new tutorial project + build validation

### What I did
- Created `esp32-s3-m5/0002-freertos-queue-demo` from the ESP-IDF 5.4.1 `get-started/hello_world` example.
- Replaced the example main with:
  - `producer_task` → `xQueueSend` every 250ms
  - `consumer_task` → `xQueueReceive` and log message age
- Added a `README.md` with build/flash instructions.
- Built successfully with ESP-IDF 5.4.1.

### Why
- This is a clean, minimal tutorial that exercises the core FreeRTOS patterns we care about (tasks + queues) without any M5Stack-specific complexity.
- It compiles quickly and is easy to validate locally with `idf.py build`.

### What worked
- Build succeeded:
  - `Project build complete.`
  - Output: `esp32-s3-m5/0002-freertos-queue-demo/build/hello_world.bin`

### What didn't work
- N/A

### What I learned
- A “copy upstream example → replace app logic” flow is a fast way to produce reproducible tutorial projects that compile on a known ESP-IDF version.

### What was tricky to build
- N/A (no IDF migration issues here)

### What warrants a second pair of eyes
- Whether we want to pin flash size / partitions via `sdkconfig.defaults` for tutorial repos (today it uses IDF defaults: 2MB flash + 1MB app partition).

### What should be done in the future
- Create tutorial 0003 as an ISR + deferred processing example (GPIO interrupt + queue/notification).

### Code review instructions
- Build:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0002-freertos-queue-demo && idf.py set-target esp32s3 && idf.py build
```

### Technical details
- Project path:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0002-freertos-queue-demo`

## Step 7: Prepare tutorial 0003 (GPIO ISR → queue → task) — waiting for terminal approval

This step is queued up as the next tutorial in the series: demonstrate the canonical pattern **“keep ISRs tiny”** by posting events from a GPIO ISR into a FreeRTOS queue, and handling the event in a normal task. Implementation requires creating a new project directory and running `idf.py set-target`, but the terminal command to scaffold the project was not approved yet, so work is paused pending confirmation.

**Commit (code):** N/A — awaiting permission to run the scaffolding/build commands

### What I did
- Proposed the command to create `esp32-s3-m5/0003-gpio-isr-queue-demo` from the upstream ESP-IDF `hello_world` example and set the target to `esp32s3`.

### Why
- This is the next natural progression after tutorial 0002 (queues): add ISR context + deferred processing, which is a core ESP-IDF/FreeRTOS pattern.

### What worked
- N/A (command not run)

### What didn't work
- Command execution was declined; no changes were made.

### What I learned
- N/A

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- Once approved, scaffold the new project, implement ISR+queue demo, and run `idf.py build`.

### Code review instructions
- N/A

### Technical details
- Proposed command (not executed):

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation && source /home/manuel/esp/esp-idf-5.4.1/export.sh && cp -a /home/manuel/esp/esp-idf-5.4.1/examples/get-started/hello_world /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0003-gpio-isr-queue-demo && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0003-gpio-isr-queue-demo && idf.py set-target esp32s3
```

## Step 8: Create tutorial 0003 (GPIO ISR → queue → task) and make it compile

This step scaffolded `esp32-s3-m5/0003-gpio-isr-queue-demo` from upstream `hello_world`, replaced the application with a GPIO interrupt demo (ISR posts events into a FreeRTOS queue; a task consumes and logs), and then fixed the one ESP-IDF 5.x build-system requirement we hit: adding `esp_driver_gpio` as a dependency for the `main` component.

**Commit (code):** N/A — new tutorial project + build validation

### What I did
- Created `esp32-s3-m5/0003-gpio-isr-queue-demo` and set target to `esp32s3`.
- Implemented ISR+queue demo in `main/hello_world_main.c`.
- Added a `README.md` describing how to run it (default GPIO0 / BOOT button).
- Fixed build error by adding `esp_driver_gpio` to `main/CMakeLists.txt` `PRIV_REQUIRES`.
- Rebuilt successfully on ESP-IDF 5.4.1.

### Why
- This is the canonical ESP-IDF pattern for interrupt-driven input:
  - keep the ISR tiny
  - use `xQueueSendFromISR`
  - do the real work in a task
- ESP-IDF 5.x requires explicit component dependencies (drivers split into `esp_driver_*`).

### What worked
- Build succeeded and produced:
  - `esp32-s3-m5/0003-gpio-isr-queue-demo/build/hello_world.bin`

### What didn't work
- First build failed with missing header `driver/gpio.h` because `main` did not depend on `esp_driver_gpio`. ESP-IDF printed a helpful hint to add it to `PRIV_REQUIRES`.

### What I learned
- In ESP-IDF 5.x, driver headers like `driver/gpio.h` live in split components and must be listed in `idf_component_register(... PRIV_REQUIRES ...)` for the component that includes them.

### What was tricky to build
- The error looks like a “missing include path,” but the correct fix is adding the missing component dependency, not tweaking include dirs manually.

### What warrants a second pair of eyes
- Confirm the default pin choice (GPIO0) is acceptable for our tutorial series as a “works on most dev boards” default.

### What should be done in the future
- Create tutorial 0004: timer ISR (or gptimer callback) → task notification, to show a second ISR-safe primitive.

### Code review instructions
- Build:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0003-gpio-isr-queue-demo && idf.py build
```

### Technical details
- Scaffold command executed:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation && source /home/manuel/esp/esp-idf-5.4.1/export.sh && cp -a /home/manuel/esp/esp-idf-5.4.1/examples/get-started/hello_world /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0003-gpio-isr-queue-demo && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0003-gpio-isr-queue-demo && idf.py set-target esp32s3
```

## Step 9: Create tutorial 0004 (I2C polling + callback + queue + rolling average) and make it compile

This step implements your requested “I2C polling + callback + queue + rolling average” tutorial. We use an `esp_timer` periodic callback as the trigger (callback → queue). A dedicated I2C polling task receives poll events, reads a common I2C sensor (SHT30/SHT31 @ `0x44`), and sends samples to a second queue. A rolling-average task consumes samples and maintains a moving average over the last N temperature readings.

We hit two small C/tooling issues during the build and fixed them:
- A variable-length array could not be initialized; we switched the window size to a compile-time constant.
- A format/argument type mismatch under `-Werror=format`; we aligned logging arguments to `PRIu32`.

**Commit (code):** N/A — new tutorial project + build validation

### What I did
- Scaffolded `esp32-s3-m5/0004-i2c-rolling-average` from upstream `hello_world`.
- Implemented:
  - `esp_timer` callback → `poll_queue`
  - `i2c_poll_task` (I2C read) → `sample_queue`
  - `avg_task` rolling average over `ROLLING_WINDOW`
- Used the ESP-IDF 5.x “new” I2C driver (`driver/i2c_master.h`) and added component deps (`esp_driver_i2c`, `esp_timer`) to `main/CMakeLists.txt`.
- Built successfully on ESP-IDF 5.4.1.

### Why
- This demonstrates a realistic pattern:
  - callbacks stay short and just schedule work
  - I/O happens in a normal task
  - derived metrics (rolling average) are computed downstream via queue decoupling

### What worked
- Build succeeded and produced:
  - `esp32-s3-m5/0004-i2c-rolling-average/build/hello_world.bin`

### What didn't work
- First build failed:
  - VLA init: `float window[ROLLING_WINDOW] = {0};` when `ROLLING_WINDOW` was not a compile-time constant.
- Second build failed:
  - `-Werror=format` due to type mismatch in one `ESP_LOGI` statement.

### What I learned
- In ESP-IDF builds with `-Werror`, it’s worth being strict about `PRI*` format macros and ensuring argument types match exactly (esp-idf’s log macros can make this more obvious).

### What was tricky to build
- The rolling window size must be a true compile-time constant in C if we want a zero-initialized array literal.

### What warrants a second pair of eyes
- Confirm the default I2C pins (`GPIO8/9`) match the common board you expect to use for this tutorial series; otherwise we should pick a different default and keep the same “edit constants to match your wiring” guidance.

### What should be done in the future
- Add a small Kconfig option for I2C pins/address so users can configure without editing code (optional).

### Code review instructions
- Build:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0004-i2c-rolling-average && idf.py build
```

### Technical details
- Project path:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0004-i2c-rolling-average`

## Step 10: Create tutorial 0005 (WiFi STA + event loop handlers) and make it compile

This step adds a WiFi + event loop tutorial. It configures WiFi in **station mode**, sets up the **default ESP-IDF event loop**, and registers handlers for `WIFI_EVENT` (start/disconnect) and `IP_EVENT` (got IP). It uses a FreeRTOS event group to wait for either “connected” or “failed after retries”, which is a standard pattern when wiring asynchronous event loops into a synchronous “startup” flow.

**Commit (code):** N/A — new tutorial project + build validation

### What I did
- Created `esp32-s3-m5/0005-wifi-event-loop` from upstream `hello_world`.
- Added Kconfig options under `main/Kconfig.projbuild`:
  - `CONFIG_TUTORIAL_0005_WIFI_SSID`
  - `CONFIG_TUTORIAL_0005_WIFI_PASSWORD`
  - `CONFIG_TUTORIAL_0005_WIFI_MAX_RETRY`
- Implemented WiFi STA init + event loop handlers in `main/hello_world_main.c`.
- Added component dependencies in `main/CMakeLists.txt`:
  - `esp_wifi`, `esp_event`, `esp_netif`, `nvs_flash`
- Built successfully on ESP-IDF 5.4.1.

### Why
- WiFi on ESP-IDF is inherently event-driven; this tutorial makes the “event loop + handlers” model concrete.
- The event-group wait shows how to bridge asynchronous events into a clean startup sequence.

### What worked
- Build succeeded and produced:
  - `esp32-s3-m5/0005-wifi-event-loop/build/hello_world.bin`

### What didn't work
- N/A

### What I learned
- ESP-IDF 5.x enforces explicit component dependencies: if `main` includes WiFi headers, it must `PRIV_REQUIRES` the corresponding components.

### What was tricky to build
- N/A (straightforward once dependencies are correct)

### What warrants a second pair of eyes
- Security: this example sets `wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK`, which is fine for “normal” WPA2 networks but may not fit WPA3-only networks; we can expand later if needed.

### What should be done in the future
- Add a follow-up tutorial showing posting a **custom event** onto the event loop (user-defined event base) to reinforce the mechanism beyond WiFi.

### Code review instructions
- Build:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0005-wifi-event-loop && idf.py build
```

### Technical details
- Project path:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0005-wifi-event-loop`

## Step 11: Security scrub — remove accidentally committed WiFi credentials from tutorial 0005

This step addressed an easy-to-miss but important issue: the checked-in `sdkconfig` for tutorial `0005` contained a real SSID/password. We scrubbed it (set both to empty strings) and immediately recorded the change in the ticket changelog + related file list.

This is a good reminder that ESP-IDF projects often produce a `sdkconfig` during local development, and it’s very easy to accidentally commit credentials if you do any WiFi work.

**Commit (code):** N/A (doc + config hygiene within the tutorial workspace)

### What I did
- Edited the checked-in `sdkconfig` for `0005` to set:
  - `CONFIG_TUTORIAL_0005_WIFI_SSID=""`
  - `CONFIG_TUTORIAL_0005_WIFI_PASSWORD=""`
- Updated docmgr changelog and related files for ticket `002-M5-ESPS32-TUTORIAL`.

### Why
- Avoid committing secrets and avoid accidentally propagating them into future tutorials by copy/paste / directory cloning.

### What worked
- The `sdkconfig` now contains empty WiFi fields (no secrets).

### What didn't work
- N/A

### What I learned
- If we keep `sdkconfig` in-tree for tutorials, we should treat it as potentially sensitive and keep WiFi credentials empty by default.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- Confirm there are no other accidentally committed credentials elsewhere in `esp32-s3-m5/*/sdkconfig`.

### What should be done in the future
- Consider adding a lightweight repo-level guardrail (documentation or tooling) reminding contributors not to commit secrets in `sdkconfig`.

### Code review instructions
- Review:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0005-wifi-event-loop/sdkconfig`

## Step 12: Create tutorial 0006 (WiFi STA + HTTP client) and make it compile

This step adds the next “practical networking” layer on top of tutorial `0005`: once WiFi is connected (using the event loop + event group), we start an `esp_http_client` task that periodically performs an HTTP GET against a configurable URL.

This keeps the tutorial focused on the **ESP-IDF networking primitives** (event loop, WiFi connect, HTTP client), and intentionally avoids diving into TLS time-sync complexity; HTTPS is supported via cert bundle attach, but may require system time to be set to pass certificate validity checks.

**Commit (code):** N/A

### What I did
- Scaffolded `0006` by copying `0005` and removing the build directory.
- Updated `0006` to:
  - Rename config symbols to `TUTORIAL_0006_*`
  - Add Kconfig options for URL/timeout/period
  - Add `esp_http_client` task that runs after WiFi connects
  - Update dependencies in `main/CMakeLists.txt`
  - Update top-level project name + README instructions
- Built with ESP-IDF 5.4.1.

### Why
- This is the first “real” networking example after WiFi bring-up: it validates DNS + TCP + HTTP on-device and gives a concrete template for future API calls.

### What worked
- The project builds successfully and produces:
  - `esp32-s3-m5/0006-wifi-http-client/build/wifi_http_client.bin`

### What didn't work
- Initial build failed with:
  - `Failed to resolve component 'esp_crt_bundle' required by component 'main': unknown name.`
- Fix: `esp_crt_bundle` isn’t a standalone component name in this IDF layout; using `mbedtls` as the explicit dependency resolved it.

### What I learned
- In ESP-IDF 5.x, component names matter: headers like `esp_crt_bundle.h` live under the `mbedtls` component tree, even though the API looks “standalone”.

### What was tricky to build
- Avoiding a TLS rabbit-hole: HTTPS can require time sync; for a simple tutorial, defaulting to `http://...` makes “first success” much more reliable.

### What warrants a second pair of eyes
- Confirm the chosen default URL (`http://example.com/`) is acceptable for our tutorial goals; some networks intercept/block or rewrite it.
- Confirm task stack size (`6144`) is reasonable on the intended devices.

### What should be done in the future
- Add a follow-up tutorial that sets system time via SNTP and demonstrates a clean HTTPS request to a modern endpoint.

### Code review instructions
- Build:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0006-wifi-http-client && idf.py build
```

### Technical details
- Project path:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0006-wifi-http-client`

## Step 13: Create tutorial 0007 (Cardputer keyboard GPIO matrix scan → realtime serial echo) and make it compile

This step creates a Cardputer-specific input tutorial: it scans the built-in keyboard matrix over GPIO and echoes typed characters to the serial console in realtime. This is intentionally “pure IDF” (no M5Unified/M5GFX), and focuses on the embedded basics: GPIO configuration, periodic polling, simple debounce, and edge-triggered key emission.

We derived the pin choices and the (x,y) → key label map from the vendor implementation under `M5Cardputer-UserDemo/main/hal/keyboard/keyboard.h` + `keyboard.cpp`, and exposed the pins + scan timing via `menuconfig` so the tutorial remains adaptable.

**Commit (code):** N/A

### What I did
- Scaffolded `esp32-s3-m5/0007-cardputer-keyboard-serial` from the known-good Cardputer baseline (`0001`).
- Implemented a minimal keymap + scan loop in `main/hello_world_main.c`:
  - Output pins drive a 3-bit scan value (0..7)
  - Input pins read a 7-bit pressed mask (pull-ups; pressed = low)
  - Mapped scan state + input bit index to a 4x14 logical key grid
  - On rising-edge key presses, echoed characters to serial and supported `del` (backspace) + `enter`
- Added `Kconfig.projbuild` to configure pins, scan period, and debounce.
- Built with ESP-IDF 5.4.1.

### Why
- We need a clean, minimal “text input” primitive for future tutorials (menus, command console, WiFi configuration, etc.).

### What worked
- Build succeeded and produced:
  - `esp32-s3-m5/0007-cardputer-keyboard-serial/build/cardputer_keyboard_serial.bin`

### What didn't work
- Initial build failed due to missing `#include <string.h>` (implicit `strcmp`/`strlen` under `-Werror`).
- Fix: added the include and rebuilt successfully.

### What I learned
- The Cardputer keyboard is a GPIO-scanned matrix (not an I2C peripheral), so polling is the simplest approach.
- With IDF 5.x, including driver headers requires explicit component dependencies (`esp_driver_gpio`).

### What was tricky to build
- Getting “realtime” serial echo: stdout buffering can hide characters; setting `stdout` unbuffered via `setvbuf(..., _IONBF, ...)` makes typing feel immediate.

### What warrants a second pair of eyes
- Validate the default pin list against the schematics + hardware across Cardputer revisions.
- Validate that GPIOs used for keyboard don’t conflict with USB/JTAG/strapping modes in your exact setup.

### What should be done in the future
- Add optional key-repeat handling (hold-to-repeat) and a more explicit modifier model (ctrl/alt combos).

### Code review instructions
- Build:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0007-cardputer-keyboard-serial && idf.py set-target esp32s3 && idf.py build
```

### Technical details
- Key references used:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Cardputer-UserDemo/main/hal/keyboard/keyboard.h`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Cardputer-UserDemo/main/hal/keyboard/keyboard.cpp`

## Step 14: ATOMS3R display schematics capture + pin mapping + open questions

This step started the ATOMS3R display tutorial work by turning the schematic into something we can quickly reason about: a high-resolution PNG plus a “cropped” pinout view showing the exact GPIO mappings for the display connector.

The most important outcome is that we now have a concrete **SPI + control pin mapping** for the ATOMS3R display connector, and we identified that the backlight is controlled separately (so the panel can be kept dark during init).

**Commit (code):** N/A (documentation + hardware recon)

### What I did
- Rendered the ATOMS3R schematic PDF to PNG (for easy sharing/review).
- Created a cropped PNG showing the display connector pin mapping.
- Drafted a “questions for schematics specialist” checklist for any remaining unknowns.

### Why
- Display bring-up is easiest when the “wiring truth” is nailed down up front; it avoids guessing pins and confusing software bugs with wiring mistakes.

### What worked
- From the schematic, we can read the panel interface is a **4-wire SPI write-only** setup (no MISO/TE visible) plus reset + D/C.

### What didn't work
- N/A

### What I learned
- The panel connector wiring on ATOMS3R is simple and matches typical SPI LCD panels.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- Panel controller identification (ST7789 vs other) and any panel RAM offsets/rotation defaults.

### What should be done in the future
- Once the panel controller is fully confirmed, implement the pure ESP-IDF `esp_lcd` initialization and an animation loop tutorial.

### Technical details
- Schematic source:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/datasheets/Sch_M5_AtomS3R_v0.4.1.pdf`
- Rendered images:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/21/002-M5-ESPS32-TUTORIAL--esp32-s3-m5stack-development-tutorial-research-guide/reference/atoms3r_schematic_v0.4.1.png`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/21/002-M5-ESPS32-TUTORIAL--esp32-s3-m5stack-development-tutorial-research-guide/reference/atoms3r_display_pins_crop.png`
- Question checklist:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/21/002-M5-ESPS32-TUTORIAL--esp32-s3-m5stack-development-tutorial-research-guide/reference/02-atoms3r-display-schematics-questions-for-specialist.md`

## Step 15: Capture “Claude answers” for ATOMS3R display pinout/backlight

This step captured the consolidated schematic interpretation in a single place (including the key backlight finding) so we can proceed with implementation without repeatedly re-deriving the same facts.

**Commit (code):** N/A

### What I did
- Added the “Claude answers” document summarizing the ATOMS3R display pin table and backlight control.

### Why
- We need a working baseline pin mapping (including backlight polarity) before implementing `esp_lcd` init + animation.

### What worked
- Confirmed display wiring (from `atoms3r_display_pins_crop.png`) and added the missing backlight detail:
  - `DISP_CS=GPIO14`, `SPI_SCK=GPIO15`, `SPI_MOSI=GPIO21`, `DISP_RS/DC=GPIO42`, `DISP_RST=GPIO48`
  - `LED_BL` is controlled by **GPIO7 via a PMOS FET**, **active-low**, and PWM-capable.

### What didn't work
- N/A

### What I learned
- Backlight control is separate and should be enabled after the panel init to avoid flashing garbage on boot.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- The panel controller / offsets / rotation are still not proven by schematics alone; we should confirm via M5Unified config or hardware test.

### What should be done in the future
- Implement tutorial `0008-atoms3r-display-animation` using `esp_lcd` with these pins and a small animation loop.

### Technical details
- Claude answers doc:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/21/002-M5-ESPS32-TUTORIAL--esp32-s3-m5stack-development-tutorial-research-guide/reference/03-schematic-claude-answers.md`

## Step 16: Create tutorial 0008 (ATOMS3R SPI LCD + esp_lcd) and make it compile

This step creates the first ATOMS3R display tutorial project: initialize the SPI LCD using ESP-IDF’s `esp_lcd` stack and render a small looping animation. It uses the pin mapping confirmed in the schematic crop + `03-schematic-claude-answers.md`, including the critical detail that backlight is **active-low** on **GPIO7** (via a PMOS FET), so we keep the backlight off until panel init is done.

Because the schematic doesn’t name the panel controller IC, the project currently assumes **ST7789** (common for M5 displays). To reduce risk, the tutorial exposes Kconfig knobs for BGR vs RGB and x/y offsets so we can fix common “looks wrong” issues without rewriting the driver.

**Commit (code):** N/A

### What I did
- Scaffolded `esp32-s3-m5/0008-atoms3r-display-animation` from the known-good baseline (`0001`) and removed the build directory.
- Implemented `esp_lcd` init + draw loop:
  - SPI bus: `SPI2_HOST`, MOSI/SCK from the schematic, no MISO
  - Panel IO: `esp_lcd_new_panel_io_spi`
  - Panel: `esp_lcd_new_panel_st7789`
  - Backlight: GPIO7 active-low (off during init, on after)
  - Animation: full-frame RGB565 buffer -> `esp_lcd_panel_draw_bitmap` in a loop
- Added `main/Kconfig.projbuild` for resolution, offsets, colorspace, SPI clock, and frame delay.
- Built with ESP-IDF 5.4.1.

### Why
- We want a minimal, reproducible display “bring-up + render loop” that doesn’t require Arduino/M5Unified and can serve as the foundation for later UI tutorials.

### What worked
- Build succeeded and produced:
  - `esp32-s3-m5/0008-atoms3r-display-animation/build/atoms3r_display_animation.bin`

### What didn't work
- First build failed because:
  - Boolean Kconfig options aren’t always defined in `sdkconfig.h` when disabled (needed preprocessor guards).
  - `esp_lcd_panel_dev_config_t` uses a union (setting both `rgb_endian` and `color_space` overwrote the same field).
- Fix:
  - Convert Kconfig bools to compile-time constants via `#if CONFIG_...`.
  - Use `rgb_ele_order` + `data_endian` in `esp_lcd_panel_dev_config_t` instead of deprecated union aliases.

### What I learned
- For IDF 5.x, `esp_lcd_panel_dev_config_t`’s `color_space/rgb_endian` fields are union aliases; prefer `rgb_ele_order`.
- For IDF Kconfig booleans: don’t reference `CONFIG_FOO` as a C identifier unless you know it’s always defined; use `#if CONFIG_FOO`.

### What was tricky to build
- Getting the `esp_lcd_panel_dev_config_t` union right (to avoid subtle misconfig + warnings).

### What warrants a second pair of eyes
- Confirm the actual panel controller IC is ST7789 (or update the tutorial if it’s not).
- Confirm any required x/y offsets and color order on real hardware (menuconfig toggles exist).

### What should be done in the future
- Add a tiny “panel self-test” screen that draws a color bar + coordinate grid to quickly confirm offsets/rotation.

### Code review instructions
- Build:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0008-atoms3r-display-animation && idf.py set-target esp32s3 && idf.py build
```

### Technical details
- Pin mapping reference:
  - `.../reference/atoms3r_display_pins_crop.png`
  - `.../reference/03-schematic-claude-answers.md`

## Step 17: Research GC9107 support in ESP-IDF (driver options + community reports)

This step investigated whether we can support the AtomS3R’s **GC9107** LCD in a pure ESP-IDF project without vendoring M5GFX/LovyanGFX. The immediate motivation is that `0008` was implemented assuming ST7789 due to schematic ambiguity, but the “real deal” M5 stack indicates the panel is GC9107 with a 128×160 internal memory and a visible 128×128 window via `offset_y = 32`.

The key outcome: there is an **official Espressif component** (`esp_lcd_gc9107`) that integrates with ESP-IDF’s `esp_lcd` API via the component manager, so we likely do *not* need to hand-write a driver.

**Commit (code):** N/A (research + documentation)

### What I did
- Searched for “GC9107 + ESP-IDF + esp_lcd” driver availability and examples.
- Collected links to:
  - an official Espressif component (`esp_lcd_gc9107`)
  - ESP-IoT-Solution’s documentation mentioning GC9107 support
  - GC9107 datasheet mirrors for register/geometry reference
  - community display libraries (useful for behavior comparison, not necessarily for IDF)
- Wrote an analysis doc capturing findings and recommended approaches:
  - `analysis/01-gc9107-on-esp-idf-atoms3r-driver-options-integration-approaches-and-findings.md`

### Queries and findings (chronological)

- Query: “GC9107 ESP-IDF esp_lcd_panel driver”
  - Found: Espressif publishes `espressif/esp_lcd_gc9107` on the component registry (installable via `idf.py add-dependency`).
- Query: “AtomS3R ESP-IDF GC9107 esp_lcd”
  - Found: most references point back to the official component + generic SPI LCD bring-up patterns.
- Query: “ESP32_Display_Panel GC9107 ESP-IDF”
  - Found: `ESP32_Display_Panel` is frequently mentioned as a multi-panel helper library; still need to validate GC9107 support and whether it’s appropriate for our “pure IDF tutorial” goal.
- Query: “LovyanGFX Panel_GC9107 source file”
  - Found: the authoritative source for “what M5 does” is still M5GFX/LovyanGFX; this remains the fallback path if the `esp_lcd_gc9107` component doesn’t match AtomS3R quirks.

### What worked
- Identified a likely “best path” for ESP-IDF: use the official component registry driver (`esp_lcd_gc9107`) with standard `esp_lcd` wiring and apply AtomS3R-specific offsets (notably y-gap 32).

### What didn't work
- N/A

### What I learned
- GC9107 is not in the ESP-IDF core tree (at least not in `esp_lcd`), but it *is* available via the component registry as `esp_lcd_gc9107`.
- Even with a driver, AtomS3R still requires board-specific correctness:
  - y-gap/offset behavior (visible 128×128 inside 128×160)
  - color order / rotation
  - backlight control mechanism (hardware revision risk: M5GFX suggests I2C brightness; schematic analysis suggests GPIO gating)

### What was tricky to build
- Separating “driver exists” from “driver matches M5’s exact geometry/offset defaults”; we’ll still need to validate on real hardware.

### What warrants a second pair of eyes
- Confirm the backlight mechanism on the actual AtomS3R hardware being used (GPIO-driven vs I2C brightness device).

### What should be done in the future
- Update `0008-atoms3r-display-animation` to use the GC9107 driver (via `espressif/esp_lcd_gc9107`) and default to `offset_y=32`, plus add a test-pattern mode (grid + color bars) for quick on-device validation.

### Technical details / sources
- Espressif component registry: `https://components.espressif.com/components/espressif/esp_lcd_gc9107`
- ESP-IoT-Solution user guide: `https://docs.espressif.com/projects/esp-iot-solution/en/latest/esp-iot-solution-en-master.pdf`
- GC9107 datasheet mirrors:
  - `https://files.waveshare.com/wiki/0.85inch-LCD-Module/GC9107_DataSheet_V1.2.pdf`
  - `https://www.buydisplay.com/download/ic/GC9107.pdf`
- Community library (behavior comparison): `https://github.com/moononournation/Arduino_GFX`

## Step 18: Start migrating tutorial 0008 to AtomS3R “real deal” (GC9107 + backlight)

This step begins the concrete “make it work on AtomS3R” migration for tutorial `0008`. The key changes are:

- Switch from the ST7789 assumption to **GC9107**.
- Use Espressif’s component-registry driver (`espressif/esp_lcd_gc9107`) so we stay in “pure ESP-IDF `esp_lcd`” territory.
- Default `y_offset` to **32** to match the M5GFX behavior (visible 128×128 inside controller RAM 128×160).
- Replace “GPIO7 PMOS backlight” with a **default I2C backlight mode** (per M5GFX deep dive), while keeping a configurable GPIO fallback for hardware revisions.

**Commit (code):** N/A

### What I did
- Added the GC9107 panel driver dependency to the `main` component using:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0008-atoms3r-display-animation && idf.py add-dependency "espressif/esp_lcd_gc9107^2.0.0"
```

- This created:
  - `esp32-s3-m5/0008-atoms3r-display-animation/main/idf_component.yml`
- Updated `main/hello_world_main.c` to:
  - use `esp_lcd_new_panel_gc9107(...)`
  - use `SPI3_HOST` (matching M5GFX)
  - add I2C backlight support scaffolding
- Updated `main/Kconfig.projbuild` to:
  - default `TUTORIAL_0008_LCD_Y_OFFSET=32`
  - add a backlight mode choice (I2C vs GPIO vs none)

### What worked
- The component manager successfully added the dependency and generated the manifest.

### What didn't work (build failure)
- A clean rebuild failed with two classes of errors:

1) The `i2c_master_bus_config_t` flags fields used in our code don’t match the actual struct in this IDF version:

```text
error: 'struct <anonymous>' has no member named 'enable_pullup_scl'
error: 'struct <anonymous>' has no member named 'enable_pullup_sda'
```

2) The backlight GPIO config symbol is conditionally defined by Kconfig, but the code referenced it unconditionally:

```text
error: 'CONFIG_TUTORIAL_0008_BL_GPIO' undeclared
```

### What I learned
- For IDF driver config structs, “flags” field names vary; we should align to the actual `driver/i2c_master.h` definition.
- For Kconfig `int` symbols gated by `depends on`, referencing `CONFIG_FOO` directly will fail when the symbol isn’t emitted into `sdkconfig.h`. We need `#if CONFIG_...` guards around any such references.

### What should be done next
- Fix I2C bus config flags usage.
- Wrap GPIO-backlight code paths in `#if CONFIG_TUTORIAL_0008_BACKLIGHT_GPIO` and provide no-op stubs otherwise.

## Step 19: Fix 0008 GC9107 migration build errors (I2C config + Kconfig guards) and rebuild

This step fixed the first compilation issues introduced by adding the AtomS3R backlight implementation. The problems were exactly what we expected: mismatch between assumed vs actual IDF struct fields, and unguarded references to Kconfig-gated symbols.

**Commit (code):** N/A

### What I did
- Updated `i2c_master_bus_config_t` initialization to use the correct flag field for this IDF version:
  - replaced `flags.enable_pullup_scl/sda` with `flags.enable_internal_pullup`
- Wrapped the GPIO-backlight fallback implementation in `#if CONFIG_TUTORIAL_0008_BACKLIGHT_GPIO` so `CONFIG_TUTORIAL_0008_BL_GPIO` is only referenced when it exists.
- Rebuilt `0008`.

### What worked
- Build succeeded (and the component manager pulled in `espressif__esp_lcd_gc9107` as a managed component).

### What didn't work
- N/A (one remaining harmless warning was silenced by marking a stub `unused`).

### What I learned
- The new I2C master API’s config structs differ from older examples; always verify the actual header for flag names.
- For Kconfig `int` symbols behind `depends on`, treat `CONFIG_*` as optional and guard code appropriately.

### Code review instructions
- Build:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0008-atoms3r-display-animation && idf.py build
```

## Step 20: Add high-signal serial logs to tutorial 0008 (bring-up + backlight + heartbeat)

This step adds “debug-friendly” serial logging to `0008` so that if the display doesn’t come up on real hardware, we can quickly see *where* it fails: SPI bus init, panel creation/init, gap offsets, and backlight control (I2C or GPIO fallback). It also adds a low-rate heartbeat log so you can tell the render loop is alive without spamming the console.

**Commit (code):** N/A

### What I did
- Added structured `ESP_LOGI/ESP_LOGE/ESP_LOGW` logs around:
  - backlight init + brightness write (I2C addr/reg/value)
  - SPI bus init parameters
  - GC9107 panel create/reset/init
  - gap offsets + invert + display on
  - framebuffer allocation + heap stats
- Added a Kconfig option:
  - `TUTORIAL_0008_LOG_EVERY_N_FRAMES` (default 60; 0 disables)
- Fixed a build break from log formatting:
  - `heap_caps_get_free_size()` returns `size_t`, so we cast to `uint32_t` when printing with `PRIu32`.

### What worked
- Project builds cleanly after the logging changes.

### What should be done in the future
- If bring-up issues persist on hardware, add an optional I2C “probe” log (scan `0x30`) and print a clear hint to swap SDA/SCL pins if NACKs occur.

## Step 21: On-device AtomS3R run report — GC9107 init succeeds but display shows moving grid artifacts

This step records an on-device observation: GC9107 panel creation/reset/init completes and backlight enable writes succeed, but the LCD shows “weird moving horizontal/vertical grid patterns overlapping” instead of the expected smooth gradient.

The most suspicious detail in the captured logs is that the panel gap is set to **(0,0)**, while the AtomS3R GC9107 mapping from the M5 stack uses **`offset_y = 32`** (visible 128×128 inside 128×160 RAM). If we’re drawing into the wrong RAM window, the display can look like noise/scrolling patterns even though the loop is running.

**Commit (code):** N/A (field report + next debug actions)

### What I saw (logs)
- `gc9107: LCD panel create success, version: 2.0.0`
- `panel reset/init...`
- `panel gap: x=0 y=0`
- `panel display on`
- `backlight enable: I2C brightness=255`
- `lcd init ok (128x128), pclk=40000000Hz, gap=(0,0), colorspace=BGR`
- `framebuffer ok: 32768 bytes ...`

### Hypothesis / next steps
- Ensure **`TUTORIAL_0008_LCD_Y_OFFSET` is 32** in `idf.py menuconfig`.
  - Note: Kconfig defaults do not override an existing checked-in `sdkconfig` value; the device log showing `gap=(0,0)` strongly suggests the project’s `sdkconfig` still has y-offset=0.
- If artifacts persist after setting `y_offset=32`, try:
  - Lower SPI clock to 26MHz (signal integrity)
  - Toggle BGR/RGB
  - Toggle invert
  - Temporarily increase frame delay (reduce bus churn)

## Step 22: Compare M5GFX GC9107 panel vs Espressif `esp_lcd_gc9107` + identify RGB565 byte-order pitfall

This step compares the M5GFX `Panel_GC9107` implementation with the managed `espressif__esp_lcd_gc9107` component, focused on issues that can cause “grid/noise” looking output even when the panel init succeeds.

### Key findings
- **M5GFX uses a 128×160 memory geometry for GC9107** (visible 128×128 is achieved via a row offset in board config). This reinforces that **AtomS3R needs a y-offset (rowstart) of 32**.
- **Init-sequence deltas**:
  - M5GFX sends an unlock-ish preamble (`0xFE`, `0xEF`) before the vendor command list.
  - Espressif’s default init sequence does *not* include `0xFE/0xEF`, but it *does* include `0x21` (INVON) and different gamma tables (`0xF0/0xF1` values).
  - Conclusion: if the panel still looks wrong after fixing offsets/byte-order, try a **custom init command list** matching the M5GFX sequence via `gc9107_vendor_config_t`.
- **RGB565 byte order over SPI**: ESP-IDF’s own SPI LCD example notes that **SPI LCD is big-endian** and the RGB565 buffer often needs a **byte swap** before `esp_lcd_panel_draw_bitmap()`. This is a prime suspect for “grid/noise” patterns when sending a CPU-endian `uint16_t` framebuffer.

### Action taken
- Added a tutorial Kconfig knob `TUTORIAL_0008_SWAP_RGB565_BYTES` (default enabled) and implemented per-pixel `bswap16` in the framebuffer generator so RGB565 is sent MSB-first.
