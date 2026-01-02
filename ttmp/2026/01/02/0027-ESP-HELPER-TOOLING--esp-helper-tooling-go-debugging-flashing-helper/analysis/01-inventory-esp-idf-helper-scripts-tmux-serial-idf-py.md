---
Title: 'Inventory: ESP-IDF helper scripts (tmux/serial/idf.py)'
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
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Inventory and analysis of existing tmux/serial/idf.py helpers; derives concrete requirements for consolidating these workflows into a Go tool."
LastUpdated: 2026-01-02T09:01:16.46143855-05:00
WhatFor: "Stop duplicating environment/port/tmux/flash logic across scripts; use this as a baseline for a unified helper tool design."
WhenToUse: "Use when implementing or reviewing new tooling around ESP-IDF build/flash/monitor workflows in this repo."
---

# Inventory: ESP-IDF helper scripts (tmux/serial/idf.py)

## Goal

Identify and analyze the repo’s existing helper/build scripts that orchestrate ESP-IDF workflows (build/flash/monitor/qemu), especially where tmux and serial console behavior creates recurring sharp edges. Use that inventory to derive requirements for a consolidated Go helper tool for debugging + flashing.

## Scope (what’s included)

Included:
- Shell scripts that run `idf.py` (build/flash/monitor/qemu) and/or orchestrate tmux sessions around those commands.
- Helper scripts that directly interact with a serial console for debugging workflows (pyserial log parsers, raw UART smoke tests).
- A small amount of ticket-scoped historical tooling under `ttmp/**/scripts/` when it encodes known failure modes/patterns (tmux + env issues, operator workflows).

Excluded:
- Firmware code itself (unless referenced for protocol context).
- Purely BLE tooling (except where it motivated tmux/env reliability scripts).
- General “project history” scripts (database/dashboard) not related to flashing/serial/tmux.

## Inventory (high-level)

