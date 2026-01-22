---
Title: Component Sizing + Dependency Rules
Ticket: 0062-FIRMWARE-COMPONENT-SIZING
Status: active
Topics:
    - esp-idf
    - architecture
    - documentation
    - firmware
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0048-cardputer-js-web/main/chain_encoder_uart.h
      Note: Current encoder driver to be absorbed into encoder_service
    - Path: esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp
      Note: Current per-firmware web server to be replaced by webui_server
    - Path: esp32-s3-m5/0048-cardputer-js-web/main/js_console.cpp
      Note: Current per-firmware console integration to move into mqjs_integrations
    - Path: esp32-s3-m5/components/mqjs_service/include/mqjs_service.h
      Note: Existing extracted concurrency boundary
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T19:57:13.234144562-05:00
WhatFor: ""
WhenToUse: ""
---


# Component Sizing + Dependency Rules

## Executive Summary

ESP-IDF “componentization” is not free: every component is a dependency boundary with CMake/Kconfig/API surface area and long-term maintenance cost. This design proposes a strategy that enables reuse without creating a micro-component explosion.

We adopt a “few public components, many private modules” approach:

- Keep a small set of **public, reusable components** with stable APIs (≈ 3–6).
- Allow each component to contain multiple `.c/.cpp` files as **internal modules**.
- Extract a new component only at a **stable responsibility boundary** (hardware ownership, concurrency ownership, or multi-firmware reuse).

For the 0048/JS/WebUI stack, this means the next extractions should be limited to:

1) `encoder_service` (hardware + polling/coalescing + event model; no Web/JS coupling)
2) `webui_server` (ESP HTTP server + embedded assets + WS hub; no JS)
3) `mqjs_integrations` (bind `mqjs_service` to console + HTTP routes; keep glue centralized)

## Problem Statement

We want to reuse proven patterns across ESP-IDF firmwares (0046/0048 and future projects):

- WiFi + console + web server + embedded SPA assets
- WebSocket broadcasting
- Encoder reading and normalized events
- MicroQuickJS evaluation + bindings + events

The risk is “micro-component explosion”: splitting into many tiny components that each have small, unstable APIs. In practice that creates:

- frequent cross-component edits and churny APIs
- harder dependency graphs (including circular-dependency temptation)
- slower builds and more CMake/Kconfig bookkeeping

We need crisp rules for:

1) what becomes a component vs an internal module,
2) allowed dependency direction to avoid cycles,
3) how to stage extraction while keeping existing firmwares working.

## Proposed Solution

### Definitions

- **Component**: an ESP-IDF component with its own `CMakeLists.txt` (optionally `Kconfig`), intended for reuse.
- **Module**: a `.c/.cpp` file (or folder) within a component; not consumable independently.
- **Integration**: glue that wires reusable components together for a product (e.g. encoder → JS callbacks).

### Core strategy: “fat components, thin APIs”

1) Prefer fewer components with clear responsibility boundaries.
2) Put complexity into internal modules inside a component; do not turn every helper into a component.
3) Export a small API that captures stable invariants (ownership, threading model, contracts).

### Proposed component set (for 0048-class firmwares)

#### Existing base components (already extracted)

- `wifi_mgr` — network bring-up/provisioning
- `wifi_console` — console framework (USB Serial/JTAG oriented)
- `httpd_assets_embed` — embedded static assets serving helpers
- `httpd_ws_hub` — WS client tracking + broadcast primitive
- `mqjs_vm` — VM host invariants (stable `ctx->opaque`, printing, deadlines)
- `mqjs_service` — VM owner task + queue (eval + jobs), **the concurrency boundary**

These should remain “library-like”: reusable and free of firmware-specific behavior.

#### Next “feature” components (keep the count small)

**A) `encoder_service` (feature component)**

Purpose:
- Own the encoder poll/coalesce loop and publish normalized events (`delta`, `click`) to registered sinks.

Hard rule:
- Must not depend on web server/WebSocket or JS components.

Internal modules (inside the same component):
- `driver_chain_uart.*` (port of `ChainEncoderUart`)
- `service_loop.*` (polling/coalescing)
- `sink_list.*` (fan-out)

**B) `webui_server` (feature component)**

Purpose:
- Own the ESP web server, embedded assets, and WS hub integration.

Hard rule:
- Must not depend on JS.

Builds on:
- `httpd_assets_embed`, `httpd_ws_hub`

Provides:
- “start server”
- “register routes” (or expose `httpd_handle_t`)
- “broadcast WS text”

**C) `mqjs_integrations` (integration component)**

Purpose:
- Standardize “MQJS as a product feature”:
  - an `esp_console` command (`js "<code>"`)
  - an HTTP eval route (`POST /api/js/eval`)
  - (future) an event stream endpoint

