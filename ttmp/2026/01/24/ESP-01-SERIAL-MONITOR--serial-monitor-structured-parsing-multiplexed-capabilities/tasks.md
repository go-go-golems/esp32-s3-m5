# Tasks

## TODO

- [x] Identify prior screenshot-over-serial projects and protocols
- [x] Analyze ESP-IDF `idf_monitor` internals (colors, filtering, decoding hooks)
- [x] Draft “Serial Monitor as Stream Processor” design doc
- [x] Create ticket-local scripts for capture/parsing experiments
- [x] Upload bundled ticket PDF to reMarkable
- [x] Create `esper` Go repo (github.com/go-go-golems/esper)
- [ ] Implement Phase 1 monitor core (serial read/write, line splitting, auto-color reset discipline)
- [ ] Implement decode hooks (panic backtrace addr2line, core dump buffering + decode, GDB stub detect)
- [x] Add ESP32-S3 test firmware (USB Serial/JTAG console) to exercise features
- [x] Add tmux exercise script(s) and a repeatable workflow
- [ ] Keep diary updated with commit hashes per step
- [x] Add `esper scan` command to find ESP32 serial consoles (Linux)
- [x] Add optional esptool-based chip probing to `esper scan` (chip name/description/features/usb mode)
- [x] Capture `glaze help build-first-command` output for Glazed integration reference
- [x] Convert `esper scan` into a Glazed command (structured output: `--output`, `--fields`, etc.)
