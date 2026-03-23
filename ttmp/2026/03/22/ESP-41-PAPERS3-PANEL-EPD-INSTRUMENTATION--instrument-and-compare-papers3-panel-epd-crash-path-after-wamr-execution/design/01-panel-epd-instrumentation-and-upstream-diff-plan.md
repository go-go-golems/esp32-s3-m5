---
Title: Panel_EPD Instrumentation and Upstream Diff Plan
Ticket: ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION
Status: active
Topics:
    - papers3
    - wasm
    - firmware
    - esp-idf
    - debugging
    - display
    - m5gfx
DocType: design
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-22T22:42:55.200508956-04:00
WhatFor: ""
WhenToUse: ""
---
---
Title: Panel_EPD Instrumentation and Upstream Diff Plan
Ticket: ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION
Status: active
Topics:
    - papers3
    - wasm
    - firmware
    - esp-idf
    - debugging
    - display
    - m5gfx
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp
      Note: PaperS3 canvas bridge that leads directly into M5GFX EPD calls
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp
      Note: Current crash choke point and the main target for instrumentation
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.hpp
      Note: Declares the EPD update task, PSRAM buffers, and queue state used by the backend
Summary: Plan for instrumenting the PaperS3 EPD write path and comparing it to newer upstream PaperS3 changes.
LastUpdated: 2026-03-22T22:38:00-04:00
---

# Goal

Narrow the remaining PaperS3 failure from "display work after WAMR is unsafe" to a more concrete driver-level explanation by instrumenting the `Panel_EPD` choke point and comparing the local backend to newer upstream PaperS3 changes.

# Context

The current evidence already rules out several broader theories:

- generic ESP32-S3 WAMR bring-up is not the problem, because AtomS3R succeeds with the recovered Espressif WAMR integration
- plain Wasm execution on PaperS3 is not the problem, because the new headless PaperS3 control build runs `return-42` successfully without touching our display path
- the failure is not limited to `screenClear()`, because both `clear-only` and `frame-no-clear` converge on the same PaperS3 EPD backend

The live choke point is the nibble-write loop in [Panel_EPD.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp), where `_buf` is a PSRAM-backed framebuffer allocated with `heap_caps_aligned_alloc(..., MALLOC_CAP_SPIRAM)`.

# Quick Reference

## Current call chain

```text
wasm command / replay
  -> wasm_host_api.cpp queue + flush
  -> papers3_canvas.cpp
  -> M5.Display.fillScreen / drawRect / fillRect
  -> Panel_EPD::writeFillRectPreclipped(...)
  -> _buf[...] nibble writes into PSRAM-backed framebuffer
```

## Immediate hypotheses worth testing

- `_buf` pointer or row math is wrong or stale at first post-WAMR draw
- PSRAM/cached write assumptions around `_buf` are no longer valid after successful WAMR execution
- EPD update/task state or display mode transitions leave `Panel_EPD` in a bad state before the next direct draw primitive
- a newer upstream PaperS3 refresh change materially alters the unsafe path

## Planned instrumentation points

- `Panel_EPD::writeFillRectPreclipped(...)`
  - rect coordinates
  - `_cfg.panel_width`, computed row stride, computed buffer base
  - `_buf` pointer and whether it looks like external RAM
  - `_epd_mode`
  - first-entry-only breadcrumbs so logs stay bounded
- optional supporting points:
  - `Panel_EPD::display(...)`
  - `Panel_EPD::waitDisplay(...)`
  - `papers3_canvas.cpp` frame begin/end boundaries

## Planned probe matrix

- clean boot `wasm replay clear-only`
- clean boot `wasm replay frame-no-clear`
- clean boot `wasm run-preflush return-42`
- same boot `wasm run-preflush return-42` then `wasm replay clear-only`
- same boot `wasm run-preflush return-42` then `wasm replay frame-no-clear`

# Usage Examples

## Review the exact choke point

```bash
sed -n '340,390p' \
  /home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp
```

## Confirm PSRAM framebuffer allocation

```bash
rg -n "_buf =|MALLOC_CAP_SPIRAM|cacheWriteBack" \
  /home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp
```

## Rebuild after a nested-repo M5GFX patch

```bash
unset IDF_PYTHON_ENV_PATH IDF_PATH
source /home/manuel/esp/esp-idf-5.3.4/export.sh >/dev/null
idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console build
```
