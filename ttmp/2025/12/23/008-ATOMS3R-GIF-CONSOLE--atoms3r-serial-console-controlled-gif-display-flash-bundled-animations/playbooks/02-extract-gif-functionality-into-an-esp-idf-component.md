---
Title: "Playbook: extract GIF functionality into an ESP-IDF component (echo_gif) — fool-proof steps"
Ticket: 008-ATOMS3R-GIF-CONSOLE
Status: active
Topics:
    - esp-idf
    - esp32s3
    - gif
    - assets
    - fatfs
    - documentation
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/components/echo_gif/
      Note: New reusable component (storage + registry + AnimatedGIF-backed player)
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/main/CMakeLists.txt
      Note: Example of depending on echo_gif from an app
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/main/hello_world_main.cpp
      Note: Example of using echo_gif APIs in a thin orchestrator
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/sdkconfig.defaults
      Note: Sets echo_gif defaults (max counts, mount paths, etc.)
ExternalSources: []
Summary: "Step-by-step procedure to move GIF storage/registry/playback from a project’s main/ into a reusable ESP-IDF component, including Kconfig, CMake dependency wiring, and common pitfalls."
LastUpdated: 2025-12-24T00:00:00.000000000-05:00
WhatFor: "Repeatable componentization workflow you can apply again without rediscovering Kconfig/CMake edge cases."
WhenToUse: "When refactoring a large firmware (like tutorial 0013) and you want GIF logic reusable across multiple projects."
---

## Executive summary

This playbook turns “GIF functionality” from “some files in `main/`” into a first-class ESP-IDF **component** that:

- has its own `CMakeLists.txt` and dependencies
- has its own `Kconfig` options (so behavior is configured in `menuconfig`)
- exposes a small public API under `include/`
- can be reused by any firmware in this repo by adding a single dependency

In this repo, the resulting component is `esp32-s3-m5/components/echo_gif/` and the reference app integration is tutorial `0013-atoms3r-gif-console`.

## Preconditions

- You are using ESP-IDF 5.x style CMake (`idf_component_register`).
- The code you want to extract is already modular (or you’re willing to split it first).

In `0013`, this extraction assumes you already had the split:

- storage (`gif_storage.*`)
- registry (`gif_registry.*`)
- player (`gif_player.*`)

## Target end state (directory layout)

The component should look like this:

```text
esp32-s3-m5/components/echo_gif/
  CMakeLists.txt
  Kconfig
  include/echo_gif/
    gif_storage.h
    gif_registry.h
    gif_player.h
  src/
    gif_storage.cpp
    gif_registry.cpp
    gif_player.cpp
```

The application that uses it should:

- remove project-local `gif_*.{h,cpp}` duplicates
- include the component headers as `#include "echo_gif/..."` (not relative paths)
- list `echo_gif` as a dependency in its `main/CMakeLists.txt`

## Step-by-step procedure

### 1) Create the component skeleton

- Create `esp32-s3-m5/components/echo_gif/`
- Add:
  - `CMakeLists.txt`
  - `Kconfig`
  - `include/echo_gif/`
  - `src/`

### 2) Decide what is public API vs private implementation

**Rule of thumb**:

- Public: headers in `include/echo_gif/` (what other projects include)
- Private: `.cpp` files in `src/` and any private headers

Keep the public API as small as possible. For example:

- `echo_gif_storage_mount()`
- `echo_gif_registry_refresh()` and friends
- `echo_gif_player_open_*()` / `echo_gif_player_play_frame()` / `echo_gif_player_reset()`

### 3) Move code into the component (copy first, delete later)

In practice, the safest order is:

1. Copy code into the component
2. Make it compile
3. Update the app to call the component
4. Delete the old project-local copies

### 4) Add a component `Kconfig` for all GIF-related tuning knobs

If you keep “GIF tuning” in project `main/Kconfig.projbuild`, you risk duplicate symbol definitions later.

Move the knobs into the component so they appear under:

- `Component config → echo_gif: GIF storage + registry + playback`

Typical knobs:

- `CONFIG_ECHO_GIF_MAX_COUNT`
- `CONFIG_ECHO_GIF_MAX_PATH_LEN`
- `CONFIG_ECHO_GIF_MIN_FRAME_DELAY_MS`
- `CONFIG_ECHO_GIF_SWAP_BYTES`
- `CONFIG_ECHO_GIF_SCALE_TO_FULL_SCREEN`
- mount-related strings:
  - `CONFIG_ECHO_GIF_STORAGE_MOUNT_PATH`
  - `CONFIG_ECHO_GIF_STORAGE_PARTITION_LABEL`
  - `CONFIG_ECHO_GIF_STORAGE_GIF_DIR`
  - `CONFIG_ECHO_GIF_STORAGE_GIF_DIR_FALLBACK`

### 5) Wire dependencies correctly in `CMakeLists.txt`

In `echo_gif/CMakeLists.txt`:

- list `SRCS` (`src/*.cpp`)
- set `INCLUDE_DIRS "include"`
- add `PRIV_REQUIRES` for what the implementation needs

**Common pitfall**: dependency names must match component names, not header names.

- `esp_vfs_fat.h` lives in the `fatfs` component; there is no component named `esp_vfs_fat`.

### 6) Update the app to depend on the component

In the app’s `main/CMakeLists.txt`:

- remove `gif_storage.cpp`, `gif_registry.cpp`, `gif_player.cpp` from `SRCS`
- add `echo_gif` to dependencies (typically `REQUIRES` for apps)

In the app source files:

- change includes to:
  - `#include "echo_gif/gif_storage.h"`
  - `#include "echo_gif/gif_registry.h"`
  - `#include "echo_gif/gif_player.h"`
- update symbol names if you renamed APIs with a prefix (recommended for components)

### 7) Remove duplicate Kconfig symbols from the app

If the app previously defined `CONFIG_TUTORIAL_0013_GIF_*`, delete that menu block.

Otherwise you’ll get Kconfig symbol redefinition errors or (worse) “same name, different meaning” bugs.

### 8) Update `sdkconfig.defaults` (and `sdkconfig` if tracked)

If your repo commits `sdkconfig.defaults`:

- replace old symbols with the new `CONFIG_ECHO_GIF_*` symbols

If your repo also tracks `sdkconfig` (as these tutorials sometimes do):

- run `idf.py reconfigure` to regenerate cleanly, or
- update the relevant block to avoid confusion

### 9) Build

From the app directory (example: `0013-atoms3r-gif-console`):

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh
idf.py reconfigure
idf.py build
```

If the build fails, jump to “Pitfalls” below.

## Pitfalls (learned the hard way)

### Pitfall A: `M5Canvas` is a type alias in M5GFX

M5GFX defines:

- `using M5Canvas = m5gfx::M5Canvas;`

So you cannot forward declare `class M5Canvas;` in a public header.

**Fix**: forward declare the underlying type instead:

```c++
namespace m5gfx { class M5Canvas; }
```

…and store pointers as `m5gfx::M5Canvas*`.

### Pitfall B: include-order macro collisions (`AnimatedGIF.h` vs LovyanGFX pgmspace)

Some combinations of:

- `AnimatedGIF.h`
- LovyanGFX/M5GFX headers

can produce confusing errors related to `memcpy_P` / `pgmspace.h`.

**Fix**: keep the include order stable inside the `.cpp` that needs both:

```c++
#include "M5GFX.h"
#include "AnimatedGIF.h"
```

Encapsulating this inside the component’s `.cpp` reduces the chance of breaking other code.

### Pitfall C: “Unknown component name” in CMake

If you see:

- `Failed to resolve component 'X' ... unknown name`

you probably used a header-derived guess (`esp_vfs_fat`) instead of the actual component name (`fatfs`).

## Verification checklist (what “done” means)

- [ ] The component builds as `__idf_echo_gif` during `idf.py build`.
- [ ] The app builds without project-local `gif_*.cpp` duplicates.
- [ ] `menuconfig` shows an “echo_gif” section under Component config.
- [ ] The app still lists and plays GIFs as before (no behavior regression).


