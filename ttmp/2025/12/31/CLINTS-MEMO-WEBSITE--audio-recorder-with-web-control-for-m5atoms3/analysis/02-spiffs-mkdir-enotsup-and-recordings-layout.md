---
Title: SPIFFS mkdir ENOTSUP and recordings layout
Ticket: CLINTS-MEMO-WEBSITE
Status: active
Topics:
    - storage
    - spiffs
    - esp-idf
    - audio-recording
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0021-atoms3-memo-website/main/storage_spiffs.cpp
      Note: Mounts SPIFFS, attempts mkdir for recordings dir, falls back to base path
    - Path: esp32-s3-m5/0021-atoms3-memo-website/main/recorder.cpp
      Note: Creates recordings as flat WAV files under storage_recordings_dir()
    - Path: /home/manuel/esp/esp-idf-5.4.1/components/spiffs/esp_spiffs.c
      Note: SPIFFS VFS implementation; mkdir/rmdir explicitly return ENOTSUP
    - Path: /home/manuel/.espressif/tools/esp-clang/15.0.0-23786128ae/esp-clang/xtensa-esp32s3-elf/include/sys/errno.h
      Note: Toolchain errno values (ENOTSUP is 134 on ESP32 toolchains)
ExternalSources: []
Summary: Explain why mkdir(/spiffs/rec) fails (ENOTSUP/134) while file-based recording still works on SPIFFS, and what layout to use going forward.
LastUpdated: 2025-12-31T17:11:40-05:00
WhatFor: Root-cause explanation for “mkdir failed” log lines and guidance for recordings directory layout on SPIFFS
WhenToUse: When debugging storage/recording startup, or deciding whether to keep SPIFFS vs switch to LittleFS
---

# SPIFFS mkdir ENOTSUP and recordings layout

## Symptom

Boot logs show SPIFFS mounts successfully, but directory creation fails:

- `SPIFFS mounted: label=storage base=/spiffs ...`
- `mkdir failed: /spiffs/rec errno=134`
- `recordings dir unavailable (/spiffs/rec); using base path: /spiffs`

Even with the `mkdir` failure, recording can still succeed and WAV files appear in `/spiffs` (flat layout).

## Root cause

ESP-IDF’s SPIFFS VFS layer does not implement POSIX directory creation/removal. In ESP-IDF v5.4.1, `vfs_spiffs_mkdir()` is a stub that always fails:

- `/home/manuel/esp/esp-idf-5.4.1/components/spiffs/esp_spiffs.c` sets `errno = ENOTSUP` and returns `-1` for `mkdir` and `rmdir`.

On ESP32 toolchains, `ENOTSUP` is numerically `134` (unlike Linux where it’s typically `95`), so logging `errno` shows `134`:

- `/home/manuel/.espressif/.../include/sys/errno.h` defines `#define ENOTSUP 134`.

## Why recording can still work

The recorder does not require directories as long as it can create files under the mounted base path:

- SPIFFS mounts at `/spiffs` and supports `open/fopen`, `write`, `close`, and `opendir/readdir` on the mount.
- Our firmware treats the “recordings directory” as best-effort:
  - `storage_spiffs_init()` tries to create `/spiffs/rec`, but if `mkdir` fails it falls back to storing recordings directly under `/spiffs`.
  - `recorder.cpp` then creates `rec_00001.wav` etc under `storage_recordings_dir()` (which becomes `/spiffs` after fallback).

So: `mkdir` failing is expected on SPIFFS, but does not prevent file-based recording when using a flat layout.

## Implications / guidance

- **Keep recordings flat on SPIFFS**: store files directly under the mount base (`/spiffs/rec_00001.wav`, …).
- If a **true directory hierarchy** is required (e.g., `/spiffs/rec/`), prefer switching to **LittleFS** (or FATFS on SD) rather than fighting SPIFFS directory semantics.
- Treat “recordings dir config” as a logical prefix, not a hard requirement, unless the selected filesystem is known to implement directories.

## Verification checklist (on-device)

- On boot: confirm `SPIFFS mounted` log line appears.
- Start recording from the web UI; it should return `{"ok":true,"filename":"rec_00001.wav"}`.
- `GET /api/v1/recordings` should list the new WAV with a non-zero `size_bytes`.
- Download/playback via `GET /api/v1/recordings/<name>` should work in the browser.