| Path | Category | Primary jobs | Notable knobs/assumptions |
|---|---|---|---|
| `0016-atoms3r-grove-gpio-signal-tester/build.sh` | thin `idf.py` wrapper | source `export.sh` if needed; `idf.py "$@"` | `REQUIRED_ESP_IDF_VERSION_PREFIX=5.4`; default `IDF_EXPORT_SH=$HOME/esp/esp-idf-5.4.1/export.sh` |
| `0017-atoms3r-web-ui/build.sh` | thin `idf.py` wrapper | same as above | same as above |
| `0018-qemu-uart-echo-firmware/build.sh` | thin `idf.py` wrapper | same as above; used with `idf.py qemu monitor` | same as above |
| `imports/esp32-mqjs-repl/mqjs-repl/build.sh` | thin `idf.py` wrapper | same as above | same as above |
| `0021-atoms3-memo-website/build.sh` | `idf.py` wrapper + tmux | ensure target; optionally run `flash` + `monitor` in tmux panes | tmux mode runs `idf.py flash` and `idf.py monitor` as separate processes |
| `0022-cardputer-m5gfx-demo-suite/build.sh` | `idf.py` wrapper + tmux | ensure target; auto-pick serial port; tmux: `flash monitor` in one pane + interactive shell | disables direnv; explicitly runs `IDF_PATH/tools/idf.py` via IDF venv python |
| `0023-cardputer-kb-scancode-calibrator/build.sh` | `idf.py` wrapper + tmux | same pattern as 0022 | same pattern as 0022 |
| `0025-cardputer-lvgl-demo/build.sh` | `idf.py` wrapper + tmux | same pattern as 0022 | same pattern as 0022 |
| `0019-cardputer-ble-temp-logger/tools/run_fw_flash_monitor.sh` | firmware runner | auto-pick port; source export; validate `esp_idf_monitor`; run `idf.py flash monitor` | hard-coded absolute project path; explicit IDF venv python selection to survive tmux/direnv |
| `imports/esp32-mqjs-repl/mqjs-repl/tools/flash_device_usb_jtag.sh` | stable flasher | parse `build/flash_args`; flash via `python -m esptool`; reboot via watchdog reset | explicitly handles USB Serial/JTAG re-enumeration issues |
| `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_tmux.sh` | tmux smoke test | run `./build.sh qemu monitor` in tmux; send keys; assert expected output | tmux custom socket; captures pane logs to `build/` |
| `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_device_tmux.sh` | tmux smoke test | optionally flash; run `idf.py monitor` in tmux; send keys; assert | uses `flash_device_usb_jtag.sh`; patches sdkconfig console mode first |
| `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_uart_stdio.sh` | raw QEMU smoke test | QEMU `-serial stdio` via pty; send bytes; assert output | uses esptool `merge_bin` to build `qemu_flash.bin` |
| `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_uart_tcp_raw.sh` | raw QEMU smoke test | QEMU UART as TCP server; drive via socket | isolates UART RX from idf_monitor/tmux paths |
| `imports/esp32-mqjs-repl/mqjs-repl/tools/test_js_repl_qemu_uart_stdio.sh` | raw QEMU JS test | similar to above, but asserts JS eval (`var x=...`) works | also verifies autoload path (`globalThis.AUTOLOAD_SEED` = 123) |
| `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_device_uart_raw.py` | raw device smoke test | pyserial direct write/read; assert REPL echo | avoids idf_monitor entirely |
| `imports/esp32-mqjs-repl/mqjs-repl/tools/test_js_repl_device_uart_raw.py` | raw device JS test | pyserial direct write/read; assert JS eval works | avoids idf_monitor entirely |
| `0022-cardputer-m5gfx-demo-suite/tools/capture_screenshot_png.py` | host serial parser | parse framed PNG bytes from serial → write file | assumes `PNG_BEGIN <len>` header and `PNG_END` footer |
| `0023-cardputer-kb-scancode-calibrator/tools/collect_chords.py` | host serial parser | parse log lines into chord observations | regex-driven; writes `chords.json` (but currently only prints; see code) |
| `0013-atoms3r-gif-console/flash_storage.sh` | partition writer | `parttool.py write_partition` to `storage` | assumes ESP-IDF env active so `parttool.py` on PATH |
| `ttmp/2025/12/30/0020-.../scripts/flash_monitor_tmux.sh` | historical tmux helper | flash via `bash -lc 'source export && idf.py ... flash'`; then tmux monitor | fixed `PORT=/dev/ttyACM0`; relies on `bash -lc` for env correctness |
| `ttmp/2025/12/30/0020-.../scripts/pair_debug.py` | historical orchestrator | runs `idf.py` via `bash -lc`; uses pyserial to drive console; uses `plz-confirm` for operator prompts | shows “operator-in-the-loop” debug automation patterns |
| `ttmp/2025/12/30/0020-.../scripts/pair_debug.sh` | historical wrapper | executes `pair_debug.py` via `python3` | very small shim for convenience |
| `ttmp/2025/12/29/0018-QEMU-UART-ECHO.../scripts/01-run-qemu-tcp-serial-5555.py` | historical QEMU UART isolator | launches `qemu-system-xtensa` with `-serial tcp::PORT,server`; connects; sends payloads; writes transcript | built to bypass `idf.py qemu monitor` + `idf_monitor` coupling |
| `ttmp/2025/12/29/0018-QEMU-UART-ECHO.../scripts/02-run-qemu-mon-stdio-pty.py` | historical QEMU UART isolator | launches QEMU with `-serial mon:stdio` under a PTY; “types” payloads; writes transcript | PTY chosen to behave more like a real terminal than piped stdin |
| `ttmp/2025/12/23/008-ATOMS3R-GIF-CONSOLE.../scripts/extract_idf_paths_from_compile_commands.py` | tooling helper (non-serial) | extracts ESP-IDF root candidates from `compile_commands.json` include paths | used to find the exact ESP-IDF checkout when debugging ESP-IDF component sources |

## Detailed script analyses

### Pattern A: thin `build.sh` wrappers (0016/0017/0018/imports)

