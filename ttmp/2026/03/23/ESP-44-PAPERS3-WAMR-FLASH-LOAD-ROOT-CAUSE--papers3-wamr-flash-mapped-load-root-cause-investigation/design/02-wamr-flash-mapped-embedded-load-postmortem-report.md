---
Title: WAMR flash-mapped embedded-load postmortem report
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
    - Path: 0082-papers3-wamr-allocator-control/main/wasm_module_registry.cpp
      Note: Defines the embedded Wasm assets whose flash-mapped storage semantics mattered to the bug
    - Path: 0082-papers3-wamr-allocator-control/main/wasm_module_runner.cpp
      Note: App-side source-buffer selection and load-method control used throughout the proof ladder
    - Path: 0082-papers3-wamr-allocator-control/main/wasm_replay_control.cpp
      Note: Contains the PSRAM control probes that turned the bug into a measurable same-boot post-load contamination test
    - Path: 0082-papers3-wamr-allocator-control/managed_components/espressif__wasm-micro-runtime/core/iwasm/interpreter/wasm_loader.c
      Note: Contains reuse_const_strings and the interpreter loader branch that determines whether the source buffer is reused in place
    - Path: 0082-papers3-wamr-allocator-control/managed_components/espressif__wasm-micro-runtime/core/iwasm/interpreter/wasm_runtime.c
      Note: Contains wasm_const_str_list_insert and the in-place mutation that rewrites flash-mapped export strings
ExternalSources: []
Summary: Detailed postmortem explaining how the apparent PaperS3 PSRAM/display/WAMR failure narrowed into a WAMR loader bug caused by in-place const-string mutation of flash-mapped embedded Wasm buffers.
LastUpdated: 2026-03-23T20:12:00-04:00
WhatFor: Explain the exact root cause, the investigation strategy, the proof ladder, and the safest production and upstream fixes for the embedded-Wasm load crash.
WhenToUse: Read when maintaining the embedded Wasm loader path, evaluating whether to keep the RAM-copy mitigation, or preparing an upstream WAMR fix/report.
---


# WAMR flash-mapped embedded-load postmortem report

## Executive summary

This ticket started as a broad “PaperS3 + WAMR + PSRAM + display” debugging problem and ended as a much narrower loader bug.

The root cause is now well explained:

- our embedded `.wasm` assets are linked into ESP-IDF firmware as flash-mapped read-only data
- WAMR’s interpreter loader has an optimization path that treats the input buffer as writable when `is_load_from_file_buf=true` and `wasm_binary_freeable=false`
- that path rewrites Wasm string bytes in place so the loader can reuse the original source buffer for C strings
- this is safe for writable RAM buffers and unsafe for embedded flash-mapped buffers
- taking that in-place rewrite path on-device later poisons PSRAM/cache behavior and eventually crashes the next PSRAM write with `Cache disabled but cached memory region accessed`

This was not just a workaround-level conclusion. We proved it in several independent ways:

- copying the embedded `.wasm` into RAM before `wasm_runtime_load(...)` avoids the failure
- using `wasm_runtime_load_ex(... wasm_binary_freeable=true)` avoids the failure while keeping the same embedded source pointer
- a stringless `empty-module.wasm` direct-embedded load does not fail
- the bad `return-42.wasm` direct-embedded load logs the exact in-place string mutations for `run` and `memory`
- patching the loader to keep direct embedded loading but forcibly disable `reuse_const_strings` makes the failure disappear

The rest of this report explains the whole system, the investigation history, the relevant code paths, the proof ladder, the remaining open questions, and the safest production options.

## Why this matters

This bug is a good example of a class of embedded-runtime failure that can waste a lot of time if the system boundaries are not made explicit early.

At first glance, the symptoms looked like one of these:

- PSRAM corruption
- e-ink driver corruption
- M5GFX bug
- ESP-IDF cache/MMU bug
- WAMR instantiate bug
- thread or `esp_console` context bug
- PaperS3 board-specific hardware issue

