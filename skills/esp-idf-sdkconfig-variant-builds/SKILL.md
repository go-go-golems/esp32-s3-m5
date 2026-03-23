---
name: esp-idf-sdkconfig-variant-builds
description: Build ESP-IDF firmware variants with trustworthy sdkconfig overrides and prove that the intended Kconfig flags actually took effect. Use when creating headless/system/internal-pool style variants, overriding sdkconfig values per build, debugging suspicious config leakage from a shared project sdkconfig, or validating a build with generated sdkconfig files plus runtime status output.
---

# ESP-IDF sdkconfig variant builds

Use this skill when an ESP-IDF project needs multiple build variants and you must be sure a config flag really changed.

## Core rule

If the project has a shared `sdkconfig` file, do not rely on `-DSDKCONFIG_DEFAULTS=...` alone for variant experiments.

Use all three:

1. A dedicated build directory per variant.
2. A dedicated build-local sdkconfig file via `-DSDKCONFIG="$BUILD_DIR/sdkconfig.variant"`.
3. A base + variant defaults chain via `-DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.<variant>"`.

This is the repo-proven pattern that fixed the allocator-variant leakage in `0082-papers3-wamr-allocator-control`.

## Bad pattern

This can silently inherit choices from the project's shared `sdkconfig`:

```bash
idf.py -B build-variant \
  -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.internal_pool" \
  build
```

If a prior build or checked-in `sdkconfig` selected another choice item, this may not produce the variant you think it does.

## Good pattern

```bash
PROJECT_DIR=/abs/project
BUILD_DIR="$PROJECT_DIR/build-internal-pool"

idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" \
  -DSDKCONFIG="$BUILD_DIR/sdkconfig.variant" \
  -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.internal_pool" \
  reconfigure build
```

The dedicated `sdkconfig.variant` isolates the build from the shared project `sdkconfig`.

## Workflow

### 1. Define the Kconfig surface

- Put the variant choice in `main/Kconfig.projbuild` or the appropriate component Kconfig.
- Keep the choices explicit and mutually exclusive when possible.
- Add small defaults files such as:
  - `sdkconfig.defaults`
  - `sdkconfig.headless`
  - `sdkconfig.system_allocator`
  - `sdkconfig.internal_pool`

Example choice pattern:

```kconfig
choice PAPERS3_WAMR_ALLOCATOR_BACKING
    prompt "WAMR allocator backing"
    default PAPERS3_WAMR_ALLOCATOR_POOL_SPIRAM

config PAPERS3_WAMR_ALLOCATOR_POOL_SPIRAM
    bool "Pool allocator backed by external SPIRAM"

config PAPERS3_WAMR_ALLOCATOR_POOL_INTERNAL
    bool "Pool allocator backed by internal RAM"

config PAPERS3_WAMR_ALLOCATOR_SYSTEM
    bool "System allocator"
endchoice
```

### 2. Build each variant in its own build directory

- `build-headless`
- `build-system-allocator`
- `build-internal-pool`

Do not reuse one build directory for multiple variants unless you intentionally want that churn.

### 3. Force a build-local sdkconfig file

Always pass:

```bash
-DSDKCONFIG="$BUILD_DIR/sdkconfig.variant"
```

That is the critical guardrail against config leakage.

### 4. Chain defaults explicitly

Use:

```bash
-DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.<variant>"
```

Notes:

- Keep `sdkconfig.defaults` first.
- Keep the variant file second so it overrides the base.
- Use semicolon-separated paths because CMake expects a list.

### 5. Reconfigure before build

Prefer:

```bash
idf.py ... reconfigure build
```

Use `reconfigure` when:

- you changed any `sdkconfig.*` file
- you changed Kconfig definitions
- you are switching variants
- you do not fully trust the current build cache

### 6. Verify generated config, not just source files

After configure, inspect the generated files inside the build directory:

- `build-variant/config/sdkconfig.json`
- `build-variant/config/sdkconfig.cmake`
- `build-variant/config/sdkconfig.h`

Use the bundled verifier:

```bash
skills/esp-idf-sdkconfig-variant-builds/scripts/check_variant_flags.sh \
  /abs/project/build-internal-pool \
  PAPERS3_WAMR_ALLOCATOR_POOL_INTERNAL \
  PAPERS3_WAMR_ALLOCATOR_SYSTEM
```

What to look for:

- `sdkconfig.json` should show the boolean you expect.
- `sdkconfig.cmake` should show the selected config as `"y"` and the opposite choice empty.
- `sdkconfig.h` should contain the active `#define`.

### 7. Add runtime proof

Generated config proof is necessary but not always sufficient. Add runtime logging or status output that surfaces the effective mode.

Examples:

- `allocator=system-allocator`
- `allocator_backing=system`
- `host_api.display=disabled`
- `wamr.pool_buffer=0x0`
- `wamr.pool_size=0`

If runtime status disagrees with the build-time files, trust the discrepancy and investigate before drawing conclusions.

## Fast diagnosis checklist

When a variant result looks suspicious:

1. Confirm the command used a unique `-B` build directory.
2. Confirm it also used `-DSDKCONFIG="$BUILD_DIR/sdkconfig.variant"`.
3. Inspect `build-variant/config/sdkconfig.json`.
4. Inspect `build-variant/config/sdkconfig.cmake`.
5. Inspect `build-variant/config/sdkconfig.h`.
6. Check runtime status strings/logs.
7. Only then interpret the experiment.

## Repo anchors

Open these first in this repo:

- `0082-papers3-wamr-allocator-control/main/Kconfig.projbuild`
- `0082-papers3-wamr-allocator-control/sdkconfig.defaults`
- `0082-papers3-wamr-allocator-control/sdkconfig.system_allocator`
- `0082-papers3-wamr-allocator-control/sdkconfig.internal_pool`
- `ttmp/2026/03/23/ESP-42-PAPERS3-WAMR-ALLOCATOR-CONTROL--papers3-minimal-wamr-allocator-control-firmware-to-isolate-instantiate-vs-psram-contamination/scripts/flash_and_probe_allocator_system.sh`
- `ttmp/2026/03/23/ESP-42-PAPERS3-WAMR-ALLOCATOR-CONTROL--papers3-minimal-wamr-allocator-control-firmware-to-isolate-instantiate-vs-psram-contamination/scripts/flash_and_probe_allocator_internal.sh`

These scripts encode the corrected variant-build pattern.

## Pseudocode

```text
for each variant:
  choose BUILD_DIR unique to variant
  choose VARIANT_DEFAULTS file

  run idf.py with:
    -B BUILD_DIR
    -DSDKCONFIG=BUILD_DIR/sdkconfig.variant
    -DSDKCONFIG_DEFAULTS=sdkconfig.defaults;VARIANT_DEFAULTS
    reconfigure build

  verify generated config files in BUILD_DIR/config/
  run firmware
  verify runtime status matches intended variant
```
