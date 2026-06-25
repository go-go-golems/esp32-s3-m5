---
Title: Analysis Design and Implementation Guide
Ticket: ATOMS3R-M12-QUICKJS-HTTP
Status: active
Topics:
    - atoms3r
    - esp32s3
    - quickjs
    - javascript
    - firmware
    - http
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../code/wesen/go-go-golems/go-go-goja/modules/express/express.go
      Note: Express-like host-owned route registration inspiration
    - Path: ../../../../../../../../../../code/wesen/go-go-golems/go-go-goja/modules/express/typescript.go
      Note: Express-like API shape reference
    - Path: ../../../../../../../../../../code/wesen/go-go-golems/go-go-goja/modules/fs/http.go
      Note: Static/SPAs asset serving inspiration
    - Path: 0103-atoms3r-m12-native-quickjs/main/storage_namespace.cpp
      Note: Storage-backed static asset dependency
    - Path: components/qjs_service/include/qjs_service.h
      Note: Owner-task API for dynamic JS route handlers
ExternalSources: []
Summary: Intern-facing guide for an Express-like QuickJS HTTP server on AtomS3R M12.
LastUpdated: 2026-06-25T23:30:00-07:00
WhatFor: Use when implementing or reviewing HTTP serving, static assets, and Express-like route registration in `0103-atoms3r-m12-native-quickjs`.
WhenToUse: Read before adding `esp_http_server`, JavaScript `http`/`app` APIs, static file serving, or route dispatch through QuickJS.
---


# AtomS3R M12 QuickJS Express-like HTTP Server — Analysis, Design, and Implementation Guide

## Executive summary

This ticket adds HTTP serving to the AtomS3R M12 native QuickJS firmware. The user requested an Express-like API inspired by `~/code/wesen/go-go-golems/go-go-goja`, but without the auth machinery. The embedded version should support simple routes, JSON/text/HTML responses, and static assets served from storage or firmware flash.

The design must respect embedded constraints. ESP-IDF HTTP handlers run on the HTTP server task, not on the QuickJS owner task. QuickJS must still be accessed through `qjs_service_run()` or an equivalent owner-task request. Static file serving should not call QuickJS at all. Dynamic JavaScript route handlers must be bounded, serialized, and limited in body size.

The first useful milestone is:

- Start WiFi and obtain an IP address.
- Start `esp_http_server` on port 80.
- Serve static files from `/storage/www` or `/storage/scripts` through a virtual-rooted path.
- Expose a JavaScript registration API for simple routes:

```js
http.get('/api/hello', (req, res) => res.json({ ok: true }))
http.static('/static', '/data/www')
http.start({ port: 80 })
```

For the first implementation, avoid auth, sessions, cookies, WebSockets, streaming uploads, and long-running JavaScript request handlers.

## Inspiration from go-go-goja

The relevant host-side design in `go-go-goja` is the `modules/express` package and the `modules/fs/http.go` static helpers.

Important ideas to reuse:

- JavaScript registers routes into a host-owned HTTP router.
- Static file serving is handled by the host, not by JavaScript per request.
- SPA/static fallback can be handled by a host file server.
- The HTTP server is owned by the host process; JavaScript does not call `listen()` directly in the Goja version.

Important ideas to omit for this embedded firmware:

- Auth builders.
- Resource authorization.
- Sessions.
- CSRF.
- Auditing.
- Generic Go `http.Handler` mounting.
- Node/CommonJS module loading.

The embedded target should copy the mental model, not the full feature set.

## System architecture

```text
WiFi / AP / STA IP
        |
        v
ESP-IDF esp_http_server on port 80
        |
        +--> static route table
        |       |
        |       v
        |   FatFs files under /storage via virtual roots
        |
        +--> dynamic route table
                |
                v
            build request DTO
                |
                v
            qjs_service_run() on QuickJS owner task
                |
                v
            JS handler fills response DTO
                |
                v
            HTTP task sends response
```

The key rule is that HTTP request tasks do not call QuickJS directly. They create a small request object, enqueue a job to the QuickJS owner task, wait with a timeout, and then send a response.

## Dependencies

ESP-IDF components:

- `esp_http_server`
- `esp_netif`
- `esp_wifi` through the WiFi ticket
- `fatfs` and `wear_levelling` through the storage ticket

Existing project components:

- `components/qjs_service`
- `components/quickjs_native`
- `0103/main/storage_namespace.*`
- future `0103/main/wifi_app.*`

## HTTP API shape

The embedded JavaScript API should be global `http` or a `require`-less object. QuickJS in this firmware does not include Node-style module loading yet, so global namespaces are consistent with `system`, `storage`, and future `wifi`.

```js
http.status()
http.start({ port: 80 })
http.stop()
http.get(path, handler)
http.post(path, handler)
http.route(method, path, handler)
http.static(prefix, root, options)
http.spa(prefix, root, options)
```

### Example

