---
Title: ESP-IDF Build and Dev Environment Playbook
Ticket: ESP32-P4-QUICKJS-WASM
Status: active
Topics:
    - esp-idf
    - tooling
    - esp32p4
    - esp32-s3
    - firmware
    - console
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: AGENTS.md
      Note: Concise agent-facing rules that point back to this playbook
    - Path: 0100-esp32-p4-quickjs-wasm/sdkconfig.defaults
      Note: P4 + WAMR + custom partition table reference
    - Path: 0099-esp32-p4-picocalc-display-keyboard/sdkconfig.defaults
      Note: ESP32-P4 target/console/PSRAM baseline
    - Path: 0079-papers3-wamr-assemblyscript-console/main/idf_component.yml
      Note: Component-manager manifest (wasm-micro-runtime) reference
ExternalSources: []
Summary: A practical playbook for building, reconfiguring, and flashing ESP-IDF firmware in this workspace, with the idf.py failure modes a coding agent will actually hit and their fixes.
LastUpdated: 2026-06-23
WhatFor: "Setting up the ESP-IDF environment, building/rebuilding firmware, and avoiding the idf.py traps that cost rebuild cycles."
WhenToUse: "Before running idf.py set-target/build/flash, or when a sdkconfig.defaults change does not take effect."
---

# ESP-IDF Build and Dev Environment Playbook

This playbook records how to build and reconfigure ESP-IDF firmware in the `esp32-s3-m5` workspace so that a coding agent does not lose time rediscovering the build system's behavior. The workspace holds firmware for several chips (ESP32-S3, ESP32-P4, ESP32-C6, ESP32-C3) built against several ESP-IDF versions, and the differences matter. Each section states what to do, why it works that way, and the failure mode it prevents.

## The ESP-IDF environment on this machine

More than one ESP-IDF is installed under `~/esp/`, and the projects in this workspace are pinned to different versions. A build only succeeds when the matching version is sourced.

```
~/esp/esp-idf-4.4.6
~/esp/esp-idf-5.1.4
~/esp/esp-idf-5.3.4
~/esp/esp-idf-5.4.1
~/esp/esp-idf-5.4.2
~/esp/esp-idf-5.5.4
```

The version a project expects is stated in its `README.md` and encoded in its `main/idf_component.yml` (`idf: version: ">=x.y.z,<x.y.z"`). Two reference points:

- ESP32-S3 WAMR projects (`0079`, `0082`) build against ESP-IDF **5.3.4**.
- ESP32-P4 PicoCalc projects (`0097`, `0098`, `0099`, `0100`) build against ESP-IDF **5.4.2** (the P4 needs 5.4+).

Always read the project's README before building. Sourcing the wrong version produces link or Kconfig errors that look like code bugs but are environment mismatches.

## Sourcing the toolchain

Source the IDF export script for the version the project uses. The export script configures `idf.py`, the toolchain, and the Python environment for the current shell.

```bash
source ~/esp/esp-idf-5.4.2/export.sh        # for P4 projects (0097–0100)
source ~/esp/esp-idf-5.3.4/export.sh        # for S3 WAMR projects (0079, 0082)
```

Confirm the toolchain is on `PATH` before running `idf.py`:

```bash
idf.py --version
```

If `idf.py` is not found, the export script was not sourced in the shell that runs the build. This is the first thing to check.

## Choosing and setting the target

Set the chip target before the first build. The target is stored in `sdkconfig` as `CONFIG_IDF_TARGET` and is also implied by `sdkconfig.defaults`.

```bash
cd <firmware-dir>
idf.py set-target esp32p4        # or esp32s3, esp32c6, esp32c3, esp32h2
```

`set-target` runs a CMake configure, which also runs the IDF Component Manager to fetch anything declared in `main/idf_component.yml`. Re-run `set-target` only when changing targets; for normal rebuilds use `idf.py build`.

## Component dependencies go in `main/idf_component.yml`

The IDF Component Manager reads the main component's manifest from `main/idf_component.yml`, not from a project-root `idf_component.yml`. A manifest placed at the project root is silently ignored, and the build then fails with:

