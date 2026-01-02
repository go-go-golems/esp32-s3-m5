# Tasks

## TODO

- [x] Capture crash logs + backtrace showing screenshot path
- [x] Capture navigation logs showing correlation with B3 scene + screenshot send
- [x] Write bug report document with file/symbol pointers
- [ ] Reproduce deterministically (connected vs disconnected host / port open vs closed)
- [x] Fix `serial_write_all()` to avoid infinite retry / yield to IDLE (ticket 0024)
- [x] Decide/init strategy for USB-Serial/JTAG driver (`usb_serial_jtag_driver_install` vs console Kconfig)
- [ ] Validate end-to-end PNG capture and “fail fast” behavior (no watchdog)
