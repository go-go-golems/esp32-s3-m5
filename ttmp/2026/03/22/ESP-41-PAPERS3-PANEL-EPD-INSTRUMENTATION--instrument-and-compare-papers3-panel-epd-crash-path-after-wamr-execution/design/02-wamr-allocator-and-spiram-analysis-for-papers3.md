---
Title: WAMR Allocator and SPIRAM Analysis for PaperS3
Ticket: ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION
Status: active
Topics:
    - papers3
    - wasm
    - firmware
    - esp-idf
    - debugging
    - display
    - m5gfx
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp
      Note: |-
        Runtime telemetry and instantiate sequencing used to observe pool usage and PSRAM contamination after instantiate.
        Instantiate-time memory telemetry and pool statistics
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp
      Note: |-
        Application-side WAMR initialization chooses Alloc_With_Pool and allocates the SPIRAM-backed runtime pool.
        App-side WAMR initialization and SPIRAM pool setup
    - Path: 0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/common/wasm_memory.c
      Note: |-
        Core runtime allocator-mode switch and linear-memory allocation path.
        Allocator-mode dispatch and linear-memory allocation path
    - Path: 0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/mem-alloc/mem_alloc.c
      Note: |-
        Suballocator implementation used inside the WAMR pool, currently EMS by default.
        Internal pool suballocator implementation
    - Path: 0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_memmap.c
      Note: |-
        ESP-IDF-specific os_mmap/os_munmap implementation that places linear memory in internal RAM for this project.
        ESP-IDF os_mmap path for linear memory and executable mappings
    - Path: 0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/shared_platform.cmake
      Note: |-
        Project-local override forcing WASM_MEM_DUAL_BUS_MIRROR=0 on PaperS3.
        Project-local WASM_MEM_DUAL_BUS_MIRROR override
ExternalSources: []
Summary: Detailed intern-facing guide to the WAMR allocator model used in 0079, with emphasis on how SPIRAM is involved, what is and is not allocated from the WAMR pool, and why that matters for the surviving PaperS3 PSRAM contamination bug.
LastUpdated: 2026-03-23T10:44:18.795469567-04:00
WhatFor: Explain the exact memory-allocation architecture of the current PaperS3 WAMR integration so later debugging work does not confuse WAMR's internal pool allocator with ESP-IDF heap allocations or linear-memory mappings.
WhenToUse: Read this before changing allocator settings, interpreting WAMR memory telemetry, or debugging PaperS3 SPIRAM/cache failures that appear after WAMR instantiate.
---


# WAMR Allocator and SPIRAM Analysis for PaperS3

## Executive Summary

The short answer is:

- Yes, this project gives WAMR a large block of SPIRAM-backed memory.
- No, that does **not** mean WAMR and ESP-IDF or FreeRTOS are both directly managing the same bytes at the same time.
- The current `0079` integration uses **two distinct memory paths**:
  - a **runtime object allocator** based on `Alloc_With_Pool`
  - a **linear-memory allocator** based on `os_mmap()`

That distinction matters because our live PaperS3 bug reproduces even when:

- WAMR linear memory is placed in **internal RAM**
- the persistent PSRAM probe buffer was allocated **before** WAMR instantiate
- the WAMR runtime itself initializes successfully

So the most useful current mental model is:

- WAMR's **runtime-owned objects** mostly come from a custom WAMR pool created inside a single external-RAM allocation
- WAMR **linear memory** currently comes from a separate ESP-IDF `os_mmap()` path, and in this project we intentionally bias that path toward internal RAM
- the surviving PaperS3 issue is **not** well explained by a simple "heap manager conflict"

It is more likely to involve one of these:

- memory corruption in or around WAMR instantiate
- a PaperS3-specific external-memory/cache side effect during instantiate
- a board-specific interaction between WAMR's memory setup and later CPU writes into PSRAM

## Why This Guide Exists

