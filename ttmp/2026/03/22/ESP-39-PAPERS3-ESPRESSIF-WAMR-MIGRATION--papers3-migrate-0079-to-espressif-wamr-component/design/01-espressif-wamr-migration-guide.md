---
Title: Espressif WAMR migration guide
Ticket: ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION
Status: active
Topics:
    - papers3
    - wasm
    - firmware
    - esp-idf
    - debugging
    - architecture
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0079-papers3-wamr-assemblyscript-console/dependencies.lock
      Note: |-
        The resolved dependency record used to verify package identity and version after migration
        Resolved dependency record proving the migration landed on espressif/wasm-micro-runtime 2.4.0~1
    - Path: 0079-papers3-wamr-assemblyscript-console/main/CMakeLists.txt
      Note: |-
        The main app component declaration that must reference the correct component alias
        Main app component alias updated to the Espressif package name
    - Path: 0079-papers3-wamr-assemblyscript-console/main/idf_component.yml
      Note: |-
        The manifest where the upstream dependency is currently declared
        Manifest switched from the upstream git dependency to Espressif's registry package
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp
      Note: |-
        Runtime status and config-symbol usage that provide fast feedback if Kconfig names changed
        Runtime config macros and status reporting compiled cleanly after the package swap
    - Path: 0079-papers3-wamr-assemblyscript-console/sdkconfig.defaults
      Note: |-
        WAMR feature-selection defaults that must still align with the migrated component
        Existing WAMR config surface validated against the migrated package
ExternalSources:
    - https://components.espressif.com/components/espressif/wasm-micro-runtime/versions/2.4.0~1
    - https://components.espressif.com/components/espressif/wasm-micro-runtime/versions/2.4.0/dependencies?language=en
    - https://components.espressif.com/components/espressif/wasm-micro-runtime/versions/2.4.0/examples/esp-idf
Summary: Detailed implementation guide for migrating `0079` from the upstream Bytecode Alliance WAMR git dependency to Espressif's official Component Registry package.
LastUpdated: 2026-03-22T19:52:00-04:00
WhatFor: Explain exactly how the WAMR dependency is currently wired, what changes when moving to the Espressif package, and how to validate the migration without confusing build issues with runtime issues.
WhenToUse: Read this before editing the WAMR dependency manifest or trying to interpret migration build failures.
---


# Espressif WAMR Migration Guide

## Goal

This ticket is not trying to solve the full PaperS3 runtime instability. It is trying to change one variable cleanly:

- current state: `0079` consumes WAMR from the upstream `bytecodealliance/wasm-micro-runtime` git repository
- target state: `0079` consumes WAMR from Espressif's official `espressif/wasm-micro-runtime` component package

That distinction matters because the project already reached a useful but uncomfortable conclusion in `ESP-38`:

- the replay crash is real
- it survives several isolation experiments
- the remaining suspicion has shifted toward the runtime/platform boundary

At that point, the highest-value next experiment is a package/integration A/B test, not another speculative low-level patch.

## Why This Migration Is Worth Doing

As of March 22, 2026, Espressif's official registry lists:

- package: `espressif/wasm-micro-runtime`
- latest stable version shown on the dependency page: `2.4.0~1`
- dependency floor: `ESP-IDF >=5.1`

That matters for `0079` because:

- the project is already on `ESP-IDF 5.3.4`
- the ticket history has accumulated local assumptions around the upstream integration
- a successful switch to the official package gives us a better-supported baseline for the next hardware comparison

## Current System State

Today the dependency enters the project through:

- [main/idf_component.yml](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/idf_component.yml)

Current shape:

```yaml
dependencies:
  idf:
    version: ">=5.3.4,<5.4.0"
  bytecodealliance/wasm-micro-runtime:
    git: https://github.com/bytecodealliance/wasm-micro-runtime.git
    version: main
```

The main app component then depends on the generated alias:

- [main/CMakeLists.txt](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/CMakeLists.txt)

Current alias:

```cmake
REQUIRES
    M5Unified
    bytecodealliance__wasm-micro-runtime
    console
```

The resolved dependency state currently confirms the upstream package:

- [dependencies.lock](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/dependencies.lock)

Important detail:

- the lockfile today points at the upstream git repository, not the Espressif registry package

## What Is Expected To Change

The migration should change three things and only three things in the first slice:

1. The dependency manifest should name Espressif's package instead of the upstream git package.
2. The main component should require the Espressif-generated alias instead of the upstream alias.
3. The lockfile and `managed_components/` state should regenerate to reflect the new package identity.

Everything else should initially be treated as suspicious churn.

That means this ticket should not start by:

- rewriting WAMR runtime code
- changing the host ABI
- changing the replay pipeline
- changing the Wasm demos
- changing the display stack

