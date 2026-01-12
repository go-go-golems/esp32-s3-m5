---
Title: 'Editor Notes: Cardputer Keyboard Input'
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - esp32
    - firmware
    - documentation
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/ttmp/2026/01/11/MO-02-ESP32-DOCS--esp32-firmware-diary-synthesis/playbook/13-cardputer-keyboard-input-gpio-matrix-scanning-and-scancode-mapping.md
      Note: The final guide document to review
ExternalSources: []
Summary: 'Editorial review checklist for the Cardputer keyboard input guide.'
LastUpdated: 2026-01-12T12:00:00-05:00
WhatFor: 'Help editors verify the guide is accurate, complete, and useful.'
WhenToUse: 'Use when reviewing the guide before publication.'
---

# Editor Notes: Cardputer Keyboard Input

## Purpose

Editorial checklist for reviewing the Cardputer keyboard input guide before publication.

---

## Target Audience Check

The guide is written for developers who:
- Have ESP-IDF experience (building, flashing, menuconfig)
- Want to add keyboard input to their Cardputer projects
- Need to understand the hardware matrix and scancode system

**Review questions:**
- [ ] Does the guide explain the matrix scan concept clearly for someone new to keyboard hardware?
- [ ] Is the pin mapping presented in a way that's easy to verify against hardware?
- [ ] Are edge detection and debounce explained with practical examples?

---

## Structural Review

### Required Sections

- [ ] **Hardware overview** — Matrix structure, pin assignments
- [ ] **Scan algorithm** — Step-by-step scan loop explanation
- [ ] **Position mapping** — scan_state + input_bit → x,y → keyNum
- [ ] **Edge detection** — How key-down/key-up events are derived
- [ ] **Debounce** — Per-key debounce strategy
- [ ] **Modifier handling** — Shift, Ctrl, Alt, Fn detection
- [ ] **Fn combos** — Navigation keys as chord bindings
- [ ] **Calibration workflow** — Using the scancode calibrator
- [ ] **Reusable component** — cardputer_kb API reference
- [ ] **Cardputer-ADV note** — TCA8418 I2C contrast

### Flow and Readability

- [ ] Does the guide progress from hardware → scan → events → mappings?
- [ ] Are diagrams clear and labeled (matrix grid, scan state encoding)?
- [ ] Is code properly explained, not just dumped?

---

## Accuracy Checks

### Claims to Verify Against Source Code

| Claim in Guide | Verify In | What to Check |
|----------------|-----------|---------------|
| OUT pins are GPIO 8, 9, 11 | matrix_scanner.cpp | `kOutPins[]` array |
| IN pins primary are GPIO 13, 15, 3, 4, 5, 6, 7 | matrix_scanner.cpp | `kInPinsPrimary[]` |
| IN pins alt are GPIO 1, 2, 3, 4, 5, 6, 7 | matrix_scanner.cpp | `kInPinsAltIn01[]` |
| Settle delay is 10µs | matrix_scanner.cpp | `esp_rom_delay_us(10)` |
| keyNum = y*14 + x + 1 | layout.h | `keynum_from_xy()` |
| Fn keyNum is 29 | layout.h | `kKeyLegend[2][0]` is "fn" → keyNum calc |
| Debounce default is 30ms | Kconfig.projbuild | `CONFIG_TUTORIAL_0007_DEBOUNCE_MS` |
| Scan period default is 10ms | Kconfig.projbuild | `CONFIG_TUTORIAL_0007_SCAN_PERIOD_MS` |

- [ ] All claims verified against source code

### Code Snippets

| Snippet | Check |
|---------|-------|
| GPIO initialization | Matches `keyboard_echo_task()` |
| Scan loop with output/input | Matches `kb_scan_pressed()` |
| Edge detection logic | Matches main loop in `keyboard_echo_task()` |
| Position derivation formula | Matches `scan()` in matrix_scanner.cpp |

- [ ] Code snippets match source or are clearly labeled as simplified

---

## Completeness Checks

### Topics That Should Be Covered

| Topic | Covered? | Notes |
|-------|----------|-------|
| 3 output + 7 input pin matrix | | |
| GPIO configuration (direction, pullup) | | |
| 8 scan states (0-7) | | |
| Active-low input reading | | |
| Position derivation math | | |
| keyNum encoding (1-indexed) | | |
| Edge detection (key-down events) | | |
| Per-key debounce timestamps | | |
| Modifier detection pass | | |
| Shift-based character selection | | |
| Fn-key chord bindings | | |
| Navigation key mapping | | |
| IN0/IN1 variant autodetection | | |
| Calibration firmware purpose | | |
| Wizard capture workflow | | |
| Binding decoder API | | |
| Cardputer-ADV TCA8418 contrast | | |
| Serial echo debug tips | | |

- [ ] All essential topics covered adequately

### Potential Missing Information

- [ ] What happens with ghosting (multiple simultaneous keys)?
- [ ] Key repeat behavior (not implemented in basic demo)
- [ ] How to use the component in your own project
- [ ] Example of integrating with display (typewriter pattern)

---

## Diagrams to Verify

- [ ] **Matrix layout** — 4×14 grid with key labels
- [ ] **Scan state diagram** — How 3-bit output selects rows
- [ ] **Position derivation** — Visual showing scan_state/input → x,y
- [ ] **Wiring diagram** — GPIO connections (if included)

---

## Code Command Verification

Commands that should be tested:

```bash
# Build keyboard serial demo
cd esp32-s3-m5/0007-cardputer-keyboard-serial
idf.py set-target esp32s3
idf.py build flash monitor

# Build calibrator
cd esp32-s3-m5/0023-cardputer-kb-scancode-calibrator
./build.sh build
./build.sh -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_* flash
```

- [ ] Build commands verified to work
- [ ] Expected outputs documented

---

## Tone and Style

- [ ] Consistent use of technical terms (scan state, keyNum, chord)
- [ ] Active voice preferred
- [ ] Clear distinction between physical keys and semantic actions
- [ ] Warnings about variant differences are prominent

---

## Final Review Questions

1. **Could someone implement keyboard input from this guide alone?**

2. **Is the scan algorithm explained clearly enough to debug issues?**

3. **Are the Fn-chord bindings explained as calibration results, not assumptions?**

4. **Is the Cardputer-ADV contrast sufficient to prevent confusion?**

5. **Does the troubleshooting section cover serial echo buffering issues?**

---

## Editor Sign-Off

| Reviewer | Date | Status | Notes |
|----------|------|--------|-------|
| ________ | ____ | ______ | _____ |

---

## Suggested Improvements

_____________________________________________________________________

_____________________________________________________________________