Representative examples:
- `0016-atoms3r-grove-gpio-signal-tester/build.sh`
- `0017-atoms3r-web-ui/build.sh`
- `0018-qemu-uart-echo-firmware/build.sh`
- `imports/esp32-mqjs-repl/mqjs-repl/build.sh`

What they do:
1. Decide whether to source ESP-IDF by checking `ESP_IDF_VERSION` prefix (`REQUIRED_ESP_IDF_VERSION_PREFIX`, default `5.4`).
2. If sourcing is needed, verify `IDF_EXPORT_SH` exists (default `$HOME/esp/esp-idf-5.4.1/export.sh`) and `source` it.
3. `cd` to project directory.
4. If no args are provided, default to `build`.
5. Execute `idf.py "$@"` directly.

Why this exists:
- Reduces “did you remember to source `export.sh`?” friction.
- Keeps tutorial commands short and repeatable (`./build.sh flash monitor`).

Key limitations:
- Port selection, tmux workflow, and python venv correctness are not addressed here; they’re delegated to the user and environment.

### Pattern B: `build.sh` with target enforcement + tmux split panes (0021)

File: `0021-atoms3-memo-website/build.sh`

Core behavior:
- Adds `ensure_target()` which:
  - if `sdkconfig` exists and looks like it’s for `esp32` (not `esp32s3`), runs `idf.py set-target esp32s3`
  - if `sdkconfig` doesn’t exist yet, also runs `idf.py set-target esp32s3`
- Adds `tmux-flash-monitor` mode:
  - `tmux new-session ... "idf.py $* flash"`
  - `tmux split-window ... "idf.py $* monitor"`
  - layout even-horizontal; attach

What it’s trying to solve:
- Avoid the default-target footgun (“IDF_TARGET not set, using default target: esp32”).
- Provide a “one command” tmux workflow for flash + monitor.

Important sharp edge:
- Separate panes mean separate processes trying to open the same serial port. If the `monitor` pane starts before `flash` releases the port (or if the driver behaves differently per host), this can lead to serial-port contention. This is explicitly avoided in the newer “single pane: `flash monitor`” style.

### Pattern C: `build.sh` with port auto-pick + tmux + explicit IDF python (0022/0023/0025)

Files:
- `0022-cardputer-m5gfx-demo-suite/build.sh`
- `0023-cardputer-kb-scancode-calibrator/build.sh`
- `0025-cardputer-lvgl-demo/build.sh`

Core behavior (shared):
- `export DIRENV_DISABLE=1` to reduce PATH/env variability when invoked from direnv-managed shells or tmux panes.
- `pick_port()` heuristic:
  1. Use `PORT` env var if set.
  2. Prefer `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*` (stable across re-enumeration).
  3. Fallback to `/dev/ttyACM*`.
  4. Otherwise error with “set PORT=...”.
- Environment resolution:
  - Source `IDF_EXPORT_SH` only if current `ESP_IDF_VERSION` doesn’t match the required prefix.
  - Locate `idf.py` as `${IDF_PATH}/tools/idf.py` and run it via python.
  - Prefer python from `${IDF_PYTHON_ENV_PATH}/bin/python`; fallback to `python` on PATH.
- `tmux-flash-monitor` mode:
  - Runs `flash monitor` in a *single* pane:
    - `'$py' '$idf_py' -p '$port' $* flash monitor`
  - Creates a second pane as an interactive shell in the project dir (after sourcing export).
  - Explicit comment: single-pane avoids serial-port locking conflicts.
- Target enforcement: `ensure_target()` switches away from `esp32` to `esp32s3` (similar to 0021).

What it’s trying to solve (explicitly):
- Fast operator workflow: one command to (a) pick the port, (b) flash, (c) attach monitor, (d) keep a usable shell nearby.
- tmux/direnv/python mismatches by running `idf.py` with the exact python from the ESP-IDF venv when available.

Notable footguns:
- `$*` expansion is used in the tmux command string; it’s convenient, but it means argument quoting must be correct at the call site. The current implementation works for the intended simple args (`-p ...`, `-b ...`), but a Go tool could make this robust without relying on shell word-splitting.

