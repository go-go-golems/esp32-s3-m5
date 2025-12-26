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

- [x] Implement SoftAP mode first (SSID like `AtomS3R-WebUI-<suffix>`)
- [x] Print IP/SSID instructions on serial log at boot
- [x] (Optional) Add STA mode behind menuconfig (SSID/password) for later

## 4. Storage (FATFS RW for uploads)

- [x] Mount `/storage` using `esp_vfs_fat_spiflash_mount_rw_wl("/storage", "storage", &cfg, &wl_handle)`
- [x] Ensure `/storage/graphics` exists (create on first boot)
- [x] Implement safe filename validation (no `..`, no `/`, enforce max length)
- [x] Enforce max upload size (menuconfig; start with e.g. 256KB)

## 5. Display subsystem

- [x] Integrate display init from `display_hal.cpp` (GC9107 via M5GFX/LovyanGFX)
- [x] Create `M5Canvas` and a `display_present_canvas(...)` equivalent (DMA wait pattern)
- [x] Implement `display_png_from_file(path)` (MVP: read file into memory with size cap, call M5GFX PNG draw)
- [ ] Define “what gets displayed when”:
  - [x] default: newest upload auto-displays
  - [ ] (optional) endpoint to select which file to display

## 6. HTTP server (`esp_http_server`)

- [x] Start server with `httpd_start(&hd, &HTTPD_DEFAULT_CONFIG())`
- [x] Serve frontend assets:
  - [x] `GET /` → `index.html`
  - [x] `GET /assets/app.js`
  - [x] `GET /assets/app.css`
  - [x] correct `Content-Type` headers
  - [ ] (optional) gzip + `Content-Encoding: gzip`
- [ ] Implement REST endpoints:
  - [x] `GET /api/status` (JSON)
  - [x] `GET /api/graphics` (JSON list)
  - [x] `PUT /api/graphics/<name>` (raw body upload streamed to FATFS)
  - [x] `GET /api/graphics/<name>` (download)
  - [x] `DELETE /api/graphics/<name>` (delete)

## 7. WebSocket (`/ws`)

- [x] Register WS handler (`httpd_uri_t` with `is_websocket = true`)
- [x] Track connected WS client fds (add/remove on connect/close)
- [ ] Device → browser:
  - [x] Button events as JSON text frames
  - [x] UART RX as binary frames
- [ ] Browser → device:
  - [x] UART TX as binary frames

## 8. UART terminal

- [x] Add menuconfig for UART num, TX GPIO, RX GPIO, baudrate
- [x] Install UART driver + ring buffers (`uart_driver_install`, `uart_param_config`, `uart_set_pin`)
- [x] UART RX task: `uart_read_bytes` → WS broadcast (binary)
- [x] WS RX handler: binary → `uart_write_bytes`

## 9. Button input

- [x] Configure GPIO + ISR (or polling) with debounce strategy
- [x] ISR → queue → WS broadcast pattern (avoid heavy work in ISR)
- [ ] Verify no conflicts with UART pins / display pins

## 10. Frontend (Preact + Zustand)

- [x] Create `web/` app (Vite + Preact + Zustand)
- [x] Configure deterministic output names (no hash) so firmware routes are stable
- [ ] Implement WS store:
  - [x] connect/reconnect
  - [x] binary RX handling (terminal)
  - [x] JSON text handling (button/status)
- [ ] Implement upload flow using raw-body PUT:
  - [x] `fetch(PUT /api/graphics/<name>, body=file)`
  - [ ] show progress (optional)
- [ ] Terminal UI:
  - [x] display RX stream
  - [x] send typed bytes on Enter
- [x] Build pipeline to embed assets into firmware (CMake `EMBED_FILES`/`EMBED_TXTFILES`)

## 11. Testing / verification playbook

- [x] Write verification playbook doc:
  - `ttmp/2025/12/26/0013-ATOMS3R-WEBSERVER--atoms3r-web-server-graphics-upload-and-websocket-terminal/playbooks/01-verification-playbook-atoms3r-web-ui.md`
- [ ] Flash firmware, connect to AP, load `/`
- [ ] Upload PNG and verify display updates
- [ ] Press button and verify browser event
- [ ] Loopback UART (or connect device) and verify RX/TX in browser
- [ ] Reboot device and confirm uploaded graphics persist (FATFS RW)