All of those were plausible. Several of them were tested seriously. The investigation only became efficient once we stopped asking “what subsystem is broken?” and started asking “what is the smallest operation after which a later PSRAM write becomes unsafe?”

That change in framing is the most important engineering lesson in this ticket.

## Audience and prerequisites

This report is written for a new intern who needs to understand:

- what `0079`, `0081`, and `0082` are
- how ESP-IDF embedded assets work
- how WAMR loader modes work at a high level
- why a flash-mapped read-only buffer is not the same thing as a file-backed writable buffer
- how to interpret the evidence chain in the diary and task history

You do not need to know all of WAMR internals ahead of time. You do need to be comfortable reading:

- C and C++
- ESP-IDF build and memory concepts
- basic Wasm loader terminology

## Scope of the report

This report covers:

- the system architecture we were debugging
- the exact symptom
- the investigation timeline
- the final root-cause explanation
- the proof hierarchy
- the remaining unknowns
- recommended fixes and upstream follow-up

This report does not attempt to be a full WAMR primer or a general ESP32-S3 memory textbook.

## System overview

### The projects involved

There are three key firmware projects in this investigation:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console`
  PaperS3 demo firmware with `esp_console`, embedded Wasm demo registry, and the original display-path experiments.
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0081-atoms3r-wamr-probe-console`
  AtomS3R comparison firmware used as a control board for the same WAMR behaviors.
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0082-papers3-wamr-allocator-control`
  reduced PaperS3 control harness that stripped the system down to console + WAMR lifecycle + PSRAM touch probes.

The root-cause proof mostly lives in `0082`, and the cross-board confirmation mostly lives in `0081`.

### High-level data flow

The app-side control path for an embedded Wasm module looks like this:

```text
embedded .wasm asset in firmware
        |
        v
WasmModuleDescriptor in wasm_module_registry.cpp
        |
        v
RunEmbeddedWasmModule(...) in wasm_module_runner.cpp
        |
        +--> choose source buffer mode
        |     - embedded-direct
        |     - copied-internal
        |     - copied-spiram
        |
        v
wasm_runtime_load(...) or wasm_runtime_load_ex(...)
        |
        v
WAMR interpreter loader
        |
        +--> parse sections
        +--> create internal loader structures
        +--> optionally reuse const strings from source buffer
        |
        v
later host-side PSRAM touch probe
```

### Important app-side files

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0082-papers3-wamr-allocator-control/main/wasm_module_registry.cpp`
  Defines the embedded Wasm modules and their metadata.
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0082-papers3-wamr-allocator-control/main/wasm_module_runner.cpp`
  Chooses the source-buffer mode, chooses the WAMR load method, and performs the runtime calls.
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0082-papers3-wamr-allocator-control/main/wasm_runtime_service.cpp`
  Initializes the runtime allocator and reports runtime memory state.
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0082-papers3-wamr-allocator-control/main/wasm_replay_control.cpp`
  Provides the control probes that touch PSRAM before and after WAMR lifecycle steps.

### Important WAMR files

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0082-papers3-wamr-allocator-control/managed_components/espressif__wasm-micro-runtime/core/iwasm/common/wasm_runtime_common.c`
  Entry points for `wasm_runtime_load(...)` and `wasm_runtime_load_ex(...)`.
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0082-papers3-wamr-allocator-control/managed_components/espressif__wasm-micro-runtime/core/iwasm/interpreter/wasm_loader.c`
  Interpreter loader path and the `reuse_const_strings` decision.
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0082-papers3-wamr-allocator-control/managed_components/espressif__wasm-micro-runtime/core/iwasm/interpreter/wasm_runtime.c`
  `wasm_const_str_list_insert(...)`, where the in-place source-buffer rewrite happens.

## The original symptom

The user-visible symptom was:

- load or run an embedded Wasm module
- later perform a PSRAM write or an e-ink display operation that eventually writes PSRAM-backed memory
- observe a crash with:

```text
Guru Meditation Error: Core / panic'ed (Cache disabled but cached memory region accessed)
Write back error occurred while dcache tries to write back to flash
```

