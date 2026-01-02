---
Title: 'Go helper tool: design (env/ports/flash/monitor/tmux)'
Ticket: 0027-ESP-HELPER-TOOLING
Status: active
Topics:
    - esp-idf
    - esp32
    - esp32s3
    - tooling
    - debugging
    - flashing
    - serial
    - tmux
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Design proposal for a single Go CLI that consolidates ESP-IDF env resolution, port selection, robust flashing, monitor/tmux workflows, and repeatable debug procedures."
LastUpdated: 2026-01-02T09:28:37.913878727-05:00
WhatFor: "Replace duplicated repo scripts with a consistent, testable, diagnostics-friendly helper tool for ESP32 build/flash/monitor/QEMU workflows."
WhenToUse: "Use when you need reliable build/flash/monitor sessions (especially under tmux) and want deterministic port selection and better error messages."
---

# Go helper tool: design (env/ports/flash/monitor/tmux)

## Executive Summary

This proposes a single Go CLI (working name: `esphelper`) that consolidates the repeated ESP-IDF workflows currently implemented as per-project `build.sh` wrappers and ad-hoc scripts:
- Resolve and validate ESP-IDF environment (export script, `IDF_PATH`, python venv, required python modules like `esp_idf_monitor`).
- Resolve serial ports deterministically (prefer `/dev/serial/by-id/*`; avoid silently picking the wrong device).
- Provide robust “flash + monitor” workflows (including an optional “stable USB-Serial/JTAG flash” path via `esptool` + `build/flash_args`).
- Provide optional tmux session management (detach/reattach convenience) without making tmux a hard dependency.
- Support repeatable “procedures” (smoke tests, transcript capture) that isolate layers (UART vs `idf_monitor` vs tmux) and produce artifacts.

The requirements and patterns are derived from:
- `ttmp/2026/01/02/0027-ESP-HELPER-TOOLING--esp-helper-tooling-go-debugging-flashing-helper/analysis/01-inventory-esp-idf-helper-scripts-tmux-serial-idf-py.md`

## Problem Statement

This repo has multiple helper scripts that each solve a subset of the same problems (build/flash/monitor with ESP-IDF), but:
- The logic is duplicated and slightly inconsistent (port selection, IDF env checks, tmux strategy).
- tmux + direnv + python selection can cause non-obvious failures (e.g. `No module named 'esp_idf_monitor'` inside a tmux pane).
- USB Serial/JTAG flashing can be flaky depending on host/environment; some projects work around it with `esptool` flows.
- QEMU and serial debugging often require “layer isolation” probes and transcript capture; today this exists as one-off scripts.

The result is high friction and low reliability:
- debugging sessions start by re-solving environment/port issues instead of investigating firmware behavior,
- multi-device setups are risky (silent wrong-port selection),
- and log/artifact capture is inconsistent across projects.

## Proposed Solution

### Product goals

- One command per “operator intent”:
  - build
  - flash
  - monitor
  - flash+monitor
  - qemu+monitor
  - capture an artifact from serial
  - run a smoke/procedure (with logs)
- Better diagnostics than equivalent shell snippets:
  - show what env is being used (IDF checkout, python)
  - warn early if required modules/binaries are missing
  - never hide ambiguous port selection
- Usable in:
  - plain terminal sessions (no tmux),
  - tmux sessions,
  - scripted runs where you want transcripts/logs.

### Non-goals

- Replace ESP-IDF itself (`idf.py`, `esptool.py`) or fork its behavior.
- Full-featured serial monitor reimplementation on day 1 (initially, wrap `idf.py monitor`).
- GUI / IDE integration in the first iteration.

### CLI shape (proposal)

Assume repo-root relative execution and an explicit project path.

- `esphelper env doctor [--project DIR]`
  - Prints: `ESP_IDF_VERSION`, `IDF_PATH`, `IDF_PYTHON_ENV_PATH`, chosen `python`, `idf.py` path.
  - Verifies importability of modules used by common flows (at least `esp_idf_monitor`, `esptool`).
  - Verifies QEMU tooling when requested (`qemu-system-xtensa` present; capture version/help output best-effort).

- `esphelper ports list [--kind usb-jtag|uart] [--json]`
  - Enumerates ports, preferring stable symlinks under `/dev/serial/by-id`.