Hard rule:
- Depends on `mqjs_service` and either `wifi_console` or `webui_server`, but should avoid app-specific drivers.

### Dependency rules (avoid cycles)

Think of dependency direction as “downwards” only:

```
base libs (wifi_mgr, wifi_console, httpd_assets_embed, httpd_ws_hub, mqjs_vm)
        ↓
feature libs (mqjs_service, webui_server, encoder_service)
        ↓
integration libs (mqjs_integrations, encoder↔webui glue if reused)
        ↓
firmware apps (0048, 0046, ...)
```

Rules:
- Base components must not depend on feature/integration/app.
- Feature components may depend on base components, but not on app or on each other unless unavoidable.
- Integration components may depend on base + feature, but must not introduce cycles.
- If something needs both encoder + webui, that is integration, not a new low-level component.

### Concrete API sketches (pseudocode)

**encoder_service**

```c
typedef enum { ENC_EV_DELTA, ENC_EV_CLICK } encoder_event_type_t;

typedef struct {
  encoder_event_type_t type;
  int32_t pos;
  int32_t delta;
  uint32_t ts_ms;
  int click_kind; // 0 single, 1 double, 2 long (valid if type==ENC_EV_CLICK)
} encoder_event_t;

typedef void (*encoder_sink_fn_t)(const encoder_event_t* ev, void* user);

typedef struct {
  bool (*init)(void* impl);
  int32_t (*take_delta)(void* impl);
  int (*take_click_kind)(void* impl); // -1 => none
} encoder_driver_vtbl_t;

esp_err_t encoder_service_start(const encoder_driver_vtbl_t* vtbl,
                               void* driver_impl,
                               const encoder_service_config_t* cfg,
                               encoder_service_t** out);
esp_err_t encoder_service_add_sink(encoder_service_t* s, encoder_sink_fn_t fn, void* user);
```

**webui_server**

```c
esp_err_t webui_server_start(const webui_server_config_t* cfg, webui_server_t** out);
httpd_handle_t webui_server_httpd(webui_server_t* s);
esp_err_t webui_server_ws_broadcast_text(webui_server_t* s, const char* text);
```

**mqjs_integrations**

```c
esp_err_t mqjs_web_register_routes(httpd_handle_t httpd, mqjs_service_t* js);
void mqjs_console_register_commands(mqjs_service_t* js);
```

The important property is that each API remains small and stable. JSON formatting and per-firmware schemas live in modules, not as separate components.

## Design Decisions

1) Avoid micro-components
   - Component boundaries are expensive in ESP-IDF; prefer internal modules.

2) Extract only at stable ownership boundaries
   - Concurrency ownership (VM owner task) → `mqjs_service`
   - Hardware poll/driver ownership → `encoder_service`
   - Web server ownership → `webui_server`

3) Keep integrations explicit
   - Encoder → JS, Encoder → WS are integrations; allow them to evolve without contaminating base components.

4) Prefer “expose handle” for HTTP route registration
   - `httpd_handle_t` is already a stable contract; a thin wrapper is enough.

## Alternatives Considered

1) Many micro-components (one per file)
   - Rejected: API churn, slow builds, hard-to-reason dependency graph.

2) One monolithic “firmware framework” component
   - Rejected: hard to reuse selectively; tends to accrete unrelated responsibilities.

3) Keep everything inside each firmware (no extraction)
   - Rejected: bugs/fixes repeat across firmwares and code diverges quickly.

## Implementation Plan

1) Update `0056-ENCODER-SERVICE-COMPONENT` tasks so `encoder_service` does not depend directly on 0048 web/JS symbols.
2) Create two additional tickets (not five):
   - `WEBUI-SERVER-COMPONENT`
   - `MQJS-INTEGRATIONS-COMPONENT`
3) Migrate 0048 incrementally:
   - replace encoder polling with `encoder_service` + sinks
   - replace per-firmware `http_server.*` with `webui_server`
   - move `js_console` + `js_runner` into `mqjs_integrations`
4) Migrate 0046 and other firmwares once APIs stabilize.

## Open Questions

1) Do we want a generic event bus component, or is a sink callback list enough?
2) Should `mqjs_integrations` also provide a standardized “JS events → WS frames” bridge?
3) Do we standardize JSON schemas for encoder telemetry across firmwares?

## References

- Ticket `0054-MQJS-VM-COMPONENT`
- Ticket `0055-MQJS-SERVICE-COMPONENT`
- Ticket `0056-ENCODER-SERVICE-COMPONENT`
- Ticket `0061-MQJS-SERVICE-BOOT-WDT`