```js
http.get('/api/hello', (req, res) => {
  res.json({ ok: true, path: req.path, query: req.query })
})

http.get('/api/text', (req, res) => {
  res.type('text/plain')
  res.send('hello from AtomS3R')
})

http.static('/static', '/data/www')
http.spa('/', '/data/www', { index: 'index.html', excludePrefixes: ['/api'] })
http.start({ port: 80 })
```

## Request object

Keep the request object small.

```js
{
  method: 'GET',
  path: '/api/hello',
  query: { name: 'm5' },
  params: {},
  headers: { 'user-agent': '...' },
  bodyText: '',
  bodyJson: null,
  remote: '192.168.1.10'
}
```

Initial limits:

| Field | Limit |
|---|---:|
| Request body | 4 KiB |
| Header count copied to JS | 16 |
| Header name/value length | 64/256 bytes |
| Query string | 512 bytes |
| Route pattern count | 16 dynamic routes |
| Static mounts | 8 |
| Handler timeout | 1000 ms |

Do not pass raw HTTP request handles into JavaScript.

## Response object

The response object is a JS facade over a firmware-owned response DTO. JavaScript methods mutate the DTO; after the handler returns, the HTTP task serializes it.

```js
res.status(201)
res.set('X-Thing', 'value')
res.type('application/json')
res.send('text')
res.html('<h1>Hello</h1>')
res.json({ ok: true })
res.redirect('/other')
res.end()
```

Response DTO pseudocode:

```c
typedef struct {
    int status;
    char content_type[64];
    header_t headers[8];
    uint8_t *body;
    size_t body_len;
    bool ended;
} http_response_dto_t;
```

Response limits:

| Limit | Initial value |
|---|---:|
| Body from JS | 8 KiB |
| Headers set by JS | 8 |
| Header name/value | 64/256 bytes |
| Redirect URL | 256 bytes |

Static file responses can exceed 8 KiB because they stream directly from FatFs and do not allocate a JavaScript string.

## Route table

The host owns the route table. JavaScript registers handler references, but the route metadata lives in C/C++.

```c
typedef struct {
    char method[8];
    char pattern[96];
    JSValue handler;      // duplicated and rooted in QuickJS runtime
} route_entry_t;
```

Because `JSValue` lifetimes are tied to a specific runtime, `js reset` must clear all dynamic route handlers or rebuild them from an explicit script. The safest initial policy is:

- `js reset` stops the HTTP server or clears dynamic routes.
- Static mounts may remain firmware-owned.
- A future `js run /scripts/server.js` re-registers routes.

Do not keep stale `JSValue` handlers after runtime reset.

## Static asset serving

Static serving should use the storage virtual-root model.

```js
http.static('/static', '/data/www')
```

Maps:

```text
GET /static/app.js  ->  /storage/data/www/app.js
```

Rules:

- Normalize URL paths.
- Reject `..`, backslashes, and native absolute paths.
- Guess MIME type by extension for common types:
  - `.html` => `text/html; charset=utf-8`
  - `.js` => `application/javascript`
  - `.css` => `text/css`
  - `.json` => `application/json`
  - `.png` => `image/png`
  - `.jpg` / `.jpeg` => `image/jpeg`
  - `.svg` => `image/svg+xml`
- Stream files in chunks, for example 1024 bytes at a time.
- Do not route static file bytes through QuickJS.

Optional SPA fallback:

```js
http.spa('/', '/data/www', { index: 'index.html', excludePrefixes: ['/api'] })
```

This mirrors the Goja static-asset helper: serve an existing file if it exists, otherwise serve `index.html` for GET/HEAD unless the request matches an excluded prefix.

## Dynamic route dispatch pseudocode

```c
http_handler(req):
    route = match_route(req.method, req.uri)
    if route is static:
        return serve_static(req, route)

    dto = build_request_dto(req)
    response = default_response()

    job.fn = call_js_handler
    job.user = { route.handler, dto, response }
    err = qjs_service_run(qjs, &job)

    if err == ESP_ERR_TIMEOUT:
        send 504
    else if JS handler threw:
        send 500 with redacted error
    else:
        send response
```

QuickJS job pseudocode:

```c
call_js_handler(ctx, user):
    req_obj = make_request_object(ctx, user->dto)
    res_obj = make_response_object(ctx, user->response)
    ret = JS_Call(ctx, handler, JS_UNDEFINED, 2, [req_obj, res_obj])
    if exception: copy exception string into response error
    return ESP_OK
```

## Express-like API design decisions

### Keep `listen()` host-owned

Unlike desktop Express, embedded firmware owns the HTTP server lifecycle. `http.start()` is allowed because it maps to a firmware command, but route registration should not create arbitrary listeners.

### Start with global `http`, not `require('express')`

The current QuickJS environment has explicit globals (`print`, `millis`, `gc`, `system`, `storage`). It does not yet have a CommonJS loader. A global `http` is simpler and consistent.

### No auth in the first version

The user explicitly requested no auth stuff. Keep the first server simple and local-network oriented. If auth is needed later, make it a separate ticket.

