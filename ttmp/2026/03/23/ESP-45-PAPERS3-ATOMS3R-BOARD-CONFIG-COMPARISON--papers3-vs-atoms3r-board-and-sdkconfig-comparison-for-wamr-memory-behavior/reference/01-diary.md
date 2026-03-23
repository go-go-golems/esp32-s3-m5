---
Title: Diary
Ticket: ESP-45-PAPERS3-ATOMS3R-BOARD-CONFIG-COMPARISON
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
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-23T19:49:00-04:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Keep the board-comparison track separate from the loader root-cause track so the evidence stays legible.

## 2026-03-23 15:05 EDT

Created this ticket because the board/config comparison is still useful, but it is now secondary. `ESP-44` is about the direct embedded-buffer mechanism. This ticket is about the environment in which that mechanism fails on PaperS3 but not on AtomS3R.

## 2026-03-23 15:48 EDT

Started the comparison with a reusable local script rather than another one-off shell diff:

- `scripts/compare_memory_configs.py`

The script compares the active AtomS3R `sdkconfig` against the active PaperS3 internal-pool `sdkconfig.variant` for flash, CPU, PSRAM, console, and WAMR-related settings.

## 2026-03-23 15:50 EDT

The first config result is more interesting for what it does **not** show than for what it does.

Shared settings:

- both are `esp32s3`
- both use octal PSRAM at `40 MHz`
- both use `80 MHz` DIO flash mode in config
- both use USB Serial/JTAG console
- both use the same narrow WAMR interpreter feature set

Important differences:

- AtomS3R config uses `8MB` flash, PaperS3 uses `16MB`
- AtomS3R config sets CPU frequency to `240 MHz`, while the active PaperS3 build is `160 MHz`
- PaperS3 has the current embedded-load mitigation and allocator-control flags; AtomS3R does not because it is a different probe project

The important interpretation is that the obvious PSRAM-mode settings are not diverging in a way that immediately explains the bug. The config-level evidence so far points more toward board topology and flash source handling than toward a simple PSRAM-mode mismatch.

## 2026-03-23 15:55 EDT

Pulled the official M5 docs for both boards.

The most relevant hardware distinction is:

- PaperS3: `ESP32-S3R8` with `8MB PSRAM` and **external `16MB` flash**
- AtomS3R-M12: `ESP32-S3-PICO-1-N8R8` with `8MB Flash` and `8MB PSRAM` integrated in the SiP

That does not prove causality on its own, but it gives the loader-root-cause theory a more concrete board backdrop. If WAMR is mutating a flash-mapped source buffer in place, the PaperS3 external-flash arrangement is a more plausible place for ugly side effects than a RAM copy or a different S3 packaging.

## 2026-03-23 18:05 EDT

Starting the board cross-check phase after `ESP-44` got the loader-root-cause theory to a much stronger state.

The Atom question is now very specific:

- does AtomS3R take the same `wasm_const_str_list_insert(...)` in-place rewrite path for direct embedded `return-42`?
- if yes, does the board survive it anyway?

That is a better comparison than a generic board/spec diff because it lets the same WAMR behavior be tested against two hardware environments.

To keep the comparison honest, I am porting only the minimum proof surface from `0082` into `0081`:

- `empty-module.wasm` as a stringless direct-embedded control
- `wasm load-only-embedded-direct <name>`
- `wasm load-only-embedded-direct-freeable <name>`
- minimal persistent-PSRAM init/touch-sync controls
- the same bounded WAMR const-string rewrite trace, preserved under this ticket’s `scripts/wamr-patches/`

That keeps the Atom experiment focused on the same question PaperS3 already answered instead of growing another broad debug harness.

## 2026-03-23 18:17 EDT

Ported the minimum proof surface into `0081`:

- embedded `empty-module.wasm`
- `wasm load-only-embedded-direct <name>`
- `wasm load-only-embedded-direct-freeable <name>`
- minimal `psram-persistent-init` / `psram-persistent-touch-sync` controls
- the same bounded `wasm_const_str_list_insert(...)` trace in the local ignored WAMR component, preserved under:
  - `scripts/wamr-patches/01-wasm-runtime-const-str-trace.diff`

This intentionally avoids porting the whole allocator-control harness from `0082`. The goal is a board comparison, not another project fork.

## 2026-03-23 18:23 EDT

Hit two small compile issues while shrinking the port:

- first build failed because the new PSRAM probe used `esp_cache.h`, so `main/CMakeLists.txt` needed `esp_mm` in `REQUIRES`
- second build failed because the reduced probe file was missing `esp_memory_utils.h` and `esp_private/esp_cache_private.h`

Both were straightforward and useful. They confirm the Atom probe is now using the same cache/alignment helpers as the PaperS3 control firmware instead of a subtly different path.

## 2026-03-23 18:30 EDT

The firmware now builds and flashes cleanly on AtomS3R, but the board exposes a new console-attach problem:

- `idf.py monitor` resets the board into `boot:0x31 (DOWNLOAD(USB/UART0))`
- stock `pyserial.Serial(...)` does the same
- even a careful open with `dtr=False` / `rts=False` still lands in ROM download mode

