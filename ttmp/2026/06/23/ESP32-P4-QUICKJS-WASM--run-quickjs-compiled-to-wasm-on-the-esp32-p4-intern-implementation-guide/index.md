---
Title: Run QuickJS compiled to WASM on the ESP32-P4 (intern implementation guide)
Ticket: ESP32-P4-QUICKJS-WASM
Status: active
Topics:
    - esp32p4
    - quickjs
    - wasm
    - esp-idf
    - firmware
    - javascript
    - console
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Run the QuickJS JavaScript engine — compiled to a WebAssembly module — sandboxed inside the WAMR runtime on an ESP32-P4. JS-in-WASM-in-WAMR stack driven from a USB console."
LastUpdated: 2026-06-23
WhatFor: "Onboard a new intern to implement firmware 0100-esp32-p4-quickjs-wasm that evaluates JavaScript via a QuickJS wasm module executed by WAMR."
WhenToUse: "When implementing or extending the QuickJS-WASM-on-ESP32-P4 firmware, or porting the WAMR embedding pattern from 0079 to the P4."
---

# Run QuickJS compiled to WASM on the ESP32-P4 (intern implementation guide)

## Overview

This ticket designs firmware `0100-esp32-p4-quickjs-wasm` that runs **QuickJS compiled to
WebAssembly** on the **ESP32-P4**, executed by the **WAMR** runtime embedded in ESP-IDF. The
architecture is a "JS-in-WASM-in-WAMR" stack with two host boundaries: WAMR↔wasm (WASI + `env`
native imports) and QuickJS↔user JavaScript (C-defined globals). A USB console `js eval` command
feeds JavaScript source into the engine.

The deliverable is an intern-ready analysis/design/implementation guide, a research source corpus,
a chronological diary, and a buildable firmware scaffold. The intern builds `quickjs.wasm` on a
host PC, embeds it, ports the WAMR host API from `0079`, and wires the `js` console commands.

## Primary documents

- **Design / implementation guide:** [design/01-quickjs-wasm-esp32p4-analysis-design-and-implementation-guide.md](./design/01-quickjs-wasm-esp32p4-analysis-design-and-implementation-guide.md)
- **Investigation diary:** [reference/01-investigation-diary.md](./reference/01-investigation-diary.md)
- **Research sources:** [sources/](./sources/) (14 files: WAMR API header + docs, QuickJS, wasi-sdk, ESP32-P4 memory, prior art)

## Target firmware

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0100-esp32-p4-quickjs-wasm/` — scaffold (CMakeLists, sdkconfig.defaults, partitions, idf_component.yml, main stub, wasm-src build).

## Prior art (in this workspace)

- `0079-papers3-wamr-assemblyscript-console` — WAMR embedding pattern to copy (runtime service, host API, runner, EMBED_FILES).
- `0082-papers3-wamr-allocator-control` — allocator/pool behavior reference.
- `0099-esp32-p4-picocalc-display-keyboard` — ESP32-P4 target/console/PSRAM baseline.
- Ticket `ESP-30-M5DIAL-MQJS-LAIN-DSL` + `microquickjs` vocab — native QuickJS lineage (this ticket is the WASM-sandboxed variant).

## Status

Current status: **active** — design complete, scaffold created, sources harvested, reMarkable upload pending.

## Topics

esp32p4, quickjs, wasm, esp-idf, firmware, javascript, console

## Tasks

See [tasks.md](./tasks.md) for the phased implementation task list.

## Changelog

See [changelog.md](./changelog.md) for recent changes and decisions.

## Structure

- design/ — Architecture and design documents
- reference/ — Investigation diary
- sources/ — Downloaded primary research documents
- playbooks/ — Command sequences and test procedures
- scripts/ — Ticket-local tooling
- various/ — Working notes
- archive/ — Deprecated or reference-only artifacts
