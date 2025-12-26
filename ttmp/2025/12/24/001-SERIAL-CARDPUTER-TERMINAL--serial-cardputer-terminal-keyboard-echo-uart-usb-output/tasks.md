# Tasks

## TODO

- [x] Create a new chapter project: `esp32-s3-m5/0015-cardputer-serial-terminal/` (clone structure from `0012-cardputer-typewriter`)
- [x] Add `main/Kconfig.projbuild` options for:
  - [x] Backend select: USB-Serial-JTAG vs hardware UART
  - [x] UART config: port number, baud, TX/RX pins (and optional RTS/CTS)
  - [x] Terminal semantics: local echo, newline mode, backspace mode
  - [x] RX display: enable/disable
- [x] Implement a small serial backend abstraction:
  - [x] USB-Serial-JTAG backend (write + optional read)
  - [x] UART backend (init + write + optional read)
- [x] Integrate keyboard scan + terminal model + renderer:
  - [x] Show what you type on-screen immediately
  - [x] Transmit bytes to the selected backend
  - [x] Preserve the `0007` “inline echo visibility” lessons (flush + optional direct USB write)
- [x] (Recommended) Implement RX-to-screen so the app behaves like a real serial terminal
- [x] Validate on hardware:
  - [x] USB backend: verify host receives bytes and screen updates feel realtime
  - [x] UART backend: verify TX/RX against another serial device (or loopback)