During the debugging conversation, an easy misunderstanding surfaced: "WAMR allocates in SPIRAM, so maybe it is conflicting with the FreeRTOS heap." That is close enough to sound plausible, but it is too imprecise to support good debugging.

This guide exists to fix that ambiguity. A new intern should come away understanding:

- what "Alloc_With_Pool" actually means in WAMR
- which memory objects are pool-backed and which are not
- how ESP-IDF `heap_caps_*` functions still matter even when WAMR uses its own pool
- why the `WASM_MEM_DUAL_BUS_MIRROR` compile flag matters even in an interpreter-first project
- why our current telemetry showed a `WAMR pool in SPIRAM` and `linear memory in internal RAM` at the same time

## System Overview

At a high level, the current `0079` PaperS3 demo stack looks like this:

```text
+-------------------------------------------------------------+
| 0079 application                                            |
|                                                             |
| - console commands                                          |
| - wasm_runtime_service.cpp                                  |
| - wasm_module_runner.cpp                                    |
| - replay controls / probes                                  |
+-----------------------------+-------------------------------+
                              |
                              v
+-------------------------------------------------------------+
| WAMR public runtime API                                     |
|                                                             |
| - wasm_runtime_full_init(...)                               |
| - wasm_runtime_load(...)                                    |
| - wasm_runtime_instantiate(...)                             |
| - wasm_runtime_call_wasm(...)                               |
| - wasm_runtime_malloc(...)                                  |
+-----------------------------+-------------------------------+
                              |
                 +------------+-------------+
                 |                          |
                 v                          v
+--------------------------------+  +----------------------------------+
| Runtime object allocator       |  | Linear memory allocator          |
|                                |  |                                  |
| - Alloc_With_Pool              |  | - wasm_allocate_linear_memory    |
| - mem_allocator_create(...)    |  | - wasm_mmap_linear_memory(...)   |
| - mem_allocator_malloc(...)    |  | - os_mmap(...)                   |
+--------------------------------+  +----------------------------------+
                 |                          |
                 v                          v
         SPIRAM-backed pool         Internal RAM in current 0079 build
         in current 0079 build      (after local PaperS3 fix)
```

The most important architectural point is that the left side and the right side are related, but they are not the same allocator.

## The Three Allocator Modes WAMR Exposes

WAMR exposes three runtime allocator modes in [wasm_export.h](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/include/wasm_export.h):

- `Alloc_With_Pool`
- `Alloc_With_Allocator`
- `Alloc_With_System_Allocator`

