# Tasks

## TODO

- [ ] Phase 0: Bring-up — stack module, set DIP switch to UART, connect 12 V, run `scripts/01-probe-qrcode-uart.py` to verify protocol
- [ ] Phase 1: Create `0118-cores3-qrcode-scanner/` skeleton (CMakeLists, idf_component.yml with m5unified/m5gfx, sdkconfig.defaults quad PSRAM, partitions.csv); boot "Hello" on CoreS3 display + USB Serial/JTAG logs
- [ ] Phase 2: Port `qrcode_m14.cpp` → `qr_engine.cpp`; init PI4IOE5V6408 expander + UART1 G17/G18; `status` console cmd returns firmware/serial
- [ ] Phase 3: M5GFX on-screen UI (current code + history + status); button start/stop; aim-and-show works for QR/DataMatrix/Code128/EAN-13/PDF417
- [ ] Phase 4: `esp_console` commands (start/stop/mode/light/brightness/beep/status/reset/info); boot banner + 12V/DIP warning; reproducible fullclean build
- [ ] Phase 5 (future): configurable suffix/protocol-format; optional image preview; NVS history; symbology menu

## DONE

- [x] Research module, gather + save sources (sources/MANIFEST.md)
- [x] Write probe script (scripts/01-probe-qrcode-uart.py) and build/flash helper (scripts/02-bringup-build-flash.sh)
- [x] Write intern-ready design/implementation guide (design-doc/01-...-guide.md)
- [x] Write investigation diary (reference/01-diary.md)
