# Changelog

## 2026-03-22

- Initial workspace created
- Added the initial design plan, task list, and diary for the PaperS3 `Panel_EPD` investigation
- Reconfirmed that the first direct crash choke point is the nibble-write loop in `Panel_EPD::writeFillRectPreclipped(...)`, writing into a PSRAM-backed framebuffer `_buf`
- Reconfirmed from local history that the current vendored PaperS3 backend already contains the older PSRAM/cache fix `c899961`, so the next slice starts with instrumentation rather than assuming that old fix is still missing
