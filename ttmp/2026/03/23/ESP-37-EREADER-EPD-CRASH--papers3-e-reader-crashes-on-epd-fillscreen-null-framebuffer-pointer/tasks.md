# Tasks

## Done

- [x] Instrument `Panel_EPD::init_intenal()` and capture real allocation-failure telemetry on hardware
- [x] Verify whether `_buf` itself was the failing allocation
- [x] Keep the console alive when display init fails by gating UI refresh on `display_ready_`
- [x] Move `_lut_2pixel` off the DMA-only heap so PaperS3 display init can complete
- [x] Trace the boot-time `540x960` clear path and confirm it happens before app code sets rotation
- [x] Fix the PaperS3 board autodetect path to default to rotation `1` before `M5Unified` runs `clear_display`
- [x] Rebuild, flash, and validate on the attached PaperS3 over `/dev/ttyACM0`

## Follow-up

- [ ] Trim or downgrade temporary EPD debug logs once ESP-37 is formally closed
