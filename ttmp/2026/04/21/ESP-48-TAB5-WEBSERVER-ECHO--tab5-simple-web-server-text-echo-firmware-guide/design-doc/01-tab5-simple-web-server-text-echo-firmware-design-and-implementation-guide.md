---
Title: Tab5 simple web server text echo firmware design and implementation guide
Ticket: ESP-48-TAB5-WEBSERVER-ECHO
Status: active
Topics:
    - firmware
    - http
    - wifi
    - webserver
    - ux
    - esp-idf
    - m5stack
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: M5Tab5-UserDemo/README.md
      Note: Confirms the official Tab5 build path and ESP-IDF v5.4.2 reference
    - Path: esp32-s3-m5/0017-atoms3r-web-ui/main/CMakeLists.txt
      Note: Shows the dependency stack and asset embedding pattern for a device-hosted web UI
    - Path: esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp
      Note: Shows embedded assets
    - Path: esp32-s3-m5/0017-atoms3r-web-ui/main/wifi_app.cpp
      Note: Shows NVS
    - Path: esp32-s3-m5/0021-atoms3-memo-website/main/http_server.cpp
      Note: Shows a smaller HTTP-only bootstrap and status endpoint shape
    - Path: esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c
      Note: Shows the compact route-table pattern and server config tuning
    - Path: esp32-s3-m5/0050-tab5-web-text-echo/CMakeLists.txt
      Note: Root project wrapper for the Tab5 web echo firmware scaffold
    - Path: esp32-s3-m5/0050-tab5-web-text-echo/build.sh
      Note: Convenience build entrypoint used for clean builds, flash, and monitor sessions
    - Path: esp32-s3-m5/0050-tab5-web-text-echo/sdkconfig.defaults
      Note: Target and Wi-Fi remote defaults needed for the Tab5/P4 build
    - Path: esp32-s3-m5/0050-tab5-web-text-echo/main/CMakeLists.txt
      Note: Declares the app component dependencies and embedded web assets
    - Path: esp32-s3-m5/0050-tab5-web-text-echo/main/app_main.c
      Note: Boots Wi-Fi, HTTP, and the shared echo state subsystem
    - Path: esp32-s3-m5/0050-tab5-web-text-echo/main/wifi_app.c
      Note: Implements the Tab5 Wi-Fi/ESP-Hosted bring-up path
    - Path: esp32-s3-m5/0050-tab5-web-text-echo/main/http_server.c
      Note: Serves the page and API routes for the echo UI
    - Path: esp32-s3-m5/0050-tab5-web-text-echo/main/echo_state.c
      Note: Holds the in-RAM text state shared by HTTP handlers
    - Path: esp32-s3-m5/0050-tab5-web-text-echo/main/assets/index.html
      Note: The embedded browser UI shell
    - Path: esp32-s3-m5/0050-tab5-web-text-echo/main/assets/app.js
      Note: Browser-side submit/render logic for the echo demo
ExternalSources:
    - https://docs.m5stack.com/en/core/Tab5
    - https://docs.m5stack.com/en/esp_idf/m5tab5/userdemo
    - https://github.com/m5stack/M5Tab5-UserDemo
Summary: A minimal browser-hosted text echo server for Tab5, written as an intern-friendly ESP-IDF design and implementation guide.
LastUpdated: 2026-04-21T18:35:00Z
WhatFor: Use this design when you want a Tab5 firmware that serves a web page, accepts text from the browser, and echoes it back with the smallest reasonable HTTP-only architecture.
WhenToUse: Use before implementing the Tab5 text echo firmware or when reviewing the architecture with a new engineer.
---


# Tab5 simple web server text echo firmware design and implementation guide

## Executive Summary

This ticket defines a very small Tab5-hosted web application whose only job is to accept text that the user types in a browser and show that text back in the web UI. The firmware should be simple enough for a new intern to understand without first learning the whole product family. That means the first version should be built from a short list of concepts: Wi-Fi bring-up, `esp_http_server`, one shared in-memory string, and a tiny HTML/JavaScript frontend.

