# Tasks

## TODO

- [ ] Review UX brief for scope/fit (80×24)
- [ ] Decide key “host command” modality (menu key vs palette vs both)
- [ ] Capture real serial transcripts (panic/coredump/gdb) for wireframing
- [x] Contract designer + share ticket + confirm deliverables/timeline
- [x] Implement root TUI app model (screen+overlay routing, WindowSizeMsg)
- [x] Implement Port Picker view (scan/rescan, list + config fields, connect flow)
- [x] Implement Monitor view (viewport+status+input, follow/scrollback, Ctrl-T mode)
- [x] Implement Help overlay (keymap + close behavior)
- [x] Integrate decoder events into UI (panic/backtrace, core dump, gdb stub)
- [x] Run gofmt + go test for esper module
- [ ] Hardware: detect Cardputer serial port (USB Serial/JTAG)
- [ ] Hardware: build+flash esper esp32s3-test firmware to Cardputer
- [ ] Hardware: smoke test esper scan + start TUI against live device
- [x] Add reproducible scripts/ for hardware test steps
- [x] Write design doc for esper tail mode (non-TUI streamer)
- [x] Implement esper tail subcommand (flags + timeout)
- [x] Implement tail pipeline runner (line split + decoders + autocolor + tee)
- [x] Hardware: validate esper tail against esp32s3-test firmware (--timeout)
- [x] Document esper tail usage in esper/README.md
- [x] Extend esper tail: add --stdin-raw bidirectional mode with Ctrl-] exit
- [x] Hardware: verify esper tail --stdin-raw can type into esp_console; Ctrl-] exits
- [x] TUI: implement search overlay (HOST) (MVP; see diffs doc)
- [x] TUI: implement filter overlay (HOST) (MVP; see diffs doc)
- [x] TUI: implement Ctrl-T host command menu/palette (MVP; see diffs doc)
- [x] TUI: add tmux-based screenshot harness + capture 80x24 and 120x40 states
- [x] TUI: compare captured screenshots against wireframes and note diffs
- [ ] TUI: revise search overlay to bottom bar + match count + match highlight
- [ ] TUI: extend filter overlay (D/V + highlight rules) per wireframe
- [ ] TUI: expand command palette commands/grouping per wireframe
