---
Title: 'Playbook: bundle GIF assets into flash (FATFS + parttool) and play them on AtomS3R'
Ticket: 0010-PORT-ANIMATED-GIF
Status: active
Topics:
    - esp32s3
    - esp-idf
    - atoms3r
    - gif
    - animatedgif
    - fatfs
    - assets
    - parttool
    - flash
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/README.md
      Note: End-to-end “real GIF playback” tutorial (console + button + decoder + storage partition)
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/partitions.csv
      Note: Partition layout (factory app + storage FATFS)
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/make_storage_fatfs.sh
      Note: Host helper to generate storage.bin using ESP-IDF fatfsgen.py
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/flash_storage.sh
      Note: Host helper to write storage.bin using parttool.py
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/convert_assets_to_128x128_crop.sh
      Note: Host helper to crop GIFs to 128x128 without padding
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/trim_gif_128x128_frames.sh
      Note: Host helper to crop to 128x128 and keep only first N frames (e.g. 30)
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/main/gif_storage.cpp
      Note: FATFS mount strategy (read-only, no wear levelling) + directory scan fallback
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/main/hello_world_main.cpp
      Note: AnimatedGIF file-callback open (streaming) + console/button control plane
ExternalSources: []
Summary: "Practical, copy/paste workflow for turning GIFs into a flash-bundled storage image and playing them on-device, including the real-world kinks we hit."
LastUpdated: 2025-12-24T00:00:00.000000000-05:00
WhatFor: "Repeatable dev workflow: get GIFs onto device flash and play them via console/button."
WhenToUse: "When you want to add/change GIF animations without rebuilding firmware (flash only the storage partition)."
---

# Playbook: bundle GIF assets into flash (FATFS + parttool) and play them on AtomS3R

## Goal

This playbook describes a reliable, repeatable workflow for getting multiple GIF animations onto an AtomS3R by **bundling them into a FATFS flash partition** and flashing that partition with `parttool.py`. It also documents the pitfalls we hit in practice (mount failures, filename weirdness, heap exhaustion) and the fixes that made the pipeline stable.

The end state is the one you want for fast iteration: you can change animations by **rebuilding `storage.bin`** and reflashing just the `storage` partition, without touching the firmware binary.

## Context (what we’re building and why)

There are two common ways to “ship assets” on ESP-IDF: embed bytes into firmware (fast to implement, slow to iterate) or store files in a flash partition and load them at runtime (slightly more plumbing, much faster iteration). For GIF iteration, the second path wins: it keeps firmware rebuilds rare and makes asset updates a single command.

In this repo, `esp32-s3-m5/0013-atoms3r-gif-console/` is the concrete implementation of the approach: it mounts a FATFS partition named `storage` at `/storage`, enumerates `*.gif`, and plays them with bitbank2/AnimatedGIF selected via `esp_console` and a GPIO button.

This playbook is intentionally written for two audiences:

- **People iterating on assets**: “how do I get GIFs into flash quickly?”
- **People building their own firmware**: “what APIs and patterns do I implement so my app can load those assets?”

## Quick Reference (copy/paste)

These are the commands you’ll run most often. They assume:

- you already have ESP-IDF 5.4.1 environment active (so `idf.py` and `parttool.py` are on PATH)
- your device is on `/dev/ttyACM0`

### 1) Normalize GIFs to AtomS3R-friendly format (optional but recommended)

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0013-atoms3r-gif-console && \
./convert_assets_to_128x128_crop.sh --force
```

If a GIF is still huge (hundreds of frames), keep only the first N frames:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0013-atoms3r-gif-console && \
./trim_gif_128x128_frames.sh assets/big.gif assets/big_128x128_30f.gif 30
```

### 2) Build a FATFS image (`storage.bin`)

By default this encodes `./assets` into `./storage.bin` sized to match `partitions.csv`:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0013-atoms3r-gif-console && \
./make_storage_fatfs.sh
```

### 3) Flash firmware (only when partition table or code changed)

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0013-atoms3r-gif-console && \
idf.py -p /dev/ttyACM0 flash
```

