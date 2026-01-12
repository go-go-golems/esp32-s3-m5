---
Title: 'Component Extraction Plan: ReplLoop, JsEvaluator, Spiffs'
Ticket: MO-02-MQJS-REPL-COMPONENTS
Status: active
Topics:
    - esp32s3
    - console
    - tooling
    - serial
    - cardputer
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/app_main.cpp
      Note: Current composition example
    - Path: esp32-s3-m5/ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/design-doc/02-split-firmware-main-into-c-components-pluggable-evaluators-repeat-js.md
      Note: Earlier componentization proposal
    - Path: esp32-s3-m5/ttmp/2026/01/11/MO-02-MQJS-REPL-COMPONENTS--extract-microquickjs-repl-components/analysis/01-existing-components-and-prior-art.md
      Note: Prior art inventory for extraction plan
ExternalSources: []
Summary: Plan to extract ReplLoop, JsEvaluator, and Spiffs helpers into reusable ESP-IDF components with clear usage guidance.
LastUpdated: 2026-01-11T18:59:06.197916193-05:00
WhatFor: Define how to structure reusable REPL components and integrate them into future firmware projects.
WhenToUse: Use when splitting mqjs-repl into reusable components or onboarding new firmware that needs a JS REPL.
---


# Component Extraction Plan: ReplLoop, JsEvaluator, Spiffs

## Executive Summary

We will extract the existing MicroQuickJS REPL loop, JS evaluator, and SPIFFS helpers from `imports/esp32-mqjs-repl` into reusable ESP-IDF components. The new components will provide stable APIs for REPL input, JS evaluation, and JS autoload storage, so future firmware can compose them without duplicating the code.

## Problem Statement

The current REPL and MicroQuickJS plumbing live inside a single example (`imports/esp32-mqjs-repl`). We need to reuse these elements across multiple firmwares, but they are not packaged as components. This makes reuse error-prone, encourages copy/paste, and hides the constraints (single-task JS execution, SPIFFS behavior, stdlib/atom header matching).

## Proposed Solution

Create three reusable ESP-IDF components and a small glue layer:

1) `mqjs_repl` (REPL loop + line editing + console abstraction)
2) `mqjs_eval` (JsEvaluator + IEvaluator interface + optional mode switching)
3) `mqjs_storage` (SPIFFS helpers + autoload directory scanning)

These components will be pulled from the existing source in `imports/esp32-mqjs-repl` and installed in `esp32-s3-m5/components/` to make them available across projects.

### Component layout

```text
components/
  mqjs_repl/
    include/mqjs_repl/ReplLoop.h
    include/mqjs_repl/LineEditor.h
    include/mqjs_repl/IConsole.h
    src/ReplLoop.cpp
    src/LineEditor.cpp
    src/IConsole.cpp
    src/UsbSerialJtagConsole.cpp
    src/UartConsole.cpp

  mqjs_eval/
    include/mqjs_eval/IEvaluator.h
    include/mqjs_eval/JsEvaluator.h
    include/mqjs_eval/RepeatEvaluator.h
    include/mqjs_eval/ModeSwitchingEvaluator.h
    src/JsEvaluator.cpp
    src/RepeatEvaluator.cpp
    src/ModeSwitchingEvaluator.cpp

  mqjs_storage/
    include/mqjs_storage/Spiffs.h
    src/Spiffs.cpp
```

### Dependency model

- `mqjs_repl` depends on `mqjs_eval` only through `IEvaluator` (no hard dependency on MicroQuickJS).
- `mqjs_eval` depends on `mquickjs` and `mqjs_storage` (for autoload).
- `mqjs_storage` depends on ESP-IDF SPIFFS only.

### Usage example (minimal app_main)

```cpp
#include "mqjs_repl/ReplLoop.h"
#include "mqjs_repl/LineEditor.h"
#include "mqjs_repl/UsbSerialJtagConsole.h"
#include "mqjs_eval/ModeSwitchingEvaluator.h"

extern "C" void app_main(void) {
  auto console = std::make_unique<UsbSerialJtagConsole>();
  auto evaluator = std::make_unique<ModeSwitchingEvaluator>();
  auto editor = std::make_unique<LineEditor>("js> ");
  auto repl = std::make_unique<ReplLoop>();

  repl->Run(*console, *editor, *evaluator);
}
```

### Component CMake usage (example)

```cmake
idf_component_register(
  SRCS "main/app_main.cpp"
  REQUIRES mqjs_repl mqjs_eval mqjs_storage mquickjs
)
```

## Design Decisions

- **Split by responsibility**: REPL loop, JS evaluation, and storage are packaged separately to minimize dependencies.
- **Preserve interfaces**: keep `IEvaluator` and `IConsole` to allow swapping transports or eval modes.
- **Keep JS single-task**: only the REPL task interacts with `JsEvaluator` to avoid thread-safety issues.
- **SPIFFS isolated**: storage helpers live in a standalone component so non-JS firmware can reuse it.

## Alternatives Considered

- **Single monolithic component**: simpler packaging but harder to reuse partial functionality.
- **Copy-paste per project**: fast in the short term but guarantees drift and hidden bugs.
- **Embed everything in the REPL component**: makes `mqjs_repl` depend on JS and SPIFFS even for non-JS uses.

## Implementation Plan

1) Create `components/mqjs_repl`, `components/mqjs_eval`, `components/mqjs_storage` and move existing sources.
2) Adjust include paths and namespaces to match component names.
3) Update `imports/esp32-mqjs-repl` to consume the new components instead of local copies.
4) Add a minimal example firmware that wires ReplLoop + JsEvaluator + Spiffs.
5) Update docs and handoff guides to point to the new component paths.

## Open Questions

- Should `ModeSwitchingEvaluator` remain in `mqjs_eval` or be kept in a separate `mqjs_repl` utility layer?
- Do we want a `mqjs_console` component to split console drivers from the REPL core?
- Should `Spiffs` expose more helpers (list/write/delete) or stay read-only?

## How to use the components (summary)

- Add `mqjs_repl`, `mqjs_eval`, and `mqjs_storage` to `REQUIRES` in your `idf_component_register`.
- Create a REPL task that owns the JS evaluator and LineEditor.
- Use `:autoload` in the REPL to load JS files from `/spiffs/autoload`.