The core design choice is to keep the device state and the browser state separate. The browser may update its own screen immediately when the user types or presses submit, but the Tab5 firmware still needs to own the canonical value so the page can reload, the server can answer repeated requests consistently, and the code can be extended later without rewriting the protocol. The result is a minimal demo that teaches the right embedded patterns without introducing storage, video, audio, or websocket complexity on day one.

## Problem Statement and Scope

The user wants a simple web server running on Tab5. The web UI should let a user type text and then see that same text displayed back in the page. That sounds trivial from the browser side, but on an embedded target it still requires a correct stack:

- Wi-Fi must come up reliably.
- The device must expose a web server with a predictable route table.
- The browser needs a frontend that can send text and render the current value.
- The firmware needs a safe shared state object so concurrent requests do not corrupt memory.

The scope for this ticket is intentionally narrow:

- **In scope**
  - Tab5-hosted web server.
  - One browser page with a text input and a visible echo area.
  - A small HTTP API to submit text and fetch current state.
  - A clear implementation guide for a new engineer.
  - A diary that records the investigation path and decisions.

- **Out of scope for the first version**
  - Persistent storage across reboot.
  - WebSocket push updates.
  - Audio/video display integration.
  - Sensor integration.
  - Home Assistant or cloud connectivity.
  - Any heavy frontend framework.

This narrower scope is important. The goal is not to build the most feature-rich device web app. The goal is to teach the smallest useful embedded web-server architecture on Tab5.

## Current-State Analysis

The repo family already contains several pieces that are directly relevant to this design.

### 1) The existing device-hosted UI tutorial already uses the right ESP-IDF primitives

`esp32-s3-m5/0017-atoms3r-web-ui/main/CMakeLists.txt:1-35` registers the exact family of components we care about: `esp_netif`, `esp_wifi`, `esp_http_server`, embedded assets, and websocket support. That tells us the project already treats a browser-hosted UI as a first-class ESP-IDF pattern rather than a one-off hack.

`esp32-s3-m5/0017-atoms3r-web-ui/main/hello_world_main.cpp:18-37` shows the tutorial boot order:

1. display init,
2. Wi-Fi init,
3. storage init,
4. HTTP server start,
5. background tasks for terminal/button input,
6. then a simple idle loop.

That ordering is a good model for a beginner because it keeps the entrypoint short and pushes behavior into named subsystems.

`esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp:35-114` shows how the tutorial embeds browser assets and serves them from `esp_http_server`. The file declares the embedded HTML/CSS/JS blobs, then exposes helper functions for sending JSON and static assets. `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp:167-260` continues with request parsing, directory listing, validation, and chunked request reads. Even though that tutorial is more complex than our target, it demonstrates the correct server shape: one `httpd_handle_t`, a compact route table, and defensive request handling.

`esp32-s3-m5/0017-atoms3r-web-ui/main/wifi_app.cpp:38-252` shows how to bring up Wi-Fi correctly in ESP-IDF: initialize NVS, initialize `esp_netif`, create the default event loop, register Wi-Fi/IP handlers, and then start in SoftAP, STA, or AP+STA mode based on configuration. For a text-echo demo, this is the right level of abstraction. We do not need to invent a custom Wi-Fi stack.

### 2) A smaller HTTP-only tutorial already exists and is closer to the new scope

`esp32-s3-m5/0021-atoms3-memo-website/main/main.cpp:1-21` is a useful contrast. It does almost nothing beyond `wifi_start()` and `http_server_start()`. That is the right philosophical direction for the new tutorial: the app entrypoint should read like a checklist, not like a framework.

`esp32-s3-m5/0021-atoms3-memo-website/main/http_server.cpp:1-120` shows a minimal route shape with a single root page and a small JSON status endpoint. It also shows the kind of defensive request logging and filename validation that becomes useful when a page grows beyond “hello world.” The new Tab5 echo server can stay smaller than this tutorial, but the pattern is worth copying.

### 3) The repository already demonstrates a pure route-table `esp_http_server` pattern

`esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c:761-808` shows a concise `esp_http_server` startup routine: create the server from `HTTPD_DEFAULT_CONFIG`, tune the task a bit when needed, then register a set of `httpd_uri_t` handlers. It also shows the websocket route flags and the comment explaining why control frames were disabled there. That is a good example of how to document non-obvious server behavior directly in code.

