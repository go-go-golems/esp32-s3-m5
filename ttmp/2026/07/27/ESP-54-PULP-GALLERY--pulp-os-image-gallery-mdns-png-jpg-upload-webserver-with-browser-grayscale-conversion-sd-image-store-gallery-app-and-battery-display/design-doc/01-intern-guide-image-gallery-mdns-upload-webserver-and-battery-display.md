---
Title: 'Intern Guide: Image Gallery, mDNS, Upload Webserver, and Battery Display'
Ticket: ESP-54-PULP-GALLERY
Status: active
Topics:
    - papers3
    - esp32s3
    - microquickjs
    - architecture
    - eink
    - wifi
    - webserver
    - image-upload
    - mdns
    - gallery
    - battery
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://0114-papers3-pulp-os/main/app_images.h
      Note: .g4 format + catalog + display contract
    - Path: repo://0114-papers3-pulp-os/main/app_power.cpp
      Note: |-
        Power quiesce sequence + PowerRead(); battery display and upload-route power policy live here
        Power quiesce sequence (WifiOff) + PowerRead(); battery display and upload-route power policy live here
    - Path: repo://0114-papers3-pulp-os/main/js_serve.cpp
      Note: serve.* JS bindings to mirror for the new images.* + mdns.* + battery.* surface
    - Path: repo://0114-papers3-pulp-os/main/js_services.cpp
      Note: js_pulp_battery_level() (level only); charging status is a new sibling binding
    - Path: repo://0114-papers3-pulp-os/main/net_mdns.cpp
      Note: mDNS wrapper (espressif/mdns) wired into serve/wifi/power lifecycle
    - Path: repo://0114-papers3-pulp-os/main/net_serve.cpp
      Note: |-
        Existing serve handoff (semaphore + ModuleDone{Serve}) that the image-upload POST route extends
        Existing serve semaphore handoff (ModuleDone{Serve}) that the image-upload POST route extends
    - Path: repo://0114-papers3-pulp-os/main/net_wifi.cpp
      Note: |-
        WiFi station module + WifiIp()/status() that mDNS announce + gallery URL depend on
        WiFi station module + WifiIp()/WifiStatus() that mDNS announce and the gallery URL depend on
    - Path: repo://0114-papers3-pulp-os/tools/js/apps/pulp.js
      Note: Product app that gains the Gallery app + battery glyph in home chrome
    - Path: repo://0114-papers3-pulp-os/tools/js/pulp_stdlib.c
      Note: Stdlib API surface (JS_OBJECT_DEF singletons) to extend with mdns/images/battery
    - Path: repo://components/s3paper_core/include/s3paper/draw_ops.h
      Note: |-
        DrawOpKind::Bitmap + BitmapPayload already declared but UNIMPLEMENTED in m5_backend — the gallery display gap
        DrawOpKind::Bitmap + BitmapPayload already declared but unimplemented in m5_backend — the gallery display gap
    - Path: repo://components/s3paper_core/src/frame_builder.cpp
      Note: FrameBuilder::Bitmap emitter (GlyphRun pattern)
    - Path: repo://components/s3paper_m5/src/m5_backend.cpp
      Note: |-
        Bitmap case is "Explicitly unsupported in Phase 2" — must be implemented for image rendering
        Bitmap case is explicitly unsupported (Phase 2 skip) — must be implemented for image rendering
ExternalSources: []
Summary: 'Full intern design + analysis + implementation guide for ESP-54: mDNS (pulp.local), a browser-driven PNG/JPG upload webserver that converts images to 4-bit grayscale in the browser and stores them on the SD card, a PULP image-gallery app to display and scroll images, and battery level + charging-status display. Builds entirely on the ESP-53 connectivity layer (serve/wifi) and the s3paper POD pipeline, extending the core''s latent DrawOpKind::Bitmap path into a real backend blit.'
LastUpdated: 2026-07-27T20:35:00-04:00
WhatFor: Onboarding a new engineer to implement ESP-54 on top of the ESP-53 connectivity layer.
WhenToUse: Read the ESP-53 onboarding guide (ESP-53 design-doc/02) first for system context, then this for the ESP-54-specific design.
---










# Intern Guide — Image Gallery, mDNS, Upload Webserver, and Battery Display

This document is a complete analysis, design, and implementation guide for the **ESP-54-PULP-GALLERY** ticket. It assumes you have already read the ESP-53 system onboarding guide (`ESP-53-PULP-CONNECTIVITY` ticket, `design-doc/02-pulp-os-system-onboarding-guide-...md`) and the ESP-53 connectivity design (`design-doc/01-connectivity-intern-guide-...md`). Those two documents explain the hardware, the POD widget tree, MicroQuickJS, the one-owner-task rule, the completion-mailbox async pattern, and the existing `wifi` / `http` / `serve` / `files` / `buzzer` modules. This document does not repeat that material except where it directly shapes a decision.

In one sentence: **ESP-54 turns the PaperS3 into a networked picture frame** — you reach it at `http://pulp.local`, drop a PNG or JPG onto a web page, the browser crops, scales, and converts the image to 4-bit grayscale, the device stores the converted frame on the SD card, and a new `Gallery` app shows and scrolls through every stored image, while a persistent battery/charging indicator joins the launcher chrome.

---

## 1. Executive summary

ESP-54 adds four cohesive capabilities to PULP OS, each mapped onto the existing architecture so that no invariant of the system is broken:

1. **mDNS (`pulp.local`)** — a tiny owner-owned wrapper around the managed component `espressif/mdns` (mDNS moved out of the IDF 5.x tree into the registry — confirmed absent from `esp-idf-5.3.4/components/`) that advertises the `_http._tcp` service whenever the web server is running and WiFi is up. This makes the device addressable by hostname instead of a looked-up IP. The module is lazy and cheap: mDNS only starts after `serve.start()`, and stops with the radio.
2. **Image upload webserver** — a new `images` JS module plus an extended `serve` POST handler. A single self-contained HTML/JS page (served from `/sdcard/www`) lets the operator pick a PNG or JPG, crop/scale it to the 540×960 panel in the browser using a `<canvas>`, quantize it to 4-bit (16-level) grayscale, and POST the resulting compact payload to the device, which streams it to the SD card. The heavy image work (decode, resize, dither/quantize) happens **in the browser**, never on the device — this is the single most important architecture decision in the ticket and is justified in §6.
3. **Gallery app** — a PULP app, written in `pulp.js`, that lists the stored grayscale images, displays one at a time on the e-ink panel, and lets the operator scroll left/right through them. This requires extending the latent `DrawOpKind::Bitmap` draw op — declared in the core but **explicitly unimplemented** in the M5 backend today — into a real grayscale blit. A new `images.display(name)` native verb drives that path.
4. **Battery display** — a persistent battery-level + charging-status glyph in the home chrome, plus a richer `battery` JS singleton (`level()`, `mv()`, `charging()`, `statusText()`) so any app can show power state. The data already exists (`s3paper::PowerRead()` returns `battery_level`, `battery_mv`, `charging`); ESP-54 only wires it through to JS and the home screen.

Each piece is a natural extension of the ESP-53 layer: mDNS sits on `wifi`/`serve`, the upload route sits on `serve` + `files`, the gallery app sits on the builder API + a new display op, and battery display sits on the existing power service. The ticket is deliberately scoped so that the four parts are **independently mergeable**: mDNS and battery display are low-risk and ship first; the upload pipeline and gallery display are the substantive work and ship last.

---

## 2. Problem statement and scope

### 2.1 What the operator wants

The operator wants to treat the PaperS3 as a networked grayscale picture frame:

- Plug the device in, join WiFi (already possible via the ESP-53 Settings app).
- Open `http://pulp.local` (or the IP) in a laptop/phone browser.
- Drop a photo; the browser shows a crop/scale preview fitted to the 540×960 panel.
- Press **Upload**; the device stores the converted image on the SD card.
- On the device, open the **Gallery** app; the image appears. Swipe left/right to browse all stored images.
- Always see battery % and a charging glyph on the home screen, so the frame does not die silently.

### 2.2 In scope

- mDNS service advertisement for `_http._tcp` with hostname `pulp` (resolvable as `pulp.local`).
- A browser upload page + a device POST endpoint that accepts a pre-converted 4-bit grayscale frame and writes it to the SD card.
- An `images` JS module: `count()`, `name(i)`, `display(name)`, `remove(name)`, and an async `received` completion callback.
- Implementation of `DrawOpKind::Bitmap` in the M5 backend so a grayscale bitmap can be blitted to the e-ink panel, plus a `canvas().bitmap(...)` widget verb and a `images.display(name)` path that uses it.
- A `Gallery` app in `pulp.js` with left/right scroll and a tap-to-delete.
- A `battery` JS singleton (`level`/`mv`/`charging`/`statusText`) and a persistent home-screen battery glyph.
- Console commands (`mdns`, `images`, `bat`) mirroring each module, and probes for validation.

### 2.3 Out of scope (explicitly deferred)

- **Image decode on the device.** No PNG/JPG decoder is added to the firmware. The browser does all decode and conversion. This is a hard scope boundary (see Decision R-IMGDECODE).
- **OTA / remote image fetch.** Pulling an image from a URL (`http.get` then display) is a natural follow-up but is not required for the operator's flow; it reuses the same `images.display` path once the bitmap blit exists.
- **Thumbnails / multi-image grid.** The gallery shows one image full-screen at a time. A grid view is future work.
- **Animated / dithered-to-1-bit transitions.** Only static 4-bit grayscale stills.
- **Encryption of stored images.** The SD card threat model is the same as ESP-53's credential model (documented plaintext, hobby device).

### 2.4 Non-goals that look like goals

- **Full-color display.** The panel is 16-gray e-ink. Color is impossible; the browser quantizes to grayscale deliberately.
- **Sub-second upload of megapixel images.** The panel is 540×960 = 518,400 px. At 4 bits packed (2 px/byte) that is 259,200 bytes ≈ 253 KiB per image. Over WiFi that is a few seconds; acceptable for a picture frame, not a video stream.

---

## 3. Current-state architecture (evidence-based)

This section anchors every claim to a concrete file so you can verify the design against reality before writing a line of code.

### 3.1 The connectivity layer ESP-54 builds on (ESP-53, shipped)

ESP-53 added five JS modules. The ones ESP-54 extends are documented here at the level you need:

- **`wifi`** (`0114/main/net_wifi.{h,cpp}`) — station mode, lazy `esp_wifi` init, scan mailbox (16 APs), `join`/`joinSaved` with timeout+retry, `save`/`forget` credentials in a new `S3WF` state file. Key accessors ESP-54 reuses: `WifiStatus()` (0 off → 4 up), `WifiIp(out, cap)` (empty string when not up), `WifiSsidCurrent()`. The completion-mailbox pattern (§3.2) is implemented here and is the template for every async verb in ESP-54.
- **`serve`** (`0114/main/net_serve.{h,cpp}`) — an `esp_http_server` instance with up to 8 exact-match GET routes plus one static-file mount (`/sdcard/www`). The hard part is the **cross-task handoff**: `esp_http_server` invokes handlers on its own task, but JS must run on the owner. The existing design claims one request slot, fills it, posts `ModuleDone{Serve, kDoneServeRequest, route_index, gen}`, and blocks on a binary semaphore for up to 5 s; the owner dispatches the route callback, which calls `serve.text()`/`json()`/`status()` to fill the response slot, then gives the semaphore. **ESP-54 adds POST support to this same handoff** — see §7.
- **`files`** (`0114/main/app_files.{h,cpp}`) — bounded SD access rooted at `/sdcard` with a path sanitizer (rejects `..`, backslashes, the state dir). ESP-54's image store is a new sibling directory `/sdcard/images/` accessed through the same sanitizer discipline, but the write path is streamed (see §8), not the 16 KiB `files.write` cap.

### 3.2 The completion-mailbox pattern (the contract every async verb follows)

This is the single most important pattern in ESP-53 and ESP-54 reuses it verbatim. Learn it from `net_wifi.cpp`:

```
JS (owner task)                     native module              worker / event task
---------------                     -------------              -------------------
wifi.scan(fn)          ── cb id ──> state=Scanning ── starts ── esp_wifi scan
                                                                 ...done...
                                    mailbox <- results  <──────  event handler
                                    PostEvent(ModuleDone) <────  (POD only)
owner loop: ModuleDone ──────────>  CallCb(cb, kind, n, err)
fn(kind, n, err) runs; reads
  wifi.ssid(i), wifi.rssi(i)  ───>  mailbox accessors (owner-only)
```

The rules (from `ESP-53 design-doc/01 §3`):

1. **One in-flight operation per module** (a `Busy` StatusCode rejects overlap). Single slots keep mailboxes bounded.
2. The worker writes the mailbox **before** posting the completion event; the owner reads it **only after** receiving the event. The queue send/receive is the memory barrier — no locks.
3. Mailbox contents are fixed-size POD arrays in module static state (PSRAM if large).
4. The completion callback signature is `fn(kind, value, err)` — three int32s. Everything richer is an accessor.
5. Callbacks are registered per call, fire once, and the id is cleared before invocation. `resetTree()` cancels pending deliveries.

ESP-54's `images.upload` completion follows this exactly: the httpd POST worker fills a small result mailbox `{name, bytes, err}` and posts `ModuleDone{Images, kDoneImagesUpload, bytes, err}`.

### 3.3 The POD widget tree and the present pipeline (ESP-51/52, shipped)

Every visible pixel on the PaperS3 comes from this pipeline (from `ESP-53 design-doc/02 §4–5`):

```
JS builder calls (js_widgets.cpp mutate the arena)
        │
        ▼
p.show() / p.update() ──► PresentPage / PresentPageUpdate
        │
        ▼
layout pass (widget_layout)
        │
        ▼
EmitNode ──► FrameBuilder ──► DrawOp list in FrameArena
        │
        ▼
diff vs previous frame ──► damage rects
        │
        ▼
RefreshPlanner ──► partial | full refresh decision
        │
        ▼
m5_backend executes ops within damage clip ──► EPD blit
```

The `DrawOp` union (`components/s3paper_core/include/s3paper/draw_ops.h`) already declares a `Bitmap` kind:

```c
enum class DrawOpKind : uint8_t {
    FillRect = 0, StrokeRect, HLine, VLine, GlyphRun, Bitmap, Line, Circle,
};
...
struct BitmapPayload {
    uint32_t data_offset;  // arena offset of pixel data
    uint32_t data_len;
    int32_t stride;        // bytes per row
};
```

**But the M5 backend does not implement it.** From `components/s3paper_m5/src/m5_backend.cpp:402`:

```c
case DrawOpKind::Bitmap:
    // Explicitly unsupported in Phase 2.
    result.ops_skipped++;
    break;
```

And there is **no `FrameBuilder::Bitmap(...)` method** declared in `frame_builder.h` (only `FillRect`, `StrokeRect`, `HLine`, `VLine`, `Line`, `Circle`, `Ring`, `GlyphRun`, clip push/pop). So the bitmap path is half-plumbed: the core has the payload type and the dispatch arm, but no emitter and no backend rasterizer. **This is the central implementation gap ESP-54 fills** (see §9). The existing canvas widget verbs (`line`/`disc`/`ring`/`box`/`paint`/`wipe`) all map onto the other ops; images need `Bitmap`.