```
Failed to resolve component 'espressif__wasm-micro-runtime' required by
component 'main': unknown name.
```

When a managed component is needed, put the manifest in `main/`:

```yaml
# main/idf_component.yml
dependencies:
  idf:
    version: ">=5.4.2,<5.5.0"
  espressif/wasm-micro-runtime:
    version: "2.4.0~1"
```

After adding or editing the manifest, run `idf.py reconfigure` (or `set-target` / `build`) so the Component Manager fetches the component into `managed_components/`. `managed_components/` is build output: it is gitignored and regenerated, so do not commit it.

## How `sdkconfig.defaults` is applied

`sdkconfig.defaults` seeds options that are absent from the generated `sdkconfig`. It does not override options already present in `sdkconfig`. This is the single most important build behavior to understand, and it is the one that costs the most rebuild cycles.

The consequence: editing `sdkconfig.defaults` and rebuilding does **not** apply the change if `sdkconfig` already exists with the old value. `idf.py fullclean` does not help here, because `fullclean` removes `build/` and `managed_components/` but leaves `sdkconfig` in place.

To force `sdkconfig.defaults` changes to take effect, delete `sdkconfig` so it regenerates from the defaults:

```bash
rm -f sdkconfig
idf.py build
```

This is required whenever changing the partition table choice, the console backend, PSRAM mode, or any Kconfig selection that was already written to `sdkconfig`. For edits to a single option that is still at its default, `idf.py menuconfig` (or `idf.py reconfigure`) can also set it directly, but deleting `sdkconfig` is the reliable reset.

## Custom partition tables

The default single-app partition table gives the `factory` app one megabyte. Any firmware that embeds a large asset — a Wasm module, a fonts/images blob, a full framework — overflows it:

```
Error: app partition is too small for binary ... size 0x1be410:
  - Part 'factory' 0/0 @ 0x10000 size 0x100000 (overflow 0xbe410)
```

Add a `partitions.csv` at the project root and enable it in `sdkconfig.defaults`. All three lines are required; matching the pattern used by `0098`:

```kconfig
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
CONFIG_PARTITION_TABLE_FILENAME="partitions.csv"
```

```
# partitions.csv
nvs,      data, nvs,     0x9000,   0x6000,
phy_init, data, phy,     0xf000,   0x1000,
factory,  app,  factory, 0x10000,  0x400000,
```

Because this is a `sdkconfig.defaults` change, apply it by deleting `sdkconfig` (see the previous section). The reference is `0100-esp32-p4-quickjs-wasm/{sdkconfig.defaults,partitions.csv}`.

## Console selection is target-specific

The console backend differs by chip and by board wiring, and choosing the wrong one produces a firmware that boots with no visible output.

- **ESP32-S3 (Cardputer/AtomS3R/PaperS3):** prefer USB Serial/JTAG. Put `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` and `# CONFIG_ESP_CONSOLE_UART is not set` in `sdkconfig.defaults`. UART pins on these boards are often repurposed for peripherals, and UART console output can corrupt protocol traffic. See `AGENTS.md` for the full rationale.
- **ESP32-P4 (PicoCalc / Waveshare ESP32-P4-WIFI6):** there is no USB Serial/JTAG console. The board uses an external CH343 USB-UART bridge on UART0 (GPIO37 TX / GPIO38 RX). Put `CONFIG_ESP_CONSOLE_UART_DEFAULT=y` and `CONFIG_ESP_CONSOLE_SECONDARY_NONE=y` in `sdkconfig.defaults`. The S3 USB-Serial/JTAG rule does not apply to the P4.

Confirm the choice by checking `sdkconfig` after a build (`grep ESP_CONSOLE sdkconfig`).

## Build, flash, and monitor

```bash
cd <firmware-dir>
source ~/esp/esp-idf-5.4.2/export.sh
idf.py set-target esp32p4          # only on first build or target change
idf.py build
idf.py -p "$PORT" flash monitor    # flash + open the monitor
```

The monitor and flashing share the serial device, so they must use the same port and must not run concurrently with another monitor/probe on that port. See `AGENTS.md` for the serial-ownership rules.