### 4) The official Tab5 demo confirms the hardware target and build environment

`M5Tab5-UserDemo/README.md` says the Tab5 demo builds from `platforms/tab5` and uses ESP-IDF v5.4.2. `M5Tab5-UserDemo/platforms/tab5/CMakeLists.txt` is equally important because it confirms that Tab5-specific code lives in a dedicated target tree, not in a generic desktop build.

`M5Tab5-UserDemo/platforms/tab5/main/app_main.cpp` shows that the official Tab5 firmware uses its own application/hardware abstraction injection path. We do **not** need that complexity for a simple text echo server, but it is a useful reminder that Tab5 is not just “another ESP32-S3 tutorial board.” It is its own hardware target with its own build assumptions.

## Gap Analysis

After reviewing the existing tutorials and the official Tab5 demo, the missing piece is clear: there is no ultra-minimal guide that teaches how to build a **browser text echo** service on Tab5 without pulling in extra subsystems.

The gaps are:

- No tutorial that focuses on a single text buffer owned by the firmware.
- No guide that explains how a browser form submits to `esp_http_server` and then re-renders state from the same device-owned buffer.
- No Tab5-specific explanation of how to keep the old tutorial layout from `esp32-s3-m5` while still targeting ESP32-P4 hardware.
- No “new engineer” walkthrough that explains the moving parts in prose instead of assuming familiarity with ESP-IDF internals.

In other words: the repo already teaches how to serve a page, how to bring up Wi-Fi, and how to build a more elaborate data-driven UI. What it does **not** yet teach is the smallest possible architecture that connects those concepts into a simple text echo demo.

## Proposed Solution

### Architectural summary

The recommended design is a **SoftAP-first, HTTP-only, RAM-backed echo server**.

At runtime, the firmware will consist of four small subsystems:

1. **Wi-Fi bring-up**
   - Create the default netif and event loop.
   - Start a SoftAP so a phone or laptop can connect directly to Tab5.
   - Log the browse URL (`http://192.168.4.1/`) so the user knows where to point the browser.

2. **Shared text state**
   - Keep one canonical text buffer in RAM.
   - Protect it with a mutex.
   - Track a monotonically increasing version counter so the browser can tell when the value changed.

3. **HTTP server**
   - Serve the browser page at `GET /`.
   - Accept text updates at `POST /api/text`.
   - Expose the current canonical state at `GET /api/state`.
   - Optionally expose `GET /api/health` for smoke testing.

4. **Browser frontend**
   - Render one text input, one submit button, one clear button, and one echo panel.
   - Call the POST endpoint when the user submits.
   - Fetch the current state on page load and after each successful update.

This architecture is intentionally boring. That is a feature. A new intern can understand it in one pass, and the code can grow later if a websocket or local-display mirror is requested.

### Suggested file layout

If the implementation lands in the `esp32-s3-m5` tutorial-style repo, a good layout would be:

```text
0042-tab5-web-text-echo/
├── CMakeLists.txt
├── build.sh
├── partitions.csv
├── sdkconfig.defaults
├── main/
│   ├── app_main.cpp
│   ├── wifi_app.cpp
│   ├── wifi_app.h
│   ├── echo_state.cpp
│   ├── echo_state.h
│   ├── http_server.cpp
│   └── http_server.h
└── web/
    ├── index.html
    └── app.js
```

That split keeps responsibilities obvious:

- `app_main.cpp` = orchestration only.
- `wifi_app.cpp` = network setup only.
- `echo_state.cpp` = shared data only.
- `http_server.cpp` = route registration and request parsing only.
- `web/` = browser assets only.

### Why this split is worth it

You could put the whole demo in one C++ file, and for a toy example that would be defensible. But for an intern-facing guide, a small number of well-named files is easier to reason about than a single dense translation unit. The separation also mirrors the pattern already used in `0017`, `0021`, and `0029`.

## Design Decisions

### 1) SoftAP-first instead of STA-first

**Decision:** start the Tab5 as a SoftAP by default.

**Why:**

- The user can connect immediately without needing to know a Wi-Fi password for an existing network.
- It makes the demo repeatable in a lab or during onboarding.
- It avoids a second round of support questions about network credentials.

