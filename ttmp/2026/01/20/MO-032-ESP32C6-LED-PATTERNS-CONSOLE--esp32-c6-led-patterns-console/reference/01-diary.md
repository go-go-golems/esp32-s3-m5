---
Title: Diary
Ticket: MO-032-ESP32C6-LED-PATTERNS-CONSOLE
Status: active
Topics:
    - esp32
    - animation
    - console
    - serial
    - esp-idf
    - usb-serial-jtag
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0030-cardputer-console-eventbus/main/app_main.cpp
      Note: Reference esp_console setup over USB Serial/JTAG + command registration
    - Path: 0043-xiao-esp32c6-ws2811-4led-d1/inspiration/led_patterns.h
      Note: Reference pattern config structs + pattern/state separation
    - Path: 0043-xiao-esp32c6-ws2811-4led-d1/main/main.c
      Note: Prior working time-based rainbow + WS281x transmit loop
    - Path: 0043-xiao-esp32c6-ws2811-4led-d1/main/ws281x_encoder.c
      Note: RMT encoder for WS281x timings + reset pulse
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/Kconfig.projbuild
      Note: MO-032 initial Kconfig knobs for WS281x
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/led_ws281x.c
      Note: WS281x driver wrapper (RMT channel + encoder + pixels)
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/led_ws281x.h
      Note: Driver wrapper API + config structs
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/main.c
      Note: MO-032 implementation project entrypoint
    - Path: ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/sources/local/patterns.jsx
      Note: Reference simulator patterns + parameter extraction
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-20T14:48:03.56417003-05:00
WhatFor: ""
WhenToUse: ""
---





# Diary

## Goal

Capture the end-to-end work for MO-032: ingest existing LED pattern code, compare against prior firmware (rainbow baseline), and produce a detailed design/analysis document for an `esp_console` REPL + realtime pattern task architecture (queue-driven reconfiguration), then upload the resulting doc to reMarkable.

## Step 1: Create Ticket + Diary Scaffolding

Created the `docmgr` ticket workspace and the initial Diary document so we can log decisions, file references, and command history as we proceed. This establishes a stable place for the upcoming pattern analysis and REPL design doc.

No code changes yet; this step is documentation scaffolding only.

### What I did
- Ran `docmgr status --summary-only` to confirm doc root and vocabulary availability.
- Created ticket `MO-032-ESP32C6-LED-PATTERNS-CONSOLE` with topics `esp32,animation,console,serial,esp-idf,usb-serial-jtag`.
- Added `reference/01-diary.md` via `docmgr doc add`.

### Why
- The ticket workspace is the canonical place to store the analysis/design artifacts and to keep related file links tidy.

### What worked
- Ticket + doc creation succeeded; workspace path is under `ttmp/2026/01/20/`.

### What didn't work
- N/A

### What I learned
- The repo vocabulary already includes `animation` and `console` topics, so we can tag LED pattern work without adding new vocabulary.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- Add RelatedFiles once we import/analyze `patterns.jsx` and identify the relevant firmware rainbow implementation.

### Code review instructions
- Review the created ticket workspace under `ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/`.
- Confirm ticket metadata in `ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/index.md`.

### Technical details
- Commands:
  - `docmgr status --summary-only`
  - `docmgr vocab list`
  - `docmgr ticket create-ticket --ticket MO-032-ESP32C6-LED-PATTERNS-CONSOLE ...`
  - `docmgr doc add --ticket MO-032-ESP32C6-LED-PATTERNS-CONSOLE --doc-type reference --title "Diary"`

## Context

This repo is currently scoped to ESP32-S3/M5 work, but this ticket targets an ESP32-C6 firmware feature set. The doc focuses on patterns + console architecture in a way that is portable across ESP-IDF targets (S3/C6/H2), with explicit notes where hardware/peripheral choices differ.

## Step 2: Import `patterns.jsx` + Identify Core Pattern Parameters

Imported the provided `/tmp/patterns.jsx` into the ticket sources so we have a stable, versioned copy to reference while writing the C/ESP-IDF architecture and console design. I focused first on the four requested patterns (rainbow, chase, breathing, sparkle) to extract a minimal configuration schema that can be translated cleanly to an embedded implementation.

This step also established the “shape” of pattern configs used by the simulator: almost every generator function accepts `{ speed, brightness, color1, color2 }` with per-pattern interpretation.

