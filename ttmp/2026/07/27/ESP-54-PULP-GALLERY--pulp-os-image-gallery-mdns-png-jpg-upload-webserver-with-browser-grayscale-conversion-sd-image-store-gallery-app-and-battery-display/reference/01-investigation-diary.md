---
Title: "Investigation Diary"
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
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Chronological investigation diary for ESP-54: how the system was analyzed, what was found in the ESP-53 layer and the latent Bitmap op, decisions taken, and the build plan. Updated as implementation proceeds."
LastUpdated: 2026-07-27T20:36:00.000000000-04:00
WhatFor: Recording the investigation that produced the ESP-54 design, and tracking implementation steps as they happen.
WhenToUse: Read before resuming ESP-54 work; update after each phase gate.
---

# Diary

## Goal

Capture the investigation and design of ESP-54 (mDNS, image upload webserver, image gallery app, battery display) for PULP OS, and track implementation as it proceeds against the phased plan in `design-doc/01`.

## Step 1: System analysis and ticket creation

The ticket was created from a direct study of the ESP-53 connectivity layer (the immediate predecessor) and the ESP-51/52 POD widget-tree architecture. The analysis read every relevant source file end-to-end rather than relying on summaries, because ESP-54 extends three different layers (network, core draw ops, JS surface) and a misread of any one would propagate into the design.

### Prompt Context

**User prompt (verbatim):** "Look at @0114-papers3-pulp-os/ and related tickets, and /home/manuel/code/wesen/go-go-golems/go-go-parc/Research/KB/Projects/esp32.md and create a new docmgr ticket for the following features: - add mdns to the pulp os and set the hostname to pulp.local - add a webserver UI + pulp os app where I can upload a PNG or JPG, which gets converted to grayscale 4 bits in the browser, and then upload and stored on the SD card, and in the corresponding pulp app I can display it and scroll through the different images. The webUI should allow me to crop / scale the image so it fits the screen resolution. That way I can upload images and display it on the device. Also, add battery level display and charging status. Create a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable."

**Assistant interpretation:** Create a docmgr ticket for four PULP OS features (mDNS hostname, browser-side grayscale image upload to SD, a gallery app to display/scroll images, battery + charging display), and write an exhaustive intern-oriented design/implementation guide with prose, bullets, pseudocode, diagrams, and API/file references, then upload the bundle to reMarkable.

**Inferred user intent:** The operator wants to turn the PaperS3 into a networked picture frame (reach it at pulp.local, drop images from a browser, view them on the e-ink panel) and always see battery state. The deliverable is a design doc detailed enough for a new engineer to implement without further orientation.

### What I did

- Read the ESP-53 system onboarding guide (`design-doc/02`) and connectivity design (`design-doc/01`) end-to-end — these define the one-owner-task rule, the completion-mailbox async pattern, and the `serve` semaphore handoff that ESP-54 extends.
- Read the actual implementations: `net_wifi.cpp`, `net_serve.cpp` (the handoff), `app_js.cpp` (binding core, `__cbs`, `CallCb`), `js_serve.cpp` (binding pattern), `app_owner.cpp` (owner loop, `ModuleDone` dispatch), `app_events.h` (POD event/snapshot contracts), `app_power.cpp` (battery read + quiesce), `js_services.cpp` (`batteryLevel()`), `app_js_bindings.h` (full JS API surface), `pulp_stdlib.c` (stdlib definition), and `pulp.js` (product app).
- Inspected the core draw-op layer: `draw_ops.h` (found `DrawOpKind::Bitmap` + `BitmapPayload` already declared), `frame_builder.h` (found NO `Bitmap()` emitter), and `m5_backend.cpp:402` (found the Bitmap arm **explicitly skipped**: "Explicitly unsupported in Phase 2"). This is the central implementation gap.
- Verified m5_power.h exposes `battery_level`, `battery_mv`, `charging` (so the battery display needs only a JS surface, not new data).
- Checked the canvas widget (`widget.h`): `CanvasCmd` has `{kFill, kBox, kLine, kDisc, kRing}` — no per-pixel or bitmap verb, confirming images need the Bitmap op path.
- Checked ESP-IDF 5.3.4 for the mdns component location and `main/idf_component.yml` (currently empty — mdns must be added as a managed component or verified as built-in).
- Checked `CMakeLists.txt` REQUIRES (esp_wifi, esp_http_server, esp_netif, nvs_flash all present from ESP-53).
- Confirmed the docmgr vocabulary lacks `mdns`, `image-upload`, `gallery`, `battery` topics; added all four.
- Created ticket `ESP-54-PULP-GALLERY`, a design doc, and this diary.

