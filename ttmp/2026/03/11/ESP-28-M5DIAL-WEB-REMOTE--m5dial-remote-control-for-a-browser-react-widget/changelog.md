# Changelog

## 2026-03-11

- Initial workspace created
- Imported `/tmp/esp32-knob-web.md` into `sources/local/` and registered it in the ticket index
- Studied `0072-m5dial-timer-demo`, `0073-m5dial-film-developer-timer`, `0045-xiao-esp32c6-preact-web`, `0048-cardputer-js-web`, and `0071-cardputer-adv-photo-timer` as implementation precedents
- Wrote the detailed design doc describing a self-contained M5Dial firmware folder with embedded React assets, HTTP endpoints, and a browser WebSocket control stream
- Wrote the research diary capturing commands run, design pivots, and the rationale for rejecting an external Go/Watermill deployment for v1
- Added frontmatter and a numeric prefix to the imported source note so `docmgr doctor` passes cleanly
- Validated the ticket with `docmgr doctor --ticket ESP-28-M5DIAL-WEB-REMOTE --stale-after 30`
- Uploaded the bundled ticket docs to reMarkable at `/ai/2026/03/11/ESP-28-M5DIAL-WEB-REMOTE`
- Rewrote the architecture after user clarification so the M5Dial connects to an external web server that serves the React app, instead of the dial hosting React itself
- Added `0074-m5dial-web-remote/firmware` as a self-contained ESP-IDF firmware client with vendored M5Dial display/input dependencies, `wifi_mgr`, `wifi_console`, an outbound WebSocket client, local NVS-backed remote config, and an on-device status screen (`d2d3690`)
- Added `0074-m5dial-web-remote/server` as a Go WebSocket/HTTP hub exposing `/ws/device`, `/ws/browser`, and `/api/status` while serving embedded static assets (`ca3e85d`)
- Added `0074-m5dial-web-remote/web` as a React/Vite dashboard and copied the production build into `server/static/` so the Go server now serves the real UI bundle (`a9bc89e`)
- Added a top-level project README and cleaned generated TypeScript build artifacts out of version control (`fed38ca`)
- Verified:
  - `idf.py build` succeeds for `0074-m5dial-web-remote/firmware`
  - `go test ./...` succeeds for `0074-m5dial-web-remote/server`
  - `npm run build` succeeds for `0074-m5dial-web-remote/web`
  - `curl http://127.0.0.1:18080/` returns the built React shell and `curl http://127.0.0.1:18080/api/status` returns an empty server snapshot
- Attempted hardware flashing on `/dev/ttyACM0`, but `idf.py -p /dev/ttyACM0 flash` failed because the ACM device disappeared before esptool could open it, and the board was no longer visible in `lsusb`
