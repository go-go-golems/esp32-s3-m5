# Changelog

## 2026-04-28

- Initial workspace created


## 2026-04-28

Step 1: Created ticket, wrote 2000-line design/implementation guide covering hardware, ESC/POS protocol, esp_console, NVS, WiFi, and file-by-file implementation plan. Created tasks.md with 27 implementation tasks across 7 phases.


## 2026-04-28

Step 2: Implemented complete firmware (Phases 1-6) — 18 files, 1414 LOC. Build verified (821 KB binary, 80% free). Tasks 1-8,10-14,16,18,21,24,26,27 checked off. Commit 5651b3f.


## 2026-04-28

Step 3: Fixed printer GPIO pins from GPIO5/GPIO6 to GPIO8/GPIO7/GPIO6 (K118 designed for ATOM Lite header positions). Added diagnostics: printer_probe, printer_raw, hex TX logging. Commits 2db79d9 and aef1068.


## 2026-04-28

Step 4: Added web UI with JS-side image dithering. web_server.c (4 endpoints: GET /, GET /api/status, POST /api/print/text, POST /api/print/bitmap), index.html (text form, image drop zone, Floyd-Steinberg dithering, preview canvas). Streaming bitmap: ESP32 relays chunks to UART with ~zero RAM. Commit ba3afcc.