That error message made the bug look like a cache or memory-mapping issue. That was directionally correct, but not specific enough.

## Why the bug looked larger than it was

The investigation started in `0079`, where the crashing path often involved:

- WAMR execution
- host drawing imports
- M5GFX
- PaperS3 e-ink display flushes

That produced a noisy system:

- WAMR was active
- the PaperS3 display stack was active
- PSRAM buffers were active
- M5GFX `Panel_EPD` was active

So the first story was naturally “WAMR somehow breaks the display path.”

That story was useful for early narrowing, but it was not the final one.

## Investigation strategy

### Strategy shift

The crucial strategy shift was this:

- old question: “which big subsystem is broken?”
- better question: “what is the smallest successful step after which a later PSRAM write becomes unsafe?”

That led to a sequence of reduced experiments:

1. prove whether guest execution is required
2. prove whether display code is required
3. prove whether WAMR instantiate is required
4. prove whether WAMR load alone is sufficient
5. prove whether all source-buffer forms are equally bad
6. prove whether in-place string mutation is the actual mechanism

### Reduction ladder

Here is the reduction ladder that mattered most:

```text
full Wasm run + display crash
    ->
instantiate-only still poisons PSRAM
    ->
load-only still poisons PSRAM
    ->
copied RAM source buffers do not poison PSRAM
    ->
direct embedded flash-mapped source buffers do
    ->
stringless direct embedded empty module does not
    ->
direct embedded return-42 does
    ->
binary_freeable direct embedded return-42 does not
    ->
disable reuse_const_strings in loader, plain direct runtime-load becomes safe
```

Once we reached the last step, the mechanism was effectively isolated.

## Key concepts an intern must understand

### Embedded Wasm assets are not ordinary files

Our embedded Wasm blobs are not being opened from a writable filesystem buffer. They are compiled into the firmware image with `EMBED_FILES` and end up as flash-mapped read-only data.

Conceptually:

```text
firmware image
  |
  +-- .text
  +-- .data
  +-- .bss
  +-- .flash.rodata
        |
        +-- embedded return-42.wasm bytes
        +-- embedded hello-frame.wasm bytes
```

That means a `const uint8_t *` to the embedded Wasm bytes behaves more like a memory-mapped ROM region than like a heap buffer.

### WAMR has two relevant loader behaviors

At a high level, WAMR has to decide whether it can reuse the source buffer for certain string/data representations or whether it must clone them.

In the interpreter loader, the relevant decision in [`wasm_loader.c`](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0082-papers3-wamr-allocator-control/managed_components/espressif__wasm-micro-runtime/core/iwasm/interpreter/wasm_loader.c) is:

```c
bool reuse_const_strings = is_load_from_file_buf && !wasm_binary_freeable;
bool clone_data_seg = is_load_from_file_buf && wasm_binary_freeable;
```

This means:

- if WAMR thinks it can reuse the input buffer, it will
- if it thinks the binary is “freeable,” it avoids that reuse path and clones where needed instead

### The failing in-place mutation

The critical code in [`wasm_runtime.c`](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0082-papers3-wamr-allocator-control/managed_components/espressif__wasm-micro-runtime/core/iwasm/interpreter/wasm_runtime.c) is:

```c
else if (is_load_from_file_buf) {
    char *c_str = (char *)str - 1;
    bh_memmove_s(c_str, len + 1, c_str + 1, len);
    c_str[len] = '\0';
    return c_str;
}
```

This rewrites the original input buffer in place:

- it moves the string back by one byte
- it overwrites the previous LEB-length byte
- it appends a null terminator

That is clever if the source buffer is writable RAM. It is not safe if the source buffer is flash-mapped read-only data.

### Why the crash appears later, during PSRAM touch

This is the part we can explain strongly but not perfectly mechanically.

What we proved:

- the direct in-place rewrite path is the trigger
- avoiding it avoids the later PSRAM crash

What we infer:

- rewriting the flash-mapped source buffer leaves the external-memory/cache subsystem in a bad state
- the visible failure may not happen at the exact write site
- instead, the system trips later when the next PSRAM writeback or cache interaction occurs

