---
Title: Connectivity Intern Guide - Analysis, Design, and Implementation
Ticket: ESP-53-PULP-CONNECTIVITY
Status: active
Topics:
    - papers3
    - esp32s3
    - microquickjs
    - architecture
    - eink
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-07-16T18:36:10.525745146-04:00
WhatFor: ""
WhenToUse: ""
---

# Connectivity Intern Guide — Analysis, Design, and Implementation

## 1. What you are building, in one page

You are extending PULP OS (`0114-papers3-pulp-os`) with its outward-facing layer: **WiFi** (scan, join, stored credentials, forget), **HTTP fetch**, an **embedded web server** with JS-defined routes, **general SD filesystem access**, and the **buzzer**. Each capability becomes a ROM-stdlib singleton with a fluent, chainable JavaScript API in the style the OS already speaks, and each is validated with console probes and transcripts like everything before it.

Two external codebases set the API taste, and you should skim both before designing anything:

- `~/code/wesen/go-go-golems/go-go-goja` — Go-native modules behind Node-style `require()`. Study `modules/fs` (backend abstraction + a `Capabilities` descriptor) and `modules/express`: routes are chainable builders with an explicit terminal verb — `app.get("/hello/:name").public().handle((ctx, res) => res.json({...}))` — and `app.listen` is deliberately absent because the HOST owns the server lifecycle. Both ideas transfer directly.
- `~/code/wesen/go-go-golems/rag-evaluation-system` — `pkg/widgetdsl` and `examples/xgoja-widgetdsl-v3`: builder-lambda DSLs (`widget.app.shell((shell) => shell.brand(...))`), namespaced asset mounts (`require("fs:assets")`), and pages assembled from small chainable pieces.

You are NOT porting those systems. mquickjs has no `require`, no promises, no event loop of its own, and a compacting GC that forbids native `JSValue` storage (ESP-51 binding report, vault). The job is to translate the *shape* of those DSLs onto PULP's constraint field: singletons instead of modules, callback ids instead of closures crossing the boundary, and a completion-mailbox pattern instead of promises. Prerequisite reading: the ESP-51 guide §6–7 (engine + binding rules), the ESP-52 guide (an end-to-end extension case), and both implementation diaries.

## 2. The constraint field (recap, with the two rules that dominate this ticket)

