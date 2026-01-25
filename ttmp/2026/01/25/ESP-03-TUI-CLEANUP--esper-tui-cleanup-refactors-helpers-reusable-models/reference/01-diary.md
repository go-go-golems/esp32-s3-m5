---
Title: Diary
Ticket: ESP-03-TUI-CLEANUP
Status: active
Topics:
  - tui
  - bubbletea
  - lipgloss
  - refactor
  - testing
  - esp32s3
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
  - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/monitor/app_model.go
    Note: App-level screen + overlay routing
  - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/monitor/monitor_view.go
    Note: Monitor model update/view; overlay requests originate here
  - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/monitor/overlay_manager.go
    Note: Unified overlay interface + rendering over background
  - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/monitor/overlay_wrappers.go
    Note: Wraps existing overlay models to fit overlay interface and emit screen messages
  - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/09-tmux-capture-esper-tui.sh
    Note: Screenshot harness used to validate no UI regression during refactor
ExternalSources: []
Summary: Implementation diary for ESP-03 (structural cleanup/refactors).
LastUpdated: 2026-01-25T14:25:59-05:00
WhatFor: Track refactor steps and validation (tests + captures) so we can iterate safely without UI regressions.
WhenToUse: Use when continuing the cleanup work (overlay unification, shared list core, modal chrome helpers, semantic styles).
---

# Diary

## Goal

Refactor the Esper Bubble Tea TUI to reduce duplicated UI code and unify behavior (especially overlays), while keeping the UI stable through incremental commits validated by tests and tmux screenshot captures against the connected `esp32s3-test` firmware.

## Step 1: Unify overlays under `appModel` (help/search/filter/palette)

The TUI previously had two overlay systems: the Help overlay was app-level, but Search/Filter/Palette were monitor-level with their own routing and layering logic. This split made close/resize behavior inconsistent and duplicated overlay rendering. I refactored overlays so `appModel` owns the single active overlay and is the only place that routes KeyMsgs to overlays and applies overlay rendering on top of the current screen.

To keep the UI stable, I validated the change in two ways: `go test ./...` in the nested `esper/` repo, and a full tmux capture run using the connected USB Serial/JTAG device (so we prove port scan/connect + the overlay key paths still work end-to-end).

### Prompt Context

**User prompt (verbatim):** "go ahead and start implementing the refactor first, making sure you are not breaking the UI as you go. Use the esp32 firmware appropriately, I have the device connected"

**Assistant interpretation:** Start the ESP-03 cleanup refactor with the overlay unification work, validating continuously with tests and real-device runs/captures.

**Inferred user intent:** Make the codebase easier to evolve (especially towards wireframe parity) without introducing UX regressions during refactors.

**Commit (code):** fcbcd29 — "tui: unify overlays under app model"

### What I did

- Implemented a unified overlay interface + overlay-over-background renderer:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/monitor/overlay_manager.go`
- Wrapped existing overlay models (help/search/filter/palette) so we didn’t have to rewrite their internal update logic:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/monitor/overlay_wrappers.go`
- Moved overlay ownership + routing to `appModel`:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/monitor/app_model.go`
- Converted monitor overlays into “overlay requests” and “overlay result messages”:
  - overlay results are forwarded to the active screen as typed messages (search/filter/palette exec), so we keep explicit routing.
- Updated the ESP-02 “architecture review” doc to reflect that overlays are now unified in `appModel`:
  - `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/02-tui-current-vs-desired-compare.md`

### Why

- A single overlay owner prevents “overlay logic in two places” and makes future screens/confirm dialogs consistent.
- Centralizing overlay rendering reduces repeated layering code and avoids subtle sizing regressions.

### What worked

- Unit-level validation:
  - `cd esper && go test ./... -count=1`
- End-to-end validation against the connected USB Serial/JTAG device:
  - `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/09-tmux-capture-esper-tui.sh`
  - Capture output:
    - `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/screenshots/20260125-141725/`

### What didn't work

- N/A (no regressions observed in tests/captures; capture harness completed successfully).

### What I learned

- The “overlay split” (help at app-level, others at screen-level) was the biggest structure pain point; unifying it makes later refactors (reusable list core, shared modal chrome) much easier.

### What was tricky to build

- Making sure overlay events can still trigger screen actions (e.g., Search “jump”, Palette “open filter”) without re-introducing “overlay state inside screen state”. The wrappers emit typed messages (`searchActionMsg`, `filterSetMsg`, `paletteExecMsg`) and `appModel` forwards them to the active screen update loop.

### What warrants a second pair of eyes

- Verify the overlay message forwarding pattern remains easy to extend once we add more overlays (confirmations, core dump progress, panic detail) and doesn’t turn into a hidden “event bus”.

### What should be done in the future

- Extract the shared selectable list core and migrate Port Picker / Palette / Inspector event list to use it (ESP-03 remaining tasks).

### Code review instructions

- Start at:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/monitor/app_model.go`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/monitor/overlay_manager.go`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/monitor/overlay_wrappers.go`
- Validate:
  - `cd esper && go test ./... -count=1`
  - Run the tmux capture harness (should connect and capture overlays):
    - `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/09-tmux-capture-esper-tui.sh`