### Firmware runner: explicit env + `esp_idf_monitor` presence check (0019)

File: `0019-cardputer-ble-temp-logger/tools/run_fw_flash_monitor.sh`

Core behavior:
1. Disables direnv (`DIRENV_DISABLE=1`).
2. Hard-codes `PROJ_DIR` and default `IDF_EXPORT` (overridable).
3. Port selection: by-id Espressif USB_JTAG → `/dev/ttyACM*` fallback → error.
4. Sources IDF export script.
5. Locates `${IDF_PATH}/tools/idf.py` and a python interpreter:
   - prefer `${IDF_PYTHON_ENV_PATH}/bin/python`
   - fallback to `python` on PATH
6. Validates `esp_idf_monitor` is importable in that python; if not, prints a crisp remediation message and exits.
7. Runs `"$py" "$idf_py" -p "$port" $ACTION` where default `ACTION="flash monitor"`.

Why it exists (from the 0019 diary):
- In tmux panes, running `idf.py monitor` could fail with `No module named 'esp_idf_monitor'` due to the wrong python environment being used; the fix is to be explicit about the IDF python venv.

### Stable flashing for USB Serial/JTAG (mqjs-repl)

File: `imports/esp32-mqjs-repl/mqjs-repl/tools/flash_device_usb_jtag.sh`

Core behavior:
- Resolves port:
  - prefers exactly one `/dev/serial/by-id/*Espressif*USB_JTAG*`
  - errors if multiple candidates exist (forces explicit `--port`)
  - fallback to `/dev/ttyACM0`
- Requires `build/flash_args` (generated by ESP-IDF build system).
  - Parses the first line as “extra args”
  - Parses remaining lines as `offset path` pairs
- Uses `python -m esptool` to flash:
  - `--before usb_reset --after no_reset` to minimize disruptive resets during flashing
- Uses a second `python -m esptool ... read_mac` with `--after watchdog_reset` to force a reboot into the flashed app.

Why it exists:
- It’s explicitly a workaround for transient USB Serial/JTAG disconnect/re-enumeration issues seen with `idf.py flash` on some setups.

### tmux-based smoke tests for “REPL input path works” (mqjs-repl)

Files:
- `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_tmux.sh`
- `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_device_tmux.sh`

Shared pattern:
- Create an isolated tmux server via `tmux -S .tmuxsock-<session> ...` so the test doesn’t interact with the user’s tmux state.
- Run `./build.sh ... monitor` inside the tmux session (and optionally build/flash first).
- Use `tmux send-keys` to inject lines (exercises the same stdin path as a human).
- Poll via `tmux capture-pane` and `grep -F` until a needle appears or a timeout triggers.
- On failures, dump captured output to `build/*.log` to preserve the evidence.

Why it exists:
- The repo has encountered cases where QEMU/monitor stdin paths were broken; these tests turn that into a deterministic check with actionable logs.

### Raw UART smoke tests (QEMU and device) to bypass idf_monitor

Files (QEMU):
- `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_uart_stdio.sh`
- `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_uart_tcp_raw.sh`
- `imports/esp32-mqjs-repl/mqjs-repl/tools/test_js_repl_qemu_uart_stdio.sh`

Files (device):
- `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_device_uart_raw.py`
- `imports/esp32-mqjs-repl/mqjs-repl/tools/test_js_repl_device_uart_raw.py`

What they do:
- Build firmware and construct a QEMU flash image using `python -m esptool ... merge_bin`.
- Run QEMU with UART wired to either:
  - stdio (driven via a pty pair), or
  - a raw TCP server (driven via a socket).
- For device tests, use pyserial directly.
- Send `:mode` / `:mode js` and assert the REPL responds and echoes.

Why it exists:
- It isolates “UART RX/TX correctness” from idf_monitor/tmux/PTY layers; when something breaks, these tests help pinpoint which layer is responsible.

### Host-side serial parsers for debugging artifacts