That is why the symptom often appears “later,” during:

- `psram-persistent-touch-sync`
- display buffer writes
- other PSRAM-backed operations

The important engineering point is that “later visible crash site” is not always “root cause site.”

## Exact proof steps

### Proof 1: RAM copy workaround works

In [`wasm_module_runner.cpp`](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0082-papers3-wamr-allocator-control/main/wasm_module_runner.cpp), we added source-buffer modes:

- `Embedded`
- `EmbeddedDirect`
- `CopiedToInternalRam`
- `CopiedToSpiram`

The simplified app-side logic is:

```text
if source == copied-internal:
    allocate internal RAM buffer
    memcpy(module.start -> buffer)
    pass buffer to WAMR
elif source == copied-spiram:
    allocate SPIRAM buffer
    memcpy(module.start -> buffer)
    pass buffer to WAMR
else:
    pass embedded pointer directly to WAMR
```

Result:

- copied-internal: safe
- copied-spiram: safe
- embedded-direct: unsafe for `return-42`

Interpretation:

- the problem is not “WAMR load always poisons PSRAM”
- the source-buffer form matters

### Proof 2: stringless embedded control is safe

We embedded `empty-module.wasm`, which is only the 8-byte Wasm header.

This module keeps:

- direct embedded loading

but removes:

- exports
- strings
- const-string rewrite opportunities

Result:

- direct embedded `empty-module` did not trigger the later PSRAM crash

Interpretation:

- direct embedded loading alone is not sufficient
- the module needs to exercise some specific loader behavior

### Proof 3: bad direct path logs in-place rewrites

We instrumented `wasm_const_str_list_insert(...)` and recorded:

- source pointer
- destination pointer (`str - 1`)
- string length
- previous byte

On bad `return-42` direct embedded loads, we observed two rewrites:

- `len=3`
- `len=6`

Then we matched those to the actual Wasm bytes:

- `run`
- `memory`

So the loader was not just “doing something different.” It was concretely rewriting the embedded export strings in place.

### Proof 4: `binary_freeable` avoids the rewrite and avoids the crash

Using `wasm_runtime_load_ex(...)` with `wasm_binary_freeable=true`:

- keeps the same embedded source pointer
- changes the loader ownership behavior
- avoids the in-place rewrite path

Result:

- no `mutate-in-place` logs
- no later PSRAM crash

This was the first strong causal A/B.

### Proof 5: disable only `reuse_const_strings`

This was the strongest proof.

We patched the loader to keep:

- direct embedded source pointer
- plain `runtime-load`

but force:

```c
bool reuse_const_strings = false;
```

Result:

- the previously bad direct embedded `return-42` path became safe
- later PSRAM touch succeeded

Interpretation:

- the critical mechanism is the const-string reuse/mutation path itself
- not just generic direct embedded loading
- not just `binary_freeable` as a package of unrelated behaviors

## Board comparison and why it mattered

Originally, the bug looked PaperS3-specific because the first dramatic failures were on PaperS3.

That made several board-level explanations attractive:

- external flash topology
- e-ink driver path
- PaperS3-specific cache interactions

So we created the AtomS3R comparison track in `ESP-45`.

### What the board comparison taught us

AtomS3R reproduced the same essential failure:

- direct embedded `return-42`
- same in-place `run` and `memory` rewrites
- later PSRAM crash

Then AtomS3R also passed the same controls:

- `binary_freeable` direct embedded path: safe
- direct embedded path with `reuse_const_strings=false`: safe

This changed the interpretation significantly:

- the core bug is not PaperS3-only board topology
- the core bug is a runtime/loader misuse of embedded flash-mapped Wasm source buffers

Board differences may still affect:

- exact timing
- exact backtrace site
- how often the bug manifests

But they are not the primary explanation anymore.

## Timeline of incorrect or incomplete theories

This section is important because good postmortems should show where the team’s mental model was wrong.

### Theory: display code is the root cause

Why it seemed plausible:

- early crashes appeared in PaperS3 display replay
- M5GFX `Panel_EPD` was in the stack

