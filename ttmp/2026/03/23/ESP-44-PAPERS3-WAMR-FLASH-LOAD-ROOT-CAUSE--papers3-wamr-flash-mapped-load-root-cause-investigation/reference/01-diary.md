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
LastUpdated: 2026-03-23T19:46:00-04:00
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

## 2026-03-23 16:52 EDT

Added an even stricter control instead of jumping straight to a conclusion.

I embedded a new `empty-module.wasm` asset in `0082`. It is only the 8-byte Wasm header:

- `00 61 73 6d 01 00 00 00`

The purpose is to keep the direct embedded flash-mapped load path intact while removing exports, names, and any other strings that might trigger `wasm_const_str_list_insert(...)`.

This matters because the earlier proof only showed that the `binary_freeable` path avoids the bug. It did not yet prove that the bug specifically needs a string-mutation opportunity.

## 2026-03-23 17:04 EDT

Ran the new negative control on a freshly flashed PaperS3:

- `wasm replay psram-persistent-init`
- `wasm load-only-embedded-direct empty-module`
- `wasm replay psram-persistent-touch-sync`

Result: success.

Important evidence:

- `binary_source=embedded-direct`
- `load_method=runtime-load`
- no `wamr_const_str.stage=mutate-in-place` log entries
- later PSRAM touch still succeeded

This is a major tightening of the hypothesis. Direct embedded loading by itself is not sufficient to poison later PSRAM access on PaperS3.

## 2026-03-23 17:11 EDT

Instrumented `wasm_const_str_list_insert(...)` directly in the ignored WAMR component and preserved the diff under this ticket in:

- `scripts/wamr-patches/01-wasm-runtime-const-str-trace.diff`

The new bounded trace logs:

- source pointer
- destination pointer (`str - 1`)
- string length
- previous byte value used as the rewritten destination

That gives a direct runtime witness for whether the in-place rewrite path is actually being taken on the device.

## 2026-03-23 17:19 EDT

Ran the known-bad direct-embedded path again on a fresh PaperS3 boot:

- `wasm replay psram-persistent-init`
- `wasm load-only-embedded-direct return-42`
- `wasm replay psram-persistent-touch-sync`

Result: reproduced the old PSRAM failure again.

New evidence from the WAMR trace:

- `wamr_const_str.stage=mutate-in-place`
- first string: `src=0x3c04f45c`, `dst=0x3c04f45b`, `len=3`, `prev_byte=0x03`
- second string: `src=0x3c04f462`, `dst=0x3c04f461`, `len=6`, `prev_byte=0x06`

So the failing path is not only “behaviorally different.” It is concretely taking the exact in-place source-buffer rewrite branch that we suspected.

## 2026-03-23 17:24 EDT

Ran the positive comparison on a fresh PaperS3 boot:

- `wasm replay psram-persistent-init`
- `wasm load-only-embedded-direct-freeable return-42`
- `wasm replay psram-persistent-touch-sync`

Result: success.

Important evidence:

- still `binary_source=embedded-direct`
- now `load_method=runtime-load-ex-binary-freeable`
- no `wamr_const_str.stage=mutate-in-place` entries appeared
- later PSRAM touch succeeded

This is the cleanest A/B in the whole investigation:

- same board
- same firmware family
- same embedded flash-mapped source pointer
- same module
- different loader ownership contract
- only the in-place rewrite path disappears, and the bug disappears with it

## 2026-03-23 17:31 EDT

Decoded the `return-42.wasm` bytes directly to tie the runtime trace back to actual section strings instead of leaving it at “length 3 and length 6”.

The binary contains:

- export name `run` with length `3`
- export name `memory` with length `6`

Relevant bytes:

- `07 10 02 03 72 75 6e 00 00 06 6d 65 6d 6f 72 79 02 00`

That matches the two runtime rewrite lengths exactly:

- `len=3` -> `run`
- `len=6` -> `memory`

At this point the explanation is no longer just “strong circumstantial evidence.” The failing path is specifically rewriting the embedded flash-mapped export strings in place, and the non-rewriting paths are the ones that stay healthy.

## 2026-03-23 19:33 EDT

Took the next proof step instead of stopping at the `binary_freeable` A/B.

I re-read the interpreter loader path and focused on the exact switch in `wasm_loader.c`:

- `reuse_const_strings = is_load_from_file_buf && !wasm_binary_freeable`

That means the existing successful `binary_freeable` control was doing two things at once:

- preventing the in-place const-string rewrite
- changing some other loader ownership behavior at the same time

To isolate the mechanism more cleanly, I patched the local ignored WAMR loader to keep the ordinary `runtime-load` path but force:

- `reuse_const_strings = false`

I preserved that patch under:

- `scripts/wamr-patches/02-disable-reuse-const-strings.diff`

The design of this proof matters. It leaves the direct embedded source-pointer path intact and does **not** rely on `binary_freeable`.

## 2026-03-23 19:46 EDT

Ran the proof on AtomS3R because that board was already attached and the bug had just been reproduced there.

Sequence:

- flash updated `0081`
- `wasm replay psram-persistent-init`
- `wasm load-only-embedded-direct return-42`
- `wasm replay psram-persistent-touch-sync`

Result: success.

Important evidence:

- still `load_method=runtime-load`
- still direct embedded load
- no in-place const-string mutation logs
- later PSRAM touch succeeded

This is the strongest root-cause proof so far.

Before this patch:

- direct embedded `runtime-load` for `return-42` mutated `run` and `memory` in place
- later PSRAM touch crashed

After this patch:

- the same direct embedded `runtime-load` path no longer reused/mutated const strings
- later PSRAM touch stayed healthy

That means the bug is not merely correlated with direct embedded loading. The critical mechanism is the in-place const-string reuse/mutation path itself.

At this point the explanation is precise enough to state plainly:

- WAMR’s interpreter loader treats the source buffer as writable when `is_load_from_file_buf` is true and `wasm_binary_freeable` is false
- embedded Wasm assets in our ESP-IDF builds are flash-mapped and not safe for that in-place string rewrite strategy
- taking that rewrite path corrupts later PSRAM/cache behavior on the device
- disabling that rewrite path, or using a mode that avoids it, removes the failure
