# Tasks

## TODO

- [x] Verify the current reblank path and confirm whether touch polling is starved during `displayBusy()` windows or whether touches are only visually delayed
- [x] Add a write-mode text/status refresh path so routine write actions do not require a whole-screen redraw
- [x] Route write-mode stroke completion and write-buffer editing through localized redraws instead of unconditional `QueueFullRender()`
- [x] Rebuild `0077-papers3-alphabet-graffiti` with ESP-IDF `5.3.4` after the redraw changes
- [ ] Measure the updated behavior on physical PaperS3 hardware and confirm whether perceived touch loss during reblank is reduced
- [ ] If touch still feels interrupted, design a second-step input capture change that does not violate `M5Unified` thread-safety assumptions
