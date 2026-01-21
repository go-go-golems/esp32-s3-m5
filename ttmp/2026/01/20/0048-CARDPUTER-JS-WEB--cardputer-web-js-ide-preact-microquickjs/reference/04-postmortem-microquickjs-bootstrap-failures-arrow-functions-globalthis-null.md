---
Title: 'Postmortem: MicroQuickJS bootstrap failures (arrow functions + globalThis=null)'
Ticket: 0048-CARDPUTER-JS-WEB
Status: active
Topics:
    - cardputer
    - esp-idf
    - webserver
    - http
    - rest
    - websocket
    - preact
    - zustand
    - javascript
    - quickjs
    - microquickjs
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0048-cardputer-js-web/main/js_service.cpp
      Note: |-
        Bootstraps for encoder/emit; globalThis fix; flush implementation
        Bootstraps and globalThis repair
    - Path: 0048-cardputer-js-web/web/src/ui/app.tsx
      Note: |-
        UI help text that previously suggested arrow functions
        Help examples must match VM syntax
    - Path: imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h
      Note: Interpreter API and limitations; JS_Eval / interrupt handler / call convention
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib.h
      Note: |-
        Generated stdlib table showing `globalThis` mapped to JS_NULL
        globalThis is JS_NULL in generated table
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T14:55:00-05:00
WhatFor: Avoid repeating a class of MicroQuickJS bootstrap failures caused by unsupported syntax and misleading global bindings; provide a checklist and guardrails.
WhenToUse: Use when you add or modify JS bootstraps/bindings (encoder/emit/etc) or when you see ReferenceError/TypeError during VM init or flush.
---


# Postmortem: MicroQuickJS bootstrap failures (arrow functions + globalThis=null)

## Goal

Capture a specific, recurring failure mode in our MicroQuickJS embedding and codify the fixes and preventive checks so we don’t lose another day to “why is `encoder` undefined?”.

The failure mode is deceptively simple:

- The firmware intends to install JS globals (`encoder`, `emit`, `__0048_take_lines`) during VM initialization.
- Bootstraps fail (often only visible in logs).
- Later, user code sees `ReferenceError` and the system appears “broken” in a confusing way.

## Context

We implement Phase 2B and 2C as **bootstraps**: small JS scripts evaluated during VM init to define an API surface:

- Phase 2B: `encoder.on/off(...)` registers callbacks stored in `globalThis.__0048.encoder`.
- Phase 2C: `emit(topic, payload)` records events; a flush helper (`__0048_take_lines`) returns newline-delimited JSON frames (`js_events` + optional `js_events_dropped`).

The firmware then:

- invokes callbacks on hardware events, and
- flushes emitted events over WebSocket.

If the bootstraps fail, the **APIs do not exist**.

## Quick Reference

### Incident signature (what it looks like)

Browser / REST eval symptoms:

- `ReferenceError: variable 'encoder' is not defined`
- `ReferenceError: variable 'emit' is not defined`

Device log symptoms:

- `bootstrap 2B failed: SyntaxError: invalid lvalue` (often arrow functions)
- `bootstrap 2C failed: SyntaxError: invalid lvalue`
- `bootstrap ... failed: TypeError: cannot read property '__0048' of null` (globalThis is null)
- `flush js events failed: ReferenceError: variable '__0048_take_lines' is not defined`

### Root causes (two independent ones)

1) **Arrow functions are unsupported** in this MicroQuickJS build.
   - `()=>{}` can throw `SyntaxError: invalid lvalue`.
2) **`globalThis` is `null`** in the generated stdlib table.
   - Any bootstrap that starts with `var g = globalThis;` will explode.

### Fix checklist (copy/paste)

1) Write bootstraps in ES5 syntax:

```js
// do
encoder.on = function(t, fn) { ... }

// don't
encoder.on = (t, fn) => { ... }
```

2) Force `globalThis` to be the real global object at VM init (C side):

```c
JSValue glob = JS_GetGlobalObject(ctx);
JS_SetPropertyStr(ctx, glob, "globalThis", glob);
```

3) Add a bootstrap health check after init:

```js
typeof encoder + " " + typeof emit + " " + typeof __0048_take_lines
```

Expected:

```text
"object function function"
```

## Usage Examples

### Minimal “am I bootstrapped?” check from the browser

Run via `/api/js/eval`:

```js
typeof encoder + " " + typeof emit
```

Expected:

```
"object function"
```

### Minimal emit smoke test

```js
emit("hello", {x:1})
"ok"
```

Then check the WS event history panel for a `type:"js_events"` frame.

## Related

- Root cause evidence (stdlib table):
  - `imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib.h`
- Implementation touchpoint:
  - `0048-cardputer-js-web/main/js_service.cpp` (`ensure_ctx`, `install_bootstrap_phase2b`, `install_bootstrap_phase2c`)

## Usage Examples

<!-- Show how to use this reference in practice -->

## Related

<!-- Link to related documents or resources -->
