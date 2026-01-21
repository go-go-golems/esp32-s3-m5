---
Title: Design + implementation plan
Ticket: 0051-WIFI-CONSOLE-COMPONENT
Status: active
Topics:
    - esp-idf
    - wifi
    - console
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0046-xiao-esp32c6-led-patterns-webui/main/led_console.c
      Note: Extra command registration used by wifi_console (0046)
    - Path: 0046-xiao-esp32c6-led-patterns-webui/main/wifi_console.c
      Note: Source implementation (0046)
    - Path: 0048-cardputer-js-web/main/js_console.cpp
      Note: Extra command registration used by wifi_console (0048)
    - Path: 0048-cardputer-js-web/main/wifi_console.c
      Note: Source implementation (0048 + js extra commands)
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T15:36:21.021196951-05:00
WhatFor: ""
WhenToUse: ""
---


# Design + implementation plan

## Executive Summary

Extract the duplicated `wifi` esp_console command + REPL startup into a reusable ESP‑IDF component, `components/wifi_console`.

Key requirement: keep the “wifi command UX” identical, while allowing each firmware to register extra commands (e.g. `led`, `js`) without the console component depending on application modules.

## Problem Statement

We currently duplicate the same console provisioning UX in multiple firmwares:

- `esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/wifi_console.c`
- `esp32-s3-m5/0048-cardputer-js-web/main/wifi_console.c`

Both implement:

- `wifi status|scan|join|set|connect|disconnect|clear`
- start an `esp_console` REPL with a prompt string
- register help + wifi command + firmware-specific “extra” commands:
  - 0046: `led_console_register_commands()`
  - 0048: `js_console_register_commands()`

Duplication makes it too easy to:

- drift prompts/backend selection behavior,
- diverge the `wifi` command semantics,
- accidentally add ticket-specific dependencies into what should be generic provisioning glue.

## Proposed Solution

Create:

- `esp32-s3-m5/components/wifi_console/`
  - `include/wifi_console.h`
  - `wifi_console.c`
  - `Kconfig` (optional: default prompt, enable/disable)
  - `CMakeLists.txt`

### API shape

Provide a minimal API with an extension hook:

```c
typedef void (*wifi_console_register_extra_cb_t)(void* ctx);

typedef struct {
  const char* prompt; // e.g. "0048> " or "c6> "
  wifi_console_register_extra_cb_t register_extra;
  void* register_extra_ctx;
} wifi_console_config_t;

void wifi_console_start(const wifi_console_config_t* cfg);
```

Behavior:

- Always registers:
  - `help`
  - `wifi` command (`cmd_wifi`)
- If `cfg->register_extra != NULL`, calls it during command registration.

### Command behavior (pseudocode)

```text
wifi_console_start(cfg):
  repl = esp_console_new_repl_<backend>()
  esp_console_register_help_command()
  register_commands():
    register wifi command
    if cfg.register_extra: cfg.register_extra(cfg.ctx)
  esp_console_start_repl(repl)
```

## Design Decisions

1) **No application dependencies in the component.**
   - The component may call a function pointer hook, but must not include `js_console.h` or `led_console.h`.

2) **Config-driven prompt, not hard-coded.**
   - Prompt is a UX detail per firmware; it belongs in the config.

3) **Keep the wifi command syntax stable.**
   - People build muscle memory; changing it creates avoidable friction.

## Alternatives Considered

1) Keep separate wifi_console.c per firmware.
   - Rejected: repeated rework and drift.

2) Build a “mega console” component that registers all possible commands.
   - Rejected: would create circular dependencies and make reuse harder.

## Implementation Plan

### 1) Implement component

Derive `cmd_wifi` and parsing helpers from:

- `esp32-s3-m5/0048-cardputer-js-web/main/wifi_console.c`

Key symbols to preserve:

- `cmd_wifi(int argc, char** argv)`
- `wifi_console_start(...)` (new signature; old per-firmware `wifi_console_start()` becomes wrapper if needed)

### 2) Migrate 0046

Change:

- `esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/wifi_console.c`
- `esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/wifi_console.h`

to either:

- remove them and call component directly from app_main, or
- keep a thin local wrapper that sets:
  - `prompt="c6> "`
  - `register_extra=led_console_register_commands`

Update build wiring:

- `esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/CMakeLists.txt` depends on `wifi_console` (and `wifi_mgr`).

### 3) Migrate 0048

Same approach; wrapper config:

- `prompt="0048> "`
- `register_extra=js_console_register_commands`

Update build wiring:

- `esp32-s3-m5/0048-cardputer-js-web/main/CMakeLists.txt` depends on `wifi_console`.

## Open Questions

1) Should the component own “console backend selection” or should that be provided by the firmware?
   - For now: keep current pattern (USB Serial/JTAG → USB CDC → UART), because it is already validated.

## References

- Prior art implementations:
  - `esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/wifi_console.c` (extra commands: `led_console_register_commands`)
  - `esp32-s3-m5/0048-cardputer-js-web/main/wifi_console.c` (extra commands: `js_console_register_commands`)
- Wi‑Fi backend:
  - `esp32-s3-m5/0048-cardputer-js-web/main/wifi_mgr.h` (consumer API)