### 4) Flash the `storage` partition (fast loop; do this for asset changes)

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0013-atoms3r-gif-console && \
./flash_storage.sh /dev/ttyACM0 storage.bin
```

### 5) Validate on-device

In the console:

- `list`
- `play 0` or `play <filename>`
- `next`
- `info`

## Mental model: how the pieces fit together

This pipeline has three layers that you want to keep conceptually separate:

1. **Host asset preparation**: crop/trim GIFs so they fit the display and flash constraints.
2. **Host filesystem image build**: take a directory of GIF files and produce a binary FATFS image (`storage.bin`) with ESP-IDF `fatfsgen.py`.
3. **Device runtime**: mount the `storage` partition and stream GIF bytes into AnimatedGIF using file callbacks.

This separation matters because most “mysterious” failures are actually layer mismatches (e.g. mounting with wear levelling while flashing a non-WL image).

## Repository conventions (important details)

### Where GIFs live (host-side)

`0013` uses an `assets/` directory for convenience:

- put source GIFs into `esp32-s3-m5/0013-atoms3r-gif-console/assets/`
- git ignores `*.gif` in this directory to avoid committing binary blobs

The conversion scripts write `*_128x128.gif` (and optionally `*_128x128_30f.gif`) alongside the originals.

### Where GIFs live (device-side)

The device mounts:

- partition label: `storage`
- mount point: `/storage`

The firmware scans:

- preferred: `/storage/gifs`
- fallback: `/storage` (for “flat” images without a `gifs/` subdirectory)

## Step-by-step: build and flash the storage image

This section is intentionally procedural. Read it once, then use “Quick Reference” above day-to-day.

### Step A — verify partition sizes and flash constraints

Before you start throwing large GIFs at the device, check the constraints:

- **Flash size**: AtomS3R variants commonly have **8MB flash**.
- **App partition size**: it’s tempting to leave it at 4MB, but that steals space from `storage`.

In our working setup, we reduced the app partition to 2MB because the firmware is far smaller than that, and used the freed space for `storage`.

**Structured checks:**
- `esp32-s3-m5/0013-atoms3r-gif-console/partitions.csv` defines the exact layout.
- `idf.py build` prints a partition table summary and the “smallest app partition” check.

### Step B — normalize GIFs (crop + trim)

On a 128×128 display, padding wastes pixels. Cropping is typically what you want visually, and it reduces per-frame work.

**Structured commands:**
- Crop to 128×128 (no padding):
  - `./convert_assets_to_128x128_crop.sh`
- If a GIF is still too large:
  - `./trim_gif_128x128_frames.sh <in> <out> 30`

**Practical note:** “too large” is often driven by **frame count**, not resolution. A 128×128 GIF with hundreds of frames can still be multi‑MB.

### Step C — build `storage.bin` with ESP-IDF fatfsgen

`make_storage_fatfs.sh` wraps ESP-IDF’s `fatfsgen.py`. It exists so you don’t have to remember paths and flags.

**Command:**

```bash
./make_storage_fatfs.sh
```

If you want to package only a subset of files, point it at a different directory:

```bash
tmpdir="$(mktemp -d)" && \
cp assets/*_128x128*.gif "$tmpdir/" && \
./make_storage_fatfs.sh "$tmpdir" ./storage.bin
```

### Step D — flash firmware (only when needed)

You must flash firmware if:

- you changed `partitions.csv` (partition table)
- you changed FATFS config (e.g. long filename support)
- you changed the player code

**Command:**

```bash
idf.py -p /dev/ttyACM0 flash
```

### Step E — flash `storage` partition with parttool

This is the fast loop.

**Command:**

```bash
./flash_storage.sh /dev/ttyACM0 storage.bin
```

## Firmware implementation playbook (how to build your own flash-loaded player)

The host pipeline only pays off if your firmware has a clean “asset provider” layer: mount the partition, enumerate files, and open/stream bytes into your decoder. This section describes the minimal contracts and the ESP-IDF APIs that matter, using our AtomS3R GIF console (`0013`) as a known-good reference.

If you build your own firmware, aim for the same separation:

- **Control plane** (console/button/UI) selects an asset by ID/name.
- **Asset layer** resolves that selection to a path in the mounted FATFS.
- **Decoder layer** reads bytes via callbacks and draws frames into your display surface.

### 1) Partition table: declare a FATFS `storage` partition

Start by defining a partition label and size. A minimal table looks like:

- `nvs` + `phy_init` (standard)
- `factory` app partition
- `storage` data partition (`fat` subtype)

**Important**: storage size is usually where you end up spending time. GIFs get big fast, so don’t over-allocate the app partition “just because”. In our working setup we shrank the app partition to **2MB** (firmware is ~0.4MB) and used the space for a ~6MB `storage`.

### 2) Mount strategy: mount the flashed FATFS image read-only (no wear levelling)

This is the single biggest gotcha we hit.

If you generate a FATFS image on the host with `fatfsgen.py` and flash it raw into a partition, you should mount it using:

- `esp_vfs_fat_spiflash_mount_ro(base_path, partition_label, mount_config)`

Do **not** mount it with wear levelling (`esp_vfs_fat_spiflash_mount_rw_wl`) unless you also formatted/wrote the filesystem through that WL layer. WL expects metadata and can make a perfectly valid “prebuilt” image look like garbage to FatFs (manifesting as `f_mount failed (13)` / `FR_NO_FILESYSTEM`).

**Mount config fields worth caring about:**
- `format_if_mount_failed`: keep this **false** for “prebuilt images” so you don’t accidentally wipe your assets on a mount error.
- `max_files`: set based on how many files you keep open concurrently (usually small).

### 3) FATFS configuration: enable long filenames (LFN)

If you pack real filenames into the FATFS image, you want LFN enabled in firmware. Otherwise you’ll see 8.3 aliases (with `~`) and you can hit confusing open failures.

**Recommended Kconfig defaults:**
- `CONFIG_FATFS_LFN_HEAP=y`
- `CONFIG_FATFS_MAX_LFN=255`

This costs some RAM, but it’s worth it for reliability and “human debuggability”.

### 4) Enumerate assets: directory scan with a fallback

On the device, you typically want a simple “list GIFs” operation:

- open a directory with `opendir()`
- filter to `*.gif` (case-insensitive)
- build a stable list of full paths

In practice, two on-flash layouts are common:

- **Preferred**: `/storage/gifs/*.gif` (keeps root clean)
- **Flat**: `/storage/*.gif` (simplest fatfsgen input directory)

Our implementation supports both: try `/storage/gifs`, fall back to `/storage`.

### 5) Avoid heap explosions: stream GIFs from FATFS (don’t malloc the whole file)

Even “small” GIFs can be 200–600KB. On an ESP32-S3 you may only have ~300KB of free internal heap at runtime, and you might not have PSRAM enabled. If you do:

- `malloc(file_size); fread(file_size); open(buf, size, ...)`

…it will work for tiny GIFs and then fail mysteriously for the rest (`ESP_ERR_NO_MEM`).

The robust solution is to use AnimatedGIF’s file API:

- `AnimatedGIF::open(path, open_cb, close_cb, read_cb, seek_cb, GIFDraw)`

Your callbacks can wrap `FILE*` on top of FATFS:

- **open_cb**: `fopen(path,"rb")`, compute size with `fseek/ftell`, return `FILE*`
- **read_cb**: `fread(buf, 1, len, file)`, update `GIFFILE::iPos`
- **seek_cb**: `fseek(file, pos, SEEK_SET)`, update `GIFFILE::iPos`
- **close_cb**: `fclose(file)`

This makes GIF size far less coupled to heap availability (the decoder uses small internal buffers).

### 6) Decode loop contract: `playFrame()` semantics matter

AnimatedGIF uses an API contract that’s easy to mis-handle if you assume “0 means failure”.

**Correct mental model:**
- `open(...)` returns **1 success / 0 failure** (not a `GIF_*` error code)
- `playFrame(...)` returns:
  - `> 0`: frame decoded, more frames likely
  - `== 0`: EOF reached (a frame may still have been rendered; check `getLastError()`)
  - `< 0`: error

In your playback loop:
- present the canvas if a frame was rendered (even if `playFrame()` returns 0 at EOF)
- apply a minimum delay so you don’t spin in a tight loop on 0ms delays
- on EOF: `reset()` and yield (avoid watchdog starvation)

### 7) Rendering contract: keep “one owner” of the display

The simplest way to avoid flicker and concurrency bugs is to ensure exactly one task “owns” display writes. Control-plane inputs (console/button) should enqueue events; the playback loop should apply them and perform all rendering.

This matters more than it sounds: race-y display usage looks like random GIF artifacts, even when the decoder is correct.

## What the helper scripts actually do (inputs/outputs and underlying tools)

These scripts are intentionally tiny wrappers around a few standard tools. This section explains them so you can reimplement the workflow in your own project (or adjust it when your constraints differ).

### `convert_assets_to_128x128_crop.sh`

This script converts all `assets/*.gif` into `*_128x128.gif` by:

- **coalescing** frames (expand partial-frame GIFs into full frames)
- resizing to fill a 128×128 square while preserving aspect ratio
- **center-cropping** down to 128×128 (no padding)
- optimizing layers to reduce output size

Under the hood it prefers ImageMagick (IM6 `convert`) when available and falls back to `ffmpeg` (with palettegen/paletteuse) when not.

### `trim_gif_128x128_frames.sh`

This script is the “escape hatch” for huge GIFs. It takes:

- an input GIF (often hundreds of frames)
- keeps only the first N frames (e.g. 30)
- then applies the same coalesce → resize → center-crop pipeline to produce a small looping asset

This is the fastest way to turn a 10–20MB “internet GIF” into something flash-friendly.

### `make_storage_fatfs.sh`

This script produces the actual image you flash:

- **input**: a directory of files (by default `./assets`)
- **output**: `./storage.bin`
- **size**: defaults to the byte size matching `partitions.csv`’s `storage` partition

Under the hood it calls ESP-IDF’s:

- `$IDF_PATH/components/fatfs/fatfsgen.py`

The key flag is `--partition_size`, because you want the image to match the partition.

### `flash_storage.sh`

This script writes `storage.bin` into the `storage` partition label using:

- `parttool.py write_partition --partition-name=storage --input storage.bin`

If you’re building your own project, this is the core command to copy.

## Troubleshooting (real kinks we hit)

This section is the “why this playbook exists” part. These are the failure modes that look mysterious until you’ve seen them once.

### 1) `f_mount failed (13)`

**Symptom:**
- `vfs_fat_spiflash: f_mount failed (13)`

**What it means:**
- FatFs `FR_NO_FILESYSTEM`: the mount layer can’t find a valid FAT filesystem.

**The important gotcha:**
- If you flash a FATFS image produced by `fatfsgen.py`, do **not** mount it through **wear levelling** (`esp_vfs_fat_spiflash_mount_rw_wl`).
  - WL expects its own metadata and will present the wrong view to FatFs.

**Fix:**
- Mount the flashed image read-only without WL:
  - `esp_vfs_fat_spiflash_mount_ro(...)`

### 2) Filenames look like `200_12~.GIF` and `fopen` fails

**Symptom:**
- Directory listing shows 8.3 shortnames (with `~`) and `fopen` returns `errno=2`.

**Root cause:**
- FATFS long filename (LFN) support is disabled (`CONFIG_FATFS_LFN_NONE=y`).

**Fix:**
- Enable LFN (heap) and set a reasonable max length:
  - `CONFIG_FATFS_LFN_HEAP=y`
  - `CONFIG_FATFS_MAX_LFN=255`
  - Rebuild + flash firmware (this is firmware config, not a storage image change).

### 3) `malloc failed for N bytes` when opening a GIF

**Symptom:**
- `gif_storage: malloc failed for 633484 bytes`
- `ESP_ERR_NO_MEM`

**Root cause:**
- The naive implementation reads the whole GIF into RAM before decoding.
- Internal heap on-device is often ~300KB free, so anything above that fails.

**Fix:**
- Stream the GIF from FATFS using AnimatedGIF’s file callback API:
  - `AnimatedGIF::open(path, open_cb, close_cb, read_cb, seek_cb, GIFDraw)`

### 4) `No free cluster available!` while building `storage.bin`

**Symptom:**
- `fatfs_utils.exceptions.NoFreeClusterException: No free cluster available!`

**Root cause:**
- Your partition image is too small for the files (and FAT overhead).

**Fix:**
- Increase the `storage` partition size in `partitions.csv` and regenerate `storage.bin`.
- If you increase `storage`, you may need to reduce the app partition size (if flash is only 8MB).

### 5) Flash fails: port busy

**Symptom:**
- `Could not exclusively lock port /dev/ttyACM0`

**Root cause:**
- A monitor or serial program is still holding the port.

**Fix:**
- Stop the monitor, close the terminal, then flash again.

## Suggested defaults (why these settings are “the happy path”)

This repo’s current working setup is opinionated for fast iteration:

- **FATFS image flashed with parttool**: fast asset updates without firmware rebuild.
- **Mount read-only**: avoids wear-levelling/image mismatch and is sufficient for asset playback.
- **LFN enabled**: keeps filenames human-readable and avoids fragile 8.3 aliases.
- **Stream from file**: avoids large heap allocations and makes “moderate” GIF sizes practical even without PSRAM.

These choices trade away “write assets on device” in exchange for a clean dev workflow. If we later need on-device writes, we can revisit WL + formatting + an on-device asset writer.