Files:
- `0022-cardputer-m5gfx-demo-suite/tools/capture_screenshot_png.py`
- `0023-cardputer-kb-scancode-calibrator/tools/collect_chords.py`

Common pattern:
- Open a serial port and parse specific firmware-emitted framing/log lines into a host artifact (PNG, JSON-ish data).
- These scripts are intentionally minimal; their existence suggests a need for a reusable “serial read + parse + artifact output” framework.

### Historical ticket scripts: tmux helpers + operator-in-the-loop tooling (0020)

Files:
- `ttmp/2025/12/30/0020-.../scripts/flash_monitor_tmux.sh` (simple tmux monitor)
- `ttmp/2025/12/30/0020-.../scripts/pair_debug.py` (much richer orchestration)

Key patterns worth preserving:
- Running `idf.py` through `bash -lc 'source export.sh && idf.py ...'` to ensure the right environment in non-interactive contexts.
- Wrapping debug sequences into a single “operator workflow” tool (pairing is a good example of: flash → monitor → user timing windows → capture logs/artifacts).

### Historical QEMU isolation scripts (0018)

Files:
- `ttmp/2025/12/29/0018-QEMU-UART-ECHO--track-c-minimal-uart-echo-firmware-to-isolate-qemu-uart-rx/scripts/01-run-qemu-tcp-serial-5555.py`
- `ttmp/2025/12/29/0018-QEMU-UART-ECHO--track-c-minimal-uart-echo-firmware-to-isolate-qemu-uart-rx/scripts/02-run-qemu-mon-stdio-pty.py`

What they do:
- Launch QEMU directly (not via `idf.py qemu monitor`) with explicit serial backends:
  - `-serial tcp::PORT,server,nowait` (driven by a TCP client in-process)
  - `-serial mon:stdio` (driven via a PTY pair, to emulate a terminal)
- Send a handful of test payloads and capture a transcript.

Why they matter:
- They are the “first generation” of the raw-UART QEMU tests later formalized in `imports/esp32-mqjs-repl/mqjs-repl/tools/test_*qemu_uart_*`.
- They encode the underlying philosophy: isolate layers (QEMU UART RX vs idf_monitor vs tmux) with targeted, scriptable probes.

## Commonalities and patterns

### 1) ESP-IDF environment activation is ubiquitous and brittle

Observed patterns:
- Conditional sourcing via `ESP_IDF_VERSION` prefix (most `build.sh`).
- Hard-coded default `export.sh` location (almost everywhere).
- Explicit python selection via `IDF_PYTHON_ENV_PATH` only in the scripts that were written after an env-related failure (0019 runner; 0022/0023/0025 tmux helpers).

Implication for Go tooling:
- “Resolve IDF env” should be a first-class function (not an afterthought), including diagnostics for python module availability (`esp_idf_monitor`, `esptool`, `parttool.py`).

### 2) Port selection is duplicated and inconsistent

Observed strategies:
- “Pick first match” (e.g. `ls ... | head -1`) vs “error if multiple”.
- by-id is preferred when used (good), but glob patterns vary (some require `USB_JTAG_serial_debug_unit_*`, others match `*Espressif*USB_JTAG*`).

Implication:
- A shared port resolver should:
  - list candidates with stable IDs + metadata
  - default safely (avoid silently picking the wrong one on multi-device setups)
  - support interactive selection or explicit profiles

### 3) tmux is used as a reliability primitive (detach/reattach + multi-pane workflows)

Observed tmux patterns:
- “One pane runs `flash monitor`” to avoid serial locking (0022/0023/0025).
- “Two panes: flash vs monitor” (0021, older).
- “Isolated tmux server via `-S socket`” for scripted smoke tests (mqjs-repl tools).

Implication:
- Go tooling can treat tmux as an *optional transport*:
  - if tmux is present and requested, create sessions/panes with robust quoting
  - if not, run in the current terminal while still enabling log capture

### 4) There is a recurring need for repeatable “debugging procedures”

Examples:
- “Is stdin working through QEMU+monitor?” → tmux send-keys test.
- “Is UART RX broken in QEMU or just in idf_monitor?” → raw UART stdio/TCP tests.
- “Can I capture a screenshot/chord table reliably?” → serial parser scripts.

