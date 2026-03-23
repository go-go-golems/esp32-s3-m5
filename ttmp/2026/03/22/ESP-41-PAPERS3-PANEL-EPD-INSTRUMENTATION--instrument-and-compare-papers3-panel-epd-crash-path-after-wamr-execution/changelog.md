# Changelog

## 2026-03-22

- Initial workspace created
- Added the initial design plan, task list, and diary for the PaperS3 `Panel_EPD` investigation
- Reconfirmed that the first direct crash choke point is the nibble-write loop in `Panel_EPD::writeFillRectPreclipped(...)`, writing into a PSRAM-backed framebuffer `_buf`
- Reconfirmed from local history that the current vendored PaperS3 backend already contains the older PSRAM/cache fix `c899961`, so the next slice starts with instrumentation rather than assuming that old fix is still missing
- Added bounded `Panel_EPD` instrumentation in the nested `M5GFX` repo and a ticket-local flash/probe script
- Observed that the PaperS3 startup splash path already reaches `writeFillRectPreclipped(...)` with out-of-range framebuffer math before any Wasm command is run
- Confirmed that fresh-boot `wasm replay clear-only` and fresh-boot `wasm replay frame-no-clear` can both succeed with the instrumented driver
- Confirmed that same-boot contamination still reproduces: `wasm run-preflush return-42` succeeds, but a following `wasm replay clear-only` crashes with `Cache disabled but cached memory region accessed`
- Added a repo-level serial-ownership rule to `AGENTS.md` after catching and correcting an invalid parallel-probe attempt against the same `/dev/ttyACM0`
- Decoded the contamination crash against the exact `0079` ELF and reconfirmed that it still dies in `Panel_EPD::writeFillRectPreclipped(...)` through `PaperCanvasScreenClear()`
- Added tighter app-side flush and canvas-state probes plus a driver-side log-budget reset hook so replay runs can capture their own `Panel_EPD` entry instead of losing the budget to startup noise
- Proved that fresh-boot `clear-only` success and post-WAMR `clear-only` crash reach `Panel_EPD` with the same logged geometry and visible mode/frame state:
  - `xs=0 ys=0 xe=959 ye=539 w=960 h=540`
  - `mode=3`
  - `_buf=0x3c17ddc0`
  - `last=259199 len=259200`
  - `busy=0`
- That means the first concrete divergence is not in the currently logged app-side frame lifecycle or in obviously bad framebuffer bounds; it is below that level, likely in lower-level cache/PSRAM/driver state not yet exposed by the current probes
- Added a non-display `psram-scratch` control probe in `wasm_replay_control.cpp` that allocates a PSRAM buffer and performs the same nibble-write pattern as the EPD clear path without touching `M5GFX`
- Confirmed that fresh-boot `wasm replay psram-scratch` succeeds and reports a stable checksum from an external-RAM buffer
- Confirmed that same-boot `wasm run-preflush return-42` followed by `wasm replay psram-scratch` crashes with the same `Cache disabled but cached memory region accessed` class of panic
- That result narrows the active bug further: the surviving contamination is broader than `Panel_EPD` and now looks like post-WAMR PSRAM/cache poisoning on PaperS3
- Fixed a command-surface bug in `wasm_command.cpp` so the headless-compatible `psram-scratch` control can run even when the display host API is disabled
- Confirmed in the true headless PaperS3 build that same-boot `wasm run-preflush return-42` followed by `wasm replay psram-scratch` still crashes
- Decoded the headless crash against the headless ELF and pinned it to `RunPsramScratchProbe(...)` at the initial `memset(...)`, proving that display initialization is not required for the contamination and that the current live boundary is broader post-WAMR PSRAM access on PaperS3
- Added an `instantiate-only` Wasm lifecycle mode so the runtime can stop after load/instantiate/lookup/exec-env creation and cleanup without ever calling guest code
- Confirmed on the headless PaperS3 build that same-boot `wasm instantiate-only return-42` followed by `wasm replay psram-scratch` still crashes
- That means actual guest execution is not required for the repro; instantiate/teardown alone is already enough to poison later PSRAM writes on PaperS3
