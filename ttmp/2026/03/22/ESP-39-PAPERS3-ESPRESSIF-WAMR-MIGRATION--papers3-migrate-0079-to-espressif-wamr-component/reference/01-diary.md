---
Title: Diary
Ticket: ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION
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
RelatedFiles:
    - Path: 0079-papers3-wamr-assemblyscript-console/main/idf_component.yml
      Note: Dependency manifest being migrated from upstream WAMR to Espressif's package
    - Path: 0079-papers3-wamr-assemblyscript-console/main/CMakeLists.txt
      Note: Main component alias wiring that must be updated during the migration
    - Path: 0079-papers3-wamr-assemblyscript-console/dependencies.lock
      Note: Resolved dependency record used to confirm the migration result
ExternalSources: []
Summary: Step-by-step diary for the WAMR dependency migration in `0079`.
LastUpdated: 2026-03-22T19:52:00-04:00
WhatFor: Record the migration sequence, build results, and any resolver or alias issues encountered while switching to Espressif's WAMR package.
WhenToUse: Read this before continuing the migration or reviewing how the dependency swap was validated.
---

# Diary

## Step 1: Inspect the current dependency layout

Before editing anything, I checked how `0079` currently pulls in WAMR.

What I found:

- the project does not use a top-level `idf_component.yml`
- the dependency is declared in [main/idf_component.yml](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/idf_component.yml)
- the current package is `bytecodealliance/wasm-micro-runtime`
- it is pulled from git and pinned to `version: main`
- the app component depends on `bytecodealliance__wasm-micro-runtime` in [main/CMakeLists.txt](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/CMakeLists.txt)
- the resolved lockfile confirms the upstream git source in [dependencies.lock](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/dependencies.lock)

Why this mattered:

- the migration surface is small
- the main risk is not code churn but package identity and alias correctness

## Step 2: Create the migration ticket and implementation guide

I created `ESP-39` to keep this work distinct from the replay-isolation ticket.

That separation matters because the success condition here is:

- migrate the dependency
- get a build

It is not:

- prove the runtime bug is fixed

That distinction should keep the implementation honest.

## Step 3: Perform the dependency swap

I then made the smallest possible code/build change in `0079`.

Files changed:

- [main/idf_component.yml](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/idf_component.yml)
- [main/CMakeLists.txt](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/CMakeLists.txt)

What changed:

- replaced `bytecodealliance/wasm-micro-runtime` with `espressif/wasm-micro-runtime`
- pinned the dependency to `2.4.0~1`
- replaced the CMake alias `bytecodealliance__wasm-micro-runtime` with `espressif__wasm-micro-runtime`

Why the change was intentionally narrow:

- if the build broke immediately, the cause would most likely be aliasing, manifest syntax, or Kconfig surface mismatch
- if the build succeeded, we would know the package swap itself was mechanically valid before touching runtime logic

## Step 4: Reconfigure and build

I validated the migration with:

- `unset IDF_PYTHON_ENV_PATH IDF_PATH && source /home/manuel/esp/esp-idf-5.3.4/export.sh >/dev/null && idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console reconfigure build`

Important observations from the build output:

- Component Manager detected the manifest change and re-solved dependencies
- it updated [dependencies.lock](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/dependencies.lock)
- it resolved `espressif/wasm-micro-runtime (2.4.0~1)`
- the active component alias in the build graph was `espressif__wasm-micro-runtime`
- the WAMR config symbols consumed by `sdkconfig.defaults` and `wasm_runtime_service.cpp` still compiled cleanly
- the full firmware build completed successfully

Build result:

- success

## Step 5: Check the resolved artifact state

After the successful build, I checked the lockfile and managed component directories.

What changed cleanly:

- [dependencies.lock](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/dependencies.lock) now names `espressif/wasm-micro-runtime`
- the resolved version is `2.4.0~1`
- the source is the Espressif Component Registry service, not the upstream git repository

What remained slightly messy:

- both of these local generated directories exist:
  - `managed_components/bytecodealliance__wasm-micro-runtime`
  - `managed_components/espressif__wasm-micro-runtime`

Interpretation:

- the old upstream directory is a stale local cache artifact
- the actual successful build used the Espressif component alias and lockfile entry
- this is cleanup debt, not evidence that the migration failed
