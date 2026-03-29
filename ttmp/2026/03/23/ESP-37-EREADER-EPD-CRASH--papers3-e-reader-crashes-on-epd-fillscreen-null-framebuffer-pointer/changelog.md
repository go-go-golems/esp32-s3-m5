# Changelog

## 2026-03-23

- Initial workspace created


## 2026-03-23

Created bugfix ticket with full investigation diary backfill. Documented crash signature, two failed fix attempts (stack overflow fix + core 0 init), analysis of Panel_EPD._buf allocation, comparison with working 0078, and 6 ranked investigation leads for handoff.

## 2026-03-23

- Proved the real init failure was `Panel_EPD::_lut_2pixel` requesting 43,520 bytes from DMA-capable internal RAM when the largest DMA block was only 31,744 bytes.
- Changed `_lut_2pixel` allocation to generic `MALLOC_CAP_8BIT`, which allowed EPD init to complete and removed the null-framebuffer crash.
- Added PaperS3 default `setRotation(1)` during M5GFX board autodetect so `M5Unified` boot-time `clear_display` uses the correct `960x540` orientation.
- Validated on attached hardware: `display_count=1`, `display_ready=yes`, no early out-of-range fill warnings, and the e-reader opens the first book without crashing.
