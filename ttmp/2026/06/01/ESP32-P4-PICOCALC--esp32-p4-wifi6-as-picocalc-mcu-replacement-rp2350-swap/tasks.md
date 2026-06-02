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
- [x] LCD performance: add repeatable perf suite with render-vs-transfer timing
- [x] LCD optimization: implement queued SPI transfer path using `spi_device_queue_trans()` / `spi_device_get_trans_result()`
- [x] LCD optimization: add double-buffered row renderer so CPU can render buffer B while SPI transfers buffer A
- [x] LCD optimization: add `lcd perf queued` comparison against current polling-transfer `lcd perf full`
- [ ] LCD optimization: operator visual confirmation of queued pseudo-text output
- [x] LCD optimization: measure queued transfer impact for generated moving-rectangle workloads
- [x] LCD optimization: measure queued transfer impact for background-restore moving-rectangle workloads
- [x] LCD optimization: measure queued transfer impact for mixed dirty-region workloads
- [ ] LCD optimization: measure queued transfer impact for solid fills, generated patterns, and row updates separately
- [ ] LCD optimization: keep current polling path as a baseline until queued path is measured and visually confirmed
- [ ] LCD optimization: replace pseudo-glyph generator with a real bitmap font renderer
- [ ] LCD optimization: add dirty-cell and dirty-row tracking for terminal-like text updates
- [ ] LCD optimization: add renderer benchmarks for full screen, dirty row, dirty cell, cursor blink, and mixed edit workloads
- [ ] LCD optimization: investigate ST7365P/ILI9488 vertical scroll commands and benchmark hardware-scroll terminal behavior
- [ ] LCD optimization: compare manual `spi_master` LCD path against ESP-IDF `esp_lcd_panel_io_spi`
- [ ] LCD optimization: move display updates and perf loops from console command context into a dedicated display task
- [ ] LCD optimization: define display command queue API for app/UI code to submit fills, blits, text rows, and scroll operations
- [ ] LCD optimization: evaluate PSRAM framebuffer or partial framebuffer for composition, while keeping active DMA buffers in internal DMA memory
- [ ] LCD optimization: add long-running display stress test task with explicit watchdog/progress handling, separate from `esp_console`
- [ ] LCD optimization: evaluate final adapter routing tradeoff: same-position GPIO-matrix LCD pins vs cross-routed SPI2 IO-MUX pins GPIO28-GPIO29-GPIO30-GPIO31
- [ ] Verify ESP32-P4 chip revision on Waveshare board (needs ≥ v3.1 for current ESP-IDF)
- [ ] Test I²C bus sharing: 10 kHz keyboard southbridge + ES8311 codec on same bus
- [ ] Check physical fit of Waveshare board inside PicoCalc case
- [ ] Phase 1 firmware: ESP-IDF blink + console on ESP32-P4-WIFI6