**Tradeoff:** the device creates its own network instead of joining the user’s home network. That is fine for the first version because the goal is a tutorial, not a production deployment.

### 2) HTTP-only instead of WebSocket-first

**Decision:** use `GET /api/state` and `POST /api/text` instead of a websocket transport.

**Why:**

- The browser flow is easier to explain.
- The firmware avoids stateful socket management.
- The intern can debug it with `curl` before touching browser JavaScript.

**Tradeoff:** if you want live key-by-key updates or multi-client broadcast, websockets become attractive later. For the first version, HTTP is enough and easier to validate.

### 3) Raw text body instead of JSON POST bodies

**Decision:** accept `text/plain` request bodies containing the user’s text.

**Why:**

- The server does not need a JSON parser.
- The handler can simply read request bytes and copy them into the shared buffer.
- The browser can still use `fetch()` with a simple `Content-Type: text/plain` header.

**Tradeoff:** the response still needs JSON escaping, because the browser wants a machine-readable state object. That is a small helper function and a good teaching moment.

### 4) RAM-backed state instead of persistent storage

**Decision:** keep the latest text in memory only.

**Why:**

- It keeps the first version small and easy to explain.
- It avoids NVS/FATFS setup.
- It lets the engineer focus on HTTP semantics first.

**Tradeoff:** rebooting the board clears the text. That is acceptable for the first version. Persistence can be added later if the product requirement changes.

### 5) Embedded static assets instead of a frontend build system

**Decision:** serve a tiny `index.html` and `app.js` from embedded text files.

**Why:**

- The browser app is tiny.
- The intern can understand the full request/response cycle without learning a frontend toolchain.
- The existing repo already demonstrates embedded asset delivery in `0017`.

**Tradeoff:** a larger frontend would eventually benefit from a build pipeline, but that would be unnecessary overhead for this ticket.

## Proposed HTTP API

A good contract for the first version is:

- `GET /` — serve the browser page.
- `GET /api/health` — return `{"ok":true}`.
- `GET /api/state` — return the canonical state.
- `POST /api/text` — replace the canonical state with the request body.
- `POST /api/clear` — optional convenience endpoint if you want an explicit clear action.

A concrete response shape could be:

```json
{
  "ok": true,
  "version": 3,
  "text": "hello tab5"
}
```

The `version` field is useful because it gives the browser a cheap way to know whether the current display is stale.

### Browser behavior

The browser page should behave like this:

- On load, call `GET /api/state` and render the current value.
- On submit, send the text in a `POST /api/text` request.
- On success, update the output panel from the response JSON.
- On clear, post an empty string or call the clear endpoint.
- If the device is unreachable, show a visible error instead of silently failing.

This is enough to make the UI feel responsive without adding websockets.

## Pseudocode and Key Flows

### System diagram

```text
+-------------------+         Wi-Fi SoftAP          +----------------------+
| Browser / laptop  |  ---------------------------> |  Tab5 ESP32-P4       |
| or phone          |  <--------------------------- |  esp_http_server     |
+-------------------+        HTTP GET/POST          |  echo_state (RAM)    |
                                                    |  esp_netif + esp_wifi |
                                                    +----------------------+
```

### Boot sequence pseudocode

```c
app_main():
    init_nvs()
    init_netif_and_event_loop()
    start_wifi_softap()
    echo_state_init()
    http_server_start()
    while true:
        sleep(1 second)
```

This is the only `app_main` logic the intern needs to understand. All real behavior should live in named helper modules.

### Shared state pseudocode

```c
struct EchoState {
    char text[MAX_TEXT_BYTES];
    uint32_t version;
    SemaphoreHandle_t mutex;
};

void echo_state_set(const char *input):
    lock(mutex)
    copy input into text, truncating safely
    version += 1
    unlock(mutex)

void echo_state_copy(char *out, size_t out_size, uint32_t *version_out):
    lock(mutex)
    copy text and version
    unlock(mutex)
```

A mutex is enough because the state is tiny and the access patterns are simple.

### POST handler pseudocode

```c
POST /api/text:
    body = read_all_request_bytes(req, MAX_TEXT_BYTES)
    if body too large:
        return 413

    echo_state_set(body)
    snapshot = echo_state_copy()
    return JSON(snapshot)
```

