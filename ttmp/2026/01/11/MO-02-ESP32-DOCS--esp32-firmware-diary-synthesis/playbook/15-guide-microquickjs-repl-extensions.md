---
Title: 'Guide: MicroQuickJS REPL + extensions'
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - esp32
    - firmware
    - documentation
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: 'Expanded research and writing guide for running a MicroQuickJS REPL with custom extensions.'
LastUpdated: 2026-01-11T19:44:01-05:00
WhatFor: 'Help a ghostwriter produce a focused doc on running a MicroQuickJS REPL and registering custom bindings.'
WhenToUse: 'Use before writing the MicroQuickJS REPL guide.'
---

# Guide: MicroQuickJS REPL + extensions

## Purpose

This guide enables a ghostwriter to create a single-topic developer doc on running a MicroQuickJS REPL on ESP32-S3 and extending it with custom native bindings. The final doc should teach how the REPL loop is wired, how console backends are selected, and where extension hooks live (using the Cardputer-ADV JS GPIO exercizer as the reference example).

## Assignment Brief

Write a focused doc that answers two questions: (1) how to run a MicroQuickJS REPL on device with a reliable console backend, and (2) how to register native JS bindings so the REPL can control hardware (GPIO/I2C engines). Treat `esp32-s3-m5/0039-cardputer-adv-js-gpio-exercizer/main/app_main.cpp` as the concrete example for extensions.

## Environment Assumptions

- ESP-IDF 5.4.x
- ESP32-S3 hardware (Cardputer or Cardputer-ADV)
- USB Serial/JTAG console access (`/dev/ttyACM*`)

## Source Material to Review

- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/docs/repl.md`
- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/docs/js.md`
- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/docs/spiffs.md`
- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/app_main.cpp` (baseline REPL loop)
- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/build.sh` (build workflow)
- `esp32-s3-m5/0039-cardputer-adv-js-gpio-exercizer/main/app_main.cpp` (extension wiring)
- `esp32-s3-m5/0039-cardputer-adv-js-gpio-exercizer/main/js_bindings.cpp` / `js_bindings.h` (binding registration)
- `esp32-s3-m5/ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/reference/01-diary.md`
- `esp32-s3-m5/ttmp/2025/12/29/0016-SPIFFS-AUTOLOAD--bug-spiffs-first-boot-format-autoload-js-parse-errors/reference/01-diary.md`
- `esp32-s3-m5/imports/ESP32_MicroQuickJS_Playbook.md`
- `esp32-s3-m5/imports/MicroQuickJS_ESP32_Complete_Guide.md`

## Technical Content Checklist

- REPL architecture: console -> line editor -> evaluator -> repl loop
- Console backend selection (USB Serial/JTAG vs UART)
- `ModeSwitchingEvaluator` and JS prompt selection
- Task model: REPL runs in its own FreeRTOS task
- SPIFFS autoload path: when JS scripts load at boot and how failures surface
- Extension binding flow: C++ binds functions into JS runtime and routes to control plane
- Example extension: `exercizer_js_bind(&s_ctrl)` from 0039 app_main

## Pseudocode Sketch

Use pseudocode to show the REPL wiring and extension registration.

```c
// Pseudocode: REPL setup
console = UsbSerialJtagConsole()
evaluator = ModeSwitchingEvaluator()
editor = LineEditor(evaluator.Prompt())
repl = ReplLoop()

repl_task:
  print_banner()
  editor.PrintPrompt(console)
  repl.Run(console, editor, evaluator)

// Extension hook
control_plane.Start()
js_bind(control_plane) // register native bindings for JS
```

## Pitfalls to Call Out

- `idf.py monitor` may not send input if the console backend is misconfigured.
- SPIFFS autoload can fail on first boot formatting; document the error symptoms.
- REPL input can collide with other UART consumers if UART0 is shared.
- Large JS scripts can exhaust heap; keep examples small and incremental.

## Suggested Outline

1. What MicroQuickJS is and what this REPL does
2. REPL wiring: console, evaluator, line editor, repl loop
3. Selecting console backend (USB Serial/JTAG vs UART)
4. Running the baseline REPL firmware
5. Adding native bindings (example from 0039)
6. SPIFFS autoload notes and troubleshooting
7. Troubleshooting checklist

## Commands

```bash
# Build and run the baseline REPL import
cd esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl
./build.sh set-target esp32s3
./build.sh build flash monitor

# Build and run the Cardputer-ADV JS GPIO exercizer
cd ../../../0039-cardputer-adv-js-gpio-exercizer
idf.py set-target esp32s3
idf.py build flash monitor
```

## Exit Criteria

- Doc explains the REPL architecture and where each component lives in code.
- Doc shows how to register native bindings and where to add new ones.
- Doc includes a troubleshooting section for console input and SPIFFS autoload.

## Notes

Keep the scope on running the REPL and adding extensions. Avoid general ESP-IDF setup material unless it is required to run the REPL firmware.
