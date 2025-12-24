# Tasks

## TODO

- [ ] Create a new chapter project: `esp32-s3-m5/0015-cardputer-serial-terminal/` (clone structure from `0012-cardputer-typewriter`)
- [ ] Add `main/Kconfig.projbuild` options for:
  - [ ] Backend select: USB-Serial-JTAG vs hardware UART
  - [ ] UART config: port number, baud, TX/RX pins (and optional RTS/CTS)
  - [ ] Terminal semantics: local echo, newline mode, backspace mode
  - [ ] RX display: enable/disable
- [ ] Implement a small serial backend abstraction:
  - [ ] USB-Serial-JTAG backend (write + optional read)
  - [ ] UART backend (init + write + optional read)
- [ ] Integrate keyboard scan + terminal model + renderer:
  - [ ] Show what you type on-screen immediately
  - [ ] Transmit bytes to the selected backend
  - [ ] Preserve the `0007` “inline echo visibility” lessons (flush + optional direct USB write)
- [ ] (Recommended) Implement RX-to-screen so the app behaves like a real serial terminal
- [ ] Validate on hardware:
  - [ ] USB backend: verify host receives bytes and screen updates feel realtime
  - [ ] UART backend: verify TX/RX against another serial device (or loopback)