- `esphelper build --project DIR [--target esp32s3] [--] <idf.py args>`
  - Ensures target is set (guard against “default target: esp32” drift).
  - Runs `idf.py build` (or any idf action) through a resolved env.

- `esphelper flash --project DIR [--port auto|PATH] [--baud N] [--stable-usb-jtag]`
  - Default: `idf.py flash` (via resolved python + `IDF_PATH/tools/idf.py`).
  - Stable USB-JTAG: parse `build/flash_args` and run `python -m esptool ... write_flash` with `--before usb_reset --after no_reset`, then reboot via watchdog reset.

- `esphelper monitor --project DIR [--port auto|PATH] [--baud N] [--log PATH]`
  - Default: `idf.py monitor` (same env resolution rules).
  - Log capture: either via `idf.py monitor` options (preferred), or via process-level tee’ing.

- `esphelper flash-monitor --project DIR [--port auto|PATH] [--baud N]`
  - Runs `idf.py flash monitor` as one process to avoid serial locking.

- `esphelper tmux flash-monitor --project DIR ...`
  - Creates a named tmux session with:
    - left pane running `flash monitor`
    - right pane an interactive shell in the project dir with the env activated
  - Optional: use an isolated tmux socket for scripted runs.

- `esphelper procedure <name> ...`
  - “Procedure” is a structured run with timeouts, artifacts, and a pass/fail result:
    - QEMU UART RX smoke test (stdio, tcp raw)
    - device UART RX smoke test (raw serial)
    - “REPL input works through tmux+monitor” smoke test
  - Writes artifacts to a consistent location (default under `build/esphelper/`).

### Configuration

Configuration is required because hard-coded paths appear throughout existing scripts (e.g. `~/esp/esp-idf-5.4.1/export.sh`, project absolute paths).

Proposal:
- A repo-local config file: `.esphelper.yaml` (checked in) + `.esphelper.local.yaml` (gitignored).
- Resolved precedence:
  1. CLI flags
  2. env vars (`IDF_EXPORT_SH`, `ESPHELPER_PORT`, etc.)
  3. `.esphelper.local.yaml`
  4. `.esphelper.yaml`
  5. defaults (with warnings)

Config fields (minimal):
- `idf.export_sh`: path to `export.sh`
- `idf.required_version_prefix`: e.g. `5.4`
- `idf.default_target`: `esp32s3`
- `ports.policy`: `error-on-multiple` vs `pick-first` (default: error)
- `tmux.session_prefix`: e.g. `esp`

### Env resolution (key behavior)

To avoid tmux/direnv problems observed in `0019-cardputer-ble-temp-logger/tools/run_fw_flash_monitor.sh` and the newer `0022/0023/0025` `build.sh` scripts, the tool should:
- Prefer executing `IDF_PATH/tools/idf.py` via python at `IDF_PYTHON_ENV_PATH/bin/python` when available.
- Avoid relying on `idf.py` on PATH.
- Provide `env doctor` output that prints the resolution and gives actionable fixes.

### Port resolution

Port resolver should:
- Prefer `/dev/serial/by-id` symlinks.
- When multiple candidates exist:
  - default behavior: error with a list of candidates and how to select,
  - optional interactive pick (TTY only) or filter rules.
- Avoid “head -1” silent selection outside an explicit, configured policy.

### tmux integration

tmux mode should:
- Be optional; if `tmux` isn’t present, run in the current terminal.
- Avoid embedding `$*` into a shell string; build explicit command arrays and quote safely when passing to tmux.
- Support two usage styles:
  - “dev session”: stable session name, attach anytime
  - “test session”: isolated socket + auto-cleanup (for procedures)

## Design Decisions

### Decision: wrap `idf.py monitor` initially (don’t reimplement monitor in Go)

Rationale:
- `idf.py monitor` already provides decoding, control sequences, and ESP-IDF integration.
- Reimplementing monitor correctness is a large surface area and risks subtle regressions.
- Existing scripts primarily solve problems *around* monitor (env/python, tmux, logs), not monitor internals.

Follow-up option:
- Add a minimal Go “serial capture” mode for transcript capture and protocol framing parsers (PNG framing, chord logs), while still defaulting to `idf.py monitor` for interactive monitoring.

