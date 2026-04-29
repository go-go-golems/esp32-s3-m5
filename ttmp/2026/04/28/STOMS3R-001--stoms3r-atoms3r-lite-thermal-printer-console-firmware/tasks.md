---
Title: Tasks
Ticket: STOMS3R-001
LastUpdated: 2026-04-28
---

# Tasks — STOMS3R-001

## Phase 1: Project Skeleton
- [x] 1. Create ESP-IDF project directory structure (`stoms3r/`)
- [x] 2. Write `sdkconfig.defaults` with USB Serial/JTAG + PSRAM settings
- [x] 3. Write `partitions.csv`
- [x] 4. Write top-level `CMakeLists.txt` and `main/CMakeLists.txt`
- [x] 5. Write minimal `app_main.c` that just starts the console
- [x] 6. Verify build succeeds with `idf.py build`

## Phase 2: Printer Driver
- [x] 7. Implement `printer_drv.c/.h` — UART init, `send_bytes()`, `reset()`
- [x] 8. Implement `printer_drv_print_text()` and `printer_drv_feed()`
- [ ] 9. Test: `printer_text "Hello"` prints on the thermal printer
- [x] 10. Implement `printer_drv_set_font_size()` and `printer_drv_set_bold()`
- [x] 11. Implement `printer_drv_print_barcode()`
- [x] 12. Implement `printer_drv_print_qr()` (3-step ESC/POS sequence)
- [x] 13. Implement `printer_drv_print_bitmap()` and test pattern

## Phase 3: Printer Console Commands
- [x] 14. Implement `printer_cmd.c/.h` — register all printer commands
- [ ] 15. Test all printer commands from the REPL

## Phase 4: NVS Storage
- [x] 16. Implement `nvs_store.c/.h` — save/load/erase WiFi credentials
- [ ] 17. Test: save credentials, reboot, verify they load

## Phase 5: WiFi Manager
- [x] 18. Implement `wifi_mgr.c/.h` — init, scan, connect, disconnect, event handler
- [ ] 19. Test: `wifi_scan` lists access points
- [ ] 20. Test: `wifi_connect` joins a network and gets an IP

## Phase 6: WiFi Console Commands
- [x] 21. Implement `wifi_cmd.c/.h` — register all WiFi commands
- [ ] 22. Test: auto-connect on boot from saved NVS credentials
- [ ] 23. Test: `wifi_forget` clears credentials and prevents auto-connect

## Phase 7: Integration & Polish
- [x] 24. Wire all modules in `app_main.c`
- [ ] 25. Run full smoke test checklist (Section 17 of design guide)
- [x] 26. Add `build.sh` helper script
- [x] 27. Write README.md for the firmware directory

## Phase 8: Web Server
- [x] 28. Create `web_server.c/.h` — start/stop HTTP server, serve static index.html
- [x] 29. Add `POST /api/print/text` endpoint that calls `printer_drv_print_text()`
- [x] 30. Add `POST /api/print/bitmap` endpoint — stream body to UART via `printer_drv_print_bitmap()`
- [x] 31. Add `GET /api/status` endpoint — WiFi status, printer swap state, baud rate
- [x] 32. Embed `index.html` via `EMBED_TXTFILES` in CMakeLists
- [x] 33. Build `index.html` — text print form, image upload with JS dithering, preview canvas
- [x] 34. JS: implement `imageToBitmap()` — canvas resize → grayscale → Floyd-Steinberg dither → bit-pack
- [x] 35. JS: wire fetch calls to POST endpoints, show print confirmation
- [x] 36. Start web server in `app_main.c` after WiFi connects
- [x] 37. Verify build succeeds
- [ ] 38. Test: open browser, print text via web UI
- [ ] 39. Test: upload image, preview dithered result, print bitmap
