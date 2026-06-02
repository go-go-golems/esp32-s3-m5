# Tasks

## TODO

- [x] Create ticket workspace and primary design document
- [x] Gather current LCD/display evidence from `0099` firmware and prior LCD optimization guide
- [x] Write intern-facing display server design and implementation guide
- [x] Define proposed public display API and internal ownership model
- [x] Define phased implementation and validation plan
- [ ] Implement Phase 1: extract low-level LCD module from `app_main.c`
- [ ] Implement Phase 2: add display server FreeRTOS task and queue
- [ ] Implement Phase 3: route fill/clear commands through display server
- [ ] Implement Phase 4: port queued dirty-op rendering into display server
- [ ] Implement Phase 5: add real bitmap font row renderer
- [ ] Implement Phase 6: add dirty-cell/dirty-row tracking and coalescing
- [ ] Implement Phase 7: add display-server benchmark/stress commands
- [ ] Operator visual confirmation for display-server queued output
- [ ] Compare manual `spi_master` display server against `esp_lcd_panel_io_spi`
- [ ] Investigate ST7365P/ILI9488 vertical scroll commands after display server baseline