Important details for the intern:

- Read the body in a loop with `httpd_req_recv`.
- Reject payloads that exceed the maximum buffer.
- Always terminate strings with `\0` before treating them as text.
- Escape the text when writing JSON.

### GET handler pseudocode

```c
GET /api/state:
    snapshot = echo_state_copy()
    return JSON(snapshot)
```

The GET path should be as simple as possible. It should not mutate state.

### Browser-side pseudocode

```js
async function loadState() {
  const res = await fetch('/api/state');
  const json = await res.json();
  render(json.text, json.version);
}

async function submitText(value) {
  const res = await fetch('/api/text', {
    method: 'POST',
    headers: { 'Content-Type': 'text/plain; charset=utf-8' },
    body: value,
  });
  const json = await res.json();
  render(json.text, json.version);
}
```

This keeps the browser code easy to read and easy to test in DevTools.

## Implementation Plan

### Phase 1 — Scaffold the tutorial

Create a new tutorial folder in the `esp32-s3-m5` style, for example `0042-tab5-web-text-echo/`.

Add the usual files:

- `CMakeLists.txt`
- `build.sh`
- `sdkconfig.defaults`
- `partitions.csv`
- `main/`
- `web/`

Keep the `build.sh` pattern consistent with the existing tutorials so the engineer can run one command without manually exporting ESP-IDF each time.

### Phase 2 — Make the board target explicit

Because Tab5 is an ESP32-P4 device, the scaffold should clearly state that the project target is **not** ESP32-S3 even though the repo family is called `esp32-s3-m5`.

In practice that means:

- set the project target to `esp32p4`,
- keep the target-specific config in `sdkconfig.defaults`,
- keep the guide explicit about why the repo naming and the hardware target are different.

This is a teaching point, not just a build detail.

### Phase 3 — Implement Wi-Fi bring-up

Use the same conceptual flow shown in `esp32-s3-m5/0017-atoms3r-web-ui/main/wifi_app.cpp:38-252`:

- initialize NVS,
- initialize `esp_netif`,
- create the default event loop,
- initialize Wi-Fi,
- create the default Wi-Fi AP netif,
- start SoftAP,
- log the browse URL.

If you later want STA support, that should be a separate configuration choice. Do not mix STA fallback logic into the first tutorial unless the use case requires it.

### Phase 4 — Add the shared state module

Create a tiny `echo_state` module with the following responsibilities:

- allocate and initialize the mutex,
- store the latest text buffer,
- increment the version counter,
- provide a copy-out helper for HTTP handlers.

Keep validation here minimal:

- enforce a maximum length,
- normalize the text to a safe printable form if needed,
- do not silently overflow.

### Phase 5 — Implement the HTTP server

Use `esp_http_server` and register a small route table, following the pattern demonstrated in `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c:761-808`.

Recommended routes:

- `GET /` → serve the HTML page,
- `GET /api/state` → emit the canonical JSON snapshot,
- `POST /api/text` → read the request body and update the shared buffer,
- `GET /api/health` → smoke-test endpoint.

Handle errors explicitly:

- `400 Bad Request` for invalid input,
- `413 Payload Too Large` for overlong text,
- `500 Internal Server Error` for storage or internal problems.

### Phase 6 — Add the frontend

The frontend should be as small as possible while still being readable:

- one text input or textarea,
- one submit button,
- one clear button,
- one preformatted output area,
- one error message area.

If you want to keep it extremely simple, embed the JavaScript directly in `index.html`. If you want better separation, keep `index.html` and `app.js` as separate embedded text files.

### Phase 7 — Validate on real hardware

After the code is written:

1. Build it.
2. Flash Tab5.
3. Connect to the SoftAP.
4. Open the page in a browser.
5. Type text and submit.
6. Reload the page and confirm the state is still visible.

That sequence proves the full end-to-end path, not just the HTTP handler in isolation.

## Test Strategy

The test strategy should be practical and visible to a newcomer.

### Build validation

- `idf.py build`
- Check that the binary links successfully for the Tab5 target.
- Check that the asset embedding step succeeds if you use `EMBED_TXTFILES`.

### Manual smoke test