### What I did
- Ran `docmgr import file --file /tmp/patterns.jsx --ticket MO-032-ESP32C6-LED-PATTERNS-CONSOLE`.
- Read the imported file and extracted the key per-pattern knobs:
  - `rainbow`: `speed`, `brightness`
  - `chase`: `color1`, `color2`, `speed`, `brightness` (with fixed `chaseLength=5`)
  - `breathing`: `color1`, `speed`, `brightness`
  - `sparkle`: `color1`, `speed` (unused in body), `brightness` (random sparkle probability fixed at `0.92`)

### Why
- The analysis/design doc needs a concrete reference implementation to define parameters, defaults, and expected visual outcomes before we translate to embedded patterns.

### What worked
- `docmgr import file` copied the file into ticket-local `sources/local/` and updated ticket index metadata.

### What didn't work
- N/A

### What I learned
- The simulator’s “frame” is an abstract counter advanced once per `requestAnimationFrame`, but embedded firmware should be time-based (`now_ms`) to avoid frame-rate dependency.
- Several patterns rely on randomness (`sparkle`, `fire`, `meteor`), so embedded configs likely need explicit RNG seeding + deterministic update semantics (or accept nondeterminism).

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- Decide whether we want to preserve exact simulator semantics (e.g., `chaseLength=5`, sparkle probability `p=0.08`) or promote these to explicit config parameters in firmware for better control.

### What should be done in the future
- Add a per-pattern config versioning strategy so console commands can evolve without breaking compatibility (e.g., `pattern_cfg_header_t { type, size, version }`).

### Code review instructions
- Read imported file: `ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/sources/local/patterns.jsx`.
- Focus on `patterns.{rainbow,chase,breathing,sparkle}` generator functions.

### Technical details
- Command:
  - `docmgr import file --file /tmp/patterns.jsx --ticket MO-032-ESP32C6-LED-PATTERNS-CONSOLE`

## Step 3: Review Prior ESP32-C6 Rainbow Firmware + Pattern Engine Inspiration

Located an existing ESP32-C6 firmware in this repo that already drives a WS2811/WS2812-style LED chain via the ESP-IDF RMT TX driver and renders a rotating rainbow in the main loop. This provides concrete, working reference code for: WS281x timing configuration, GRB byte order, brightness scaling, and a time-based rainbow hue offset.

In addition, the same project includes an “inspiration” LED pattern engine (`led_patterns.[ch]`) that already defines unified pattern configuration structs for rainbow/chase/breathing/sparkle. This is a strong starting point for the config schema and queue-driven runtime reconfiguration described in this ticket.

### What I did
- Searched for “rainbow” references across the repo.
- Read the prior firmware implementation and its config knobs:
  - `0043-xiao-esp32c6-ws2811-4led-d1/main/main.c`
  - `0043-xiao-esp32c6-ws2811-4led-d1/main/Kconfig.projbuild`
  - `0043-xiao-esp32c6-ws2811-4led-d1/main/ws281x_encoder.[ch]`
- Read the inspiration pattern engine:
  - `0043-xiao-esp32c6-ws2811-4led-d1/inspiration/led_patterns.[ch]`
  - `0043-xiao-esp32c6-ws2811-4led-d1/inspiration/led_patterns.yaml`

### Why
- The ticket’s design doc should be grounded in known-good ESP-IDF primitives and existing patterns/config idioms already present in this repo.

### What worked
- Found a time-based rainbow loop that is already decoupled from frame rate (`now_ms` based hue rotation).
- Found a unified `pattern_config_t` + `pattern_state_t` design in the inspiration engine that maps well onto a “single owner task + queue” control model.

### What didn't work
- N/A

### What I learned
- The prior rainbow code uses a custom RMT encoder that appends a configurable reset pulse and encodes MSB-first bits (matching WS281x expectations).
- The inspiration engine already treats “pattern config” and “pattern state” as separate structs, which is important when switching patterns live (reset state but keep driver online).

### What was tricky to build
- N/A (read-only investigation)

### What warrants a second pair of eyes
- Confirm which parts of “driver config” we truly want to be mutable at runtime (timings, GPIO, LED count) vs. only configurable via `menuconfig` + reboot.

### What should be done in the future
- Decide whether to adopt the inspiration engine’s config struct names verbatim (for speed) or to introduce a new schema that better matches `esp_console` ergonomics and future patterns.