### 3.4 The canvas widget (ESP-52, shipped)

The `canvas()` widget (`components/s3paper_core/include/s3paper/widget.h`) holds up to `kCanvasCmds = 96` `CanvasCmd` records, each 12 bytes: `{kind, gray, thickness, _pad, a, b, c, d}` where kind ∈ `{kFill, kBox, kLine, kDisc, kRing}`. A canvas emits `PushClip(frame)`, replays its commands translated to absolute coordinates, then `PopClip`. There is **no per-pixel command** and **no bitmap command** in the canvas today — `paint(x,y,w,h,gray)` is a solid fill, not an image blit. ESP-54 adds a bitmap verb to the canvas (§9.3) so an image can live inside a widget tree exactly like the `ink` app's scenes do.

### 3.5 Battery / power (ESP-51, shipped)

Battery is already readable. From `components/s3paper_m5/include/s3paper_m5/m5_power.h`:

```c
struct PowerStatus {
    int32_t battery_level;   // 0..100, -1 unknown
    int32_t battery_mv;      // millivolts, -1 unknown
    bool charging;
    uint8_t wakeup_cause;
    uint8_t reset_reason;
};
PowerStatus PowerRead();  // owner-only, after M5 init
```

The JS side exposes **only the level**: `js_pulp_battery_level()` in `0114/main/js_services.cpp` returns `st.battery_level`. The `PowerSnapshot` struct in `app_events.h` already carries `battery_level`, `battery_mv`, and `charging` — so the console `sleep` status command already prints charge state. ESP-54 adds `charging()` and `mv()` to JS and surfaces them in the UI; the data path exists, only the JS surface is missing.

### 3.6 The stdlib/bytecode toolchain (the regeneration protocol)

Every new JS identifier (function name, method, global) must be added to `tools/js/pulp_stdlib.c` (the device-truth API surface), then regenerated:

```bash
cd 0114-papers3-pulp-os
./tools/js/gen_pulp_stdlib.sh        # 1. regenerate js_stdlib.h + mquickjs_atom.h
./tools/js/build_bytecode_apps.sh    # 2. recompile apps/*.js -> js_<name>.h bytecode
idf.py build                          # 3. rebuild firmware
```