- Flash the device.
- Confirm the log prints the SoftAP SSID and browse address.
- Join the AP from a laptop or phone.
- Open `http://192.168.4.1/`.
- Submit a short string like `hello tab5`.
- Confirm the browser UI displays the same string.

### Boundary tests

- Try empty text.
- Try a string at the maximum allowed length.
- Try an overlong string and confirm the server rejects it cleanly.
- Try characters that must be escaped in JSON: quotes, backslashes, newlines.
- Refresh the browser and confirm the canonical state is still there.

### Negative tests

- Disconnect Wi-Fi and reload the page.
- Reboot the board and confirm the state resets by design.
- Open the page from a second browser client and confirm the last write wins consistently.

## Risks, Alternatives, and Open Questions

### Risk: confusing the repo name with the hardware target

The repo family is called `esp32-s3-m5`, but Tab5 is an ESP32-P4 device. The guide should say this plainly so a new engineer does not copy the wrong target assumptions into the new tutorial.

### Risk: overengineering the frontend

It is easy to turn a tiny text echo demo into a frontend project. Resist that temptation. A single static page and a little vanilla JavaScript are enough.

### Risk: state escaping bugs

If the server returns the text in JSON, the implementation must escape quotes, backslashes, and newlines. This is a common source of “it works for ASCII but breaks for punctuation” bugs.

### Risk: buffer overflow

The request body must be bounded. Never trust the browser to stay within limits.

### Alternative: websocket live sync

A websocket could make the UI feel more dynamic, especially if the requirement becomes “echo as the user types.” But it adds more moving parts:

- connection state,
- broadcast logic,
- reconnection behavior,
- more complex browser code.

That is unnecessary for the first version.

### Alternative: persistent storage

Writing the text to NVS or FATFS would make the state survive reboot. That may be useful later, but it is not needed for the tutorial. Keep the first version RAM-only so the intern learns the network/server flow first.

### Open questions

- Should the first implementation support only SoftAP, or SoftAP + STA?
- Should the browser submit on Enter only, or also on debounced input changes?
- Should a clear button be explicit, or is empty text enough?
- Should the page also mirror to the Tab5 display later, or stay browser-only?

These questions do not block the first version, but they are worth deciding before the code is written.

## References

### Primary code references

- `esp32-s3-m5/0017-atoms3r-web-ui/main/CMakeLists.txt`
- `esp32-s3-m5/0017-atoms3r-web-ui/main/hello_world_main.cpp`
- `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp`
- `esp32-s3-m5/0017-atoms3r-web-ui/main/wifi_app.cpp`
- `esp32-s3-m5/0021-atoms3-memo-website/main/main.cpp`
- `esp32-s3-m5/0021-atoms3-memo-website/main/http_server.cpp`
- `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c`
- `M5Tab5-UserDemo/README.md`
- `M5Tab5-UserDemo/platforms/tab5/CMakeLists.txt`
- `M5Tab5-UserDemo/platforms/tab5/main/app_main.cpp`

### External references

- Tab5 product page: https://docs.m5stack.com/en/core/Tab5
- Tab5 factory firmware build guide: https://docs.m5stack.com/en/esp_idf/m5tab5/userdemo
- Tab5 factory firmware source: https://github.com/m5stack/M5Tab5-UserDemo

### API references to remember

- `esp_netif_init()`
- `esp_event_loop_create_default()`
- `esp_wifi_init()`
- `esp_wifi_set_mode()`
- `esp_wifi_start()`
- `esp_netif_create_default_wifi_ap()`
- `httpd_start()`
- `httpd_register_uri_handler()`
- `httpd_req_recv()`
- `httpd_resp_send()`
- `httpd_resp_sendstr()`
- `httpd_resp_set_type()`
- `nvs_flash_init()`
- `xSemaphoreCreateMutex()` / `xSemaphoreTake()` / `xSemaphoreGive()`

### Where an intern should start reading

1. `esp32-s3-m5/0017-atoms3r-web-ui/main/hello_world_main.cpp`
2. `esp32-s3-m5/0017-atoms3r-web-ui/main/wifi_app.cpp`
3. `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp`
4. `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c`
5. `M5Tab5-UserDemo/README.md`

Read those in that order and the overall architecture will make sense before any code is written.