So the current blocker is not WAMR behavior yet. It is AtomS3R console attach over USB Serial/JTAG. I also confirmed this is not just a probe-script issue, because `idf_monitor` reproduces the same reset-to-download behavior.

At this point the Atom cross-check firmware is ready, but I need a clean way to get the Atom application running while the serial session remains attached. The most likely next step is to hold a monitor open and have the device manually reset once, or otherwise adjust the board-specific attach/reset behavior before trusting any comparison result.

## 2026-03-23 18:44 EDT

Recorded a process lesson from the Atom attach failure.

For this repo, if a board is already showing “attach resets into ROM download mode” behavior, I should not keep retrying quietly. The correct collaboration pattern is:

- start the monitor or probe session in the right one-owner configuration
- then explicitly ask the user for the manual reset step
- only after that trust the resulting boot/probe evidence

I also added this as a repo-level instruction in `AGENTS.md` so future sessions do not relearn it by trial and error.

## 2026-03-23 19:12 EDT

Resumed the Atom cross-check using the older known-good workflow from `ESP-40` instead of trying to fight `idf.py monitor`.

The important operational correction was:

- flash with `--before usb_reset --after no_reset`
- then force the app boot with `--after watchdog_reset`
- only then run a single-owner probe session against the already-running app

That got the Atom back into a stable `boot:0x18 (SPI_FAST_FLASH_BOOT)` app boot and restored trust in the comparison environment. I used `wasm status` first as a sanity check before the actual root-cause probes.

## 2026-03-23 19:20 EDT

Ran the first decisive same-boot Atom sequence:

- `wasm replay psram-persistent-init`
- `wasm load-only-embedded-direct return-42`
- `wasm replay psram-persistent-touch-sync`

This changed the interpretation of the whole board-comparison ticket.

AtomS3R did **not** survive the direct embedded `return-42` path. It logged the same bounded WAMR in-place string mutation events as PaperS3:

- `len=3` for `run`
- `len=6` for `memory`

and then crashed on the later PSRAM touch with the same `Cache disabled but cached memory region accessed` family of failure.

That means the previous working theory in this ticket, “maybe PaperS3 is uniquely sensitive and AtomS3R survives the same bad write,” is no longer tenable. The bad WAMR rewrite path is toxic on AtomS3R too.

The board difference story is therefore much weaker than it looked a few hours ago. The most important difference now is not “PaperS3 external flash versus Atom SiP flash.” The important commonality is “both boards are executing the same in-place rewrite against the same kind of embedded flash-mapped input buffer.”

## 2026-03-23 19:27 EDT

Ran the matching Atom freeable control:

- `wasm replay psram-persistent-init`
- `wasm load-only-embedded-direct-freeable return-42`
- `wasm replay psram-persistent-touch-sync`

This one succeeded cleanly.

The control matters because it preserves the same direct embedded source pointer while changing the WAMR loader mode to the `binary_freeable` path. In practice, that means:

- no `wamr_const_str.stage=mutate-in-place` logs
- no later PSRAM crash
- later persistent PSRAM touch succeeds

So AtomS3R now matches PaperS3 on the point that actually matters:

- direct embedded `return-42` with source-buffer mutation: bad
- direct embedded `binary_freeable` path without source-buffer mutation: good

The broad conclusion is now cross-board rather than board-specific.

## 2026-03-23 19:38 EDT

This comparison ticket is still useful, but its value has changed.

What it now says is:

- the original “maybe this is only PaperS3 board topology” explanation was a useful hypothesis, but it is no longer supported by the direct experiment
- the root cause is better modeled as a runtime/loader misuse of embedded flash-mapped Wasm input buffers on both boards
- any remaining board differences are secondary details about how the resulting failure manifests, not the primary cause of the bug

That is a good debugging outcome even though it invalidated one of our intermediate stories. The point of this ticket was to test the board theory honestly, and the result is that the board theory does **not** explain the core bug.

## 2026-03-23 19:49 EDT

Used AtomS3R for one more comparison step after `ESP-44` isolated the loader mechanism more precisely.

I patched the local WAMR loader in `0081` so direct embedded `runtime-load` still used the original embedded source buffer, but forcibly disabled:

- `reuse_const_strings`

Then I reflashed the Atom and reran the previously bad sequence:

- `wasm replay psram-persistent-init`
- `wasm load-only-embedded-direct return-42`
- `wasm replay psram-persistent-touch-sync`

Result: success.

That is important for the board-comparison ticket because it closes the most plausible remaining escape hatch for the old board-specific theory. The same Atom board that previously crashed on direct embedded `return-42` stopped crashing as soon as the in-place const-string reuse was removed, without needing a copied buffer and without needing `binary_freeable`.

So the board ticket can now say something stronger than “the board theory looks weak.” It can say:

- the board theory failed its best direct control
- the runtime/loader theory passed its strongest control

Any remaining board differences are now secondary details about manifestation or tolerance, not the primary explanation of the bug.
