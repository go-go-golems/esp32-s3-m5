---
Title: Diary
Ticket: 0014-CARDPUTER-JS
Status: active
Topics:
    - esp32s3
    - esp-idf
    - cardputer
    - javascript
    - microquickjs
    - qemu
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-29T13:24:53.129865004-05:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Document the step-by-step process of analyzing and porting the MicroQuickJS REPL firmware to Cardputer, focusing on actions taken, decisions made, challenges encountered, and lessons learned.

## Step 1: Initial Exploration and Documentation Setup

Started by exploring the imported firmware and setting up documentation structure to track findings and analysis.

### What I did

- Created ticket `0014-CARDPUTER-JS` using docmgr
- Created diary document for tracking process
- Examined imported firmware structure in `imports/esp32-mqjs-repl/`
- Read delivery documentation to understand what was delivered
- Searched codebase for Cardputer hardware specs and console patterns

### Why

Need to establish baseline understanding before making changes. Also wanted to capture analysis in structured documents rather than ad-hoc notes.

### What worked

- Found comprehensive delivery docs (`DELIVERY_SUMMARY.md`, `FINAL_DELIVERY_README.md`) that explain the firmware well
- Located Cardputer reference implementations (`0015-cardputer-serial-terminal`) showing USB Serial JTAG usage
- Identified key differences: console interface (UART vs USB Serial JTAG), flash size (2MB vs 8MB), memory constraints

### What didn't work

- Initially put too much detailed analysis in the diary - user correctly pointed out that analysis should be in separate documents
- Had to restructure: moved detailed technical analysis to separate analysis documents

### What I learned

- ESP-IDF has distinction between "console output" (where `printf` goes) and "REPL" (interactive command loop). Current firmware uses raw UART driver, not `esp_console` framework.
- Cardputer tutorials consistently use 8MB flash, 4MB app partition, 1MB storage partition pattern
- USB Serial JTAG requires driver installation before use, different from UART which is always available

### What was tricky

- Understanding the console vs REPL distinction - had to search multiple examples to clarify
- Determining what belongs in diary vs analysis documents - user feedback helped clarify

### What warrants a second pair of eyes

- Memory budget calculation: need to verify 64KB JS heap + buffers fits in Cardputer's 512KB SRAM
- Console migration approach: should we keep UART option for QEMU testing?

### What should be done in the future

- Create separate analysis documents for detailed technical findings (done)
- Test QEMU execution of current firmware to verify it works
- Create implementation plan document outlining code changes needed

### Code review instructions

- Review analysis documents: `analysis/01-current-firmware-configuration-analysis.md` and `analysis/02-cardputer-port-requirements.md`
- Check imported firmware: `imports/esp32-mqjs-repl/mqjs-repl/main/main.c`
- Compare with Cardputer reference: `0015-cardputer-serial-terminal/main/hello_world_main.cpp`

## Step 2: Make builds reproducible + run the baseline build

This step makes it easy to build the imported `mqjs-repl` project without remembering to source ESP-IDF in each shell. It also validates that the project still compiles cleanly in *this* environment before we start Cardputer-specific changes.

**Commit (code):** 1a42a4e7049383498a3473765889c219815137ad — "Build: add ESP-IDF auto-source wrapper for mqjs-repl"

### What I did

- Found the repo’s standard “auto-source ESP-IDF” wrapper script at:
  - `0017-atoms3r-web-ui/build.sh`
  - `0016-atoms3r-grove-gpio-signal-tester/build.sh`
- Copied that pattern into the imported project as `imports/esp32-mqjs-repl/mqjs-repl/build.sh` and committed it.
- Ran a baseline build using the wrapper:

```bash
cd imports/esp32-mqjs-repl/mqjs-repl
./build.sh set-target esp32s3
./build.sh build
```

- Attempted a QEMU run (to validate the workflow on this machine):

```bash
./build.sh qemu monitor
```

### Why

- I want every subsequent compile step to be “one command”, so we can iterate quickly and keep commits tightly scoped.
- Before porting to Cardputer, we need a known-good baseline build in our local toolchain (ESP-IDF 5.4.1).

### What worked

- `./build.sh set-target esp32s3 && ./build.sh build` succeeded and produced `build/mqjs-repl.bin`.

### What didn’t work

- QEMU run failed with:
  - `qemu-system-xtensa is not installed. Please install it using "python $IDF_PATH/tools/idf_tools.py install qemu-xtensa"`

### What I learned

- The wrapper script does what we want: it detects `ESP_IDF_VERSION` and sources `~/esp/esp-idf-5.4.1/export.sh` only when needed.
- On this environment, ESP-IDF is installed, but the **QEMU Xtensa binary** is not yet installed via `idf_tools.py`.

### What was tricky

- Keeping work scoped: the git repo has unrelated local changes, so every commit needs explicit `git add <paths...>` to avoid staging noise.

### What warrants a second pair of eyes

- Whether we want to keep QEMU support as a first-class workflow for Cardputer (USB Serial JTAG vs UART/monitoring differences may matter).

### What should be done in the future

- Install the required ESP-IDF QEMU tool (`qemu-xtensa`) and re-run `./build.sh qemu monitor` to confirm we can boot into the REPL before starting the Cardputer port.