- **One owner task.** All JS runs on the UI owner (core 1) under a deadline. WiFi/HTTP/httpd work happens on OTHER tasks (esp_wifi's event loop, esp_http_client's caller, httpd's worker) — those tasks may ONLY post bounded POD events (`PostEvent`) and read/write their own module state under a documented handoff. No JS, no widget-tree access, no `s3paper_storage` calls off-owner.
- **`CallCb(id, a, b, c)` takes int32 arguments only.** This is a GC-safety boundary, not a convenience (binding report §5.2: integer immediates can be pushed after the stack check; strings cannot without re-deriving the push order). Result payloads therefore travel through **native mailboxes read back by accessor functions** — the pattern the book service already uses (`bookLine(i)`), and the single most important design decision in this ticket. A callback tells JS *that* something completed and its integer essentials; accessors deliver the strings.
- Everything else you know: stdlib additions require the four-step regeneration protocol (both headers, bytecode, firmware); nodes and events are POD; text caps at 63 bytes so mailbox accessors, not widget text, carry long strings; owner stack is 8 KiB (buffers live in module statics/PSRAM, never stack).

## 3. The async model: completion mailboxes

Every long operation in this ticket follows one pattern. Learn it once:

```
JS (owner task)                     native module              worker context
---------------                     -------------              --------------
wifi.scan(fn)          ── cb id ──> state=Scanning ── starts ── esp_wifi scan
                                                                 ...done...
                                    mailbox <- results  <──────  event handler
                                    PostEvent(ModuleDone) <────  (POD only)
owner loop: ModuleDone ──────────>  CallCb(cb, kind, n, err)
fn(kind, n, err) runs; reads
  wifi.ssid(i), wifi.rssi(i)  ───>  mailbox accessors (owner-only)
```

Rules of the pattern:

1. One in-flight operation per module (a `Busy` StatusCode rejects overlap). Single slots keep mailboxes bounded and eliminate correlation ids in v1.
2. The worker context writes the mailbox BEFORE posting the completion event; the owner reads it only AFTER receiving the event. The event is the memory barrier (FreeRTOS queue send/receive orders the accesses); no locks.
3. Mailbox contents are fixed-size POD arrays in the module's static state (PSRAM if large).
4. The completion callback signature is `fn(kind, value, err)` — three ints. Everything richer is an accessor.
5. Callbacks are registered per call (`RegisterCb`), fire once, and the id is cleared before invocation so a callback that starts a new operation can re-register safely. `resetTree()` clears module callbacks like everything else — an app switch cancels delivery (the operation itself keeps running to completion and drops its callback if none is registered).

A new `AppEventKind::ModuleDone { uint8 module; int32 a, b; }` carries completions through the existing owner queue. Add it to `app_events.h` next to `TimerDue`.

## 4. `wifi` — scan, join, remember, forget

### 4.1 JS surface

```js
wifi.status()                    // 0 off, 1 idle, 2 scanning, 3 joining, 4 up
wifi.ip()                        // "192.168.1.23" or ""
wifi.ssidCurrent()               // joined SSID or ""
wifi.rssiCurrent()               // dBm or 0

wifi.scan(function (k, n, err) { ... })   // k=1 scan-done, n = result count
wifi.count(); wifi.ssid(i); wifi.rssi(i); wifi.secure(i)   // scan mailbox

wifi.join(ssid, pass, function (k, ok, err) { ... })  // k=2 join-done
wifi.joinSaved(function (k, ok, err) { ... })          // best stored network
wifi.save(ssid, pass)      // persist credentials (see 4.3)
wifi.forget(ssid)          // remove stored credentials
wifi.savedCount(); wifi.savedSsid(i)
wifi.off()                 // radio down (power!)
```

Chaining is not fluent here on purpose: WiFi calls are verbs with one callback, not builders. The builder shape (goja express) is reserved for `serve`, where composition is real.

### 4.2 Native architecture

- `main/net_wifi.{h,cpp}` (module state + bindings) — station mode only in v1. `esp_wifi` init is LAZY (first scan/join), because the radio costs ~80 mA and this is an e-reader; `wifi.off()` deinits.
- The esp event handlers (`WIFI_EVENT_*`, `IP_EVENT_STA_GOT_IP`) run on the system event task: they update a small POD status block (written with `std::atomic<uint8_t>` state enum), fill the scan mailbox (`esp_wifi_scan_get_ap_records` into a static array of 16 `{ssid[33], rssi, authmode}`), and `PostEvent(ModuleDone{module=Wifi, a=kind, b=count_or_ok})`.
- Join has a timeout (owner-side: 15 s tick check) and a bounded retry (2) before reporting failure; the callback's `err` is the disconnect reason code.

### 4.3 Credential storage: a new record type in s3paper_storage

The settings store is `{key[16], int32}` — passwords do not fit. Add a sibling file to the persistence family, same discipline as every other state file (magic + version + count + fixed records + FNV CRC, tmp/fsync/bak/rename, coalesced flush):

```
wifi.bin   magic "S3WF"   8 × { char ssid[33]; char pass[65]; uint32 last_ok_ms; }
```

- API: `WifiCredsLoad/Save/Set/Forget/Get(i)` in `s3paper_storage` (the component, so 0112 could reuse it and the fault-injection console covers it — add kind 5 to `DebugCorruptStateFile`).
- `joinSaved()` tries records ordered by `last_ok_ms` descending against the scan results.
- Plaintext on the card is accepted and DOCUMENTED (the threat model of a hobby e-reader's SD card); do not invent crypto here.

## 5. `http` — bounded fetch

### 5.1 JS surface (builder with a terminal verb, per the express taste)

```js
http.get('http://example.com/api')
    .header('Accept', 'application/json')
    .limit(32768)                      // body cap, default 16 KiB
    .done(function (k, status, len) {  // k=3
        if (status === 200) { var body = http.body(); ... }
    })
    .send();                           // terminal verb; Busy if in flight

http.body()      // response body (one string; truncated at limit)
http.bodyLine(i) // line-indexed accessor when body > text caps
http.status(); http.length(); http.abort()
```

The builder mutates a single native request slot (`method, url[256], up to 4 headers, limit`); `.send()` validates, spawns the worker, and registers the callback. No response streaming in v1; the mailbox is one PSRAM buffer of `limit` bytes.

### 5.2 Native architecture

- `main/net_http.{h,cpp}` over `esp_http_client`. The request runs on a short-lived worker task (stack 6 KiB, deleted on completion) because the client API blocks; the worker writes status/length/body to the mailbox and posts `ModuleDone{module=Http, a=3, b=status}`.
- HTTPS: enable the cert bundle (`esp_crt_bundle_attach`) — flash cost ~100 KB, acceptable at 76% free. `http.get` accepts both schemes.
- Redirect limit 3, timeout 10 s, and the worker NEVER touches JS or the arena — it is the reference violation-tempting spot; the guide's rule 1 in §2 applies.

## 6. `serve` — JS routes on the device

### 6.1 JS surface

```js
serve.get('/status').handle(function (req) {
    return serve.json('{"battery":' + batteryLevel() + '}');
});
serve.get('/note').handle(function (req) {
    // req is an integer request slot; accessors read it:
    var q = serve.query(req);          // raw query string
    appendPostcard(q);
    return serve.text('ok');
});
serve.files('/', '/sdcard/www');       // static mount (index.html default)
serve.start(80);                       // owns the listener; Busy if running
serve.stop();
serve.url()                            // "http://192.168.1.23:80" or ""
```

Route handlers RETURN a response token (`serve.text(s)` / `serve.json(s)` / `serve.status(404)` write the pending-response slot and return a marker int). This keeps the handler synchronous and the response fully formed when it returns — no `res` object to retain, nothing async inside a route. Up to 8 routes, exact-match paths in v1 (no params; the express `:name` pattern is noted as future work).

### 6.2 The cross-task handoff (the hard part — read twice)

`esp_http_server` invokes handlers on ITS OWN task; JS must run on the owner. The handoff:

```
httpd task                              owner task
----------                              ----------
match route -> fill request slot
  (method, uri[128], query[256],
   body first 4 KiB)
PostEvent(ModuleDone{Serve, slot})
xSemaphoreTake(resp_sem, 5s)   ----->   owner loop: CallCb(route.cb, slot,0,0)
                                        JS writes response slot via serve.text()
                               <-----   xSemaphoreGive(resp_sem)
httpd_resp_send(from response slot)
timeout -> 503, slot recycled
```

- One request slot in v1 (`Busy` → immediate 503 for concurrent hits; a PaperS3 is not a web farm).
- The 5 s semaphore timeout protects the httpd task from a wedged owner; the owner's JS deadline (1 s) protects the owner from a wedged route.
- Static file routes bypass JS entirely: the httpd task streams from `/sdcard/www` itself (SD reads off-owner are safe — the VFS layer is task-safe; document that this is the ONE sanctioned off-owner storage read, files only, never state files).

## 7. `files` — general SD access

The storage component already does books, positions, and the postcard append; this module adds bounded general access rooted at `/sdcard`:

```js
files.list('/notes', function (k, n, err) { ... })   // async: SD can be slow
files.name(i); files.size(i); files.isDir(i)         // listing mailbox (32)
files.read('/notes/a.txt', fn)   // -> files.line(i), files.lineCount()
files.write('/notes/a.txt', body, fn)  // whole-file, 16 KiB cap
files.append('/notes/a.txt', line, fn)
files.remove('/notes/a.txt', fn)
files.exists('/notes/a.txt')      // sync (stat is fast)
```

- **Path discipline** is the security surface: a single native sanitizer rejects `..`, backslashes, and anything not matching `/[a-zA-Z0-9._\-/]+/`; every path is prefixed with `/sdcard` natively. JS never sees or supplies absolute FS paths. The state directory `/sdcard/.s3paper` is DENIED (loaders own it).
- Reads deliver through a line-indexed mailbox (like `bookLine`) because widget text and `CallCb` cannot carry bodies. Whole-file ops run on the owner when < 100 ms worst case is provable (list, small read); otherwise same worker pattern as http. Measure with the console before deciding — the ESP-50 catalog work has the timing method.

## 8. `buzzer` — GPIO21, LEDC, verified facts

From the in-repo `M5PaperS3-UserDemo/main/hal/hal.cpp:385`: the PaperS3 has a passive buzzer on **GPIO_NUM_21**, driven by **LEDC timer 0, low-speed mode, 13-bit resolution, duty 4096 (50%)**; tone = `ledc_set_freq`, silence = duty 0. That file is the reference implementation — port, do not reinvent.

```js
buzzer.tone(880, 200)      // Hz, ms (owner-timed stop via the tick)
buzzer.beep()              // 1 kHz, 60 ms convenience
buzzer.stop()
buzzer.melody('880:120,0:40,1175:200')   // freq:ms list, max 16 notes
```

- `main/app_buzzer.{h,cpp}`: LEDC init lazy on first tone; the note sequencer runs off the existing owner tick (a due-time check like `JsTimerTick` — no new task, ±250 ms tick resolution is fine for UI chimes).
- Product hooks once it exists: tea timer READY chime, postcard SEAL click, 2048 merge tick — one line each in `pulp.js`.
- LEDC channel 0 is free in this firmware (nothing else uses LEDC); note it in the code.

## 9. Power and product integration

- WiFi is the largest power consumer in the device. Policy: radio OFF at boot; anything that needs it calls a shared `netUp(fn)` JS helper (joinSaved with a status toast); `wifi.off()` on home-return is NOT automatic (serving a page from your e-reader is a feature) but the sleep quiesce sequence gains step 0: radio down before touch-off.
- The launcher gains a status glyph zone (battery exists; add `wifi.status()` to the home chrome via a dynamic text).
- A **Settings app** (new launcher row) is part of this ticket: scan list (tap to join, postcard-keyboard reuse for the password), saved networks with forget, serve on/off with the URL displayed, margin toggle relocated here from the long-press easter egg.
- Demo apps that make it real: **Radio** (http fetch of a text feed onto the shelf) and the serve default site (`/sdcard/www/index.html` shipping a device status page).

## 10. Gotchas inherited (the ones this ticket will meet)

1. Stdlib regeneration protocol: both headers + bytecode + firmware, every ABI change (this ticket adds ~40 functions — batch them; regenerate ONCE per phase).
2. `JS_CLASS_COUNT` only if you add classes — this design adds none (singletons are `JS_OBJECT_DEF`s, like `paper`).
3. Never store a `JSValue` natively; cb ids only. Mailboxes are POD.
4. Worker tasks post events; they never call JS, storage, or the arena. The httpd static-file path is the single documented exception (VFS reads only).
5. Owner stack 8 KiB: HTTP bodies and scan records live in module statics/PSRAM.
6. `sdkconfig.defaults` seeds absent values only (`rm sdkconfig` after enabling WiFi/httpd options); esp_wifi drags in nvs_flash — init it in app_main (currently absent!).
7. Console client discipline unchanged; add `net`, `serve`, `buzz` console commands mirroring each module for probe-free validation.
8. Tap targets ≥ ~72 px; the Settings app rows follow the launcher pattern.

## 11. Phases and acceptance gates

- **P0 Orientation:** build/flash 0114, read this + ESP-51 §6-7; skim the two reference repos' modules named in §1.
- **P1 Buzzer:** module + console `buzz` + tea/postcard/2048 hooks. Gate: audible beep on device; melody plays; tea READY chimes (transcript + operator ear).
- **P2 Files:** sanitizer + sync ops + list/read mailboxes + probes. Gate: probe writes/reads/lists/removes under `/notes`; `..` and absolute paths rejected; state dir denied.
- **P3 WiFi:** module, S3WF credentials in s3paper_storage (+ fault-injection kind), scan/join/saved flows, `net` console command. Gate: scan lists real networks; join acquires IP (transcript with `wifi.ip()`); credentials survive reboot; forget works; wrong password reports err.
- **P4 HTTP:** fetch builder + mailbox + TLS bundle. Gate: probe fetches an http and an https URL, status/len/body correct; limit truncates; timeout errs cleanly.
- **P5 Serve:** route table + handoff + static mount + default site. Gate: `curl` from the workstation hits a JS route and a static file; concurrent second request gets 503; owner-wedge timeout returns 503 (probe 13's runaway as the wedge).
- **P6 Settings + demos:** Settings app (join UI, saved list, serve toggle), launcher wifi glyph, Radio demo. Gate: operator joins a network entirely on-device.
- **P7 Hardening:** module fault probes (scan during join, send during send, stop during request), 30-min serve soak under repeated curl, sleep sequence with radio, diary/changelog/doctor, host suite green throughout.

## 12. Where to look for X

| You need... | Go to |
|---|---|
| Binding patterns (singletons, cb ids, mailbox accessors) | `0114/main/js_pages.cpp` (paper), `js_services.cpp` (book/library) |
| The completion-event pattern to copy | `app_input.cpp` (tick producer -> owner) |
| Persistence record family to extend | `components/s3paper_storage/src/storage.cpp` (positions/settings) |
| Buzzer reference implementation | `M5PaperS3-UserDemo/main/hal/hal.cpp:385` |
| Route-builder DSL taste | go-go-goja `modules/express` + `examples/xgoja/20-express-hello-world` |
| fs backend/capabilities taste | go-go-goja `modules/fs/backend*.go` |
| Builder-lambda DSL taste | rag-evaluation-system `pkg/widgetdsl`, `examples/xgoja-widgetdsl-v3` |
| Stdlib regeneration protocol | ESP-51 guide §6.2-6.3; `0114/tools/js/*.sh` |
| Validation techniques | ESP-51 diary Step 6 (`js hits`, probes, traces) |

## 13. Glossary

**Mailbox** — fixed POD result storage written by a worker before its completion event, read by owner-side accessors after. **Completion callback** — `fn(kind, value, err)`, three ints, fired once via `__cbs`. **Module** — a ROM-stdlib singleton (`JS_OBJECT_DEF`) plus a native state block plus console commands. **Handoff** — the httpd request/response slot exchange guarded by a semaphore with a timeout on both sides. **S3WF** — the WiFi credentials state file, fifth member of the persistence family.