Implication:
- The Go tool should support:
  - scripted “procedures” (smoke tests) with timeouts and artifacts
  - consistent log capture and structured result reporting

## Opportunities: a consolidated Go helper tool

### Problems to solve (derived from existing scripts)

1. **Environment correctness across shells and tmux**
   - Avoid “wrong python” failures (`esp_idf_monitor` missing).
   - Avoid reliance on implicit `idf.py` resolution on PATH.
2. **Safe, explicit serial port selection**
   - Prefer stable by-id paths.
   - Don’t silently pick the wrong device in multi-device setups.
3. **Robust flashing**
   - Support both `idf.py flash` and a “stable USB-JTAG flash” path (esptool + `build/flash_args`).
4. **Reliable monitor sessions**
   - One-command “flash + monitor” that avoids port locking.
   - Optional tmux session management (attach/detach).
5. **Repeatable debug procedures + artifact capture**
   - Tests/probes that isolate layers (UART vs monitor vs tmux).
   - Built-in “serial parse → artifact” helpers.

### Suggested Go CLI shape (proposal)

One binary (name TBD, e.g. `esphelper`) with subcommands:
- `esphelper env doctor` — show ESP-IDF env, python, module availability, `IDF_PATH`, `IDF_PYTHON_ENV_PATH`, `ESP_IDF_VERSION`.
- `esphelper ports list` / `esphelper ports pick` — enumerate candidates; pick interactively or by rules.
- `esphelper build` — `idf.py build` with target enforcement and consistent env setup.
- `esphelper flash` — choose flashing backend:
  - `idf.py flash` (default)
  - `esptool flash` (stable USB-JTAG mode; reads `build/flash_args`)
- `esphelper monitor` — run `idf.py monitor` (or a minimal Go monitor) with log capture.
- `esphelper flash-monitor` — run as a single process to avoid serial contention.
- `esphelper tmux flash-monitor` — create/attach a session (optional).
- `esphelper qemu monitor` — wrapper for `idf.py qemu monitor` plus optional smoke tests.
- `esphelper capture screenshot-png` / `esphelper capture chords` — reusable serial framing parsers (replacing one-off python scripts over time).

### Design notes (how to avoid repeating the current pitfalls)

- Prefer explicit process execution with arg arrays (no `$*` shell splitting).
- Treat “IDF env resolution” as structured data:
  - discover export.sh location (config + env + common defaults)
  - verify `idf.py` path and python
  - verify python modules needed for a command
- Make port selection explicit:
  - `--port auto|/dev/...` with deterministic behavior
  - error on multiple candidates unless a selection policy is configured
- Keep tmux optional; provide a consistent non-tmux UX that still captures logs.

## Notes for the ticket (next docs to write)

- A design doc that chooses between “wrap `idf.py monitor`” vs “implement monitor in Go”.
- A “port resolver contract” doc (what “auto” means, what errors look like).
- A “stable flash” contract doc (USB-JTAG vs UART, expected behavior around resets).

## Related prior tickets consulted (context)

- `ttmp/2025/12/30/0019-BLE-TEST--ble-temperature-sensor-logger-on-cardputer/reference/02-diary.md` — documents the tmux/python mismatch that caused `No module named 'esp_idf_monitor'` and motivated explicit-IDF-python runners.
- `ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/scripts/flash_monitor_tmux.sh` — earlier tmux monitor helper pattern (`bash -lc` + sourced export).
- `ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/scripts/pair_debug.py` — richer “operator workflow” automation (idf.py + serial + approvals) that’s relevant when thinking about debug playbooks.
- `ttmp/2025/12/29/0018-QEMU-UART-ECHO--track-c-minimal-uart-echo-firmware-to-isolate-qemu-uart-rx/reference/01-diary.md` — captures QEMU/ESP-IDF environment sharp edges (watchdog disable not taking effect; `sdkconfig.defaults` not overriding existing `sdkconfig` without `fullclean`).