## Serial port patterns

The port depends on the board's bridge chip. Prefer the stable `/dev/serial/by-id/...` path over `/dev/ttyUSB*`, because the `ttyUSB` number can change between plugs.

- **ESP32-P4 PicoCalc (CH343 bridge):** `/dev/serial/by-id/usb-1a86_USB_Single_Serial_*-if00`
- **ESP32-S3 USB Serial/JTAG:** `/dev/serial/by-id/usb-Espressif_USB_JTAG_debugger_*` (or the `ttyACM*` it resolves to)

Discover the exact path before flashing:

```bash
ls /dev/serial/by-id/
```

If a board needs a manual reset or boot button to enter download mode, ask the user before retrying serial opens. Do not loop on a port that is not responding.

## What is and is not committed

The workspace `.gitignore` excludes build outputs and generated files. Commit source and configuration; do not commit generated artifacts.

Committed: `main/` sources, `main/idf_component.yml`, `sdkconfig.defaults`, `partitions.csv`, `CMakeLists.txt`, embedded binary assets that are deliberate firmware inputs (for example `main/quickjs.wasm` in `0100`).

Not committed (gitignored): `build/`, `managed_components/`, `sdkconfig`, `dependencies.lock` is committed (it pins component versions for reproducibility), `**/wasm-build/`, `**/wasm-src/quickjs/` (vendored clone). Confirm with `git check-ignore -v <path>` before staging.

## Troubleshooting

| Problem | Cause | Solution |
|---|---|---|
| `Failed to resolve component 'X' required by component 'main': unknown name` | Component manifest is at the project root instead of `main/`. | Move the manifest to `main/idf_component.yml`; run `idf.py reconfigure`. |
| A `sdkconfig.defaults` edit (partition table, console, PSRAM) does not take effect | Defaults only seed options absent from `sdkconfig`; `sdkconfig` already holds the old value. | `rm -f sdkconfig && idf.py build`. `idf.py fullclean` does **not** remove `sdkconfig`. |
| `app partition is too small ... overflow` | App binary exceeds the default 1 MB `factory` partition. | Add a custom `partitions.csv` with a larger `factory` and enable it (`CONFIG_PARTITION_TABLE_CUSTOM=y` + both `_FILENAME` lines); `rm sdkconfig`. |
| `idf.py: command not found` | The IDF export script was not sourced in this shell. | `source ~/esp/esp-idf-<ver>/export.sh`. |
| Kconfig/link errors that look like code bugs | Wrong IDF version sourced for the project. | Read the project README; source the matching `~/esp/esp-idf-<ver>`. |
| `'printf' was not declared` in a `.cpp` | ESP-IDF C++ does not transitively include `<cstdio>`. | `#include <cstdio>` in the file. |
| `esp_console_cmd_t` missing-field-initializers warnings | Designated initializers leave newer struct fields unset. | Zero-initialise: `esp_console_cmd_t cmd = {};` then assign fields. |
| Firmware boots with no console output | Wrong console backend for the board. | S3: USB Serial/JTAG; P4: UART0. Check `grep ESP_CONSOLE sdkconfig`. |
| WAMR load fails: `reference types feature which is disabled` | clang-built Wasm emits reference types; WAMR has them off. | `CONFIG_WAMR_ENABLE_REF_TYPES=y` (device) / `WAMR_BUILD_REF_TYPES=1` (host). |
| `app heap is corrupted ... export malloc and free` | Wasi-sdk Wasm module does not export its allocator. | Link with `-Wl,--export=malloc -Wl,--export=free`. |

## See Also

- `AGENTS.md` — agent-facing rules (console default, serial ownership, manual reset) that complement this playbook.
- Per-project `README.md` files — each states its IDF version, target, console, and flash port.
- `0099-esp32-p4-picocalc-display-keyboard` — ESP32-P4 target/console/PSRAM reference build.
- `0079-papers3-wamr-assemblyscript-console` — WAMR embedding + `main/idf_component.yml` reference.
- `0100-esp32-p4-quickjs-wasm` — current QuickJS-WAMR firmware with a custom 4 MB partition table and embedded Wasm asset.
