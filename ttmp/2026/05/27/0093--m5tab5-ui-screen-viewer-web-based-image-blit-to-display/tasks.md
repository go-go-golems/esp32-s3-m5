# Tasks

## Phase 1: Fork and scaffold

- [x] Copy 0051-tab5-boot-logo → 0093-tab5-ui-screen-viewer
- [x] Update CMakeLists.txt project name
- [x] Strip boot logo assets and display_app.c — replace with black screen + SPIRAM screen buffer + LVGL image object
- [x] Update sdkconfig.defaults AP SSID to Tab5-UI-Viewer
- [x] Build, flash, verify black screen + WiFi + console

## Phase 2: Raw RGB565 upload endpoint

- [x] Add POST /api/upload handler accepting raw binary RGB565 body (1,843,200 bytes max)
- [x] Allocate SPIRAM receive buffer, copy pixels, update LVGL image, invalidate
- [x] Add GET /api/screen endpoint (resolution, format, current state)
- [x] Add POST /api/clear endpoint (fill black)
- [x] Build, flash, test with curl — verify image renders on display

## Phase 3: Browser upload UI

- [x] Write index.html with drag-drop zone + file picker
- [x] Write app.js: load image → canvas resize to 1280×720 → RGBA→RGB565 conversion → POST binary
- [x] Add upload progress indicator and status display
- [x] Add clear screen button
- [x] Test full flow: drag PNG → display shows image

## Phase 4: Polish and hardening

- [x] Thread safety: wrap LVGL updates in lvgl_port_lock/unlock
- [x] Error handling: reject oversized/invalid uploads gracefully
- [x] Fix: UTF-8 chars in embedded assets (IDF assembly corrupts multi-byte UTF-8)
- [x] Fix: NUL terminator in EMBED_TXTFILES (strip in HTTP handler)
- [x] Fix: has_image field in /api/screen (use display_app_has_image instead of s_server)
- [x] Fix: HTTP recv_wait_timeout (5s → 30s for large uploads)
- [ ] Update README with build/flash/usage instructions
- [ ] Verify RGB565 byte order on actual display (visual color check)
- [ ] Test with real PNG/JPG drag-drop from a non-Playwright browser
- [ ] Test with non-solid-color image (gradient/photo)