`N_ROM_ATOM_TABLES_MAX = 2` (one stdlib + one bytecode image) means **all apps concatenate into one image** — you cannot add a second. New singletons are `JS_OBJECT_DEF`s (like `paper`, `wifi`, `serve`), not new classes, so `JS_CLASS_COUNT` does not change. ESP-54 adds two singletons (`mdns`, `battery`) and extends `serve` + a new `images` singleton. The regeneration protocol is the most failure-prone part of the workflow; batch identifier additions per phase (gotcha #1, §11).

---

## 4. Gap analysis

| Requirement | Current state | Gap |
|---|---|---|
| Reach device by `pulp.local` | WiFi up gives an IP; `serve.url()` returns `http://<ip>` | No mDNS; hostname not advertised |
| Upload an image from a browser | `serve` does GET routes + static files only; no POST; `files.write` caps at 16 KiB | No POST handler; no streamed SD write; no upload page |
| Convert PNG/JPG to 4-bit grayscale | Nothing on device; `DrawOpKind::Bitmap` declared but unimplemented | Browser must do it (scope decision); device needs a 4-bit packed format contract |
| Store converted image on SD | `files` module exists; `/sdcard` mounted | New `/sdcard/images/` directory + streamed write + an index |
| Display a stored image | Canvas has no bitmap verb; `DrawOpKind::Bitmap` skipped by backend | Implement backend blit + frame emitter + canvas verb + `images.display` |
| Scroll through images | Apps navigate by swipe (reader, daily) | New Gallery app; left/right = prev/next image |
| Battery level display | `batteryLevel()` exists; home chrome has wifi glyph only | Add `battery` singleton (`charging`, `mv`); home glyph; refresh on tick |

The gaps cluster into two engineering tracks: **(A) the network/upload track** (mDNS + POST + SD stream + upload page) and **(B) the display track** (backend bitmap blit + canvas verb + gallery app). Track B's backend work is the riskiest because it touches the shared `s3paper_m5` component and the present pipeline; it is sequenced last.

---

## 5. Proposed architecture

### 5.1 System map

```
                            ┌──────────────────────────────────┐
                            │  Operator's browser (laptop/phone) │
                            │  http://pulp.local  (mDNS resolve) │
                            └───────────────┬──────────────────┘
                                            │ HTTP (WiFi)
                                            ▼
┌───────────────────────────────────────────────────────────────────────┐
│  PaperS3 (ESP32-S3)                                                    │
│                                                                       │
│  esp_http_server (httpd task)                                         │
│   ├─ GET  /            → /sdcard/www/index.html  (upload page)         │
│   ├─ GET  /status      → JS route (battery + wifi + app) [ESP-53]      │
│   ├─ GET  /images/list → JS route (count + names)            [ESP-54]  │
│   └─ POST /images/upload→ streamed SD write + completion      [ESP-54] │
│                          │                                            │
│   httpd POST worker ─────┼──► fills ImagesUpload mailbox (POD)        │
│                          │     PostModuleDone(Images, Upload, bytes)   │
│                          ▼                                            │
│  ┌─────────────────────────────────────────────────────────────────┐  │
│  │  Owner task (core 1) — the ONLY task that touches UI/JS/arena    │  │
│  │   ├─ ModuleDone{Images} → CallCb → images.received(k,bytes,err) │  │
│  │   ├─ mDNS announce (after serve.start + wifi up)                │  │
│  │   ├─ battery glyph (home chrome dyn-value, tick-refreshed)      │  │
│  │   └─ Gallery app: images.display(name) →                       │  │
│  │         FrameBuilder.Bitmap(...) → DrawOp::Bitmap →             │  │
│  │         m5_backend blit (NEW) → EPD                             │  │
│  └─────────────────────────────────────────────────────────────────┘  │
│                                                                       │
│  SD card:  /sdcard/images/*.g4   (4-bit packed grayscale frames)      │
│             /sdcard/images/index.txt (catalog: one name per line)     │
└───────────────────────────────────────────────────────────────────────┘
```

### 5.2 The 4-bit packed grayscale frame format (`.g4`)

The wire and storage format is a compact, device-trivial format defined once and used everywhere:

```
Offset  Size  Field
0       4     magic "G4IM"           (0x47 0x34 0x49 0x4D)
4       2     width  (uint16 LE)     (always 540 for full-frame v1)
6       2     height (uint16 LE)     (always 960 for full-frame v1)
8       1     depth  (uint8)         (4, fixed)
9       1     version (uint8)        (1)
10      2     reserved (0)
12      ...   pixel data, 4 bits/px, 2 px/byte, high nibble first,
              row-major, width padded to an even byte count
```

- Full frame pixel data = `ceil(540/2) * 960 = 270 * 960 = 259,200` bytes ≈ 253 KiB.
- Header is 12 bytes; total file ≈ 253 KiB. The SD card (multi-GB) holds thousands.
- 4-bit grayscale maps directly onto the e-ink panel's 16 gray levels: a 0–15 value indexes the panel's LUT. The browser does the 256→16 quantization (with optional Floyd–Steinberg dithering, §6.3), so the device only unpacks nibbles and writes gray levels.

### 5.3 Module ownership summary

| Module | File(s) | New/extend | Owner-task contract |
|---|---|---|---|
| mDNS | `main/net_mdns.{h,cpp}` | **new** | Owner-only start/stop; announces after serve+wifi up |
| images (JS) | `main/app_images.{h,cpp}`, `main/js_images.cpp` | **new** | Owner owns the catalog + display; httpd streams to SD (the one sanctioned off-owner SD write, documented like ESP-53's static-file read) |
| serve POST | `main/net_serve.cpp` | **extend** | Add POST route table + a streamed-body receive on the httpd task |
| battery (JS) | `main/js_services.cpp` | **extend** | Owner-only reads via `PowerRead()` |
| bitmap blit | `components/s3paper_m5/src/m5_backend.cpp` | **extend** | Backend rasterizer for `DrawOpKind::Bitmap` |
| bitmap emit | `components/s3paper_core/src/frame_builder.cpp` | **extend** | New `FrameBuilder::Bitmap(...)` + canvas verb |
| Gallery app | `tools/js/apps/pulp.js` | **extend** | New `gallery()` app + home battery glyph |
| stdlib | `tools/js/pulp_stdlib.c` | **extend** | New `mdns`/`images`/`battery` singletons + serve POST surface |

---

## 6. Decision records

### Decision: R-IMGDECODE — Image decode and quantization happen in the browser, never on the device

- **Context:** The operator uploads a PNG/JPG. The device could decode it (libpng/libjpeg/TJpegDec) and quantize, or the browser could do all of it and ship a compact 4-bit frame. The ESP32-S3 has 512 KB internal SRAM (≈220–230 KB free at steady state) and 8 MB PSRAM; a single 540×960 RGB888 frame is 1.5 MB, a decoded JPEG intermediate often more.
- **Options considered:**
  1. Device decodes PNG/JPG with libpng + TJpegDec, then quantizes. ~60–100 KB flash for the decoders, plus multi-hundred-KB working RAM per upload, plus the decode blocks the httpd worker for seconds.
  2. Browser decodes, resizes, and quantizes to 4-bit; device only stores and displays a 253 KiB `.g4` frame. Zero decoder on device; device work is a straight byte copy.
  3. Hybrid: browser decodes+scales to 8-bit grayscale, device quantizes to 4-bit. Saves wire bytes (8-bit vs 4-bit) but reintroduces per-pixel device work.
- **Decision:** Option 2. Browser does everything; device receives a ready 4-bit packed frame.
- **Rationale:** The browser has a mature `<canvas>` + `ImageData` pipeline, gigabytes of RAM, and zero flash cost to the device. The device's scarce resources (internal RAM, flash) stay scarce; spending them on a decoder for a picture-frame use case is a poor trade. The 4-bit wire format is already the storage format, so the device does a pure streaming copy — the simplest, most robust path. This also matches the project's documented pattern (`Research/KB/Tribal/browser-side-processing-for-embedded`): push pixel work to the host where the host is strictly better at it.
- **Consequences:** The upload page must be self-contained and do crop/scale/dither (more browser JS, §8.2). The device never sees the original image — only the 4-bit frame — so re-quantization requires re-upload (acceptable). The `.g4` format becomes a public contract that must be versioned (`version` field).
- **Status:** accepted

### Decision: R-BITMAPBLIT — Implement `DrawOpKind::Bitmap` as a grayscale-nibble blit in the M5 backend, not a new widget kind

- **Context:** The core already declares `DrawOpKind::Bitmap` + `BitmapPayload {data_offset, data_len, stride}` but the M5 backend skips it. Images need a display path. Options: (a) implement the existing `Bitmap` op; (b) add a brand-new image-specific op; (c) render images as thousands of `FillRect` 1×1 ops.
- **Options considered:**
  1. Implement the existing `DrawOpKind::Bitmap` with the existing `BitmapPayload`, treating `data` as packed 4-bit grayscale. Reuses the declared core contract; one new backend arm.
  2. Add `DrawOpKind::Image` with a richer payload (width/height/format). Avoids overloading `Bitmap` but duplicates plumbing (payload, dispatch, name).
  3. Emit one `FillRect` per pixel. 518,400 ops/frame — blows the `FrameArena` op budget and the EPD timing budget; obviously unviable.
- **Decision:** Option 1. Implement the existing `Bitmap` op; the pixel data is packed 4-bit grayscale, `stride` is bytes-per-row.
- **Rationale:** The core already committed to the `Bitmap` op and payload shape; the only missing piece is the backend rasterizer and a `FrameBuilder::Bitmap()` emitter. Reusing it honors the "POD everywhere at the core" commitment and keeps the change localized to one backend arm + one emitter + one canvas verb. Overloading `Bitmap` to mean "4-bit grayscale bitmap" is fine because no other code emits it today.
- **Consequences:** The `BitmapPayload` pixel format is now implicitly "4-bit packed grayscale, high nibble first." Document it in `draw_ops.h`. The backend must allocate a PSRAM scratch buffer (540 bytes/row of gray levels) to call `M5.Display.drawPixel`-equivalents efficiently — see §9.2 for the rasterization strategy. Host tests gain a fake-backend bitmap trace.
- **Status:** accepted

### Decision: R-POSTHANDOFF — Image upload reuses the serve semaphore handoff, but the body is streamed to SD on the httpd task, not handed to JS

- **Context:** ESP-53's serve handoff passes a request *slot* (URI/query/4 KiB body) to the owner and waits for a JS-produced response. An image body is 253 KiB — far too large for the 4 KiB slot. Options: (a) chunk the body through the owner mailbox; (b) stream the body straight to SD on the httpd task and only notify the owner of completion; (c) stream to a PSRAM buffer then have the owner write it.
- **Options considered:**
  1. Chunked owner handoff: the owner reassembles 4 KiB chunks. Keeps all SD writes on the owner but floods the event queue and holds the httpd task for many seconds.
  2. httpd task streams the POST body directly to an SD file (the one sanctioned off-owner SD *write*, mirroring ESP-53's sanctioned off-owner SD *read* for static files), then posts a single completion `ModuleDone{Images}` with a result mailbox `{name, bytes, err}`. The owner only does the JS callback + catalog update.
  3. PSRAM buffer then owner write: needs 253 KiB contiguous PSRAM (available) plus a second copy.
- **Decision:** Option 2. The httpd POST handler streams the body to `/sdcard/images/<ts>.g4` via `fopen`/`fwrite` on the httpd task, validates the header, appends to `index.txt`, then posts completion. The owner only runs the JS callback.
- **Rationale:** SD VFS writes are task-safe (ESP-53 already relies on this for static-file reads). Streaming avoids a 253 KiB bounce buffer and keeps the owner queue uncluttered. The owner still owns the *catalog* and the *JS notification*, preserving the one-owner rule for UI state. This is the direct analog of ESP-53's documented static-file exception, extended to writes, and must be documented with the same prominence.
- **Consequences:** The httpd task does file I/O — it must be robust to a missing/failed SD (return 503, log, post completion with `err`). The POST handler must enforce the header + size cap (reject early if `Content-Length` > ~280 KiB). The single-slot busy rule still applies (a second concurrent POST gets 503). The completion mailbox is small and POD.
- **Status:** accepted

### Decision: R-MDNSLIFE — mDNS starts after `serve.start()` succeeds and WiFi is up; stops with the radio

- **Context:** mDNS advertises `_http._tcp` so `pulp.local` resolves. It is only useful when both the server is listening and WiFi is up.
- **Options considered:**
  1. Start mDNS at boot regardless. Wastes packets and power when offline.
  2. Start mDNS inside `serve.start()` once WiFi is up; stop it in `serve.stop()` and in the power quiesce (which already calls `WifiOff`).
  3. A separate `mdns.start()/stop()` the app calls.
- **Decision:** Option 2 with a thin `mdns` JS singleton wrapper for probing. `serve.start()` calls `MdnsAnnounce()` internally; the power quiesce and `wifi.off()` call `MdnsStop()`.
- **Rationale:** The operator never needs to think about mDNS — if the server is up, the name resolves. Tying it to `serve.start()`/`serve.stop()` and the existing radio-down path (in `app_power.cpp`'s `PowerSleep`) keeps lifecycle simple and power-honest. A tiny `mdns.status()` JS verb exists only for the probe.
- **Consequences:** `net_serve.cpp::ServeStart` gains a call to `net_mdns.cpp::MdnsAnnounce()`; `ServeStop` and `WifiOff` call `MdnsStop()`. mDNS init is lazy and idempotent. The hostname is fixed `pulp` (configurable later via a settings record if needed).
- **Status:** accepted

---

## 7. mDNS — `pulp.local`

### 7.1 JS surface

```js
mdns.status()        // 0 off, 1 announced
mdns.host()         // "pulp" (the hostname; pulp.local resolves)
mdns.url()          // "http://pulp.local" or "" when not announced
```

mDNS has no async verbs. It is a side effect of `serve.start()` / `serve.stop()` / `wifi.off()`. The singletons are read-only accessors for the probe and for the Settings app to display the URL.

### 7.2 Native architecture (`main/net_mdns.{h,cpp}`)

mDNS in modern ESP-IDF (5.x) ships as the **managed component `espressif/mdns`**, not as a component in the IDF tree. Confirmed: `find ~/esp/esp-idf-5.3.4 -iname '*mdns*'` returns nothing under `components/`; only `esp_local_ctrl` and `openthread` reference it internally. So the dependency must be added to `0114-papers3-pulp-os/main/idf_component.yml` (the per-component manifest — a root manifest is ignored per AGENTS.md) and `mdns` added to `REQUIRES` in `main/CMakeLists.txt`. The component pulls in the standard `mdns.h` API (`mdns_init`, `mdns_hostname_set`, `mdns_service_add`, `mdns_service_remove`, `mdns_free`). The wrapper:

```c
// net_mdns.h
namespace pulp {
StatusCode MdnsInit();          // owner-only, lazy, idempotent; sets hostname "pulp"
StatusCode MdnsAnnounce(uint16_t port);  // owner-only; add _http._tcp service
StatusCode MdnsStop();          // owner-only; deinit
uint8_t MdnsStatus();           // 0 off, 1 announced
void MdnsHost(char *out, size_t cap);    // "pulp"
void MdnsUrl(char *out, size_t cap);     // "http://pulp.local" or ""
}
```

- `MdnsInit()` calls `mdns_init()` then `mdns_hostname_set("pulp")` and `mdns_instance_name_set("PULP OS")`. Lazy on first `MdnsAnnounce`.
- `MdnsAnnounce(port)` calls `mdns_service_add(NULL, "_http", "_tcp", port, NULL, 0)`. Idempotent: if already announced, no-op.
- `MdnsStop()` calls `mdns_service_remove` + `mdns_free`; sets status off.
- `MdnsUrl()` returns `http://pulp.local` only when announced (status 1) AND `ServeRunning()`; empty otherwise (so the Settings app shows the URL only when it is actually reachable).

### 7.3 Wiring

In `net_serve.cpp::ServeStart`, after `httpd_start` succeeds:

```c
if (s_state.server != nullptr) {
    MdnsAnnounce(s_state.port);   // announce pulp.local:<port>
}
```

In `ServeStop` and in `app_power.cpp::PowerSleep` (which already calls `WifiOff`), call `MdnsStop()` before the radio drops. In `net_wifi.cpp::WifiOff`, also call `MdnsStop()` (the name is useless without a link).

### 7.4 Pseudocode — the announce lifecycle

```
on serve.start(port):
    if httpd_start ok:
        MdnsAnnounce(port)        # pulp.local now resolves to the device IP

on serve.stop / wifi.off / sleep quiesce:
    MdnsStop()                    # name withdrawn; browser falls back to IP

probe (js probe N):
    mdns.status() -> 1
    mdns.url()    -> "http://pulp.local"
    (operator verifies `ping pulp.local` / browser opens pulp.local)
```

---

## 8. Image upload webserver

### 8.1 The device side: a POST route on `serve`

ESP-53's `serve` only does GET. ESP-54 extends the route table to carry a method flag and adds a POST handler for `/images/upload`. The handoff (Decision R-POSTHANDOFF) streams the body to SD on the httpd task.

**Extended route entry** (`net_serve.h`):

```c
struct RouteEntry {
    bool in_use = false;
    uint8_t method;   // 0 = GET (default), 1 = POST     [ESP-54]
    char path[kServeMaxPath] = {};
    int32_t cb_id = 0;
};
```

**New console op** (so `images` has a console mirror, like `net`/`serve`): add `ConsoleOp::Images` to `app_events.h` (arg: 0 status, 1 list, 2 display index, 3 remove index, 4 received-callback status).

**The POST handler** (`net_serve.cpp::ServeUpload`, runs on the httpd task):

```c
// HTTPD TASK. Streams the POST body to /sdcard/images/<ts>.g4.
esp_err_t ServeUpload(httpd_req_t *req) {
    // 1. Cap: reject early if Content-Length > kImageMaxBytes (280 KiB).
    size_t total = req->content_len;
    if (total > kImageMaxBytes) return Send413(req, "too large");

    // 2. Claim the single upload slot (Busy -> 503, like JS routes).
    if (!ClaimUploadSlot()) return Send503(req, "busy");

    // 3. Generate a timestamped name; open /sdcard/images/<ts>.g4 for write.
    char path[64]; snprintf(path, sizeof(path), "/sdcard/images/%llu.g4", now_ms());
    mkdir("/sdcard/images", 0775);
    FILE *f = fopen(path, "wb");
    if (!f) { ReleaseUploadSlot(); return Send503(req, "no card"); }

    // 4. Stream: read in 1 KiB chunks, validate the 12-byte header on the
    //    first chunk (magic G4IM + dims 540x960 + depth 4), fwrite each.
    static char chunk[1024];
    size_t received = 0; bool header_ok = false;
    while (received < total) {
        ssize_t n = httpd_req_recv(req, chunk, MIN(sizeof(chunk), total - received));
        if (n <= 0) break;
        if (!header_ok) {
            if (!ValidateG4Header(chunk, n)) { fclose(f); remove(path);
                ReleaseUploadSlot(); return Send400(req, "bad frame"); }
            header_ok = true;
        }
        fwrite(chunk, 1, n, f);
        received += n;
    }
    fflush(f); fsync(fileno(f)); fclose(f);

    // 5. Append to the catalog index (one name per line).
    AppendImageCatalog(path);

    // 6. Post completion to the owner; httpd responds 200 with the name.
    ImagesUploadResult res = { .name = <basename>, .bytes = received, .err = 0 };
    ImagesSetUploadResult(res);
    PostModuleDone(ModuleId::Images, kDoneImagesUpload, (int32_t)received, 0);
    ReleaseUploadSlot();
    return Respond200(req, "text/plain", res.name);
}
```

The owner, on `ModuleDone{Images, Upload}`, runs the registered `images.received` callback (if any) so the Gallery app can refresh. The catalog (`/sdcard/images/index.txt`) is the source of truth for `images.count()` / `images.name(i)`; it is read on demand by the owner.

### 8.2 The browser side: the upload page (`/sdcard/www/index.html`)

A single self-contained HTML page with inline JS. It is seeded into `/sdcard/www/index.html` by `ServeFilesMount` (ESP-53 already writes a default index; ESP-54 replaces that default with the upload UI). The page does:

1. **File pick** — an `<input type="file" accept="image/png,image/jpeg">`.
2. **Decode** — `FileReader` → `Image` → drawn onto an offscreen `<canvas>`.
3. **Crop/scale** — a preview canvas sized 540×960 (portrait). The operator drags/zooms the source within a crop box; the page computes the transform that maps the crop region to the full 540×960 destination. A "fit" button sets contain-mode; "fill" sets cover-mode.
4. **Quantize to 4-bit** — read `ImageData` (RGBA), convert each pixel to luminance `Y = (R*299 + G*587 + B*114)/1000`, optionally Floyd–Steinberg dither, then quantize to 0–15 (`>> 4`). Pack 2 px/byte, high nibble first, row-major.
5. **Build the `.g4` payload** — 12-byte header + packed pixels.
6. **POST** — `fetch('/images/upload', { method: 'POST', body: ArrayBuffer })`. Show progress and the server-returned name.
7. **List** — `fetch('/images/list')` (a JS route returning JSON) to show what is already on the device.

### 8.3 Browser pseudocode — the quantize-and-pack step

```js
// imgData: ImageData over a 540x960 canvas. dither: boolean.
function buildG4(imgData, dither) {
  const W = 540, H = 960;
  const hdr = new Uint8Array([
    0x47, 0x34, 0x49, 0x4D,        // "G4IM"
    W & 0xFF, (W >> 8) & 0xFF,      // width LE
    H & 0xFF, (H >> 8) & 0xFF,      // height LE
    4, 1, 0, 0                      // depth, version, reserved
  ]);
  const rowBytes = Math.ceil(W / 2);          // 270
  const pixels = new Uint8Array(rowBytes * H); // 259200 bytes
  const src = imgData.data;
  const errBuf = new Float32Array(W);         // dither carry
  for (let y = 0; y < H; y++) {
    for (let x = 0; x < W; x++) {
      const i = (y * W + x) * 4;
      let y_ = (src[i] * 299 + src[i + 1] * 587 + src[i + 2] * 114) / 1000;
      if (dither) y_ += errBuf[x];
      let g = Math.max(0, Math.min(15, Math.round(y_ / 17))); // 0..15
      if (dither) {
        const q = g * 17;
        const e = (y_ - q) ;
        if (x + 1 < W) errBuf[x + 1] += e * 7 / 16;
        if (x > 0)    nextRowErr[x - 1] += e * 3 / 16; // (manage two rows)
        errBuf[x] = e * 5 / 16; ... // standard FS kernel
      }
      // pack: high nibble for even x, low nibble for odd x
      const off = y * rowBytes + (x >> 1);
      if ((x & 1) === 0) pixels[off] = g << 4;
      else                pixels[off] |= g;
    }
  }
  const out = new Uint8Array(hdr.length + pixels.length);
  out.set(hdr, 0); out.set(pixels, hdr.length);
  return out; // ~253 KiB
}
```

The dither implementation above is illustrative (a correct Floyd–Steinberg needs two error rows); the point is the device receives a finalized 4-bit frame and does no pixel math.

### 8.4 The `images` JS module

```js
images.count()                      // number of stored .g4 frames
images.name(i)                      // basename of the i-th image
images.display(name)                // blit a stored frame to the panel (sync)
images.remove(name)                 // delete a frame + catalog entry
images.received(function (k, bytes, err) { ... })  // upload completion
```

- `count()`/`name(i)` read `/sdcard/images/index.txt` (owner-only, cached; rescanned on `received`).
- `display(name)` opens `/sdcard/images/<name>`, validates the header, loads the packed pixels into a PSRAM buffer, builds a canvas widget with a bitmap verb, and presents. Sync (a 253 KiB SD read is ~50–100 ms, within the owner budget; if measured slower, move to a worker — see §11 gotcha).
- `received(fn)` registers the completion callback (the mailbox pattern). Fires once per upload.

### 8.5 End-to-end upload flow

```
1. Operator opens http://pulp.local
2. serve static handler streams /sdcard/www/index.html (ESP-53 static path)
3. Operator picks cat.jpg, crops to 540x960, clicks Upload
4. Browser: decode -> resize -> quantize+dither -> pack .g4 (253 KiB)
5. fetch POST /images/upload  (Content-Type: application/octet-stream)
6. httpd ServeUpload: claim slot -> stream to /sdcard/images/<ts>.g4
   -> validate header -> append catalog -> PostModuleDone(Images, Upload, bytes)
   -> respond 200 + name
7. Owner: ModuleDone{Images} -> CallCb(received_cb, kind, bytes, err)
8. Gallery app (if open): received -> refresh list -> operator swipes to new image
9. images.display(name) -> FrameBuilder.Bitmap -> DrawOp::Bitmap -> m5_backend blit
```

---

## 9. Displaying an image — implementing the bitmap blit

This is the core engineering work of Track B. It touches the shared `s3paper_m5` component, so it is sequenced after the network track and gated on host tests.

### 9.1 The frame emitter (`components/s3paper_core/src/frame_builder.cpp`)

Add a `Bitmap()` method to `FrameBuilder`:

```c
// frame_builder.h
// Blit a packed 4-bit grayscale bitmap. `data` is copied into the arena;
// the DrawOp references it by offset (GlyphRun pattern). stride is bytes/row.
Status Bitmap(const Rect &bounds, const uint8_t *data, uint32_t data_len,
              int32_t stride, Gray8 gray_unused = 0);
```

```c
// frame_builder.cpp (sketch)
Status FrameBuilder::Bitmap(const Rect &bounds, const uint8_t *data,
                            uint32_t data_len, int32_t stride, Gray8) {
  // 1. Clip bounds to CurrentClip(); if empty, drop.
  // 2. Copy data into the frame arena (like GlyphRun copies text).
  uint32_t off = arena_.Append(data, data_len);
  DrawOp op{};
  op.kind = DrawOpKind::Bitmap;
  op.bounds = clipped_bounds;
  op.clip = CurrentClip();
  op.bitmap.data_offset = off;
  op.bitmap.data_len = data_len;
  op.bitmap.stride = stride;
  return Emit(op);
}
```

### 9.2 The backend rasterizer (`components/s3paper_m5/src/m5_backend.cpp`)

Replace the skip arm with a real blit. M5GFX has no native 4-bit grayscale blit, so the rasterizer unpacks nibbles to the panel's gray LUT and writes pixels. The efficient strategy (verified against M5GFX's EPD path) is a **row-at-a-time `pushImage` with an RGB565 scratch**, because `M5.Display.pushImage(x, y, w, h, rgb565*)` is the fast M5GFX blit and the panel quantizes 565 internally:

```c
case DrawOpKind::Bitmap: {
  const BitmapPayload &b = op.bitmap;
  // b.stride bytes/row, 2 px/byte, high nibble first.
  // Allocate a one-row RGB565 scratch in PSRAM (540 * 2 = 1080 bytes).
  static uint16_t *row = (uint16_t*)heap_caps_malloc(
      bounds.w * 2, MALLOC_CAP_SPIRAM);
  const uint8_t *data = arena_base + b.data_offset;
  M5.Display.startWrite();
  M5.Display.setClipRect(op.clip.x, op.clip.y, op.clip.w, op.clip.h);
  for (int32_t y = 0; y < bounds.h; y++) {
    const uint8_t *src = data + y * b.stride;
    for (int32_t x = 0; x < bounds.w; x++) {
      uint8_t nib = (x & 1) ? (src[x >> 1] & 0x0F) : (src[x >> 1] >> 4);
      // map 0..15 -> 0..255 gray -> RGB565 gray
      uint8_t g = (nib * 255) / 15;
      row[x] = M5.Display.color565(g, g, g);
    }
    M5.Display.pushImage(bounds.x, bounds.y + y, bounds.w, 1, row);
  }
  M5.Display.clearClipRect();
  M5.Display.endWrite();
  result.ops_drawn++;
  break;
}
```

Notes:
- A full 540×960 blit at one `pushImage` per row (960 calls) is the conservative first implementation. If too slow, collapse to fewer, taller `pushImage` calls (e.g. 60 rows at a time = 16 calls) using a larger PSRAM scratch (60×540×2 = 64 KB, acceptable). Measure first (gotcha #11).
- The clip rect must be honored per pixel (the `GlyphRun` pattern) so a bitmap inside a `canvas()` never scribbles outside its frame.
- Damage: the op's `bounds` becomes the damage rect; the refresh planner may choose a full refresh for a new image (ghosting is worst on large grayscale changes — set `paper.refreshTurns(1)` from the gallery so each image is a clean full).

### 9.3 The canvas verb (`js_widgets.cpp`)

Add a `bitmap(name)` verb to the canvas, mirroring `paint`:

```js
cv.bitmap(name)   // load /sdcard/images/<name>.g4 and queue a Bitmap canvas cmd
```

Because `CanvasCmd` is a fixed 12-byte POD and a bitmap is variable-length, the canvas verb cannot store the pixels in the command list. Instead, `js_w_bitmap` loads the file into a PSRAM buffer held by the images module and records a `CanvasCmd` with a new kind `kBitmap` whose `a,b` store an index into the images module's loaded-image table; the emitter resolves that to a `FrameBuilder::Bitmap` call at present time (like `GlyphRun` resolves text). This keeps the POD canvas contract intact.

Alternatively (simpler, recommended for v1): `images.display(name)` bypasses the canvas entirely and calls `FrameBuilder::Bitmap` directly on a full-page present, exactly like the `ink` app's scenes. The canvas verb is a v2 nicety. **v1 uses the direct path** — simpler, fewer moving parts, and the gallery is full-screen anyway.

### 9.4 The `images.display(name)` path (v1, direct)

```c
// app_images.cpp (sketch)
StatusCode ImagesDisplay(const char *name) {
  AssertOwner();
  char path[64]; snprintf(path, sizeof(path), "/sdcard/images/%s", name);
  FILE *f = fopen(path, "rb");
  // read 12-byte header, validate G4IM + 540x960 + depth 4
  // allocate PSRAM buffer (259200 bytes), fread pixel data
  // build a one-widget page: canvas().bitmap -> OR direct present:
  s3paper_runtime::Arena().Reset();
  // Build a full-page present with a single Bitmap draw op:
  // (use a thin native helper that calls FrameBuilder::Bitmap then PresentPage)
  fclose(f);
  return ok;
}
```

### 9.5 Host test additions (`components/s3paper_core/tests/host/`)

The fake backend (`fake_backend.cpp`) gains a `Bitmap` trace:

```
Bitmap x=.. y=.. w=.. h=.. stride=.. len=..
```

Add a host test that builds a frame with one `Bitmap` op and asserts the trace. This catches emit/clip/damage regressions before any flash (ESP-53 guide §3.2 — host tests are the first gate).

---

## 10. Battery display

### 10.1 The `battery` JS singleton

Extend `js_services.cpp` (or a new `js_battery.cpp` for cleanliness):

```js
battery.level()       // 0..100, -1 unknown   (renamed from batteryLevel() for clarity;
                      //                        keep batteryLevel() as an alias for compat)
battery.mv()          // millivolts, -1 unknown
battery.charging()    // 1 charging, 0 not, -1 unknown
battery.statusText()  // "82%" / "82% +" / "?"
```

All four are sync, owner-only, thin wrappers over `s3paper::PowerRead()`. Add to `pulp_stdlib.c` as a new `battery` singleton (`JS_OBJECT_DEF`); keep `batteryLevel()` as a deprecated alias so existing `pulp.js` calls keep working until regenerated.

### 10.2 The home-screen battery glyph

In `pulp.js::home()`, the header already has a dynamic `wifiGlyph()`. Add a `batteryGlyph()` dyn-value sibling:

```js
function batteryGlyph() {
  var lvl = battery.level();
  if (lvl < 0) { return ''; }
  var ch = battery.charging() === 1 ? ' +' : '';
  return lvl + '%' + ch;
}
// in home():
text(function () { return batteryGlyph() + '  ' + wifiGlyph(); })
  .size('xs').gray(96)
```

Because it is a `text(fn)` dyn value, the home tick (5 s) re-evaluates it and only blits when the string changes — so the battery updates lazily, no polling thread, no extra power. The `PowerAutoTick` low-battery shutdown (already in `app_power.cpp`) keeps running independently.

### 10.3 Console + probe

Add a `bat` console command (`ConsoleOp::Battery`, arg 0 = status print of level/mv/charging) and probe 19 that asserts `battery.level()` is in `[-1, 0..100]` and `battery.charging()` in `[-1, 0, 1]`.

---

## 11. The gotcha catalog (inherited + new)

Inherited from ESP-51/52/53 (still apply): stdlib regeneration protocol; `JS_CLASS_COUNT` consistency; never store a `JSValue` natively (cb ids only); worker tasks never call JS/storage/arena; owner stack 8 KiB (buffers in PSRAM); `sdkconfig.defaults` seeds absent values only (`rm sdkconfig` after enabling mdns); console-client serial discipline; tap targets ≥ ~72 px.

New for ESP-54:

1. **mDNS is a managed component dependency (`espressif/mdns`).** Confirmed absent from the IDF 5.3.4 tree (only `esp_local_ctrl`/`openthread` reference it internally). Add `espressif/mdns` to `0114-papers3-pulp-os/main/idf_component.yml` (per-component manifest; a root manifest is ignored per AGENTS.md) and `mdns` to `REQUIRES` in `main/CMakeLists.txt`. Run `idf.py reconfigure` so `managed_components/mdns` resolves before the first build.
2. **The `Bitmap` op is shared.** Implementing it in `s3paper_m5` changes a component used by other firmwares (0112 reader). Run the full host suite (`make run`) — the fake-backend trace test is the regression gate. The change is additive (a previously-skipped arm now draws), so existing frames are unaffected.
3. **POST body cap.** Enforce `Content-Length <= ~280 KiB` before streaming; a malicious/large upload must not fill the SD card. Also cap total stored images (`images.count()` hard cap, e.g. 64) so the SD does not fill silently.
4. **The upload page is the API.** Because the browser does the conversion, the `.g4` format and the `/images/upload` contract are coupled to the page. Version both (`version` byte in the header; `serve.url()`/`paper.version()` in the page).
5. **SD write on the httpd task is the second sanctioned off-owner SD access.** Document it with the same prominence as ESP-53's static-file read exception. It is file writes only, to `/sdcard/images/`, never to state files.
6. **mDNS hostname collisions.** If two PaperS3s are on the same LAN, both announce `pulp`. v1 accepts this (last one wins in the resolver); a future settings record can suffix the hostname.
7. **Full refresh on image change.** A new image is a large grayscale change; partial updates ghost badly. The gallery sets `paper.refreshTurns(1)` so each image is a clean full refresh. This is slower (≈1 s) but correct.
8. **`images.display` blocking budget.** A 253 KiB SD read + full present is ~150–250 ms. Measure; if it exceeds ~300 ms, move the read to a worker and present on completion (the mailbox pattern) so the owner stays responsive.
9. **Catalog consistency.** `/sdcard/images/index.txt` is the source of truth. If a `.g4` file is deleted out-of-band, `index.txt` drifts. `images.count()` rescans the directory on a miss and rebuilds the index (like `libraryRescan`).
10. **Browser dither cost.** Floyd–Steinberg over 518,400 px in JS is ~50–100 ms; show a progress spinner. Provide a "no dither" toggle for faster upload (nearest-level quantization, more banding).

---

## 12. Implementation phases and acceptance gates

- **P0 Orientation (½ day):** build/flash 0114, confirm ESP-53 connectivity works (`net status`, `serve start`, `curl` the `/status` route). Read this guide + ESP-53 design-doc/02. Gate: `serve.url()` non-empty on your LAN.
- **P1 Battery display (½ day):** `battery` singleton (`level`/`mv`/`charging`/`statusText`), home glyph, `bat` console + probe 19. Gate: home shows `82%`; plugging/unplugging USB toggles `+`; probe 19 green.
- **P2 mDNS (½ day):** `net_mdns.{h,cpp}`, `mdns` JS singleton, wire into `ServeStart`/`ServeStop`/`WifiOff`/power quiesce, `mdns` console + probe 20. Gate: `ping pulp.local` resolves; browser opens `http://pulp.local`; `mdns.url()` shows it; stops on `wifi off`.
- **P3 Upload page + POST route (2 days):** extend `serve` with POST + `ServeUpload`, the `.g4` format, `/sdcard/images/` + catalog, `images` JS module (`count`/`name`/`received`), the browser upload page (crop/scale/quantize/pack/POST). Gate: upload a JPEG from the browser; `images.count()` increments; the `.g4` file appears on the SD; `cat index.txt` lists it; concurrent POST gets 503.
- **P4 Bitmap display (2 days):** `FrameBuilder::Bitmap`, m5 backend rasterizer, host fake-backend trace test, `images.display(name)` direct path. Gate: host suite green; `js probe 21` displays a stored frame on the panel (clean full refresh); a bitmap inside a canvas clip does not overflow.
- **P5 Gallery app (1 day):** `gallery()` in `pulp.js` (list + display + left/right scroll + tap-to-delete), launcher row. Gate: upload 3 images; swipe through all 3; delete one; home glyph persists.
- **P6 Hardening (1 day):** module fault probes (upload during display, display during upload, POST during sleep), 30-min upload+display soak, sleep sequence with mDNS stop, diary/changelog/doctor, host suite green, reMarkable upload. Gate: doctor clean; soak heap flat; sleep wakes cleanly.

---

## 13. Test strategy

1. **Host tests** (`components/s3paper_core/tests/host && make run`) — the first gate for any core/backend change. Add a `Bitmap` trace test; assert `OK (N checks)` with N ≥ current.
2. **Compile gate** — `idf.py build` after each phase.
3. **Console probes** — `js probe 19` (battery), `20` (mdns), `21` (images display), `22` (upload). Each prints PASS/FAIL with evidence.
4. **End-to-end** — the operator flow (browser → upload → gallery → swipe) is the acceptance test; capture a transcript.
5. **Soak** — 30 min of repeated upload+display; `js status` heap lines before/after must be flat (no leak in the PSRAM bitmap buffer — free it after every present).
6. **Serial discipline** — use the console client (`52-papers3-console-client.py`), never `idf.py monitor` (modem control resets the device — ESP-53 guide §12.1).

---

## 14. Risks, alternatives, and open questions

- **Resolved: mDNS is not in the IDF 5.3.4 tree.** It is the managed component `espressif/mdns` (confirmed via Kagi + filesystem check). Added to `main/idf_component.yml` + `REQUIRES`. No fallback needed; the component resolves cleanly.
- **Risk: backend bitmap blit too slow.** A 960-row `pushImage` loop may exceed 1 s. Mitigation: larger PSRAM scratch, fewer/taller blits; measure before optimizing.
- **Risk: SD card write contention.** The httpd write + a concurrent `files` op could contend. Mitigation: the single upload slot serializes uploads; `files` ops are owner-only and brief. Document.
- **Alternative: device-side decode.** Rejected (R-IMGDECODE) — revisit only if the browser path proves unusable on the operator's target browsers.
- **Open: crop UX.** A full crop/drag/zoom UI in raw canvas JS is fiddly. v1 can ship "contain" + "fill" buttons (no free drag) and add drag later.
- **Open: image rotation.** Portrait phone photos may upload sideways. The browser can read EXIF orientation; v1 should honor it (one `canvas` transform).
- **Open: catalog as a state file.** `/sdcard/images/index.txt` is a plain file, not in the s3paper_storage family. Acceptable for v1; a future `IMGX` state file with CRC + fault injection would match the credentials discipline.

---

## 15. References (key files)

| Concern | File |
|---|---|
| serve handoff (template for POST) | `0114/main/net_serve.{h,cpp}` |
| serve JS bindings (template) | `0114/main/js_serve.cpp` |
| wifi module + mailbox pattern | `0114/main/net_wifi.{h,cpp}` |
| power + battery read + quiesce | `0114/main/app_power.cpp`, `components/s3paper_m5/include/s3paper_m5/m5_power.h` |
| battery JS (level only, extend) | `0114/main/js_services.cpp` (`js_pulp_battery_level`) |
| DrawOp kinds + BitmapPayload (declared) | `components/s3paper_core/include/s3paper/draw_ops.h` |
| Bitmap skip arm (implement) | `components/s3paper_m5/src/m5_backend.cpp:402` |
| FrameBuilder (add Bitmap) | `components/s3paper_core/include/s3paper/frame_builder.h`, `src/frame_builder.cpp` |
| canvas widget + verbs | `components/s3paper_core/include/s3paper/widget.h`, `0114/main/js_widgets.cpp` |
| product app (add Gallery + glyph) | `0114/tools/js/apps/pulp.js` |
| stdlib API surface (extend) | `0114/tools/js/pulp_stdlib.c` |
| event/snapshot/op contracts | `0114/main/app_events.h` |
| owner loop (dispatch + tick) | `0114/main/app_owner.cpp` |
| console registration (mirror) | `0114/main/app_console.cpp` |
| build config + partition | `0114/sdkconfig.defaults`, `0114/partitions.csv`, `0114/main/CMakeLists.txt` |
| ESP-53 system onboarding (read first) | `ttmp/2026/07/16/ESP-53-PULP-CONNECTIVITY--*/design-doc/02-*.md` |
| ESP-53 connectivity design | `ttmp/2026/07/16/ESP-53-PULP-CONNECTIVITY--*/design-doc/01-*.md` |

---

## 16. Glossary (ESP-54 additions)

- **`.g4`** — the 4-bit packed grayscale frame format (12-byte header + 2 px/byte pixel data). The wire, storage, and display format.
- **`pulp.local`** — the mDNS hostname the device advertises when serving.
- **upload slot** — the single in-flight POST claim (Busy → 503), the POST analog of ESP-53's request slot.
- **catalog** — `/sdcard/images/index.txt`, one image basename per line; the source of truth for `images.count()`/`name(i)`.
- **bitmap blit** — the `DrawOpKind::Bitmap` rasterization in the M5 backend; the display path for stored images.
- **completion mailbox (images)** — the `{name, bytes, err}` POD result the httpd POST worker fills before posting `ModuleDone{Images}`; the owner drains it into the `received` callback.

---

*Companion documents: this ticket's `reference/01-investigation-diary.md` (chronological build log), the ESP-53 onboarding guide and connectivity design (read first), and the ESP-51 intern guide (engine + binding rules).*