These are declared around [wasm_export.h:172](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/include/wasm_export.h#L172).

The associated option union is `MemAllocOption`, which can carry either:

- a `heap_buf + heap_size` pair for `Alloc_With_Pool`
- function pointers for `Alloc_With_Allocator`

### What Each Mode Means

`Alloc_With_Pool`

- The embedder gives WAMR one large buffer.
- WAMR creates an internal allocator over that buffer.
- Most runtime-owned objects are then allocated *inside that pool*.

`Alloc_With_Allocator`

- The embedder gives WAMR function pointers like `malloc`, `realloc`, and `free`.
- WAMR calls those functions directly for runtime-owned objects.
- In this mode, allocator instrumentation at the host heap layer is much more directly informative.

`Alloc_With_System_Allocator`

- WAMR uses the platform's default `os_malloc`, `os_realloc`, and `os_free`.
- On ESP-IDF that usually means following the platform's normal allocation path.

## Which Mode 0079 Uses

The current PaperS3 project chooses `Alloc_With_Pool` explicitly in [wasm_runtime_service.cpp:74](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp#L74).

The critical code in [wasm_runtime_service.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp) does this:

```cpp
g_runtime_pool_buffer = heap_caps_malloc(kRuntimePoolSizeBytes,
                                         MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
if (g_runtime_pool_buffer == nullptr) {
    g_runtime_pool_buffer = heap_caps_malloc(kRuntimePoolSizeBytes,
                                             MALLOC_CAP_8BIT);
}

init_args.mem_alloc_type = Alloc_With_Pool;
init_args.mem_alloc_option.pool.heap_buf = g_runtime_pool_buffer;
init_args.mem_alloc_option.pool.heap_size = kRuntimePoolSizeBytes;
```

So the ownership chain is:

1. ESP-IDF heap allocates one large `512 KiB` block.
2. That block is preferably in `SPIRAM`.
3. WAMR builds its own allocator *inside that block*.

That means the correct sentence is:

"WAMR uses a SPIRAM-backed pool buffer allocated from ESP-IDF."

It is **not** correct to say:

"WAMR and ESP-IDF are both independently allocating and freeing the same individual runtime objects from the same heap region."

## The Current Concrete Pool Configuration

The project-level configuration is visible in [wasm_runtime_service.cpp:19](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp#L19):

- `kRuntimePoolSizeBytes = 512 * 1024`

The runtime status printer now exposes:

- `wamr.pool_buffer`
- `wamr.pool_buffer_external`
- `wamr.pool_size`
- `wamr.heap_total`
- `wamr.heap_free`
- `wamr.heap_highmark`

Those come from:

- [wasm_runtime_service.cpp:34](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp#L34)
- [wasm_runtime_service.cpp:139](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp#L139)

In the latest traced PaperS3 run, that reported values like:

- `wamr.pool_buffer=0x3c1bd244`
- `wamr.pool_buffer_external=yes`
- `wamr.pool_size=524288`
- `wamr.heap_total=524096`

Notice that `wamr.heap_total` is slightly smaller than the raw pool size. That is expected because the pool allocator needs metadata and structure space for itself.

## What Suballocator Lives Inside the Pool

Inside WAMR, `Alloc_With_Pool` eventually calls `mem_allocator_create(...)` in [mem_alloc.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/mem-alloc/mem_alloc.c).

The default allocator selection is declared in [config.h:64](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/config.h#L64):

- `MEM_ALLOCATOR_EMS = 0`
- `MEM_ALLOCATOR_TLSF = 1`
- `DEFAULT_MEM_ALLOCATOR = MEM_ALLOCATOR_EMS`

That means the current build is using the `EMS`-backed path, not `TLSF`, unless some other compile override changes it.

In [mem_alloc.c:9](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/mem-alloc/mem_alloc.c#L9), the active branch is:

```c
#if DEFAULT_MEM_ALLOCATOR == MEM_ALLOCATOR_EMS
    return gc_init_with_pool((char *)mem, size);
#else
    tlsf_create_with_pool(...)
#endif
```

So the pool behavior in this project is:

- one large host-supplied buffer
- EMS allocator initialized on top of it
- `wasm_runtime_malloc()` and `wasm_runtime_free()` route into EMS when `memory_mode == MEMORY_MODE_POOL`

The key internal dispatch is in [wasm_memory.c:873](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/common/wasm_memory.c#L873):

```c
if (memory_mode == MEMORY_MODE_POOL) {
    return mem_allocator_malloc(pool_allocator, size);
}
```

and free is symmetric:

```c
if (memory_mode == MEMORY_MODE_POOL) {
    mem_allocator_free(pool_allocator, ptr);
}
```

## Important Consequence: Heap Telemetry Only Shows Part of the Story

This is where many debugging attempts go wrong.

If you only look at:

- `heap_caps_get_free_size(...)`
- `heap_caps_check_integrity(...)`
- `heap_trace_*`

you are seeing the **host heap layer**, not the internal state of the WAMR pool allocator.

That means:

- the initial `512 KiB` pool allocation is visible at the host heap layer
- later WAMR pool allocations are **not** individually visible as fresh ESP-IDF heap allocations

The correct observability stack is:

```text
ESP-IDF heap telemetry
    sees:
    - pool buffer allocation
    - linear-memory mmap allocations
    - any non-pooled host allocations

WAMR pool telemetry
    sees:
    - pool total size
    - free bytes inside the pool
    - pool high-water mark
```

That is exactly why the new telemetry in [wasm_module_runner.cpp:147](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp#L147) was useful: it let us see the WAMR pool shrinking during load and instantiate even when `spiram_free` at the ESP-IDF layer barely moved.

## What Actually Uses the Pool

Most runtime-owned objects are allocated through `wasm_runtime_malloc()` or helpers that call it. That includes many pieces in [wasm_runtime_common.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/common/wasm_runtime_common.c), such as:

- registered-module nodes
- loading-module structures
- argv/result temporaries
- WASI context objects
- thread argument objects
- many other runtime-side metadata structures

Because `0079` uses `Alloc_With_Pool`, most of those are effectively:

```text
wasm_runtime_malloc(...)
    -> wasm_runtime_malloc_internal(...)
        -> mem_allocator_malloc(pool_allocator, size)
            -> EMS allocator inside the 512 KiB pool
```

### Pseudocode

```c
void *app_runtime_pool = heap_caps_malloc(512 KiB, SPIRAM | 8BIT);

RuntimeInitArgs args = {
  .mem_alloc_type = Alloc_With_Pool,
  .mem_alloc_option.pool.heap_buf = app_runtime_pool,
  .mem_alloc_option.pool.heap_size = 512 KiB,
};

wasm_runtime_full_init(&args);

// later
module_inst = wasm_runtime_instantiate(...);

// inside WAMR
ptr = wasm_runtime_malloc(sizeof(ModuleInstance));
// becomes:
ptr = mem_allocator_malloc(pool_allocator, sizeof(ModuleInstance));
```

## What Does Not Primarily Use the Pool

The most important exception is **linear memory**.

Linear memory is handled in [wasm_allocate_linear_memory(...)](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/common/wasm_memory.c#L1963).

In the current build path, this goes through:

- `wasm_mmap_linear_memory(...)`
- `wasm_mremap_linear_memory(...)`
- `os_mmap(...)`

not through the WAMR runtime pool.

The relevant flow is:

```text
wasm_runtime_instantiate(...)
  -> memory_instantiate(...)
     -> wasm_allocate_linear_memory(...)
        -> wasm_mmap_linear_memory(...)
           -> os_mmap(...)
```

That is why our traced logs could simultaneously show:

- pool-backed module structures at `0x3c...` external-RAM-looking addresses
- linear memory at `0x3fcb...` internal-RAM-looking addresses

Those are two different allocation subsystems.

## Why `os_mmap()` Matters More Than It Sounds Like

The name `os_mmap()` sounds like a normal POSIX virtual-memory detail, but on ESP-IDF it is a platform abstraction implemented in [espidf_memmap.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_memmap.c).

This is one of the highest-value files in the PaperS3 investigation because it decides where WAMR places:

- executable AOT memory
- non-executable linear-memory mappings

### Current Project-Specific Behavior

For non-executable mappings, `0079` currently uses this local logic in [espidf_memmap.c:116](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_memmap.c#L116):

```c
/* Prefer internal RAM for linear-memory buffers on ESP32-S3.
 * Using cached external RAM here can trip cache-disabled panics
 * during WAMR instantiate on PaperS3. */
uint32_t mem_caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
```

That comment is not upstream-generic theory. It is a project-local debugging decision that came out of this exact PaperS3 work.

So today:

- runtime pool prefers `SPIRAM`
- linear memory prefers `INTERNAL RAM`

This split is deliberate.

## The `WASM_MEM_DUAL_BUS_MIRROR` Flag

This is probably the "compile flag related to SPIRAM" that looked suspicious in the discussion.

The flag is documented in [config.h:642](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/config.h#L642) and forced locally in [shared_platform.cmake:22](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/shared_platform.cmake#L22):

```cmake
add_definitions(-DWASM_MEM_DUAL_BUS_MIRROR=0)
```

### What It Means

When `WASM_MEM_DUAL_BUS_MIRROR != 0`, the ESP-IDF platform layer assumes some executable external memory needs to be handled through mirrored instruction/data bus regions.

In that mode:

- executable mappings may be created in SPIRAM-like space
- cache flush paths become more aggressive
- special mirror conversion logic is active

In our current PaperS3 work, that path was judged too risky, so it was forced off.

### Why It Matters Even in an Interpreter-First Project

Two reasons:

- it changes the implementation of `os_mmap()` and `os_dcache_flush()`
- it changes the set of possible external-memory addresses and cache operations the runtime may exercise

So even though `0079` is interpreter-first and `AOT` is disabled in [sdkconfig.defaults](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/sdkconfig.defaults), the flag still matters because it changes the platform memory implementation compiled into the component.

## How SPIRAM Is Used in the Current PaperS3 WAMR Stack

Here is the most accurate simplified picture:

```text
SPIRAM
  |
  +-- PaperS3 framebuffers and probe buffers
  |     - e-ink framebuffer in M5GFX Panel_EPD
  |     - persistent PSRAM probe buffers in wasm_replay_control.cpp
  |
  +-- WAMR runtime pool buffer
        - one 512 KiB external allocation
        - WAMR pool allocator suballocates runtime objects inside it

Internal RAM
  |
  +-- WAMR linear memory in current PaperS3 build
        - allocated through os_mmap()
        - intentionally biased toward MALLOC_CAP_INTERNAL
```

This is the reason the current bug is so interesting:

- WAMR runtime data is partly in SPIRAM
- WAMR linear memory is in internal RAM
- a persistent PSRAM probe buffer allocated before instantiate still becomes unsafe after instantiate

That combination strongly suggests the problem is not simply:

"WAMR put guest memory in PSRAM and later wrote to it incorrectly."

## Why This Is Not a Straightforward Heap Conflict

It is worth stating the false model explicitly.

### Incorrect model

"WAMR allocates in SPIRAM and FreeRTOS allocates in SPIRAM, so they are probably both managing the same heap and conflicting."

### Better model

- ESP-IDF owns the large external allocation that becomes the WAMR pool.
- WAMR owns the suballocation policy inside that pool.
- FreeRTOS itself is not the key allocator API here; the relevant host allocator is ESP-IDF `heap_caps`.

So the likely failure classes are narrower:

- WAMR pool corruption
- overwrite outside a pool allocation
- bug in instantiate/deinstantiate path
- ESP-IDF external-memory/cache interaction triggered by WAMR setup
- PaperS3-specific board effect that AtomS3R does not share

## Why AtomS3R Was Useful

The AtomS3R cross-check showed that the recovered WAMR integration can succeed on another `ESP32-S3 + PSRAM` target after the same baseline fixes.

That means the allocator story is not:

- "any WAMR pool in PSRAM on any ESP32-S3 is broken"

Instead, the remaining suspicion is more board-specific:

- PaperS3 flash/PSRAM/cache topology
- PaperS3 display and external-memory coexistence
- or a PaperS3-specific sensitivity in the instantiate path

## What the Current Telemetry Proved

The current telemetry from:

- [wasm_runtime_service.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp)
- [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)
- WAMR-side logging in [wasm_memory.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/common/wasm_memory.c)
- and [espidf_memmap.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_memmap.c)

proved these points:

- the runtime pool buffer is external RAM
- WAMR pool free space decreases during `load` and `instantiate`
- linear memory mapping succeeds in internal RAM
- no module instantiate means the persistent PSRAM touch succeeds
- module instantiate is sufficient to poison the later persistent PSRAM write on PaperS3

## What We Still Do Not Know

The allocator analysis narrows the problem, but it does not finish it.

Still open:

- Is the pool allocator itself corrupting memory?
- Is instantiate touching some external-memory state outside the pool?
- Is there a cache, writeback, or MMU transition in the ESP-IDF path that leaves later CPU writes to PSRAM unsafe on PaperS3?
- Is there some overlap or aliasing bug involving the WAMR pool allocation and other PSRAM users?

## Practical Debugging Implications

If a future intern wants to continue the investigation, the allocator model suggests these rules.

### Use WAMR pool telemetry when asking:

- how much runtime-owned metadata was allocated
- whether instantiate consumed pool space
- whether the runtime object allocator is leaking or fragmenting

### Use ESP-IDF heap telemetry when asking:

- where the initial pool buffer came from
- where `os_mmap()` linear memory came from
- whether non-pooled SPIRAM or internal-RAM allocations changed
- whether heap integrity checks fail at the host allocator layer

### Do not assume:

- a drop in `spiram_free` means "that was definitely a WAMR object"
- a stable `spiram_free` means "WAMR allocated nothing important"

Those inferences are wrong when `Alloc_With_Pool` is active.

## Recommended Mental Checklist

Before interpreting a memory observation, ask:

1. Is this object allocated through `wasm_runtime_malloc()`?
2. If yes, is runtime memory in `Alloc_With_Pool`, `Alloc_With_Allocator`, or `Alloc_With_System_Allocator` mode?
3. If it is pool mode, am I looking at host heap telemetry or pool telemetry?
4. Is the object actually linear memory instead of runtime metadata?
5. Is the code path using `os_mmap()` rather than the runtime pool?

## Pseudocode Model of the Current PaperS3 Integration

```c
// app startup
pool = heap_caps_malloc(512 KiB, SPIRAM | 8BIT);

init_args.mem_alloc_type = Alloc_With_Pool;
init_args.mem_alloc_option.pool.heap_buf = pool;
init_args.mem_alloc_option.pool.heap_size = 512 KiB;

wasm_runtime_full_init(&init_args);

// later, module load and instantiate
module = wasm_runtime_load(bytes);
module_inst = wasm_runtime_instantiate(module, guest_stack, guest_heap);

// runtime-owned objects
registered_nodes = wasm_runtime_malloc(...);   // from pool
module_inst_obj  = wasm_runtime_malloc(...);   // from pool

// linear memory
memory_data = os_mmap(...);                    // current PaperS3 build: internal RAM preferred
```

## API Reference

Important APIs and what they mean in this project:

- `wasm_runtime_full_init(...)`
  - initializes WAMR with the chosen allocator mode
- `wasm_runtime_malloc(...)`
  - runtime-owned object allocation, usually pool-backed in `0079`
- `wasm_runtime_get_mem_alloc_info(...)`
  - reports allocator stats only when runtime memory is in pool mode
- `wasm_allocate_linear_memory(...)`
  - guest linear memory path
- `os_mmap(...)`
  - ESP-IDF platform hook used by the linear-memory path
- `heap_caps_malloc(...)`
  - ESP-IDF allocator used by the application to create the WAMR pool buffer
- `esp_ptr_external_ram(...)`
  - host-side classification helper used in our telemetry

## File Reference Map

Application layer:

- [wasm_runtime_service.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp)
- [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)

WAMR public types and allocator mode enums:

- [wasm_export.h](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/include/wasm_export.h)

WAMR runtime memory mode and pool dispatch:

- [wasm_memory.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/common/wasm_memory.c)
- [wasm_memory.h](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/common/wasm_memory.h)

WAMR internal pool suballocator:

- [mem_alloc.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/mem-alloc/mem_alloc.c)
- [mem_alloc.h](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/mem-alloc/mem_alloc.h)

ESP-IDF platform memory mapping:

- [espidf_memmap.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_memmap.c)
- [shared_platform.cmake](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/shared_platform.cmake)

## Final Conclusion

The current PaperS3 setup does **not** look like a naive allocator conflict where WAMR and the system heap are both managing the same runtime objects.

The real picture is more specific:

- WAMR runtime objects mostly live inside a single SPIRAM-backed pool
- linear memory currently lives in internal RAM
- the contamination bug is triggered by module instantiate, not by runtime startup alone
- therefore the most plausible remaining failure is some instantiate-time corruption or PaperS3-specific external-memory/cache side effect, not a generic "WAMR uses SPIRAM so the heap managers must be fighting" explanation

That distinction is the main thing future debugging work must preserve.
