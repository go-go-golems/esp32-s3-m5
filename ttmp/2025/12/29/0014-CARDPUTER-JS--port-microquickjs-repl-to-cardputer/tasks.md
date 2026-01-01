# Tasks

## TODO

- [x] Test QEMU execution of current firmware to verify it builds and runs correctly
- [ ] Create implementation plan document outlining code changes needed for Cardputer port
- [x] Create sdkconfig.defaults with Cardputer configuration (8MB flash, 240MHz CPU, 8000 byte main task stack)
- [x] Update partitions.csv for Cardputer (4MB app partition, 1MB SPIFFS storage partition)
- [x] Replace UART driver with USB Serial JTAG driver in main.c (repl_task and initialization)
- [ ] Verify memory budget: ensure 64KB JS heap + buffers fit within Cardputer's 512KB SRAM constraint
- [ ] Test firmware on Cardputer hardware (verify USB Serial JTAG works, SPIFFS mounts, REPL functions)
- [ ] Consider optional enhancements: keyboard input integration, display output, speaker feedback
- [x] Create analysis docs for current firmware config + Cardputer port requirements
- [x] Add mqjs-repl build.sh wrapper for reproducible ESP-IDF env sourcing
- [x] Install qemu-xtensa via idf_tools so idf.py qemu can run
- [x] Spin off QEMU REPL input bug into ticket 0015-QEMU-REPL-INPUT
- [x] Spin off SPIFFS/autoload JS parse errors into ticket 0016-SPIFFS-AUTOLOAD
- [ ] Split mqjs firmware into C++ components (console/repl/eval/storage) per design-doc/02; keep behavior unchanged initially
- [x] Introduce IConsole + UartConsole wrapper (uart_read_bytes/uart_write_bytes) so REPL loop is transport-agnostic
- [x] Implement LineEditor (byte->line, backspace, prompt) and ReplLoop that prints prompt and dispatches completed lines
- [x] Add IEvaluator interface + EvalResult and implement RepeatEvaluator (echo line) to validate REPL I/O without JS
- [ ] Add REPL meta-commands (e.g. :help, :mode, :prompt) and default to repeat mode in QEMU/dev builds
- [x] Build 'REPL-only' firmware variant: disable SPIFFS/autoload and do not initialize MicroQuickJS (repeat evaluator only)
- [x] Update ESP-IDF build config (main/CMakeLists, .cpp sources, includes) to compile the new component split cleanly
- [x] Smoke-test REPL-only firmware under QEMU: verify interactive input echoes and prompt redraw works (no JS, no storage)
- [ ] Stdlib: trace provenance of esp_stdlib.h (host generator) and document how to regenerate for ESP32 (use -m32)
- [ ] Stdlib: generate and commit a 32-bit stdlib header (e.g. esp32_stdlib.h) using esp_stdlib_gen -m32; verify it contains keyword atoms (var/function/return)
- [ ] Stdlib: add a reproducible script/Make target to regenerate the ESP32 stdlib header (and optionally atom defines via -a)
- [ ] Firmware: add build-time selection between minimal_stdlib and generated esp32 stdlib (Kconfig/sdkconfig.defaults), without mixing 64-bit tables on ESP32
- [ ] Validation: once esp32 stdlib is wired, run a minimal script that starts with 'var' (and a function) to confirm parsing works on target/QEMU
- [x] Add tmux-driven RepeatEvaluator REPL smoke-test scripts (QEMU + device)
- [x] Add UART-direct REPL smoke tests (QEMU stdio + raw TCP; device pyserial) to isolate QEMU UART RX
- [x] QEMU workaround: set UART0 RX full threshold=1 in UartConsole (post uart_driver_install)
- [x] Validate QEMU interactive input after threshold change (tmux + raw TCP + stdio UART scripts)
- [x] Device validation: run repeat REPL smoke tests on Cardputer (/dev/ttyACM0) (tmux + pyserial)
- [x] Document QEMU UART RX workaround and evidence in design-doc/03
- [x] Implement UsbSerialJtagConsole (IConsole) for Cardputer interactive REPL on /dev/ttyACM*
- [x] Add build-time console selection (UART0 vs USB Serial/JTAG) and default to UART0 for QEMU
- [x] Update device smoke test script to flash+monitor with USB Serial/JTAG console enabled
- [x] Verify Cardputer REPL prompt+echo works on /dev/ttyACM0 (tmux + pyserial)