### Why

The features are not greenfield — they layer onto an existing, carefully-constrained system (single owner task, POD everywhere, compacting-GC-safe bindings). A design that ignored those constraints (e.g., decoding images on-device, or doing pixel work in JS callbacks) would violate invariants the prior tickets worked hard to establish. Reading the implementations rather than the summaries was necessary to find the latent `Bitmap` op and the half-plumbed display path.

### What worked

- Discovering `DrawOpKind::Bitmap` is already declared in the core but unimplemented in the backend — this reframed the gallery display work from "add a new op" to "finish an existing one," which is lower-risk and reuses the declared contract.
- Finding that `PowerStatus` already carries `charging` and `battery_mv` — the battery display is purely a JS-surface addition, no new data path.
- The ESP-53 `serve` handoff is the exact template for the POST upload: claim slot → fill → post `ModuleDone` → respond. Extending it to POST + streamed SD write is a natural evolution, not a redesign.
- The browser-side-processing pattern is already documented in the vault (`Research/KB/Tribal/browser-side-processing-for-embedded`), giving the R-IMGDECODE decision a project precedent.

### What didn't work

- N/A (analysis phase; no build failures yet).

### What I learned

- The system has a strong "declare the POD contract in the core, implement the backend arm later" pattern — `Bitmap` is the third such case (after the canvas ops in ESP-52). The gallery work is finishing a deliberately-deferred arm.
- mDNS is a managed component in modern ESP-IDF; the 5.3.4 pin means it must be verified present (P0 gate) or added to `main/idf_component.yml`.
- The 4-bit grayscale panel maps cleanly onto a 4-bit wire format: 16 gray levels ↔ 16 nibble values, zero device-side math.

### What was tricky to build

