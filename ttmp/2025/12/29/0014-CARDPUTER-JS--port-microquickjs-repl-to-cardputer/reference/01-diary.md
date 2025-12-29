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