### Technical details

- Overlay rendering uses Lip Gloss placement + line replacement over a dimmed background (no manual box drawing).
- Esc closes overlays consistently at the `appModel` layer; Help also supports `q`.

## Step 2: Reusable selectable list core (port picker, palette, inspector)

After overlay unification, the next biggest duplication was selection + “windowed list” logic: port picker list selection, command palette selection, and inspector event selection all implemented slightly different versions of the same behavior. I extracted a tiny shared list core (`selectList`) and migrated these three list usages to it, aiming for “no UX changes” while reducing repeated clamping/window math.

Validation remained the same: run unit tests, then run the tmux capture harness. I attempted to use the connected device again, but the USB serial device nodes disappeared from the environment; I captured the exact errors and fell back to the virtual PTY mode for regression screenshots.

### Prompt Context

**User prompt (verbatim):** "go ahead and start implementing the refactor first, making sure you are not breaking the UI as you go. Use the esp32 firmware appropriately, I have the device connected"

**Assistant interpretation:** Continue the cleanup refactor with the next “high leverage” extraction (shared selectable list core) and keep validating via tests + captures.

**Inferred user intent:** Reduce UI code duplication so future UX iterations (ESP-04 visual parity work) are faster and less error-prone, without regressing behavior.

**Commit (code):** 0ccf5df — "tui: reuse selectable list core"

### What I did

- Added `selectList` reusable core:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/monitor/select_list.go`
- Migrated:
  - Port picker list selection/window:
    - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/monitor/port_picker.go`
  - Command palette selection:
    - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/monitor/palette_overlay.go`
  - Inspector event selection/window:
    - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/monitor/monitor_view.go`

### Why

- This keeps list selection/window behavior consistent across screens and removes repeated “selected clamp + window start/end” logic.
- It also sets us up to later add separators and more complex list rows in one place (needed for the wireframe palette grouping).

### What worked

- Tests:
  - `cd esper && go test ./... -count=1`
- Screenshot regression capture (virtual PTY):
  - `USE_VIRTUAL_PTY=1 ./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/09-tmux-capture-esper-tui.sh`
  - Output:
    - `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/screenshots/20260125-142512/`

### What didn't work

- Real-device capture attempt failed because the serial device nodes disappeared:
  - `ERROR: could not auto-detect an ESP32 serial port (no /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_* and no /dev/ttyACM*).`
- A prior run of the capture harness also failed mid-run due to leaked tmux/go-run sessions:
  - `can't find session: esper_tui_cap_80x24_3502645`
  - Fix: killed leaked tmux sessions and `go run ./cmd/esper` processes before retrying.

### What I learned

- Even with explicit cleanup, leaked tmux/go-run sessions can leave the system in a confusing state; we should keep the capture harness cleanup robust and avoid running multiple captures concurrently.

### What was tricky to build

- Keeping the refactor “behavior-preserving”: selection/window logic is subtle, and a shared helper must match existing clamp/window behavior or the UI will “feel different” immediately.

### What warrants a second pair of eyes

- Inspector selection + windowing: confirm the selected event stays stable and the same event remains highlighted when new events arrive (especially when the list truncates to `maxEvents`).

### What should be done in the future

- Extend the reusable list core to support separators and richer row rendering (needed for wireframe-faithful command palette grouping).

### Code review instructions

- Start at:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/monitor/select_list.go`
- Validate:
  - `cd esper && go test ./... -count=1`
  - `USE_VIRTUAL_PTY=1 ./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/09-tmux-capture-esper-tui.sh`

### Technical details

- `selectList.Window()` intentionally matches the “selected pulls the window down once it exceeds visible height” behavior used previously in port picker and inspector.
