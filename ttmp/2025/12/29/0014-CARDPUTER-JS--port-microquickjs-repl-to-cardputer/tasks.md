# Tasks

## TODO

- [x] Test QEMU execution of current firmware to verify it builds and runs correctly
- [ ] Create implementation plan document outlining code changes needed for Cardputer port
- [ ] Create sdkconfig.defaults with Cardputer configuration (8MB flash, 240MHz CPU, 8000 byte main task stack)
- [ ] Update partitions.csv for Cardputer (4MB app partition, 1MB SPIFFS storage partition)
- [ ] Replace UART driver with USB Serial JTAG driver in main.c (repl_task and initialization)
- [ ] Verify memory budget: ensure 64KB JS heap + buffers fit within Cardputer's 512KB SRAM constraint
- [ ] Test firmware on Cardputer hardware (verify USB Serial JTAG works, SPIFFS mounts, REPL functions)
- [ ] Consider optional enhancements: keyboard input integration, display output, speaker feedback
- [x] Create analysis docs for current firmware config + Cardputer port requirements
- [x] Add mqjs-repl build.sh wrapper for reproducible ESP-IDF env sourcing
- [x] Install qemu-xtensa via idf_tools so idf.py qemu can run
- [x] Spin off QEMU REPL input bug into ticket 0015-QEMU-REPL-INPUT
- [x] Spin off SPIFFS/autoload JS parse errors into ticket 0016-SPIFFS-AUTOLOAD