### Code review instructions
- Start with the working rainbow + RMT driver loop:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0043-xiao-esp32c6-ws2811-4led-d1/main/main.c`
- Then review the WS281x encoder:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0043-xiao-esp32c6-ws2811-4led-d1/main/ws281x_encoder.c`
- Finally, review the “inspiration” config structs and pattern algorithms:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0043-xiao-esp32c6-ws2811-4led-d1/inspiration/led_patterns.h`

### Technical details
- Key ESP-IDF APIs used in prior firmware:
  - `rmt_new_tx_channel()`, `rmt_enable()`
  - `rmt_new_bytes_encoder()`, `rmt_new_copy_encoder()`
  - `rmt_transmit()`, `rmt_tx_wait_all_done()`
  - `esp_timer_get_time()`, `vTaskDelay()`

## Step 4: Write Detailed Design Doc (Patterns + Realtime Task + Console)

Drafted the core “textbook style” design document that formalizes the pattern configuration structures (rainbow/chase/breathing/sparkle), the realtime animation task model, the queue protocol for live updates, and an `esp_console` REPL verb set with WS281x driver staging/apply semantics.

This converts the simulator patterns (`patterns.jsx`) and the known-good C6 rainbow firmware (`0043`) into a cohesive embedded architecture proposal with pseudocode and explicit API/file references.

### What I did
- Created a `design-doc` document in the ticket and filled it with:
  - Pattern config structs (C) for rainbow/chase/breathing/sparkle
  - Algorithm notes mapping simulator semantics → embedded semantics
  - Realtime task loop + queue message protocol pseudocode
  - Proposed `esp_console` command set (`led ...`, including `led ws ...` and `led ws apply`)
- Referenced and linked the existing repo baselines used to ground the design.

### Why
- We need a clear, shared spec before implementing firmware changes, so the console verbs, config schema, and concurrency rules are explicit and reviewable.

### What worked
- The doc cleanly reuses known-good patterns from existing code in the repo, minimizing invention risk.
- The staged driver config + apply boundary makes runtime tuning safer.

### What didn't work
- N/A

### What I learned
- The repo already contains a minimal, clear `esp_console` initialization example that we can reuse nearly verbatim for USB Serial/JTAG backend selection.

### What was tricky to build
- Getting the “runtime mutability” story right: some WS281x fields are trivially changeable (pattern params), while others require reinitialization (GPIO/count/timing). The doc explicitly separates staged vs applied driver config to keep semantics unambiguous.

### What warrants a second pair of eyes
- Review the queue protocol: ensure we don’t end up with “partial config” states that are invalid or surprising during interactive use.
- Review the proposed command set for usability (short, consistent verbs; discoverable via `help`).

### What should be done in the future
- Add a playbook “smoke test” doc describing expected visuals for each pattern and basic wiring validation steps.

### Code review instructions
- Read the design doc:
  - `ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/design-doc/01-led-pattern-engine-esp-console-repl-ws281x.md`

### Technical details
- Key input sources referenced by the design doc:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/sources/local/patterns.jsx`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0043-xiao-esp32c6-ws2811-4led-d1/main/main.c`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0030-cardputer-console-eventbus/main/app_main.cpp`

## Step 5: Validate Frontmatter + Prep for reMarkable Upload

Before uploading, I validated that the ticket documents’ YAML frontmatter passes `docmgr` checks, and corrected an initial path mistake where I accidentally double-prefixed the `ttmp/` root.

This step is small, but it prevents future doc tooling friction and keeps the ticket in a “healthy” state before exporting.

### What I did
- Ran `docmgr validate frontmatter` on the design doc and diary.
- Fixed invocation to use absolute paths after the initial failure.

### Why
- `docmgr`’s frontmatter validator is sensitive to doc path resolution; validating now reduces later surprise during ticket review and exporting.

### What worked
- Frontmatter validated successfully for both documents once absolute paths were used.

### What didn't work
- First attempt failed due to an incorrect doc path:
  - Command: `docmgr validate frontmatter --doc ttmp/2026/01/20/.../design-doc/01-led-pattern-engine-esp-console-repl-ws281x.md --suggest-fixes`
  - Error: `Error: open /home/manuel/.../ttmp/ttmp/2026/01/20/...: no such file or directory`

### What I learned
- `docmgr validate frontmatter --doc ...` treats `--doc` as relative to the configured doc root unless an absolute path is provided; passing `ttmp/...` duplicates the root.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- N/A

### Code review instructions
- Re-run:
  - `docmgr validate frontmatter --doc /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/design-doc/01-led-pattern-engine-esp-console-repl-ws281x.md --suggest-fixes`

### Technical details
- Successful commands:
  - `docmgr validate frontmatter --doc /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/design-doc/01-led-pattern-engine-esp-console-repl-ws281x.md --suggest-fixes`
  - `docmgr validate frontmatter --doc /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/reference/01-diary.md --suggest-fixes`

## Step 6: Upload Design Doc to reMarkable

Uploaded the design document to the reMarkable under a ticket-scoped folder so it’s easy to find later and avoids naming collisions. I used `remarquee upload bundle` (even with a single input) so I could control the output name and get a clickable ToC in the generated PDF.

