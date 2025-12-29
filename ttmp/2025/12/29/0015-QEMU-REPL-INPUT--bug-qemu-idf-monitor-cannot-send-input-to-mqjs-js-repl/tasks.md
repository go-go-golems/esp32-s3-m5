# Tasks

## TODO

- [x] Add tasks here

- [x] Capture QEMU+idf_monitor transcript showing boot reaches js>
- [x] Write initial bug report + relate relevant firmware/config/log files
- [x] Demonstrate that idf_monitor receives input (Ctrl+T Ctrl+H) but REPL does not echo/eval

- [x] Manual typing confirmation (no tmux automation): attach to the running monitor and type `1+2<enter>`; record whether any bytes echo or evaluate. (Result: broken; no echo/eval.)
- [ ] Bypass `idf_monitor`: run `imports/test_storage_repl.py` against `localhost:5555` and capture its stdout; determine whether *any* RX reaches `uart_read_bytes()`. (Attempted once; got connection refused — QEMU not listening on :5555 at that moment.)
- [ ] Try an even more minimal “raw bytes” sender (e.g. `nc`/`socat`) to validate whether CR/LF or buffering is the issue.
- [ ] Test the “storage broke it” hypothesis: build a no-SPIFFS/no-autoload variant (same UART REPL loop) and see if RX works under QEMU.
- [ ] Build a minimal UART echo firmware (no MicroQuickJS, no SPIFFS): prove whether QEMU UART RX works at all on esp32s3 `-serial tcp::5555,server`.
- [ ] Confirm console/UART ownership: does ESP-IDF console already install a UART0 driver that conflicts with our `uart_driver_install()` / `uart_read_bytes()` usage?
- [ ] Capture QEMU command line used by `idf.py qemu` (the “Running qemu (bg): ...” line) and note any serial-related args.
