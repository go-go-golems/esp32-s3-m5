---
Title: PaperS3 WAMR execution primitives for embedded AssemblyScript demos
Ticket: ESP-37-PAPERS3-WAMR-EXECUTION-PRIMITIVES
Status: active
Topics:
    - papers3
    - wasm
    - assemblyscript
    - wamr
    - firmware
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp
      Note: Current console command entrypoint with placeholder run path
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp
      Note: Current WAMR runtime bootstrap and status reporting
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_module_registry.cpp
      Note: Embedded module catalog that execution will consume
    - Path: 0079-papers3-wamr-assemblyscript-console/wasm-src/shared/host.ts
      Note: Guest-side import contract that host execution primitives must satisfy
ExternalSources:
    - https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/export_native_api.md
Summary: Ticket for implementing the first runnable PaperS3 WAMR execution pipeline and host drawing primitives for embedded AssemblyScript demos.
LastUpdated: 2026-03-22T11:19:53.896461308-04:00
WhatFor: Track the host ABI, execution lifecycle, implementation tasks, and validation work needed to replace the current placeholder `wasm run` path with a real end-to-end runner.
WhenToUse: Use this ticket when working on WAMR host imports, module loading/instantiation, or execution/debugging of embedded AssemblyScript demos on PaperS3.
---

# PaperS3 WAMR execution primitives for embedded AssemblyScript demos

## Overview

This ticket is the second half of the `0079-papers3-wamr-assemblyscript-console` effort. The first ticket established the board, console, runtime bootstrap, AssemblyScript build pipeline, and embedded module registry. This ticket covers the missing execution path: native host imports, display primitives, module load/instantiate/execute/unload flow, and hardware validation of `wasm run <name>`.

The goal is not a generic Wasm operating system. The goal is a deliberately small, understandable, intern-friendly runtime slice that lets a curated set of precompiled AssemblyScript demos draw on the PaperS3 display through a numeric host ABI.

## Key Links

- Design guide: [design-doc/01-papers3-wamr-execution-primitives-analysis-design-and-implementation-guide.md](./design-doc/01-papers3-wamr-execution-primitives-analysis-design-and-implementation-guide.md)
- Diary: [reference/01-diary.md](./reference/01-diary.md)
- Task list: [tasks.md](./tasks.md)
- Changelog: [changelog.md](./changelog.md)

## Status

Current status: **active**

Current focus:

- write the execution-primitives guide for a new intern
- implement PaperS3 canvas and WAMR native host bindings
- replace the placeholder `wasm run` code path with a real module runner
- validate the first runnable demo on hardware

## Topics

- papers3
- wasm
- assemblyscript
- wamr
- firmware

## Tasks

See [tasks.md](./tasks.md) for the detailed execution plan.

## Changelog

See [changelog.md](./changelog.md) for recorded milestones and commit links.

## Structure

- design-doc/ - long-form design and implementation guidance
- reference/ - diary and quick-reference artifacts
- playbooks/ - command sequences and validation procedures
- scripts/ - temporary ticket-specific tooling if needed
- various/ - scratch notes and secondary analysis
- archive/ - deprecated material
