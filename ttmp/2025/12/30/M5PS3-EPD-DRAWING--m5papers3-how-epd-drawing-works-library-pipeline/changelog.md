# Changelog

## 2025-12-30

- Initial workspace created


## 2025-12-30

Step 1-3: Created ticket + initial research; confirmed PaperS3 EPD uses M5GFX Panel_EPD/Bus_EPD (not EPDiy) and documented app-level refresh flow.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp — Source of truth for refresh algorithm
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/M5PS3-EPD-DRAWING--m5papers3-how-epd-drawing-works-library-pipeline/reference/01-diary.md — Diary entries for initial investigation


## 2025-12-30

Step 4-5: Built firmware (idf.py build) and documented Panel_EPD/Bus_EPD drawing + refresh internals in analysis doc; updated diary with build + deep dive notes.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/M5PS3-EPD-DRAWING--m5papers3-how-epd-drawing-works-library-pipeline/analysis/01-epd-drawing-pipeline-library-deep-dive.md — Analysis now contains the end-to-end pipeline
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/M5PS3-EPD-DRAWING--m5papers3-how-epd-drawing-works-library-pipeline/reference/01-diary.md — Diary now includes build + Panel_EPD deep dive


## 2025-12-31

Step 6: Added in-depth Panel_EPD.cpp explanation (LUT stepper, per-pixel state machine, Bus_EPD timing) and transcribed pin map into analysis.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/M5PS3-EPD-DRAWING--m5papers3-how-epd-drawing-works-library-pipeline/analysis/01-epd-drawing-pipeline-library-deep-dive.md — Added Panel_EPD deep dive and pin mapping
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/M5PS3-EPD-DRAWING--m5papers3-how-epd-drawing-works-library-pipeline/reference/01-diary.md — Recorded Step 6