### What I did
- Confirmed remarquee tooling works: `remarquee status`.
- Dry-ran the bundle upload to confirm remote dir and output name.
- Uploaded the PDF bundle to `/ai/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE`.
- Verified the remote listing via `remarquee cloud ls`.

### Why
- The reMarkable is the preferred reading surface for long “textbook style” docs; uploading early enables review/annotation while implementation proceeds.

### What worked
- Upload completed successfully and the document is visible in the expected remote directory.

### What didn't work
- Initial attempt used `remarquee upload md --name ...` but `md` uploads do not support `--name`:
  - Error: `Error: unknown flag: --name`

### What I learned
- Use `remarquee upload bundle` when you need to control output document name (and/or ToC), even if there is only one Markdown input.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- Consider uploading the Diary too if we want the same “reading surface” for progress logs.

### Code review instructions
- On the device/cloud, browse to:
  - `/ai/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE/`
  - Document name: `MO-032 LED Pattern Engine + esp_console REPL (WS281x)`

### Technical details
- Commands:
  - `remarquee status`
  - `remarquee upload bundle --dry-run /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/design-doc/01-led-pattern-engine-esp-console-repl-ws281x.md --name "MO-032 LED Pattern Engine + esp_console REPL (WS281x)" --remote-dir "/ai/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE" --toc-depth 2`
  - `remarquee upload bundle /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/design-doc/01-led-pattern-engine-esp-console-repl-ws281x.md --name "MO-032 LED Pattern Engine + esp_console REPL (WS281x)" --remote-dir "/ai/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE" --toc-depth 2 --non-interactive`
  - `remarquee cloud ls /ai/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE --long --non-interactive`

## Step 7: Update Ticket Changelog

Recorded a concise changelog entry capturing the key artifacts produced in this session (imported simulator source, created design doc, reMarkable upload location). This makes the ticket timeline readable without requiring the reviewer to scan the Diary first.

### What I did
- Ran `docmgr changelog update` for MO-032 with file notes pointing at the design doc and imported `patterns.jsx`.

### Why
- Changelog entries act as the “executive summary over time”; they’re useful for later archaeology even if the diary is long.

### What worked
- Changelog updated successfully.

### What didn't work
- N/A

### What I learned
- `docmgr` prints a reminder to refresh relationships in impacted docs; in this session we already related the key source files directly to the design doc and diary.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- When implementation starts, add changelog entries per “done milestone” (e.g., queue protocol implemented, console verbs implemented, pattern correctness validated on hardware).

### Code review instructions
- Read `ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/changelog.md`.

### Technical details
- Command:
  - `docmgr changelog update --ticket MO-032-ESP32C6-LED-PATTERNS-CONSOLE --entry \"Added LED pattern engine + esp_console REPL design doc; imported patterns.jsx reference; uploaded PDF to reMarkable (/ai/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE).\" ...`

## Step 8: Create Implementation Task List (Docmgr)

Created a concrete task list in the ticket so implementation can proceed in small, check-off-able milestones (scaffold → driver → patterns → queue/task → console → build). This gives us a stable checklist that we can update as each incremental commit lands, which is especially useful when we’re enforcing “commit per step”.

### What I did
- Added tasks to the ticket with `docmgr task add`.
- Removed the placeholder “Add tasks here” line from the generated `tasks.md`.

### Why
- A structured task list makes it easier to keep the diary frequent and to ensure we validate via `idf.py build` at the right point.

### What worked
- `docmgr task add` appended tasks in `tasks.md` and printed stable task IDs for check-off.

### What didn't work
- N/A

### What I learned
- `docmgr task list` uses numeric IDs that are convenient to reference in diary/changelog entries.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- As each milestone is completed, run `docmgr task check --id ...` and record the matching commit hashes in the diary.

### Code review instructions
- Read task list:
  - `ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/tasks.md`

### Technical details
- Commands:
  - `docmgr task add --ticket MO-032-ESP32C6-LED-PATTERNS-CONSOLE --text "..."`
  - `docmgr task list --ticket MO-032-ESP32C6-LED-PATTERNS-CONSOLE`

## Step 9: Scaffold Firmware Project `0044` + Verify `idf.py build`

Created a new ESP32-C6 firmware project (`0044`) as the implementation home for MO-032, seeded with the proven WS281x RMT encoder code from `0043` and the correct console backend defaults (USB Serial/JTAG). I then ran a clean `idf.py set-target esp32c6` + `idf.py build` to make sure the scaffold compiles in our ESP-IDF 5.4.1 environment before layering on the pattern engine and console verbs.

