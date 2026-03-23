---
Title: Diary
Ticket: ESP-43-PAPERS3-WAMR-EMBEDDED-BUFFER-MAPPING
Status: active
Topics:
    - papers3
    - wasm
    - firmware
    - esp-idf
    - debugging
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-23T13:46:41.047563537-04:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Start the next investigation phase from a sharper boundary than `ESP-42` had at the beginning: the remaining PaperS3 fault appears to depend on loading directly from the embedded module bytes, not on generic WAMR load behavior from arbitrary RAM buffers.

## Context

The immediately previous ticket proved:

- embedded `load-only` still poisons later PSRAM writes
- copied internal-RAM `load-only` does not
- copied SPIRAM `load-only` also does not

That is strong enough to justify a new ticket. The open question is now about mapping and buffer provenance, not broad allocator behavior.

## Quick Reference

- New ticket: `ESP-43-PAPERS3-WAMR-EMBEDDED-BUFFER-MAPPING`
- Primary project: `0082-papers3-wamr-allocator-control`
- Primary code targets:
  - `main/wasm_module_registry.cpp`
  - `main/wasm_module_runner.cpp`
  - `main/wasm_command.cpp`

## Usage Examples

### 2026-03-23 12:30 EDT

Created `ESP-43` to split the investigation cleanly. This was a deliberate scoping move rather than a paperwork exercise. `ESP-42` had become too broad: allocator mode, pool backing, instantiate, load, and loader instrumentation were all in scope there. Now that the boundary is narrower, the work should be grouped around the embedded-buffer path itself.

### 2026-03-23 12:31 EDT

Seeded the new ticket with:

- a design note for the embedded-buffer mapping phase
- a fresh task list
- this diary

The immediate aim is to avoid losing the exact transition in thinking. We are no longer asking "why does WAMR poison PSRAM on PaperS3?" in the abstract. We are asking "what is special about the embedded flash-mapped Wasm source path on PaperS3?"

### 2026-03-23 12:35 EDT

Mapped the embedded asset path in `0082` itself. The modules are declared in `main/wasm_module_registry.cpp` via linker-generated `_binary_*_wasm_start/end` symbols, and `main/CMakeLists.txt` uses `EMBED_FILES` for those `.wasm` assets. In the built internal-pool ELF, the relevant symbols land around:

- `_binary_return_42_wasm_start = 0x3c04f008`
- `_binary_log_only_wasm_start = 0x3c04f03e`

`objdump -h` shows those addresses sit in `.flash.rodata`, not in a copied RAM section. That matches the current working hypothesis cleanly: the toxic path is the embedded flash-mapped asset path.

### 2026-03-23 12:38 EDT

Confirmed the comparison matrix is real and not an instrumentation illusion:

- embedded `load-only` still crashes later in `TouchPersistentPsramProbe(...)`
- copied-internal `load-only` succeeds and leaves later PSRAM touch healthy
- copied-spiram `load-only` also succeeds and leaves later PSRAM touch healthy

The important nuance is that copied-SPIRAM success rules out a simplistic "only internal-RAM source buffers are safe" theory. The meaningful difference is not just RAM type. It is much closer to "embedded flash-mapped asset bytes versus RAM-backed copied bytes."

### 2026-03-23 12:46 EDT

Started the next mitigation-oriented experiment by adding copy-backed `instantiate-bare` and `run` commands in `0082`. This is intentionally a narrow extension rather than a full command explosion. The question is simply whether "copy before load" recovers only `load-only`, or whether it also recovers later lifecycle stages strongly enough to treat it as a viable workaround.

The new command surface is:

- `wasm instantiate-bare-copy-internal`
- `wasm instantiate-bare-copy-spiram`
- `wasm run-copy-internal`
- `wasm run-copy-spiram`

The updated build succeeded cleanly.

### 2026-03-23 12:49 EDT

The next hardware step failed for an operational reason, not a firmware logic reason. Flashing the rebuilt `0082` image reported:

- `Could not open /dev/ttyACM0, the port is busy or doesn't exist`
- then a direct check showed there were no `/dev/ttyACM*` nodes at all
- `/dev/serial/by-id` was empty
- `lsusb` did not show the PaperS3 USB device

So this slice is currently blocked at the host/device connection layer. I am recording that explicitly because it defines the exact stopping point:

- code for the next experiment exists
- build succeeded
- hardware validation did **not** run because the board disappeared from USB enumeration after reset

## Related

- [design/01-embedded-buffer-mapping-investigation-plan.md](../design/01-embedded-buffer-mapping-investigation-plan.md)
- [tasks.md](../tasks.md)