This is a dependency swap first.

## Risks To Watch

### Risk 1: Component Alias Changes

ESP Component Manager turns a package name like:

- `bytecodealliance/wasm-micro-runtime`

into a CMake component alias like:

- `bytecodealliance__wasm-micro-runtime`

So the Espressif package is expected to become:

- `espressif__wasm-micro-runtime`

If the CMake alias is wrong, the build will fail early with component resolution errors.

### Risk 2: Kconfig Surface Changes

The code and defaults currently rely on symbols such as:

- `CONFIG_WAMR_ENABLE_INTERP`
- `CONFIG_WAMR_ENABLE_AOT`
- `CONFIG_WAMR_INTERP_FAST`
- `CONFIG_WAMR_ENABLE_LIB_PTHREAD`

Those appear in:

- [sdkconfig.defaults](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/sdkconfig.defaults)
- [main/wasm_runtime_service.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp)

If Espressif renamed or removed those symbols, the build will fail or the runtime status printouts will become misleading.

### Risk 3: Lockfile and Managed Components Lag

ESP-IDF's Component Manager may continue to use stale resolved state if the project is not forced through a fresh dependency solve.

That means a migration is not complete just because the manifest file changed. You must also inspect:

- `dependencies.lock`
- `managed_components/`

## Implementation Plan

### Step 1: Swap the Manifest Dependency

Edit:

- [main/idf_component.yml](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/idf_component.yml)

Expected direction:

```yaml
dependencies:
  idf:
    version: ">=5.3.4,<5.4.0"
  espressif/wasm-micro-runtime:
    version: "^2.4.0~1"
```

Reasoning:

- use the official package
- pin to a stable version line rather than `main`
- reduce ambiguity when comparing later behavior

### Step 2: Swap the CMake Component Alias

Edit:

- [main/CMakeLists.txt](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/CMakeLists.txt)

Expected direction:

```cmake
REQUIRES
    M5Unified
    espressif__wasm-micro-runtime
    console
```

Reasoning:

- the dependency package namespace changes
- the generated CMake alias is namespaced by vendor

### Step 3: Regenerate the Dependency Resolution

Build the project in a way that forces Component Manager to fetch and resolve the new package.

Expected outputs:

- `dependencies.lock` now names `espressif/wasm-micro-runtime`
- `managed_components/` contains an Espressif-prefixed component directory

### Step 4: Validate the Build Surface

Do not start with hardware. First confirm:

- the firmware config still parses
- the build still compiles
- the runtime service still sees the expected config symbols

### Step 5: Record the Exact Resolved State

After build success, record:

- resolved package name
- resolved version
- any lockfile changes
- any new warnings

This matters because a later runtime comparison only makes sense if we know exactly what package was built.

## Pseudocode For The Migration

```text
inspect current manifest
inspect current CMake component alias

replace upstream dependency with espressif package
replace upstream alias with espressif alias

run fresh build

if component resolution fails:
    inspect alias naming and manifest syntax

if Kconfig symbols fail:
    inspect sdkconfig.defaults and runtime_service config usage

if build succeeds:
    record resolved package/version in lockfile and ticket
```

## File Walkthrough

### `main/idf_component.yml`

This is the source of truth for the package selection.

If this file is wrong, the rest of the migration does not matter.

### `main/CMakeLists.txt`

This is where the build graph names the dependency that the main app needs.

If this file still points at `bytecodealliance__wasm-micro-runtime`, the build will not bind to the new package correctly.

### `dependencies.lock`

This file is the audit trail of what actually resolved, not what we hoped resolved.

Treat it as the build truth.

### `sdkconfig.defaults`

This file matters because it locks in the intended WAMR feature profile:

- interpreter-first
- AOT disabled
- pthread disabled
- narrow feature set

If those symbols disappear or change shape under the Espressif package, this file will reveal it quickly.

### `main/wasm_runtime_service.cpp`

This file is the first runtime-layer code likely to compile-break if config macros or headers change.

It is also the first place that will tell us at runtime whether the build still has interpreter support enabled.

## Validation Checklist

- `main/idf_component.yml` references `espressif/wasm-micro-runtime`
- `main/CMakeLists.txt` references the correct Espressif alias
- `dependencies.lock` references the Espressif package rather than the upstream git repo
- `idf.py build` succeeds on `ESP-IDF 5.3.4`
- no unrelated runtime or host-ABI changes are mixed into the migration commit

## What Success Looks Like

Success for this ticket is narrower than “the runtime bug is fixed.”

Success means:

- `0079` builds cleanly against Espressif's WAMR package
- the resolved dependency state is documented
- the project is ready for a later runtime comparison ticket

That is enough for this slice.
