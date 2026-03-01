---
Title: Cardputer-Adv Photo Development Timer Implementation Plan
Ticket: ESP-23-PHOTO-TIMER
Status: active
Topics:
    - cardputer
    - cardputer-adv
    - esp32-s3
    - firmware
    - timer
    - photo-development
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0038-cardputer-adv-serial-terminal/main/hello_world_main.cpp
      Note: Cardputer-ADV display loop and interactive control patterns
    - Path: 0039-cardputer-adv-js-gpio-exercizer/main/storage/Spiffs.cpp
      Note: SPIFFS mount and file I/O helper pattern
    - Path: 0047-cardputer-adv-lvgl-chain-encoder-list/main/app_main.cpp
      Note: Reusable LVGL encoder input binding pattern
    - Path: 0047-cardputer-adv-lvgl-chain-encoder-list/main/chain_encoder_uart.cpp
      Note: Reusable chain encoder UART protocol driver
    - Path: 0048-cardputer-js-web/main/http_server.cpp
      Note: HTTP endpoint and embedded asset serving patterns
    - Path: 0071-cardputer-adv-photo-timer/main/app_main.cpp
      Note: Main runtime wiring for LVGL UI
    - Path: 0071-cardputer-adv-photo-timer/main/assets/index.html
      Note: Embedded web UI for preset editing/upload
    - Path: 0071-cardputer-adv-photo-timer/main/chain_encoder_uart.cpp
      Note: Encoder UART driver used for navigation
    - Path: 0071-cardputer-adv-photo-timer/main/http_server.cpp
      Note: REST API and preset upload endpoints
    - Path: 0071-cardputer-adv-photo-timer/main/preset_store.cpp
      Note: SPIFFS JSON preset storage and validation
    - Path: 0071-cardputer-adv-photo-timer/main/timer_engine.cpp
      Note: Timer state machine implementation
    - Path: components/wifi_mgr/wifi_mgr.c
      Note: Wi-Fi runtime and NVS credential management
ExternalSources: []
Summary: Implementation plan for a Cardputer-ADV firmware that runs film development timers with preset files loaded at boot and editable through a small device-hosted web UI.
LastUpdated: 2026-03-01T00:00:00Z
WhatFor: Guide implementation and validation of a new photo development timer firmware.
WhenToUse: Use when implementing or reviewing firmware scope for ESP-23-PHOTO-TIMER.
---



# Cardputer-Adv Photo Development Timer Implementation Plan

## Executive Summary

This ticket delivers a new Cardputer-ADV firmware that lets users run film-development workflows (starting with C41/CineStill-style presets) as timed, step-by-step sequences. Presets are stored as JSON configuration files, loaded at boot, selectable with the attached chain encoder, and editable/uploadable through a lightweight HTTP UI hosted on the device.

The fastest implementation path is to compose proven pieces from existing firmware:

1. Reuse chain-encoder UART driver and LVGL encoder input wiring from `0047`.
2. Reuse Cardputer/ESP-IDF console and display bring-up patterns from `0038`.
3. Reuse Wi-Fi manager and HTTP server + embedded assets patterns from `0048` and shared components.
4. Reuse SPIFFS mounting/file I/O patterns from `0039`.

The firmware will be implemented as a new project (`0071-cardputer-adv-photo-timer`) with clear module boundaries for timer engine, preset storage, UI, and web API.

## Problem Statement and Scope

### Requested outcome

User asks for:

1. A photo-development timer on Cardputer-ADV.
2. Preset workflows (for example C41 CineStill) with multiple timed steps.
3. Presets stored in config files loaded at boot.
4. Optional upload/update from a small web interface.
5. Navigation via attached encoder.

### In-scope

1. On-device preset selection and run control with encoder navigation.
2. Multi-step timer state machine (start/pause/resume/next/reset).
3. JSON preset storage in flash-backed filesystem.
4. Boot-time load of active preset and available preset list.
5. HTTP endpoints + simple web page for listing/uploading/replacing presets.
6. USB Serial/JTAG console as default interactive console.

### Out-of-scope (for this ticket)

1. Cloud sync.
2. Full phone app.
3. Advanced chemistry calculations (temperature compensation curves, dilution calculators).
4. Multi-user auth on the device-hosted web interface.

## Current-State Architecture (Evidence-Based)

### 1) Encoder and LVGL input path already exists

`0047` already solves chain-encoder-over-UART input and LVGL mapping:

1. `ChainEncoderUart` config and task lifecycle: `0047-cardputer-adv-lvgl-chain-encoder-list/main/chain_encoder_uart.cpp:35`.
2. Protocol framing and CRC parsing: `.../chain_encoder_uart.cpp:185`, `:268`.
3. Click event handling (`0xE0`) and pending-click state: `.../chain_encoder_uart.cpp:314`.
4. LVGL encoder read callback (`enc_diff` + press/release pulse): `0047.../app_main.cpp:46`.
5. UART pin defaults and polling exposed in Kconfig: `0047.../main/Kconfig.projbuild:3`.

This is directly reusable for encoder-driven menu navigation.

### 2) Display and runtime loop patterns exist for Cardputer-ADV

`0038` demonstrates robust device bring-up and display refresh loop:

1. `M5.begin()` and display init path: `0038.../hello_world_main.cpp:432`.
2. Canvas-backed UI redraw loop with periodic update: `.../hello_world_main.cpp:696`.
3. Pattern for local action hotkeys/commands that mutate runtime state: `.../hello_world_main.cpp:568` and `:637`.

This provides practical structure for UI update cadence and responsive control handling.

### 3) USB Serial/JTAG console pattern is already aligned with repo guidance

1. Console start over USB Serial/JTAG in `0038`: `0038.../main/console_repl.cpp:53`.
2. Guard to avoid transport conflicts: `0038.../main/console_repl.cpp:40`.
3. Defaults explicitly selecting USB Serial/JTAG in multiple projects:
   - `0037-cardputer-adv-fan-control-console/sdkconfig.defaults:20`
   - `0047-cardputer-adv-lvgl-chain-encoder-list/sdkconfig.defaults:25`
   - `0048-cardputer-js-web/sdkconfig.defaults:18`

This matches AGENTS guidance and avoids UART collision with the encoder link.

### 4) Wi-Fi + HTTP server patterns already exist and are modularized

`0048` + shared components provide a ready path:

1. Start HTTP server after STA gets IP: `0048.../main/app_main.cpp:11`, `:14`, `:20`.
2. Embedded asset serving helper (`httpd_assets_embed_send`): `components/httpd_assets_embed/httpd_assets_embed.c:11`.
3. `wifi_mgr` handles NVS credentials, connect state, retry, scan:
   - NVS init/load/save: `components/wifi_mgr/wifi_mgr.c:77`, `:106`, `:141`
   - connect/disconnect/scan APIs: `.../wifi_mgr.c:406`, `:427`, `:436`
4. `wifi_console` provides runtime `esp_console` commands for network setup: `components/wifi_console/wifi_console.c:88`, `:300`.

This avoids writing network bootstrap from scratch.

### 5) SPIFFS file-mount/read pattern already exists

`0039` has a compact reusable SPIFFS helper:

1. Mounting `/spiffs` on `storage` partition: `0039.../main/storage/Spiffs.cpp:20`.
2. File read helper with heap buffer and null terminator: `.../Spiffs.cpp:39`.
3. Partition table includes `storage` SPIFFS partition: `0039.../partitions.csv:6`.

This can be extended for read/write preset JSON files.

## Gap Analysis

Relative to requested behavior, current firmwares are missing:

1. A domain model for photo process presets and timed steps.
2. A persistent preset registry contract (schema/versioning).
3. A timer engine with step transitions and deterministic timing behavior.
4. Boot-time loading of active preset + fallback defaults when storage is empty.
5. A web API that accepts uploaded preset definitions and persists them.
6. UI screens specifically for workflow run/step progress.

## Proposed Architecture

## 1) New firmware project

Create `0071-cardputer-adv-photo-timer` with C++ app entry and modules:

1. `main/app_main.cpp` boot orchestration.
2. `main/chain_encoder_uart.*` reused/adapted from `0047`/`0048`.
3. `main/lvgl_port_m5gfx.*` reused from `0047`.
4. `main/preset_store.*` SPIFFS + JSON persistence.
5. `main/timer_engine.*` step-timer state machine.
6. `main/ui_screen.*` LVGL UI model and callbacks.
7. `main/http_server.*` REST API + embedded page.
8. `main/state.*` synchronized shared runtime state.

Dependencies:

1. `M5GFX`, `lvgl`, `esp_http_server`, `spiffs`, `cjson`, `esp_timer`, `driver`.
2. Shared components: `wifi_mgr`, `wifi_console`, `httpd_assets_embed`.

## 2) Data model and JSON schema

### Preset file schema (`/spiffs/presets.json`)

