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

### 2026-03-23 14:03 EDT

The board came back and I resumed at the exact blocked point instead of branching to new work. The first check showed the device was still on the older image, because `wasm examples` did not yet list the new copy-backed `instantiate` and `run` commands. I reflashed the current internal-pool build and confirmed the new command surface before trusting the next probes.

### 2026-03-23 14:06 EDT

Ran the first mitigation-extension check:

- `wasm replay psram-persistent-init`
- `wasm instantiate-bare-copy-internal return-42`
- `wasm replay psram-persistent-touch-sync`

Result: success. This matters because it shows the copied-internal mitigation is not limited to `load-only`. It remains healthy through full module instantiation and later cleanup, and the later persistent PSRAM touch still succeeds.

### 2026-03-23 14:08 EDT

Ran the second mitigation-extension check:

- `wasm replay psram-persistent-init`
- `wasm run-copy-internal return-42`
- `wasm replay psram-persistent-touch-sync`

Result: also success. The run path loaded, instantiated, looked up the export, created the exec env, executed the guest, returned `42`, cleaned up, and still left the later PSRAM touch healthy.

That is the strongest workaround evidence we have so far. At this point the live conclusion is:

- embedded flash-mapped source buffer path is toxic on PaperS3
- copied-internal RAM source buffer path survives `load-only`
- copied-internal RAM source buffer path survives `instantiate-bare`
- copied-internal RAM source buffer path survives full `run`

So the next sensible step is no longer another probe of the same shape. It is to turn "copy before load" into a default or feature-flagged mitigation and rerun the plain commands under that behavior.

### 2026-03-23 14:15 EDT

Started that default-mitigation slice in `0082` rather than making the operator remember a special command family forever. The code change is intentionally small:

- add a Kconfig flag: `CONFIG_PAPERS3_WAMR_COPY_EMBEDDED_TO_INTERNAL_BEFORE_LOAD`
- enable it in the normal defaults and the internal-pool variant
- resolve plain `Embedded` requests to `CopiedToInternalRam` inside `wasm_module_runner.cpp`
- expose the active state in `wasm status` as `embedded_load_copy_internal=yes/no`

This keeps the mitigation visible both at build time and at runtime.

### 2026-03-23 14:18 EDT

Hit an important configuration trap immediately after the first rebuild: the device still printed `embedded_load_copy_internal=no`. That did not mean the code path was wrong. It meant the existing build-local `sdkconfig.variant` had preserved stale settings and was overriding the new default.

That matters because it could have quietly invalidated later conclusions if I had trusted the first flashed image. The corrective action was:

- add the flag explicitly to `sdkconfig.internal_pool`
- delete the stale `build-internal-pool/sdkconfig.variant`
- rebuild with the explicit `-DSDKCONFIG=.../build-internal-pool/sdkconfig.variant` pattern

After reflashing, `wasm status` finally reported `embedded_load_copy_internal=yes`, which is the only point at which the mitigation results became trustworthy.

### 2026-03-23 14:24 EDT

Reran the old toxic plain path under the real mitigation:

- `wasm replay psram-persistent-init`
- `wasm load-only return-42`
- `wasm replay psram-persistent-touch-sync`

Result: success. The runtime log now shows:

- `binary_source=copied-internal`
- `wamr_rt_load.buf_external=no`
- later PSRAM touch still succeeds

That is the decisive reversal of the earlier failure mode. Plain `load-only` used to poison later PSRAM access when it read directly from the embedded flash-mapped buffer. With the mitigation enabled, the same user-facing command no longer does so.

### 2026-03-23 14:29 EDT

Extended the same check to the plain instantiate command, not the special copy-named helper:

- `wasm replay psram-persistent-init`
- `wasm instantiate-bare return-42`
- `wasm replay psram-persistent-touch-sync`

Result: also success. The log again shows `binary_source=copied-internal`, and the post-instantiate PSRAM touch remains healthy. This matters because it proves the mitigation is not just masking a loader-only issue. It survives the full load plus instantiate lifecycle on the ordinary command path.

### 2026-03-23 14:34 EDT

Ran the final plain command check:

- `wasm replay psram-persistent-init`
- `wasm run return-42`
- `wasm replay psram-persistent-touch-sync`

Result: success. The guest runs, returns `42`, deinstantiates, unloads, and the later persistent PSRAM write still succeeds.

At this point the mitigation is strong enough to treat as a working default for `0082`:

- plain `load-only` recovered
- plain `instantiate-bare` recovered
- plain `run` recovered

The remaining open question is no longer "does copy-before-load fix the PaperS3 repro?" It does. The remaining question is architectural: should the long-term fix stay as "copy embedded Wasm to RAM before load" or should we find a lower-level flash-mapping explanation and fix that instead?

## Related

- [design/01-embedded-buffer-mapping-investigation-plan.md](../design/01-embedded-buffer-mapping-investigation-plan.md)
- [tasks.md](../tasks.md)
