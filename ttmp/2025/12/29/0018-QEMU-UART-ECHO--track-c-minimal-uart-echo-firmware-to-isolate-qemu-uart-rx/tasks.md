# Tasks

## TODO

- [ ] Create the minimal UART echo firmware project directory under `esp32-s3-m5/` next to `0016-atoms3r-grove-gpio-signal-tester/`.
- [ ] Ensure firmware uses UART0 and echoes bytes via `uart_write_bytes()` (avoid stdio buffering).
- [ ] Ensure firmware reads with a finite timeout and prints a periodic heartbeat (prove the task loop is alive).
- [ ] Ensure `sdkconfig.defaults` disables USB-Serial-JTAG console (QEMU doesn’t emulate it) and uses UART0 console settings suitable for QEMU.
- [ ] Run firmware under QEMU with `-serial mon:stdio` and test manual typing; capture a transcript.
- [ ] Run firmware under QEMU with TCP serial (e.g. `-serial tcp::5555,server`) and test via a simple TCP client; capture a transcript.
- [ ] If UART echo RX still fails under both serial backends, document it as a likely QEMU UART RX limitation and link upstream references/issues if any.
- [ ] If UART echo RX works, feed that result back into ticket `0015-QEMU-REPL-INPUT` (it means the bug is higher-level: monitor/console/REPL app).

