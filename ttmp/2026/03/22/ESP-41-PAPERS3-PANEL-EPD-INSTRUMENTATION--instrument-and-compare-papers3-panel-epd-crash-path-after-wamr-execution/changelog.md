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