### Static serving first, dynamic routes second

Static serving is safer because it does not enter QuickJS per request. It validates WiFi, HTTP server startup, storage reads, MIME types, and chunked file sending before dynamic JavaScript handler dispatch is added.

## Implementation plan

### Phase 1: HTTP host service

1. Add `http_server.h/cpp` or `http_namespace.h/cpp` to `0103/main`.
2. Add `esp_http_server` to `main/CMakeLists.txt`.
3. Implement `http_server_start(port)` and `http_server_stop()`.
4. Add console commands:
   - `http status`
   - `http start [port]`
   - `http stop`
5. Add a built-in health route:
   - `GET /healthz` -> `ok`
6. Validate over WiFi.

### Phase 2: Static assets

1. Add static mount table.
2. Add console command:
   - `http static <prefix> <virtual-root>`
3. Add static route handler that streams from storage.
4. Validate:
   - write `/data/www/index.html` through `storage.writeText()`
   - mount `/static` to `/data/www`
   - fetch `/static/index.html`

### Phase 3: QuickJS registration API

1. Install global `http` namespace through `qjs_service_run()`.
2. Implement `http.status()`.
3. Implement `http.start()` and `http.stop()` as firmware lifecycle calls.
4. Implement `http.static()` and `http.spa()` registration.
5. Implement `http.get()` and one dynamic response path.
6. Validate with small JSON/text responses.

### Phase 4: Script boot workflow

1. Add `js run <virtual-path>`.
2. Store `/scripts/server.js`.
3. Run it manually after boot.
4. Only later consider explicit autoload, with console disable/clear commands.

## Validation checklist

Prerequisites:

- Storage mounted and persistent.
- WiFi connected, or AP mode enabled.
- USB Serial/JTAG console available.

Console:

```text
wifi status
storage status
http status
http start 80
```

Static smoke:

```text
js eval "storage.writeText('/data/www/index.html','<h1>AtomS3R</h1>')"
js eval "http.static('/static','/data/www')"
```

Client:

```bash
curl http://<device-ip>/healthz
curl http://<device-ip>/static/index.html
```

Dynamic smoke:

```text
js eval "http.get('/api/hello', (req,res)=>res.json({ok:true,path:req.path}))"
```

Client:

```bash
curl http://<device-ip>/api/hello
```

Expected result: `{"ok":true,"path":"/api/hello"}`.

## Risks and mitigations

| Risk | Why it matters | Mitigation |
|---|---|---|
| HTTP task calls QuickJS directly | Violates runtime ownership and may crash. | Always use `qjs_service_run()` for JS handlers. |
| Long JS handler blocks server | One owner task serializes QuickJS work. | 1000 ms timeout and small request bodies. |
| Large static files exhaust memory | ESP32-S3 memory is limited. | Stream static files in chunks outside QuickJS. |
| Runtime reset leaves stale handlers | `JSValue` handlers belong to old runtime. | Clear dynamic routes on `js reset`; require rerun of server script. |
| Password leakage | HTTP may expose WiFi status. | `wifi.status()` and any HTTP diagnostics must omit passwords. |
| Accidental public server | Guest network may be shared. | No auth in milestone, but keep APIs simple and avoid sensitive endpoints. |

## Decision records

### Decision: Use `esp_http_server`

- **Context:** ESP-IDF provides a native HTTP server component.
- **Decision:** Use `esp_http_server` rather than embedding a third-party HTTP stack.
- **Rationale:** It is integrated, small, and suitable for ESP32-S3.
- **Status:** proposed.

### Decision: Serve static assets outside QuickJS

- **Context:** Static files may be larger than QuickJS's comfortable heap budget.
- **Decision:** Stream static files from storage in C/C++.
- **Rationale:** Avoids allocating file bytes as JavaScript strings.
- **Status:** proposed.

### Decision: Use global `http` instead of CommonJS `express`

- **Context:** Current firmware has explicit globals and no CommonJS loader.
- **Decision:** Expose a global `http` object with Express-like method names.
- **Rationale:** Minimal implementation that keeps the API familiar.
- **Status:** proposed.

### Decision: No auth in milestone 1

- **Context:** User requested simple serving and explicitly said auth is unnecessary.
- **Decision:** Omit auth, sessions, CSRF, and resource policy.
- **Rationale:** Keeps the first embedded server small and understandable.
- **Status:** accepted.

## References

- `~/code/wesen/go-go-golems/go-go-goja/modules/express/express.go`
- `~/code/wesen/go-go-golems/go-go-goja/modules/express/typescript.go`
- `~/code/wesen/go-go-golems/go-go-goja/modules/fs/http.go`
- `0103-atoms3r-m12-native-quickjs/main/storage_namespace.cpp`
- `0103-atoms3r-m12-native-quickjs/main/app_main.cpp`
- `components/qjs_service/include/qjs_service.h`
- ESP-IDF `esp_http_server` component