What falsified it:

- later PSRAM touch probes crashed even without display replay
- reduced control firmware reproduced the failure without app-owned display path

### Theory: guest execution is required

Why it seemed plausible:

- the user was running Wasm modules

What falsified it:

- `instantiate-only`
- then `load-only`

Eventually, load alone was enough.

### Theory: WAMR’s SPIRAM pool conflicts with ESP-IDF heap ownership

Why it seemed plausible:

- WAMR had an external-memory pool
- PSRAM was involved in the symptom

What falsified it:

- system allocator mode still reproduced the bug
- internal-pool variants did not remove it
- copied RAM source buffers fixed it without changing the high-level runtime architecture

### Theory: PaperS3 board topology is the root cause

Why it seemed plausible:

- PaperS3 showed the problem first
- its external flash arrangement differs from AtomS3R

What falsified it:

- AtomS3R reproduced the same direct embedded `return-42` failure

This does not mean board topology is irrelevant. It means it is not the core bug.

## API references and contracts

### App-side load entry points

The app uses:

- `wasm_runtime_load(...)`
- `wasm_runtime_load_ex(...)`

Relevant file:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0082-papers3-wamr-allocator-control/main/wasm_module_runner.cpp`

Important conceptual contract:

- if the runtime is going to mutate or retain pointers into the input binary buffer, the caller must pass a buffer whose mutability/lifetime matches that expectation

Our embedded firmware asset pointer does not satisfy that contract for the mutation path.

### WAMR loader contract problem

Today, the public knob that changes this behavior is `wasm_binary_freeable`.

That is not semantically ideal for our use case.

What we actually need is something more like:

- `source_buffer_writable`
- or `allow_source_buffer_const_string_reuse`

because “freeable” and “writable” are not the same property.

That is an important API design takeaway.

## Diagrams

### Failing path

```text
embedded return-42.wasm in flash-mapped .rodata
        |
        v
wasm_runtime_load(...)
        |
        v
load_from_sections(..., is_load_from_file_buf=true, wasm_binary_freeable=false)
        |
        v
reuse_const_strings = true
        |
        v
wasm_const_str_list_insert(...)
        |
        +--> mutate "run" in place
        +--> mutate "memory" in place
        |
        v
loader returns success
        |
        v
later PSRAM touch / cache interaction
        |
        v
Guru Meditation: Cache disabled but cached memory region accessed
```

### Safe path 1: copy to RAM first

```text
embedded return-42.wasm in flash-mapped .rodata
        |
        v
copy bytes into writable RAM buffer
        |
        v
wasm_runtime_load(...)
        |
        v
same reuse path is now safe enough for the source buffer
        |
        v
later PSRAM touch succeeds
```

### Safe path 2: keep embedded pointer, disable reuse path

```text
embedded return-42.wasm in flash-mapped .rodata
        |
        v
wasm_runtime_load_ex(... wasm_binary_freeable=true)
or patched loader with reuse_const_strings=false
        |
        v
no in-place const-string rewrite
        |
        v
later PSRAM touch succeeds
```

## Pseudocode summary

### Current bad logic

```pseudo
function load_wasm(buf, freeable):
    is_load_from_file_buf = true
    reuse_const_strings = is_load_from_file_buf and not freeable

    for each exported/imported/name string:
        if reuse_const_strings:
            mutate source buffer in place
        else:
            clone string
```

### Correct mental model

```pseudo
function load_wasm(buf, writable, reusable, lifetime_ok):
    if writable and reusable and lifetime_ok:
        reuse source buffer for strings
    else:
        clone strings
