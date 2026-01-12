---
Title: 'Guide: GIF asset pipeline (AnimatedGIF + FATFS)'
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - esp32
    - firmware
    - documentation
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: 'Expanded research and writing guide for the GIF asset pipeline on ESP32-S3.'
LastUpdated: 2026-01-11T19:44:01-05:00
WhatFor: 'Help a ghostwriter draft the GIF asset pipeline doc.'
WhenToUse: 'Use before writing the AnimatedGIF + FATFS guide.'
---

# Guide: GIF asset pipeline (AnimatedGIF + FATFS)

## Purpose

This guide enables a ghostwriter to create a narrow, practical doc on how to prepare GIF assets, pack them into a FATFS image, and play them back using the AnimatedGIF component. The reader should be able to follow the pipeline from source GIF to on-device playback without guessing about sizes, frame limits, or storage layout.

## Assignment Brief

Write a how-to guide that emphasizes the asset pipeline. The doc should focus on the concrete steps and constraints: 128x128 target size, frame trimming for size limits, and the FATFS image structure used by the GIF console firmware.

## Environment Assumptions

- ESP-IDF 5.4.x
- AtomS3R hardware
- Host tooling for image conversion (shell scripts in repo)

## Source Material to Review

- `esp32-s3-m5/0013-atoms3r-gif-console/README.md`
- `esp32-s3-m5/0013-atoms3r-gif-console/assets/README.md`
- `esp32-s3-m5/0014-atoms3r-animatedgif-single/README.md`
- `esp32-s3-m5/ttmp/2025/12/23/0010-PORT-ANIMATED-GIF--port-animatedgif-bitbank2-into-esp-idf-project-as-a-component/reference/01-diary.md`
- `esp32-s3-m5/ttmp/2025/12/23/009-INVESTIGATE-GIF-LIBRARIES--investigate-gif-decoding-libraries-for-esp-idf-esp32-atoms3r/reference/01-diary.md`
- `esp32-s3-m5/0013-atoms3r-gif-console/flash_storage.sh` (if present)

## Technical Content Checklist

- AnimatedGIF (bitbank2) component integration basics
- Asset constraints: 128x128 target size, frame count limits
- Conversion scripts:
  - `convert_assets_to_128x128_crop.sh`
  - `trim_gif_128x128_frames.sh`
- Storage layout: FATFS image + `/storage/gifs` directory
- Runtime scan order: `/storage/gifs` preferred, `/storage` fallback

## Pseudocode Sketch

Use a small snippet to explain the runtime scan and playback logic.

```c
// Pseudocode: runtime scan + playback
mount_fatfs("/storage")
files = list_dir("/storage/gifs")
if files.empty:
  files = list_dir("/storage")
for gif in files:
  animatedgif.open(gif)
  while animatedgif.has_frame:
    frame = animatedgif.next_frame()
    display.draw(frame)
```

## Pitfalls to Call Out

- Large GIFs can exceed flash or decode budgets; trim frames.
- Non-square GIFs need center crop to avoid padding artifacts.
- Asset binaries are gitignored; document where to place them.

## Suggested Outline

1. Pipeline overview (source GIF to device playback)
2. Asset requirements (size, frames, format)
3. Conversion scripts and examples
4. Building and flashing the FATFS image
5. Runtime behavior and troubleshooting

## Commands

```bash
# Convert assets to 128x128 (crop)
cd esp32-s3-m5/0013-atoms3r-gif-console
./convert_assets_to_128x128_crop.sh

# Trim to first N frames if needed
./trim_gif_128x128_frames.sh assets/input.gif assets/input_128x128_30f.gif 30
```

## Exit Criteria

- Doc includes a step-by-step asset conversion flow.
- Doc explains FATFS storage location and runtime scan order.
- Doc includes at least one troubleshooting tip for oversize GIFs.

## Notes

Keep the scope on asset preparation and packaging. Do not expand into general display bring-up.
