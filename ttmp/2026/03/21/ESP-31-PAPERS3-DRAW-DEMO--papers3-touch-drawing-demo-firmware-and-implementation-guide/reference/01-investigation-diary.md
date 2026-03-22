---
Title: Investigation diary
Ticket: ESP-31-PAPERS3-DRAW-DEMO
Status: active
Topics:
    - esp32-s3
    - esp32s3
    - firmware
    - m5stack
    - m5gfx
    - ui
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0075-papers3-touch-draw-demo/CMakeLists.txt
      Note: Captured the component path failure and fix
    - Path: 0075-papers3-touch-draw-demo/README.md
      Note: Recorded build and flash commands
    - Path: 0075-papers3-touch-draw-demo/main/app_main.cpp
      Note: Implementation log and review instructions
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-21T20:02:37.400873467-04:00
WhatFor: ""
WhenToUse: ""
---


# Investigation diary

## Goal

Record the implementation work for the new PaperS3 drawing demo, including what was inspected, what changed, what failed, how it was fixed, and how another engineer should review the result.

## Context

The user requested a new PaperS3 demo using the donor `M5PaperS3-UserDemo` for display and GT911 touch knowledge, pinned to ESP-IDF 5.3.4, plus full ticket documentation and a reMarkable upload.

## Quick Reference

### 2026-03-21 20:00 EDT - Initial discovery

Commands run:

```bash
pwd
rg --files
rg --files /home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo
docmgr status --summary-only
```

What worked:

- confirmed the repo already has `ttmp/`
- confirmed the donor PaperS3 app exists locally
- confirmed `docmgr` is configured for this workspace

Key findings:

- top-level project numbering ends at `0074`, so `0075` is a clean next slot
- donor project includes local `M5GFX`, `M5Unified`, `mooncake`, and `mooncake_log`

### 2026-03-21 20:05 EDT - Donor code inspection

Commands run:

```bash
sed -n '1,220p' /home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/main/hal/hal.h
sed -n '1,260p' /home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/main/hal/hal.cpp
sed -n '1,220p' /home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/main/main.cpp
```

What worked:

- identified the minimal initialization sequence
- confirmed touch access pattern through `M5.Touch`

Key findings:

- donor HAL uses `M5.begin(); M5.Display.setRotation(1);`
- donor touch helper simply wraps `M5.Touch.getCount()` and `M5.Touch.getDetail()`

### 2026-03-21 20:10 EDT - Ticket creation

Commands run:

```bash
docmgr ticket create-ticket --ticket ESP-31-PAPERS3-DRAW-DEMO --title "PaperS3 touch drawing demo firmware and implementation guide" --topics esp32-s3,esp32s3,firmware,m5stack,m5gfx,ui
docmgr doc add --ticket ESP-31-PAPERS3-DRAW-DEMO --doc-type design-doc --title "PaperS3 touch draw demo detailed implementation plan"
docmgr doc add --ticket ESP-31-PAPERS3-DRAW-DEMO --doc-type design-doc --title "PaperS3 touch draw demo analysis design and implementation guide"
docmgr doc add --ticket ESP-31-PAPERS3-DRAW-DEMO --doc-type reference --title "Investigation diary"
```

What worked:

- ticket and document skeletons were created successfully

### 2026-03-21 20:15 EDT - IDF 5.3.4 verification

Command run:

```bash
find /home/manuel/esp -maxdepth 2 -type d | rg 'esp-idf-5\.3\.4|esp-idf'
```

What worked:

- confirmed `/home/manuel/esp/esp-idf-5.3.4` exists locally

### 2026-03-21 20:20 EDT - New firmware project creation

Files added:

- `0075-papers3-touch-draw-demo/CMakeLists.txt`
- `0075-papers3-touch-draw-demo/main/CMakeLists.txt`
- `0075-papers3-touch-draw-demo/main/app_main.cpp`
- `0075-papers3-touch-draw-demo/sdkconfig.defaults`
- `0075-papers3-touch-draw-demo/partitions.csv`
- `0075-papers3-touch-draw-demo/dependencies.lock`
- `0075-papers3-touch-draw-demo/README.md`

Implementation choice:

- reuse donor components via `EXTRA_COMPONENT_DIRS`
- keep the app in one file for tutorial clarity

### 2026-03-21 20:25 EDT - First build attempt failed

Command run:

```bash
source /home/manuel/esp/esp-idf-5.3.4/export.sh && idf.py set-target esp32s3 && idf.py build
```

What did not work:

- CMake failed because the donor component path was one directory too shallow

Observed error:

```text
Directory specified in EXTRA_COMPONENT_DIRS doesn't exist:
/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/M5PaperS3-UserDemo/components
```

Fix:

- changed `../M5PaperS3-UserDemo/components` to `../../M5PaperS3-UserDemo/components`

### 2026-03-21 20:27 EDT - Build succeeded after path fix

Command run:

```bash
source /home/manuel/esp/esp-idf-5.3.4/export.sh && idf.py build
```

What worked:

- full build completed successfully

Key result:

- `papers3_touch_draw_demo.bin` size: `0x667f0`
- free space in smallest app partition: about `90%`

### 2026-03-21 20:35 EDT - Documentation and bookkeeping

Work completed:

- wrote the detailed implementation plan
- wrote the intern-oriented analysis/design/implementation guide
- updated index, tasks, and changelog
- prepared file relations and validation/upload steps

## Usage Examples

### Build

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0075-papers3-touch-draw-demo
source /home/manuel/esp/esp-idf-5.3.4/export.sh
idf.py build
```

### Flash

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0075-papers3-touch-draw-demo
source /home/manuel/esp/esp-idf-5.3.4/export.sh
idf.py -p /dev/ttyACM0 flash monitor
```

## What Worked

- donor bring-up knowledge mapped cleanly into a much smaller project
- local donor components avoided any need for network dependency fetching
- the build completed successfully with ESP-IDF 5.3.4
- the single-file runtime stayed readable while still separating responsibilities by method

## What Did Not Work

- the first `EXTRA_COMPONENT_DIRS` path was wrong

Why it failed:

- the new project lives two levels deeper than the donor, not one

## What Was Tricky To Build

- The main subtlety was the e-paper update model. The code has to match the driver’s expectation that `startWrite()`/`endWrite()` bound an EPD update transaction.
- The second subtlety was routing touch gestures so the clear button does not also leave draw marks.
- The third subtlety was using a clip rect so brush stamps cannot bleed into the header area.

## Verification

Verified in this session:

- `idf.py build` using `/home/manuel/esp/esp-idf-5.3.4/export.sh`

Not yet verified in this session:

- flashing to a real PaperS3
- physical touch responsiveness
- physical clear-button behavior

## Code Review Instructions

Review in this order:

1. `0075-papers3-touch-draw-demo/CMakeLists.txt`
   - confirm donor component path is correct
2. `0075-papers3-touch-draw-demo/sdkconfig.defaults`
   - confirm USB Serial/JTAG and ESP32-S3 settings
3. `0075-papers3-touch-draw-demo/main/app_main.cpp`
   - confirm layout code
   - confirm `handleTouch()` gesture routing
   - confirm `drawBrushStroke()` mode selection and clipping
4. `0075-papers3-touch-draw-demo/README.md`
   - confirm exact build and flash commands use IDF 5.3.4
5. Ticket design docs
   - confirm all major claims map back to files and line references

## Related

- `../design-doc/01-papers3-touch-draw-demo-detailed-implementation-plan.md`
- `../design-doc/02-papers3-touch-draw-demo-analysis-design-and-implementation-guide.md`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0075-papers3-touch-draw-demo`

## 2026-03-21 21:15 EDT - Commit packaging backfill

Commands run:

```bash
find 0075-papers3-touch-draw-demo -maxdepth 2 -type f | sort
git status --short 0075-papers3-touch-draw-demo
```

What worked:

- confirmed the project contained generated local files that should not be committed
- added a local `.gitignore` so the commit can stay focused on source and docs

What was intentionally excluded from git:

- `build/`
- generated `sdkconfig`
- local `.envrc`

Why:

- these are machine-local or reproducible artifacts and would add noise to review

Review note:

- the committed project is intended to be rebuilt from `sdkconfig.defaults`, not to carry local build outputs