```json
{
  "version": 1,
  "active_preset_id": "c41-cinestill",
  "presets": [
    {
      "id": "c41-cinestill",
      "name": "C41 CineStill CS41",
      "steps": [
        {"name": "Pre-wet", "seconds": 60},
        {"name": "Developer", "seconds": 210},
        {"name": "Blix", "seconds": 480},
        {"name": "Wash", "seconds": 180},
        {"name": "Stabilizer", "seconds": 60}
      ]
    }
  ]
}
```

Validation rules:

1. `version` required and currently `1`.
2. `presets` non-empty.
3. Each preset requires unique `id`, non-empty `name`, non-empty `steps`.
4. Each step requires non-empty `name`, `seconds >= 1`.
5. If `active_preset_id` missing/invalid, fallback to first preset.

## 3) Timer engine behavior

State machine:

1. `IDLE`: selected preset loaded, no countdown running.
2. `RUNNING`: active step countdown in progress.
3. `PAUSED`: countdown suspended.
4. `STEP_DONE`: transient transition state.
5. `COMPLETE`: all steps finished.

Core operations:

1. `start()` from `IDLE` or `COMPLETE` (reset index to first step).
2. `pause()` / `resume()`.
3. `next_step()` manual advance.
4. `reset()` to step 0 and full duration.
5. `set_preset(id)` rebinds sequence; if running, force `IDLE`.

Timing approach:

1. Track `step_started_us` and `remaining_ms` using `esp_timer_get_time()`.
2. Periodic update tick (e.g., every 100 ms) computes remainder.
3. Transition to next step when remainder reaches zero.

## 4) UI interaction model (encoder-driven)

LVGL screen sections:

1. Header: preset name, state, step `i/N`.
2. Main list: actions (`Start/Pause`, `Next`, `Reset`) and preset selector items.
3. Footer status: current step name + remaining `MM:SS`.

Encoder mapping:

1. Rotate: focus next/previous list item.
2. Click: activate focused item.
3. Long click (optional): quick reset.

UI refresh:

1. LVGL timer callback every 200 ms updates status labels.
2. Rebuild preset list when config revision increments.

## 5) Web interface and API

Network flow:

1. Boot Wi-Fi manager.
2. Start `wifi_console` for local credential management.
3. Register `on_got_ip` callback to start HTTP server.

HTTP endpoints:

1. `GET /api/status` -> current preset, step, timer state, remaining seconds.
2. `GET /api/presets` -> full preset JSON document.
3. `POST /api/presets` -> replace/import preset JSON (validation + persist + reload).
4. `POST /api/control` -> `{action:"start|pause|resume|next|reset", preset_id?:"..."}`.

UI page:

1. Simple embedded HTML + JS (`main/assets/index.html`).
2. Textarea for JSON upload/edit.
3. Buttons: Fetch presets, Save presets, Start/Pause/Next/Reset.
4. Status panel polling `/api/status`.

## Key Flows (Pseudocode)

### Boot sequence

```cpp
app_main() {
  init_display_and_lvgl();
  init_encoder_uart();

  preset_store_mount();
  cfg = preset_store_load_or_seed_defaults();

  timer_engine_init(cfg.active_preset);
  ui_init(cfg, timer_engine);

  wifi_mgr_start();
  wifi_mgr_set_on_got_ip_cb(start_http_server);
  wifi_console_start();

  while (true) {
    lv_timer_handler();
    timer_engine_update();
    ui_refresh_if_needed();
    vTaskDelay(10ms);
  }
}
```

### Preset upload path

```cpp
POST /api/presets {
  body = read_request_body();
  parsed = parse_and_validate_json(body);
  if (!parsed.ok) return 400;

  preset_store_write(parsed.config);
  runtime_state_replace_config(parsed.config);
  timer_engine_bind_active_preset(parsed.config.active_preset_id);
  ui_mark_rebuild_needed();

  return 200 with status;
}
```

### Timer tick

```cpp
timer_engine_update(now_us) {
  if (state != RUNNING) return;

  elapsed_ms = (now_us - step_started_us)/1000;
  remaining_ms = max(0, step_duration_ms - elapsed_ms);

  if (remaining_ms == 0) {
    if (has_next_step()) {
      step_index++;
      step_started_us = now_us;
      step_duration_ms = next_step.seconds * 1000;
    } else {
      state = COMPLETE;
    }
  }
}
```

## Phased Implementation Plan

### Phase 1: Scaffold firmware project

1. Create `0071-cardputer-adv-photo-timer` from `0047` baseline.
2. Wire shared component directories (`wifi_mgr`, `wifi_console`, `httpd_assets_embed`).
3. Keep USB Serial/JTAG console defaults in `sdkconfig.defaults`.

