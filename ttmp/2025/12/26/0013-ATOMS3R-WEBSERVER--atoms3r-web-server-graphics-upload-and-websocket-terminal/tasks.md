# Tasks

This is the implementation checklist for ticket `0013-ATOMS3R-WEBSERVER`.

## 0. Ground rules / acceptance criteria (read first)

- [ ] **MVP demo**: AtomS3R starts WiFi (SoftAP), serves the web UI, accepts PNG upload, renders it on the display, and streams button + UART RX to the browser over WebSocket while accepting UART TX from the browser.
- [ ] **Repo fit**: The firmware project is ESP-IDF-first (matches `esp32-s3-m5` tutorial style) and reuses known-good AtomS3R display bring-up patterns from existing tutorials.
- [ ] **No giant RAM buffers**: Upload handler streams request body to disk (FATFS) via repeated reads (no “read whole upload into RAM”).
- [ ] **Pins are configurable**: UART TX/RX and button GPIO are menuconfig options with sensible defaults.

## 1. Repo investigation checkpoints (do early)

- [x] Validate display init + present pattern to reuse:
  - `esp32-s3-m5/0013-atoms3r-gif-console/main/display_hal.cpp`
- [x] Validate FATFS mount strategy options:
  - prebuilt RO mount: `esp32-s3-m5/components/echo_gif/src/gif_storage.cpp` (`esp_vfs_fat_spiflash_mount_ro`)
  - upload RW mount: use `esp_vfs_fat_spiflash_mount_rw_wl` (ESP-IDF API)
- [ ] Validate GROVE UART pin mapping on *your* AtomS3R revision (known ambiguity):
  - start with defaults: TX=GPIO1, RX=GPIO2 (see `0013-atoms3r-gif-console/main/Kconfig.projbuild`)
- [ ] Validate button GPIO on your revision (default often 41; configurable)

## 2. Firmware project scaffolding (new tutorial project)

- [x] Create new project directory (proposed): `esp32-s3-m5/0017-atoms3r-web-ui/`
- [x] Add `CMakeLists.txt`, `main/CMakeLists.txt`, `main/Kconfig.projbuild`, `sdkconfig.defaults`
- [x] Add `partitions.csv` including a `storage` FATFS partition sized for uploads
- [x] Reuse M5GFX component wiring approach (EXTRA_COMPONENT_DIRS consistent with existing tutorials)

## 3. WiFi bring-up

- [ ] Implement SoftAP mode first (SSID like `AtomS3R-WebUI-<suffix>`)
- [ ] Print IP/SSID instructions on serial log at boot
- [ ] (Optional) Add STA mode behind menuconfig (SSID/password) for later

## 4. Storage (FATFS RW for uploads)

- [ ] Mount `/storage` using `esp_vfs_fat_spiflash_mount_rw_wl("/storage", "storage", &cfg, &wl_handle)`
- [ ] Ensure `/storage/graphics` exists (create on first boot)
- [ ] Implement safe filename validation (no `..`, no `/`, enforce max length)
- [ ] Enforce max upload size (menuconfig; start with e.g. 256KB)

## 5. Display subsystem

- [ ] Integrate display init from `display_hal.cpp` (GC9107 via M5GFX/LovyanGFX)
- [ ] Create `M5Canvas` and a `display_present_canvas(...)` equivalent (DMA wait pattern)
- [ ] Implement `display_png_from_file(path)` (MVP: read file into memory with size cap, call M5GFX PNG draw)
- [ ] Define “what gets displayed when”:
  - [ ] default: newest upload auto-displays
  - [ ] (optional) endpoint to select which file to display

## 6. HTTP server (`esp_http_server`)

- [ ] Start server with `httpd_start(&hd, &HTTPD_DEFAULT_CONFIG())`
- [ ] Serve frontend assets:
  - [ ] `GET /` → `index.html`
  - [ ] `GET /assets/app.js`
  - [ ] `GET /assets/app.css`
  - [ ] correct `Content-Type` headers
  - [ ] (optional) gzip + `Content-Encoding: gzip`
- [ ] Implement REST endpoints:
  - [ ] `GET /api/status` (JSON)
  - [ ] `GET /api/graphics` (JSON list)
  - [ ] `PUT /api/graphics/<name>` (raw body upload streamed to FATFS)
  - [ ] `GET /api/graphics/<name>` (download)
  - [ ] `DELETE /api/graphics/<name>` (delete)

## 7. WebSocket (`/ws`)

- [ ] Register WS handler (`httpd_uri_t` with `is_websocket = true`)
- [ ] Track connected WS client fds (add/remove on connect/close)
- [ ] Device → browser:
  - [ ] Button events as JSON text frames
  - [ ] UART RX as binary frames
- [ ] Browser → device:
  - [ ] UART TX as binary frames

## 8. UART terminal

- [ ] Add menuconfig for UART num, TX GPIO, RX GPIO, baudrate
- [ ] Install UART driver + ring buffers (`uart_driver_install`, `uart_param_config`, `uart_set_pin`)
- [ ] UART RX task: `uart_read_bytes` → WS broadcast (binary)
- [ ] WS RX handler: binary → `uart_write_bytes`

## 9. Button input

- [ ] Configure GPIO + ISR (or polling) with debounce strategy
- [ ] ISR → queue → WS broadcast pattern (avoid heavy work in ISR)
- [ ] Verify no conflicts with UART pins / display pins

## 10. Frontend (Preact + Zustand)

- [ ] Create `web/` app (Vite + Preact + Zustand)
- [ ] Configure deterministic output names (no hash) so firmware routes are stable
- [ ] Implement WS store:
  - [ ] connect/reconnect
  - [ ] binary RX handling (terminal)
  - [ ] JSON text handling (button/status)
- [ ] Implement upload flow using raw-body PUT:
  - [ ] `fetch(PUT /api/graphics/<name>, body=file)`
  - [ ] show progress (optional)
- [ ] Terminal UI:
  - [ ] display RX stream
  - [ ] send typed bytes on Enter
- [ ] Build pipeline to embed assets into firmware (CMake `EMBED_FILES`/`EMBED_TXTFILES`)

## 11. Testing / verification playbook

- [ ] Flash firmware, connect to AP, load `/`
- [ ] Upload PNG and verify display updates
- [ ] Press button and verify browser event
- [ ] Loopback UART (or connect device) and verify RX/TX in browser
- [ ] Reboot device and confirm uploaded graphics persist (FATFS RW)


