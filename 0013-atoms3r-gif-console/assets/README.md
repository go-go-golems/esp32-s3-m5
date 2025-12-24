## GIF assets (local-only)

Drop your `*.gif` files into this folder if you want `./make_storage_fatfs.sh` to package them into `storage.bin`.

### Convert arbitrary GIFs to AtomS3R-friendly 128×128 (crop, no padding)

If your source GIFs aren’t square, convert them to **128×128** by center-cropping (aspect-ratio preserving, no letterboxing):

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0013-atoms3r-gif-console && \
./convert_assets_to_128x128_crop.sh
```

### If a GIF is still too large: limit to first N frames (e.g. 30)

Some GIFs have **hundreds of frames** and remain huge even at 128×128. You can keep only the first N frames while still cropping to 128×128:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0013-atoms3r-gif-console && \
./trim_gif_128x128_frames.sh assets/input.gif assets/input_128x128_30f.gif 30
```

Notes:

- GIF binaries in this folder are **ignored by git** (see `.gitignore`).
- The device scans `/storage/gifs` (preferred) and falls back to `/storage` (flat FATFS images).