```

### Why the current public lever is awkward

```pseudo
freeable != writable
freeable != safe_to_mutate
freeable != embedded_flash_mapped
```

So the current WAMR API uses a knob that is too indirect for this case.

## What we proved vs what we inferred

### Proved

- direct embedded `return-42` is bad
- copied RAM source buffers are good
- direct embedded `empty-module` is good
- bad `return-42` path rewrites `run` and `memory` in place
- `binary_freeable` avoids that rewrite and avoids the crash
- forcibly disabling `reuse_const_strings` also avoids the crash
- AtomS3R reproduces the same core behavior

### Inferred

- the crash appears later because the invalid flash-buffer rewrite destabilizes later cache/writeback interactions, not because the later PSRAM touch is intrinsically wrong
- the system-visible error message is downstream of the real source-buffer misuse

These inferences are strong, but they remain inferences because we did not instrument the lowest-level flash/PSRAM/cache hardware path directly.

## Production options

### Option 1: keep current mitigation

Always copy embedded Wasm into internal RAM before load.

Pros:

- simple
- robust
- already proven
- no vendor patch required

Cons:

- extra RAM copy
- somewhat wasteful for larger modules
- works around the bug instead of expressing the real contract

### Option 2: narrower app-side fix

Use `wasm_runtime_load_ex(... wasm_binary_freeable=true)` for embedded assets in a way that avoids the rewrite path.

Pros:

- narrower than copying every module
- preserves direct embedded source pointer

Cons:

- semantically awkward because `freeable` is not really the property we mean
- may be surprising to future maintainers

### Option 3: upstream-quality fix

Patch WAMR so the loader has an explicit notion of:

- source buffer is writable
- source buffer may be reused for const strings

Pros:

- correct API shape
- removes ambiguity
- best long-term fix

Cons:

- requires vendor patch or upstream contribution
- higher process cost than the local mitigation

## Recommended conclusion

If the goal is immediate product progress, keep the copy-before-load mitigation.

If the goal is correctness and maintainability, follow up with a WAMR patch that decouples:

- “buffer lifetime/ownership”
- “buffer mutability”
- “safe to reuse source bytes for const strings”

The investigation is now complete enough that we should not spend more time proving the same mechanism from another angle.

## Suggested upstream issue text

The essential upstream bug report would be:

- embedded Wasm binaries in ESP-IDF may be flash-mapped read-only buffers
- WAMR interpreter loader currently reuses and mutates input-buffer const strings when `is_load_from_file_buf=true` and `wasm_binary_freeable=false`
- this assumes the input buffer is writable
- that assumption is invalid for embedded flash-mapped Wasm assets
- the loader should expose an explicit “writable/reusable source buffer” decision instead of overloading `wasm_binary_freeable`

## Review checklist for a new engineer

- Read [`wasm_module_runner.cpp`](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0082-papers3-wamr-allocator-control/main/wasm_module_runner.cpp) and identify where `EmbeddedDirect`, `CopiedToInternalRam`, and `CopiedToSpiram` diverge.
- Read [`wasm_loader.c`](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0082-papers3-wamr-allocator-control/managed_components/espressif__wasm-micro-runtime/core/iwasm/interpreter/wasm_loader.c) and find `reuse_const_strings`.
- Read [`wasm_runtime.c`](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0082-papers3-wamr-allocator-control/managed_components/espressif__wasm-micro-runtime/core/iwasm/interpreter/wasm_runtime.c) and confirm the in-place mutation logic.
- Read the ticket diary in [01-diary.md](../reference/01-diary.md) from top to bottom to see how the scope narrowed over time.
- Read the saved patch artifacts in `scripts/wamr-patches/`.

## Related artifacts

- plan doc: [01-flash-load-root-cause-plan.md](./01-flash-load-root-cause-plan.md)
- diary: [../reference/01-diary.md](../reference/01-diary.md)
- trace patch: [../scripts/wamr-patches/01-wasm-runtime-const-str-trace.diff](../scripts/wamr-patches/01-wasm-runtime-const-str-trace.diff)
- proof patch: [../scripts/wamr-patches/02-disable-reuse-const-strings.diff](../scripts/wamr-patches/02-disable-reuse-const-strings.diff)

## Final statement

The bug is no longer “mysterious PSRAM corruption after WAMR.”

It is:

- a loader bug caused by treating embedded flash-mapped Wasm input as writable
- specifically through WAMR’s in-place const-string reuse path
- with the later PSRAM crash acting as the downstream visible symptom rather than the original mistake
