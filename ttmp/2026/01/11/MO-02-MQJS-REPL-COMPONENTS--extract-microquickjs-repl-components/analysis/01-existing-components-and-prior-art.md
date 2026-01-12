---
Title: Existing Components and Prior Art
Ticket: MO-02-MQJS-REPL-COMPONENTS
Status: active
Topics:
    - esp32s3
    - console
    - tooling
    - serial
    - cardputer
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp
      Note: Current JS evaluator implementation
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/repl/ReplLoop.cpp
      Note: Current REPL loop implementation
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/storage/Spiffs.cpp
      Note: Current SPIFFS helper used by autoload
    - Path: esp32-s3-m5/ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/reference/01-diary.md
      Note: Prior REPL bring-up diary
    - Path: esp32-s3-m5/ttmp/2026/01/01/0023-CARDPUTER-JS-NEXT--cardputer-js-repl-enhancements-and-next-steps-handoff/reference/01-enhancements-and-next-developer-handoff.md
      Note: Follow-up handoff with REPL component notes
ExternalSources: []
Summary: Inventory of existing MicroQuickJS REPL components and related ticket docs to reuse.
LastUpdated: 2026-01-11T18:59:02.013916278-05:00
WhatFor: Provide source-of-truth references before extracting ReplLoop/JsEvaluator/Spiffs into reusable components.
WhenToUse: Use when planning component extraction or reviewing how existing REPL pieces behave.
---


# Existing Components and Prior Art

## Goal

Identify the current component implementations and the ticket docs/diaries that already describe them, so the extraction work reuses the right patterns and avoids re-deriving known constraints.

## Existing components (current mqjs-repl)

These are the concrete implementations already in use in `imports/esp32-mqjs-repl/mqjs-repl`.

### REPL core and line editing

- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/repl/LineEditor.{h,cpp}`
  - Key symbols: `LineEditor::FeedByte`, `LineEditor::PrintPrompt`
  - Purpose: byte-to-line editing with prompt handling
- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/repl/ReplLoop.{h,cpp}`
  - Key symbols: `ReplLoop::Run`, `ReplLoop::HandleLine`
  - Purpose: read bytes, update LineEditor, dispatch complete lines, handle meta-commands

### Evaluators and modes

- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/IEvaluator.h`
  - Interface for evaluator backends
- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.{h,cpp}`
  - Key symbols: `JsEvaluator::EvalLine`, `JsEvaluator::Reset`, `JsEvaluator::Autoload`
  - Purpose: wraps MicroQuickJS context, evals JS, prints values/exceptions
- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/RepeatEvaluator.{h,cpp}`
  - Simple echo evaluator for bring-up
- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/ModeSwitchingEvaluator.{h,cpp}`
  - Supports `:mode` switching between evaluator implementations

### Storage helpers (SPIFFS)

- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/storage/Spiffs.{h,cpp}`
  - Key symbols: `SpiffsEnsureMounted`, `SpiffsReadFile`
  - Purpose: SPIFFS mount + read helper used by JS autoload

### Console abstraction

- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/console/IConsole.{h,cpp}`
  - Interface for REPL transport
- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/console/UsbSerialJtagConsole.{h,cpp}`
  - USB Serial/JTAG implementation
- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/console/UartConsole.{h,cpp}`
  - UART implementation

## Related ticket diaries and design docs

These existing docs already describe or reference the REPL pieces and should be treated as prior art.

### Core bring-up and REPL structure (Ticket 0014)

- `esp32-s3-m5/ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/reference/01-diary.md`
  - Diary entries for LineEditor/ReplLoop/JsEvaluator and autoload wiring
- `esp32-s3-m5/ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/design-doc/02-split-firmware-main-into-c-components-pluggable-evaluators-repeat-js.md`
  - Prior design proposing componentized ReplLoop/JsEvaluator/SpiffsStorage
- `esp32-s3-m5/ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/changelog.md`
  - Changelog entries for `ReplLoop`, `JsEvaluator`, and `Spiffs`

### Follow-up handoff + enhancements (Ticket 0023)

- `esp32-s3-m5/ttmp/2026/01/01/0023-CARDPUTER-JS-NEXT--cardputer-js-repl-enhancements-and-next-steps-handoff/reference/01-enhancements-and-next-developer-handoff.md`
  - Highlights key symbols and next steps for ReplLoop/JsEvaluator/Spiffs

### SPIFFS autoload bug context (Ticket 0016)

- `esp32-s3-m5/ttmp/2025/12/29/0016-SPIFFS-AUTOLOAD--bug-spiffs-first-boot-format-autoload-js-parse-errors/reference/01-diary.md`
  - Captures autoload failure modes and SPIFFS formatting behavior
- `esp32-s3-m5/ttmp/2025/12/29/0016-SPIFFS-AUTOLOAD--bug-spiffs-first-boot-format-autoload-js-parse-errors/analysis/01-bug-report-spiffs-mount-format-autoload-js-parse-errors.md`
  - Bug report on autoload parse errors

## Observed constraints from prior art

- MicroQuickJS is single-task; other tasks must not call JS APIs directly.
- JS evaluation depends on matching stdlib + atom table (generated headers).
- REPL meta-commands (e.g., `:autoload`, `:stats`) depend on both ReplLoop and JsEvaluator.

## Gaps to address in extraction

- The current code lives under `imports/` and is not a reusable ESP-IDF component.
- SPIFFS helpers are tied directly to JS evaluator behavior, instead of being a shared storage module.
- The console abstraction is present but not packaged as a standalone reusable component.