### Decision: provide a stable flashing backend (esptool) as a first-class option

Rationale:
- `imports/esp32-mqjs-repl/mqjs-repl/tools/flash_device_usb_jtag.sh` exists because `idf.py flash` can be flaky on USB Serial/JTAG for some setups.
- Making this an explicit flag/command avoids copy/pasting a “magic workaround” across projects.

### Decision: default to “error on multiple ports”

Rationale:
- Some existing scripts silently pick the first match; this is risky in multi-device environments.
- A safer default forces explicit selection when ambiguity exists.

## Alternatives Considered

### Keep bash scripts; refactor into a shared `tools/lib.sh`

Pros:
- Low initial cost; minimal new dependencies.

Cons:
- Still relies on shell word-splitting/quoting; harder to make robust.
- Harder to provide structured diagnostics and machine-readable output.
- Harder to test consistently.

### Write the helper in Python

Pros:
- Fast iteration; pyserial integration is natural.

Cons:
- Reintroduces the python-environment fragility we are trying to avoid (multiple venvs, missing modules inside tmux panes).
- Distribution is harder than a single Go binary.

### Standardize on ESP-IDF `pytest_embedded` harnesses for everything

Pros:
- Structured framework; good for CI and repeatable tests.

Cons:
- Heavyweight for day-to-day interactive debugging.
- Doesn’t directly replace tmux-centric operator workflows.

## Implementation Plan

### Phase 1: core wrappers (environment + ports)

- Implement repo root discovery + config loading.
- Implement env resolver + `env doctor`.
- Implement port enumeration + deterministic `--port auto`.
- Implement `build`, `flash`, `monitor`, `flash-monitor` wrappers that execute `idf.py` via resolved python + `IDF_PATH/tools/idf.py`.

Exit criteria:
- Replaces “source export.sh + idf.py …” muscle memory for common workflows.
- Provides actionable errors for missing export script / missing idf.py / missing python modules.

### Phase 2: robustness + tmux

- Implement `flash --stable-usb-jtag` using `build/flash_args`.
- Implement tmux session management mirroring the `0022/0023/0025` layout (single pane `flash monitor` + shell pane).
- Add log capture options (`--log`) with consistent behavior.

Exit criteria:
- One-command tmux workflow works reliably without serial contention.
- Stable flash path works when `idf.py flash` is flaky.

### Phase 3: procedures + artifacts

- Implement “procedure runner” abstraction (timeouts, artifact directory, result codes).
- Port/replace the existing layer-isolation probes:
  - QEMU UART RX (stdio / TCP raw)
  - raw device UART probes (pyserial-equivalent behavior)
  - tmux send-keys probe for monitor stdin path
- Add built-in “serial capture” helpers for common framed protocols (e.g. screenshot PNG framing).

Exit criteria:
- Deterministic smoke tests exist and preserve transcripts as debugging artifacts.

## Open Questions

- Tool name: `esphelper` vs something repo-specific.
- Should the tool ever mutate `sdkconfig` / `sdkconfig.defaults` (e.g. “ensure target”, “disable watchdog for QEMU”), or should it only detect and warn?
- How should multiple ESP-IDF versions be supported (pin per project vs per repo default)?
- tmux semantics: “attach if exists, otherwise create” (persistent) vs always recreate.
- Artifact layout: prefer `build/esphelper/` (per-project) or `ttmp/.../sources` (per-ticket) when running procedures during investigations.

## References

- Inventory and analysis:
  - `ttmp/2026/01/02/0027-ESP-HELPER-TOOLING--esp-helper-tooling-go-debugging-flashing-helper/analysis/01-inventory-esp-idf-helper-scripts-tmux-serial-idf-py.md`
- Scripts that motivated requirements:
  - `0022-cardputer-m5gfx-demo-suite/build.sh`
  - `0019-cardputer-ble-temp-logger/tools/run_fw_flash_monitor.sh`
  - `imports/esp32-mqjs-repl/mqjs-repl/tools/flash_device_usb_jtag.sh`
  - `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_device_tmux.sh`
  - `ttmp/2025/12/29/0018-QEMU-UART-ECHO--track-c-minimal-uart-echo-firmware-to-isolate-qemu-uart-rx/reference/01-diary.md`