### Phase 2: Preset storage and schema validation

1. Implement SPIFFS mount and JSON load/save (`preset_store`).
2. Seed default preset file when none exists.
3. Add schema validation and informative error logs.

### Phase 3: Timer engine

1. Implement state machine + snapshot API.
2. Add deterministic transitions and manual controls.
3. Add unit-like validation function for duration bounds.

### Phase 4: LVGL UI and encoder controls

1. Build main screen and action list.
2. Bind encoder click to actions and preset selection.
3. Add periodic UI refresh from engine snapshot.

### Phase 5: Wi-Fi and HTTP API

1. Integrate `wifi_mgr` + `wifi_console` startup.
2. Implement API endpoints for status/control/preset upload.
3. Serve embedded HTML for minimal browser-based editing.

### Phase 6: Integration hardening

1. Ensure safe config replacement while timer is active.
2. Protect shared state with mutex.
3. Add bounds/size checks for HTTP body and JSON arrays.

### Phase 7: Validation and docs

1. Add smoke-test playbook commands (`curl` + on-device actions).
2. Update README with wiring and usage.
3. Finalize ticket tasks, changelog, and diary.

## Testing and Validation Strategy

### Build-level

1. `idf.py set-target esp32s3`
2. `idf.py build`

### Device smoke tests

1. Boot with empty storage; verify default preset creation.
2. Rotate/click encoder; verify list navigation and action activation.
3. Start/pause/reset timer; verify step transitions and countdown.
4. Connect Wi-Fi via `wifi` console commands.
5. Load web page and POST new preset JSON.
6. Reboot and verify uploaded presets persist and auto-load.

### API tests

1. `curl GET /api/status` returns valid JSON.
2. `curl GET /api/presets` returns schema-compliant body.
3. `curl POST /api/presets` rejects malformed JSON with `400`.
4. `curl POST /api/control` enforces valid action names.

## Risks, Tradeoffs, and Mitigations

1. UART pin conflict with encoder and console.
   Mitigation: keep console on USB Serial/JTAG; avoid UART console.
2. Corrupt preset file bricks startup flow.
   Mitigation: fallback to in-firmware defaults and rewrite safe file.
3. Concurrent writes from web handler and UI thread.
   Mitigation: central state mutex + copy-on-write config swap.
4. LVGL/UI complexity on small display.
   Mitigation: keep one-screen model with compact action list and status line.
5. Timing drift under load.
   Mitigation: derive remaining time from monotonic microsecond clock, not loop tick count.

## Alternatives Considered

1. Keyboard-first UI (from `0038`) instead of encoder-first.
   Rejected: user explicitly requested encoder navigation.
2. NVS-only preset storage (single blob) instead of file-based JSON.
   Rejected: user requested config-file workflow and easy upload/replacement.
3. WebSocket-heavy UI.
   Rejected: polling REST is sufficient for low-rate timer/status updates.
4. FATFS partition over SPIFFS.
   Rejected for now: SPIFFS patterns already proven in this repo and sufficient for small JSON files.

## Open Questions

1. Exact canonical CineStill C41 timings the user wants preloaded (temperature-dependent variants).
2. Whether audio alerts (buzzer/speaker) are required in first release.
3. Whether preset upload should merge or replace existing set by default.

## References

1. `0047-cardputer-adv-lvgl-chain-encoder-list/main/app_main.cpp:46`
2. `0047-cardputer-adv-lvgl-chain-encoder-list/main/chain_encoder_uart.cpp:35`
3. `0047-cardputer-adv-lvgl-chain-encoder-list/main/chain_encoder_uart.cpp:314`
4. `0047-cardputer-adv-lvgl-chain-encoder-list/main/Kconfig.projbuild:3`
5. `0047-cardputer-adv-lvgl-chain-encoder-list/main/lvgl_port_m5gfx.cpp:55`
6. `0038-cardputer-adv-serial-terminal/main/hello_world_main.cpp:427`
7. `0038-cardputer-adv-serial-terminal/main/console_repl.cpp:53`
8. `0048-cardputer-js-web/main/app_main.cpp:11`
9. `0048-cardputer-js-web/main/http_server.cpp:60`
10. `components/wifi_mgr/wifi_mgr.c:77`
11. `components/wifi_mgr/wifi_mgr.c:406`
12. `components/wifi_console/wifi_console.c:88`
13. `components/httpd_assets_embed/httpd_assets_embed.c:11`
14. `0039-cardputer-adv-js-gpio-exercizer/main/storage/Spiffs.cpp:15`
