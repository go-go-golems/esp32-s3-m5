# Tasks

## TODO

- [x] Add tasks here

- [ ] Measure PicoCalc VSYS voltage and current capacity under all power states
- [ ] Phase 3 firmware: I²C keyboard southbridge driver
- [ ] Design interposer PCB in KiCad (Pico footprint → Waveshare headers)
- [ ] Phase 2 firmware: SPI2 LCD driver (ST7365P/ILI9488) with PSRAM framebuffer
- [x] LCD throughput: select SPLL and validate actual 80 MHz SPI clock
- [x] LCD throughput: replace tiny fill chunks with 32 KiB DMA-capable transfer buffer
- [x] LCD throughput: benchmark optimized full-screen fill and color bars
- [x] LCD throughput: operator visual inspection of 80 MHz color bars
- [x] LCD throughput: add checker/stripe/diagonal pattern tests
- [x] LCD throughput: add dirty-rectangle benchmark command
- [x] LCD throughput: add terminal-cell, row, and scroll benchmarks
- [x] LCD throughput: visual confirmation of checker/pattern/terminal benchmark output
- [x] LCD throughput: add text-render/glyph-like benchmark path
- [ ] LCD throughput: evaluate queued DMA transfers after visual baseline is confirmed
- [ ] Verify ESP32-P4 chip revision on Waveshare board (needs ≥ v3.1 for current ESP-IDF)
- [ ] Test I²C bus sharing: 10 kHz keyboard southbridge + ES8311 codec on same bus
- [ ] Check physical fit of Waveshare board inside PicoCalc case
- [ ] Phase 1 firmware: ESP-IDF blink + console on ESP32-P4-WIFI6
