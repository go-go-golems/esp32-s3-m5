# Tasks

## TODO

- [ ] Define scope + acceptance criteria
- [ ] Flash + verify browser loads UI (no `U+0000`) and devtools loads sourcemap
<!-- Keep TODO focused on remaining validation/polish. -->

## Done

- [x] Create new ESP32-C6 tutorial project skeleton (0045)
- [x] Port wifi_mgr (NVS creds + scan + connect) from 0029
- [x] Implement esp_console wifi commands incl. join-by-index
- [x] Implement esp_http_server static asset serving + /api/status
- [x] Create web/ Vite+Preact+Zustand counter app with deterministic outputs
- [x] Add build.sh helper + README instructions
- [x] Build web bundle (npm) and embed assets
- [x] Build firmware (idf.py) and capture sdkconfig
- [x] Fix `U+0000` parse error by trimming embedded NUL terminators
- [x] Enable and serve JS sourcemap at `/assets/app.js.map` (gzipped)

<!-- Notes:
  - Keep TODO as "needs validation" items; move here once implemented. -->