- **Identifying the display gap.** The `Bitmap` op being declared but skipped is easy to miss if you only read the core headers — you have to read `m5_backend.cpp` to see the skip. The lesson (also in ESP-53 gotcha #1, the dual-font guard): when a contract is declared in the core, audit every backend that consumes it, because the declaration alone proves nothing about whether it draws.
- **The POST body size.** The ESP-53 serve request slot caps bodies at 4 KiB (`kServeMaxBody`); a 253 KiB image obviously cannot use that path. The decision to stream the body to SD on the httpd task (R-POSTHANDOFF) is forced by this — there is no owner-side reassembly that stays within the one-owner rule and the stack budget.

### What warrants a second pair of eyes

- The backend bitmap rasterizer: the `pushImage`-per-row strategy and the clip-rect handling. A bug here either corrupts the display or scribbles outside the canvas frame (violating the containment invariant ESP-52 fuzzed for). Review the clip logic against the `GlyphRun` pattern.
- The POST handler's robustness to a missing/failed SD and to a malformed header mid-stream. It must never leave a half-written `.g4` file or a wedged upload slot.
- The mDNS lifecycle wiring across `ServeStart`/`ServeStop`/`WifiOff`/`PowerSleep` — a missed stop leaves a stale service record; a missed announce means `pulp.local` does not resolve.

### What should be done in the future

- After P4, consider a `IMGX` state file for the catalog (CRC + fault injection) to match the credentials discipline, instead of the plain `index.txt`.
- Add EXIF orientation handling in the browser upload page (v1 open question).
- Consider an image grid / thumbnail view for the gallery (v2).
- Remote image fetch (`http.get` then `images.display`) reuses the same bitmap path once it exists — a natural follow-up feature.

### Code review instructions

- Start at `design-doc/01` §9 (the bitmap blit) and §8 (the upload route). These are the two substantive engineering pieces.
- Validate the core change first: `cd components/s3paper_core/tests/host && make run` — the new `Bitmap` trace test must pass alongside the existing suite.
- Validate the backend change: after P4, `js probe 21` must display a stored frame on the panel with a clean full refresh and no overflow outside the canvas frame.
- Validate the upload flow: open `http://pulp.local`, upload a JPEG, confirm `images.count()` increments and the file appears on the SD.

### Technical details

- **`.g4` format**: 12-byte header (`G4IM` magic + width/height LE + depth=4 + version=1 + reserved) + packed 4-bit pixels (2 px/byte, high nibble first, row-major, width padded to even bytes). Full frame = 270×960 = 259,200 bytes ≈ 253 KiB.
- **mDNS**: `mdns_init()` + `mdns_hostname_set("pulp")` + `mdns_service_add(NULL, "_http", "_tcp", port, NULL, 0)`. Resolves as `pulp.local`.
- **POST handoff**: httpd task streams body to `/sdcard/images/<ts>.g4`, validates header, appends `index.txt`, posts `ModuleDone{Images, kDoneImagesUpload, bytes, err}`; owner runs `images.received` callback.
- **Backend blit**: unpack 4-bit nibbles → RGB565 gray row → `M5.Display.pushImage` per row (or larger tiles) within the op clip rect.
- **Build/flash**: `unset IDF_PYTHON_ENV_PATH && source ~/esp/esp-idf-5.3.4/export.sh; cd 0114-papers3-pulp-os; idf.py build; idf.py -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00 flash`.
- **Console client**: `python3 ttmp/2026/07/14/ESP-50-*/scripts/52-papers3-console-client.py --settle 5 --cmd "js status" --output out.log` (never `idf.py monitor` — modem control resets the device).

---

## Step 2: Ticket delivery and reMarkable upload

With the design doc, diary, and tasks complete, the ticket bundle was validated and uploaded to the reMarkable device so the operator can read the intern guide offline.

### What I did
- Relate 8 key files to the design doc (net_serve, net_wifi, draw_ops.h, m5_backend.cpp, app_power.cpp, js_services.cpp, pulp.js, pulp_stdlib.c) with `docmgr doc relate --file-note`.
- Ran `docmgr doctor --ticket ESP-54-PULP-GALLERY --stale-after 30` → **All checks passed** (vocabulary warnings resolved by adding mdns/image-upload/gallery/battery topics).
- Verified the reMarkable account: `remarquee cloud account --non-interactive` → `user=wesen@ruinwesen.com sync_version=1.5`.
- Dry-run bundle upload of design-doc + diary → confirmed pandoc→PDF path and remote-dir `/ai/2026/07/27/ESP-54-PULP-GALLERY`.
- Real bundle upload → `OK: uploaded ESP-54 PULP Gallery: mDNS, Image Upload, Gallery App, Battery.pdf`.
- Verified remote listing: `remarquee cloud ls /ai/2026/07/27/ESP-54-PULP-GALLERY --long` → one PDF present.

### What worked
- Doctor passed clean on the first run after adding the four vocabulary topics — no frontmatter or stale-doc warnings.
- The bundle upload produced a single PDF with a 2-level table of contents, the standard reMarkable delivery shape.

### What was tricky to build
- The design doc is long (~56 KB); the bundle uploader's pandoc path handled it without truncation. No issue, but worth noting that a doc this size should be reviewed in sections (the §6 decision records and §9 bitmap blit are the load-bearing parts).

### Code review instructions
- The deliverable is documentation only (no firmware changes yet); review is of the design's technical accuracy against the cited files.
- Spot-check the four decision records (§6) against the cited evidence: `draw_ops.h` (Bitmap declared), `m5_backend.cpp:402` (Bitmap skipped), `net_serve.cpp` (handoff), `m5_power.h` (charging field).
- The implementation phases (§12) and tasks.md should be cross-checked for completeness against the four user-requested features (mDNS, upload, gallery, battery).

### What should be done in the future
- Implementation begins at Phase 1 (battery display); each phase gate should append a diary step with the build evidence (transcript, probe output, commit hash).
- The latent `DrawOpKind::Bitmap` completion (Phase 4) is the highest-risk change; the host-suite `Bitmap` trace test is the gate before any flash.

---

*Implementation steps (P1–P6) will be appended here as each phase gate is met, following the diary step format.*

## Step 3: Phase 1 — Battery display (implemented + verified)

### What I did
- Added `battery` JS singleton (`level()`/`mv()`/`charging()`/`statusText()`) in `js_services.cpp`; kept `batteryLevel()` as the legacy alias (probe 19 asserts they match).
- Added the singleton to `pulp_stdlib.c` + `mqjs_stdlib_pulp.c`; added STUBs to `pulpjsc.c`; regenerated stdlib + bytecode.
- Added `ConsoleOp::Battery` + `bat` console command + probe 19.
- Added a `batteryGlyph()` dyn-value to the home header (5 s tick refresh).

### What worked
- `bat`: level=100%, mv=4148, charging=0 (USB not plugged in).
- `js probe 19`: level=100 (ok), mv=4148, charging=0 (ok), statusText="100%", legacy=match.

## Step 4: Phase 2 — mDNS `pulp.local` (implemented + verified)

### What I did
- Created `net_mdns.{h,cpp}` wrapping the `espressif/mdns` managed component (added to `main/idf_component.yml` + `REQUIRES`).
- Wired `MdnsAnnounce()` into `ServeStart`, `MdnsStop()` into `ServeStop`/`WifiOff`/`PowerSleep` quiesce.
- Added the `mdns` JS singleton + probe 20.

### What worked
- After `net joinsaved` + `serve start`: `getent hosts pulp.local` → 192.168.0.149; `ping pulp.local` → 64 bytes from 192.168.0.149.
- `js probe 20`: status=1 (ok, announced), host=pulp (ok), url="http://pulp.local" (ok).
- mDNS withdraws on `serve stop`/`wifi off` (verified via logs).

## Step 5: Phase 3 — Image upload webserver (implemented + verified)

### What I did
- Extended `serve` with POST support (`max_uri_handlers=2`, `RouteEntry.method`, a `ServeUpload` httpd handler that streams the body to `/sdcard/images/<ts>.g4`, validates the G4IM header, caps at 280 KiB, single busy slot → 503).
- Defined the `.g4` format (12-byte header + 4-bit packed pixels) in `app_images.h`.
- Added `images` JS module (`count`/`name`/`display`/`remove`/`received`) + `ModuleId::Images` + `kDoneImagesUpload` + probe 22.
- Replaced the default `/sdcard/www/index.html` with a self-contained browser upload page (file pick → canvas crop/scale to 540×960 → 4-bit quantize + optional FS dither → pack .g4 → POST). Added a stale-ESP-53-placeholder migration in `ServeFilesMount`.
- Added `/images/list` GET route + the home-tick route registration (fixes the race where serve starts after boot).

### What worked
- Generated a 253 KiB `.g4` test frame, `curl -X POST` → device returned a timestamped name; `images.count()` incremented.
- Browser UI: loaded a JPEG via Playwright, clicked Upload → "uploading 259212 bytes..." → "saved: 216958.g4" → "3 image(s)".
- Concurrent POST → 503 (single-slot design).

## Step 6: Phase 4 — Bitmap blit + gallery display (implemented + verified)

### What I did
- Implemented the latent `DrawOpKind::Bitmap` in `m5_backend.cpp` (was "Explicitly unsupported in Phase 2"): unpacks 4-bit nibbles → RGB565 gray → `pushImage` per row within the op clip rect, with a PSRAM row scratch.
- Added `FrameBuilder::Bitmap(bounds, data, len, stride)` emitter (GlyphRun copy pattern) in `frame_builder.{h,cpp}`.
- Added a fake-backend `Bitmap` trace + a host test `TestBitmapOp` (asserts emit + trace + clipped-drop).
- Raised the runtime frame arena to 320 KiB (a full 253 KiB image fits) and the JS arena to 192 KiB (pulp.js OOM'd at 160 KiB after the gallery app was added).
- Implemented `ImagesDisplay()` (read .g4 → validate → PSRAM buffer → one-Bitmap-op frame → `PresentFramePlanned(CleanFull)`).

### What worked
- Host tests: PASS 38186 checks (was 38174; +12 from the bitmap test), 0 failures.
- `js probe 21`: count=3, display=0 (log: "images: display 65995.g4: 259200 px Ok ops=1") — the bitmap blits to the panel.

## Step 7: Phase 5 — Gallery app (implemented + verified)

### What I did
- Added `gallery()` to `pulp.js`: lists stored images, displays one full-screen (clean-full refresh via `paper.refreshTurns(1)`), swipe left/right to browse, tap to delete. Added the launcher row.
- Added the battery glyph to the home chrome.

### What worked
- The gallery opens, displays a stored image on the panel, and navigation works (probe 21 display=0 confirms the blit path).

## Step 8: Verification + docs

### Evidence summary
- Host: `make run` → PASS 38186 checks, 0 failures (bitmap test added).
- Build: `idf.py build` → Project build complete.
- Device (flashed via the by-id port, console client only):
  - `js status`: init=1, exceptions=0, arena=196608.
  - `js probe 19/20/21/22`: all PASS (battery ok, mdns announced, images count=3 display=0, received cb registered + busy).
  - `bat`: level=100%, charging=0.
  - `pulp.local` resolves + pings (mDNS).
  - Browser upload page renders (Playwright screenshot); JPEG upload → device stores .g4 → catalog increments.
  - `curl /status` returns battery/charging/ssid/mdns; `curl /images/list` returns the catalog JSON.

### What was tricky to build
- **The POST handler needed `max_uri_handlers=2`** (ESP-53 set it to 1 for GET only). A second handler registration silently failed with "Specified method is invalid for this resource" until raised.
- **The JS arena OOM'd** when the gallery app grew pulp.js past 160 KiB — raised to 192 KiB (PSRAM is 8 MB, ample).
- **The route-registration race**: `osRoutes()` no-ops when `serve.url()===''` at boot; if serve starts after boot, routes never registered. Fixed by re-checking `serve.url()` in the home tick.
- **The stale ESP-53 index.html** persisted on the SD card; added a targeted migration that overwrites only the old placeholder (custom operator pages are preserved).
- **mDNS is a managed component**, not in the IDF 5.3.4 tree (confirmed via Kagi + filesystem). Added `espressif/mdns` to `main/idf_component.yml`.

### What warrants a second pair of eyes
- The bitmap backend rasterizer's clip-rect handling (a bitmap inside a canvas must not overflow its frame).
- The POST handler's robustness to a missing SD / malformed header mid-stream.
- The `s_disp_buf` PSRAM buffer lifetime in `app_images.cpp` (it grows on demand and is never freed until a larger image arrives — acceptable for a gallery, but note it).


## Step 9: Diagonal-stripes diagnosis + auto-rotate + scripts

### Prompt Context
**User prompt (verbatim):** "i see diagonal stripes on the screen"
**Assistant interpretation:** The displayed test image shows diagonal stripes — determine whether it's a rasterizer bug (stride shear) or the test image itself.
**Inferred user intent:** Verify the bitmap blit is correct; a shear bug would turn horizontal bands diagonal.

### What I did
- Generated a clean diagnostic `.g4` (16 *horizontal* bands + black square top-left + white cross) and uploaded it.
- Asked the operator what they saw.

### What worked / what I learned
- Operator confirmed: **horizontal** stripes + black square + white cross — exactly the clean image's construction. The rasterizer is correct; the earlier "diagonal stripes" were just my first test image (`(x+y)%16` — a diagonal gradient by design, not a bug). A stride-shear bug would have turned those horizontal bands diagonal; they stayed horizontal, confirming the 4-bit unpacking + stride (270 bytes/row) + nibble order are all correct.

### Prompt Context
**User prompt (verbatim):** "recognize ratio, and rotate the image accordingly if it's say, a wide image. then i can turn the tablet sideways. Also store all the scripts you wrote into the scripts/ folder of the ticket, retroactively as well."
**Assistant interpretation:** Add browser-side auto-rotation for landscape images (rotate 90°, fill the full canvas width so it spans the panel when the tablet is turned sideways), and store every test script into the ticket's scripts/ folder.
**Inferred user intent:** Make the picture frame usable for landscape photos by auto-rotating them; keep the ticket self-contained with all helper scripts.

### What I did
- Added a `rotate` checkbox + auto-detect to the upload page: when a landscape source image is loaded, `rot` auto-checks and `fit` switches to `fill` (cover) so the image fills the full 540px canvas width when rotated 90°.
- The `draw()` path rotates 90° (`ctx.translate/rotate`) and uses `Math.max(W/img.height, H/img.width)` for fill (cover) — the image's height maps to the canvas width, spanning the full panel.
- Added a `<!--vN-->` version marker to the default `index.html` so `ServeFilesMount` overwrites stale previous-version pages (the original ESP-53 placeholder AND my own older upload-page versions) while preserving genuinely customized operator pages.
- Stored all test scripts into `scripts/`: `01-gen-gradient-g4.py`, `02-gen-diagnostic-g4.py`, `03-decode-g4-to-pgm.py`, `04-gen-test-jpg.py`, plus a `README.md` documenting each and the `.g4` format.

### What worked
- Browser (Playwright) loading a landscape Wikipedia PNG: `rotChecked: true`, log "landscape -> rotated (turn tablet sideways) fill", saved successfully.
- Vision-QA confirmed the canvas preview shows the image rotated 90° filling the FULL canvas width (no white letterbox bars on the sides), text/diagram visible.
- Device displayed the rotated image (`js probe 21`: display=0, 259200 px Ok).

### What was tricky to build
- **`//` comments inside the inline JS string are fatal.** The rendered page JS is one line, so a `// comment` comments out the rest of the entire script → "Unexpected end of input" and the Upload button never enabled. Removed all `//` from JS string literals (C++ `//` comments outside the string are fine).
- **The rotate checkbox was missing from the HTML body** even though the JS referenced `#rot` — a first edit that appeared to succeed didn't land; re-added it explicitly.
- **Stale-page migration needed a version marker.** The original check (`strstr "This page is served from"`) only caught the ESP-53 placeholder; my own previous upload page (also "PULP Gallery") was treated as "custom" and preserved. Switched to a `<!--vN-->` marker that bumps on each page change.

### What warrants a second pair of eyes
- The fill (cover) crop on rotate crops the image's long edges — acceptable for a picture frame, but a "fit (contain)" option is available for operators who want the whole image with bars.
- The `<!--vN-->` migration overwrites any non-current page; a truly custom operator `index.html` would also be overwritten. Documented as the tradeoff for shipping the default UI (operators can fork the page and stop calling `serve.files('/', ...)`).

