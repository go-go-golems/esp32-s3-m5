---
Title: Diary
Ticket: ESP-44-PAPERS3-WAMR-FLASH-LOAD-ROOT-CAUSE
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
LastUpdated: 2026-03-23T15:05:00-04:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Turn the successful “copy embedded Wasm into RAM before load” mitigation into a more precise explanation of what the direct embedded path was doing wrong.

## 2026-03-23 15:05 EDT

Created this ticket because `ESP-43` answered the practical question but not the explanatory one.

We now know:

- embedded direct path is bad on PaperS3
- copied-RAM source buffers are good
- the current mitigation works

That is enough to ship a workaround, but not enough to explain the bug.

## 2026-03-23 15:11 EDT

Read the interpreter loader more closely instead of adding logs blindly.

The important finding is in `wasm_runtime.c`, inside `wasm_const_str_list_insert(...)`. When `is_load_from_file_buf` is true, it does not simply reference the source bytes as-is. It mutates the source buffer:

- shifts the string back by one byte with `bh_memmove_s`
- writes a null terminator into the original buffer

That optimization only makes sense if the source buffer is writable. For a copied RAM buffer, it is fine. For the embedded `.flash.rodata` path, it is exactly the kind of thing that could produce undefined or cache-sensitive behavior.

## 2026-03-23 15:13 EDT

This is the first root-cause candidate that feels mechanically coherent rather than circumstantial.

It also explains why “copy to SPIRAM first” worked. Earlier, that result weakened the naive “internal RAM good, everything else bad” theory. Under the new theory, SPIRAM can still be fine because the important property is not “internal.” It is “writable.”

## 2026-03-23 15:22 EDT

Implemented the smallest proof harness in `0082` instead of jumping straight to another workaround:

- added `EmbeddedDirect` as an explicit binary-source mode that bypasses the default copy-before-load mitigation
- added `WasmLoadMethod` so a command can choose between plain `wasm_runtime_load(...)` and `wasm_runtime_load_ex(...)`
- exposed two new commands:
  - `wasm load-only-embedded-direct`
  - `wasm load-only-embedded-direct-freeable`

The design intent was very narrow. Both commands keep the same embedded flash-mapped source pointer. The only meaningful difference is whether the loader is allowed to treat the input buffer as reusable/mutable metadata storage.

## 2026-03-23 15:28 EDT

Rebuilt and reflashed the internal-pool PaperS3 image with the new proof commands. Verified the command surface first with `wasm examples` before trusting the hardware comparison.

This matters because the investigation has already been bitten by stale variant config once. The current image reports app version `564db82-dirty`, which is the expected pre-commit state for this slice.

## 2026-03-23 15:34 EDT

Ran the negative control:

- `wasm replay psram-persistent-init`
- `wasm load-only-embedded-direct return-42`
- `wasm replay psram-persistent-touch-sync`

Result: reproduced the old failure.

Important evidence from the log:

- `binary_source=embedded-direct`
- `load_method=runtime-load`
- later PSRAM touch still crashed with `Cache disabled but cached memory region accessed`

So the direct embedded path is still toxic when it goes through the legacy plain load behavior.

## 2026-03-23 15:39 EDT

Ran the positive proof:

- `wasm replay psram-persistent-init`
- `wasm load-only-embedded-direct-freeable return-42`
- `wasm replay psram-persistent-touch-sync`

Result: success.

Important evidence from the log:

- `binary_source=embedded-direct`
- `load_method=runtime-load-ex-binary-freeable`
- later persistent PSRAM touch succeeded

This is the strongest result in the whole investigation so far. It keeps the same embedded flash-mapped source pointer, but changes the loader behavior enough to avoid the later PaperS3 PSRAM failure.

That means the issue is much more likely to be:

- “the loader’s reuse/mutation strategy for the original source buffer is incompatible with embedded flash-mapped input on PaperS3”

than:

- “any direct read from embedded flash-mapped Wasm bytes is toxic”

It is still possible that other low-level details participate, but the source-buffer reuse path is now the leading explanation by a wide margin.