This step establishes a buildable baseline commit so future changes can be validated incrementally.

**Commit (code):** dccb2cd — "0044: scaffold ESP32-C6 WS281x console"

### What I did
- Created directory `0044-xiao-esp32c6-ws281x-patterns-console/` with:
  - top-level `CMakeLists.txt`
  - `main/` with `main.c`, `Kconfig.projbuild`, component CMake, and `ws281x_encoder.[ch]`
  - `sdkconfig.defaults` with `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`
- Built the project:
  - `source ~/esp/esp-idf-5.4.1/export.sh`
  - `idf.py set-target esp32c6`
  - `idf.py build`
- Checked off the scaffold task (and corrected an initial mistaken check-off of the wrong task ID using `docmgr task uncheck`).

### Why
- A clean baseline build reduces risk: subsequent steps can focus on logic changes rather than build-system issues.

### What worked
- `idf.py build` completed successfully for target `esp32c6`.

### What didn't work
- I initially checked the wrong task ID due to the task list being renumbered after removing the placeholder line; fixed immediately with `docmgr task uncheck` + `docmgr task check`.

### What I learned
- Task IDs in `docmgr task list` are positional; removing/reordering tasks can shift IDs, so it’s important to re-run `docmgr task list` before checking.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- Keep `idf.py build` passing after each milestone commit (driver module, patterns, queue/task, console).

### Code review instructions
- Start at:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/main/main.c`
- Confirm USB Serial/JTAG console defaults:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/sdkconfig.defaults`

### Technical details
- Commands:
  - `bash -lc 'source ~/esp/esp-idf-5.4.1/export.sh && idf.py set-target esp32c6 && idf.py build'`
  - `docmgr task list --ticket MO-032-ESP32C6-LED-PATTERNS-CONSOLE`
  - `docmgr task check --ticket MO-032-ESP32C6-LED-PATTERNS-CONSOLE --id 1`
  - `docmgr task uncheck --ticket MO-032-ESP32C6-LED-PATTERNS-CONSOLE --id 2`

## Step 10: Implement `led_ws281x` Driver Wrapper Module (RMT + Encoder)

Implemented a reusable `led_ws281x` module that owns the RMT TX channel, WS281x encoder, and a wire-order pixel buffer. This pulls the critical “WS281x communication” pieces into a single place and provides a clean API (`init/deinit/clear/show/set_pixel`) that the upcoming pattern engine + animation task can build on.

I validated compilation immediately, captured a build failure (`portMAX_DELAY` undeclared), fixed it by adding the correct FreeRTOS include, and re-built successfully.

**Commit (code):** 485ed78 — "0044: add led_ws281x driver wrapper"

### What I did
- Added `0044-xiao-esp32c6-ws281x-patterns-console/main/led_ws281x.[ch]`.
- Updated `main/CMakeLists.txt` to compile the new module.
- Updated `main/main.c` to do a minimal bring-up:
  - init strip from Kconfig
  - clear + set pixel 0 to red
  - transmit one frame
- Ran `idf.py build` to verify.

### Why
- The pattern engine should not directly manage RMT handles; encapsulating driver lifetimes reduces complexity and makes “reinit on apply” straightforward later.

### What worked
- The driver wrapper cleanly reuses the existing `ws281x_encoder` implementation and compiles/builds on ESP-IDF 5.4.1.

### What didn't work
- Initial build failed:
  - Error: `portMAX_DELAY undeclared`
  - Root cause: missing `#include "freertos/FreeRTOS.h"` in `led_ws281x.c`.
  - Fix: added the include and rebuilt.

### What I learned
- Even when a translation unit includes ESP-IDF headers, you still need an explicit FreeRTOS include for `portMAX_DELAY`.

### What was tricky to build
- Getting the “ownership” boundary right: the driver now owns the pixel buffer, which simplifies the animation task design but requires careful sequencing when we later add driver reinit.

### What warrants a second pair of eyes
- Confirm we want the driver module to own the pixel buffer (vs. the animation task owning it). Either is valid; the later queue/task design must match.

### What should be done in the future
- Add staged-vs-applied driver config and a safe `apply()` reinit path (per design doc).

### Code review instructions
- Review driver wrapper:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/main/led_ws281x.c`
- Review minimal bring-up usage:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/main/main.c`

### Technical details
- Build command:
  - `bash -lc 'source ~/esp/esp-idf-5.4.1/export.sh && idf.py build'`
- Task check:
  - `docmgr task check --ticket MO-032-ESP32C6-LED-PATTERNS-CONSOLE --id 2`
