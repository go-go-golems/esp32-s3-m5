# Tasks

## TODO

- [ ] Phase 0 - Orientation: build/flash 0114, confirm ESP-53 connectivity (net status, serve start, curl /status), read this guide + ESP-53 design-doc/02, verify esp_mdns availability in IDF 5.3.4
- [ ] [P1.1] battery JS singleton: level()/mv()/charging()/statusText() in js_services.cpp (or js_battery.cpp); keep batteryLevel() alias for compat
- [ ] [P1.2] bat console command (ConsoleOp::Battery) + probe 19 (level in [-1,0..100], charging in [-1,0,1])
- [ ] [P1.3] home-screen battery glyph (dyn-value batteryGlyph() in home chrome, 5s tick refresh)
- [ ] [P2.1] net_mdns.{h,cpp}: lazy MdnsInit/MdnsAnnounce/MdnsStop/MdnsStatus/MdnsHost/MdnsUrl; hostname "pulp"
- [ ] [P2.2] mdns JS singleton (status/host/url); wire MdnsAnnounce into ServeStart, MdnsStop into ServeStop/WifiOff/PowerSleep
- [ ] [P2.3] mdns console command + probe 20: ping pulp.local resolves; mdns.url() shows it; stops on wifi off
- [ ] [P3.1] extend serve with POST: RouteEntry method flag, ServeUpload httpd handler (stream body to /sdcard/images/<ts>.g4, validate G4 header, 280 KiB cap, single busy slot -> 503)
- [ ] [P3.2] images catalog: /sdcard/images/ dir + index.txt; ConsoleOp::Images (list/display/remove/received)
- [ ] [P3.3] images JS module: count()/name(i)/remove(name)/received(fn) + completion mailbox {name,bytes,err} via ModuleDone{Images}
- [ ] [P3.4] browser upload page /sdcard/www/index.html: file pick, canvas crop/scale to 540x960, 4-bit quantize (+ optional FS dither), pack .g4, POST /images/upload, progress + list
- [ ] [P3.5] Gate: upload JPEG from browser; images.count() increments; .g4 on SD; index.txt lists it; concurrent POST -> 503
- [ ] [P4.1] FrameBuilder::Bitmap(bounds, data, len, stride) emitter in s3paper_core (GlyphRun copy pattern)
- [ ] [P4.2] m5_backend DrawOpKind::Bitmap rasterizer (nibble->RGB565->pushImage, clip-rect honored); replace skip arm
- [ ] [P4.3] host fake-backend Bitmap trace test; make run green (N checks >= current)
- [ ] [P4.4] images.display(name) direct path (load .g4, PSRAM buffer, full-page present, paper.refreshTurns(1))
- [ ] [P4.5] Gate: js probe 21 displays a stored frame; clean full refresh; no overflow outside canvas clip
- [ ] [P5.1] gallery() app in pulp.js: list + display + left/right scroll + tap-to-delete; launcher row
- [ ] [P5.2] Gate: upload 3 images, swipe through all 3, delete one, home glyph persists
- [ ] [P6.1] Hardening: fault probes (upload during display, display during upload, POST during sleep), 30-min upload+display soak (heap flat)
- [ ] [P6.2] sleep sequence with MdnsStop; doctor clean; host suite green; diary/changelog complete
- [ ] [P6.3] reMarkable bundle upload (dry-run then real) of design-doc + diary
