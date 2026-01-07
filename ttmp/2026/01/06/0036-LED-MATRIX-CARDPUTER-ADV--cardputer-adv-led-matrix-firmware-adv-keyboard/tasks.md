# Tasks

## TODO

- [ ] Add tasks here

- [ ] Phase 0: Verify Cardputer-ADV pinout (TCA8418 I2C SDA/SCL + INT, MAX7219 SPI + CS) and record pin reservations/conflicts
- [ ] Phase 0: Choose MAX7219 wiring + orientation test pattern (document expected rotation/bit order)
- [ ] Phase 1: Create new ESP-IDF firmware project directory for 0036 (Cardputer-ADV LED matrix console)
- [ ] Phase 1: Add sdkconfig.defaults baseline selecting USB Serial/JTAG esp_console backend (disable UART backends)
- [ ] Phase 1: Implement minimal MAX7219 driver (init, clear, intensity, write rows)
- [ ] Phase 1: Add esp_console REPL startup (USB Serial/JTAG) + register 'matrix' commands
- [ ] Phase 1: Add matrix calibration commands (show rows/cols, rotate/flip) to quickly fix module orientation
- [ ] Phase 1: Write a quick playbook: wiring, flash/monitor commands, and REPL usage
- [ ] Phase 2: Implement I2C init + TCA8418 minimal configuration (enable key-event interrupts)
- [ ] Phase 2: Wire GPIO INT ISR -> FreeRTOS queue -> keyboard task that drains FIFO
- [ ] Phase 2: Add 'kbd' console commands for debugging (status, dump last N events, clear)
- [ ] Phase 2: Build Cardputer-ADV keymap table (key number -> ASCII, Shift/Fn behavior)
- [ ] Phase 2: Render keypresses to LED matrix (glyph render + optional scroll)
- [ ] Phase 3: Add robustness (FIFO overflow handling, I2C error recovery, log-noise reduction while REPL active)
- [ ] Phase 3: Update docs with verified pinout + known-good module wiring
- [x] Phase 1: Add drop-bounce text animation command (REPL)
- [x] Phase 1: Add wave text animation command (REPL)
- [ ] Phase 1: Validate drop/wave on 12-module chain
