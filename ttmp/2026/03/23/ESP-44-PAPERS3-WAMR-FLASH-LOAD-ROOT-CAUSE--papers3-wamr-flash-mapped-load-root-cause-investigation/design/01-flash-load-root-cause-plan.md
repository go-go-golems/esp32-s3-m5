---
Title: Flash-mapped load root-cause plan
Ticket: ESP-44-PAPERS3-WAMR-FLASH-LOAD-ROOT-CAUSE
Status: active
Topics:
    - papers3
    - wasm
    - firmware
    - esp-idf
    - debugging
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0082-papers3-wamr-allocator-control/main/wasm_module_runner.cpp
      Note: Controls whether embedded modules are copied before load or passed through directly
    - Path: 0082-papers3-wamr-allocator-control/managed_components/espressif__wasm-micro-runtime/core/iwasm/interpreter/wasm_loader.c
      Note: Parses the original source buffer and chooses reuse-vs-clone behavior for loader metadata
    - Path: 0082-papers3-wamr-allocator-control/managed_components/espressif__wasm-micro-runtime/core/iwasm/interpreter/wasm_runtime.c
      Note: Contains wasm_const_str_list_insert, which mutates the source buffer for reusable const strings
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-23T15:05:00-04:00
WhatFor: ""
WhenToUse: ""
---

# Flash-mapped load root-cause plan

## Goal

Move from “copy before load fixes PaperS3” to a more specific explanation of why the embedded direct-buffer path is toxic.

## Leading hypothesis

The strongest code-level candidate is no longer generic flash reading. It is WAMR’s “reuse const strings from the input buffer” optimization.

The relevant chain is:

1. `wasm_runtime_load()` sets `LoadArgs.wasm_binary_freeable = false`
2. `wasm_loader_load()` calls `load(...)`
3. `load_from_sections(..., is_load_from_file_buf=true, wasm_binary_freeable=false, ...)`
4. `reuse_const_strings = true`
5. `wasm_const_str_list_insert(..., is_load_from_file_buf=true, ...)`
6. That function rewrites the original source buffer in place by shifting the string backward one byte and appending `'\0'`

That is reasonable for a writable file/RAM buffer. It is a bad fit for an embedded `.flash.rodata` buffer.

## Why this is promising

- It explains why copied-internal and copied-spiram buffers are healthy.
- It explains why the mitigation “copy embedded Wasm into RAM before load” works.
- It provides a more concrete mechanism than “flash-mapped access is weird.”
- It matches the observed boundary better than allocator, instantiate, or display theories.

## Immediate experiment

Use the embedded direct source buffer again, but force WAMR down a load path that clones metadata instead of reusing the original source buffer.

Concretely:

- keep `module.start` as the source buffer
- bypass the current “copy embedded to internal RAM” mitigation for a special experimental command
- call `wasm_runtime_load_ex(...)` with `LoadArgs.wasm_binary_freeable = true`

Expected result:

- if the later PSRAM touch stays healthy, the mutation/reuse path is the likely culprit
- if it still fails, the problem is lower-level than this optimization

## Follow-up paths

If the experiment succeeds:

- document the mutation path as the likely root cause
- decide whether production should keep the simpler “copy to internal RAM first” fix or switch to a more surgical `load_ex`-style path

If the experiment fails:

- move back down to flash mapping and board-level cache behavior
- use `ESP-46` for explicit partition-read and mapping A/B tests

