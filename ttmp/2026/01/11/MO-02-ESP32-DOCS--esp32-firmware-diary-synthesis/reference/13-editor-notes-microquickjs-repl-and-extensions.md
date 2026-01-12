---
Title: 'Editor Notes: MicroQuickJS REPL and Extensions'
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - esp32
    - firmware
    - documentation
    - javascript
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/ttmp/2026/01/11/MO-02-ESP32-DOCS--esp32-firmware-diary-synthesis/playbook/17-microquickjs-repl-running-javascript-on-esp32-s3-with-native-extensions.md
      Note: The final guide document to review
ExternalSources: []
Summary: 'Editorial review checklist for the MicroQuickJS REPL guide.'
LastUpdated: 2026-01-12T15:00:00-05:00
WhatFor: 'Help editors verify the guide is accurate and useful.'
WhenToUse: 'Use when reviewing the guide before publication.'
---

# Editor Notes: MicroQuickJS REPL and Extensions

## Purpose

Editorial checklist for reviewing the MicroQuickJS REPL guide before publication.

---

## Target Audience Check

The guide is written for developers who:
- Have ESP-IDF experience
- Want to run JavaScript on ESP32-S3
- Want to add native C bindings callable from JS

**Review questions:**
- [ ] Is the REPL architecture explained clearly enough to understand where each component lives?
- [ ] Can someone add a new native binding by following the guide?
- [ ] Are SPIFFS autoload and `load()` semantics documented?

---

## Structural Review

### Required Sections

- [ ] **What is MicroQuickJS** — Brief context for the engine
- [ ] **REPL architecture** — Console, line editor, loop, evaluator
- [ ] **Console backend selection** — USB Serial/JTAG vs UART
- [ ] **Running the baseline REPL** — Build/flash/use
- [ ] **Meta-commands** — All `:` commands documented
- [ ] **JS mode basics** — Evaluating code, built-in functions
- [ ] **SPIFFS and autoload** — How scripts are loaded at boot/runtime
- [ ] **Adding native bindings** — Extension registration pattern
- [ ] **GPIO/I2C example** — Concrete extension from 0039
- [ ] **Troubleshooting** — Common issues

### Flow and Readability

- [ ] Does the guide progress logically from basics to extensions?
- [ ] Is the architecture diagram clear?
- [ ] Are code snippets contextualized?

---

## Accuracy Checks

### Claims to Verify Against Source Code

| Claim in Guide | Verify In | What to Check |
|----------------|-----------|---------------|
| REPL uses USB Serial/JTAG by default | Kconfig.projbuild | `CONFIG_MQJS_REPL_CONSOLE_*` |
| JS heap is 64 KiB | JsEvaluator.cpp | `kJsMemSize` |
| `load()` has 32 KiB limit | esp32_stdlib_runtime.c | Size check |
| Autoload scans `/spiffs/autoload/*.js` | JsEvaluator.cpp | `Autoload()` |
| `gpio.high(pin)` sends control event | esp32_stdlib_runtime.c | `js_gpio_high` |

- [ ] All claims verified against source code

### Code Snippets

| Snippet | Check |
|---------|-------|
| REPL task creation | Matches `app_main.cpp` |
| Mode switching | Matches `ModeSwitchingEvaluator.cpp` |
| Meta-command handling | Matches `ReplLoop.cpp` |
| Native binding example | Matches `esp32_stdlib_runtime.c` |

- [ ] Code snippets match source or are clearly labeled as simplified

---

## Completeness Checks

### Topics That Should Be Covered

| Topic | Covered? | Notes |
|-------|----------|-------|
| REPL component roles | | |
| Console abstraction | | |
| Line editor behavior | | |
| Mode switching | | |
| JS evaluation | | |
| Memory model (64 KiB arena) | | |
| SPIFFS mount behavior | | |
| Autoload formatting | | |
| `load()` function | | |
| Built-in functions (print, gc) | | |
| Native binding registration | | |
| Control plane pattern | | |
| GPIO extension example | | |
| I2C extension example | | |

- [ ] All essential topics covered

### Potential Missing Information

- [ ] How to regenerate stdlib tables (`gen_esp32_stdlib.sh`)
- [ ] How to add new evaluator modes
- [ ] History/arrow key support (intentionally minimal)

---

## Code Command Verification

Commands that should be tested:

```bash
# Baseline REPL
cd esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl
./build.sh set-target esp32s3
./build.sh build flash monitor

# GPIO exercizer REPL
cd esp32-s3-m5/0039-cardputer-adv-js-gpio-exercizer
idf.py set-target esp32s3
idf.py build flash monitor
```

REPL session:
```
repl> :mode js
mode: js
js> print("hello")
hello
js> :autoload --format
autoload: /spiffs/autoload/00-seed.js
js> globalThis.AUTOLOAD_SEED
123
```

GPIO exercizer session:
```
js> gpio.high(1)
js> gpio.square(2, 1000)
js> gpio.stop()
```

- [ ] Build commands verified to work
- [ ] REPL commands produce expected results

---

## Tone and Style

- [ ] Consistent terminology (REPL, evaluator, console, transport)
- [ ] Active voice preferred
- [ ] Clear distinction between baseline REPL and extensions
- [ ] Warnings about SPIFFS formatting are prominent

---

## Final Review Questions

1. **Could someone run the baseline REPL from the guide alone?**

2. **Could someone add a new native JS function by following the extension pattern?**

3. **Is the console backend selection explained (USB vs UART)?**

4. **Are the SPIFFS autoload semantics clear (format vs no-format)?**

5. **Does the troubleshooting section cover input issues and SPIFFS failures?**

---

## Editor Sign-Off

| Reviewer | Date | Status | Notes |
|----------|------|--------|-------|
| ________ | ____ | ______ | _____ |

---

## Suggested Improvements

_____________________________________________________________________

_____________________________________________________________________
