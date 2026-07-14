# PaperS3 factory firmware V0.5 provenance

- Upstream repository: <https://github.com/m5stack/M5PaperS3-UserDemo>
- Release: <https://github.com/m5stack/M5PaperS3-UserDemo/releases/tag/V0.5>
- Source tag/commit: `V0.5` / `5e275ad4b70abb85f7193fda137844730e64c4db`
- Release asset: `C139-PaperS3-FactoryTest-V0.5_0x0.bin`
- Asset URL: <https://github.com/m5stack/M5PaperS3-UserDemo/releases/download/V0.5/C139-PaperS3-FactoryTest-V0.5_0x0.bin>
- Asset size: 1,439,168 bytes
- SHA-256: `d6733a0ca378f95335fa5fba4d4d992fb1dd97c17557b20e9aebfca08ba6d624`
- Download command: `gh release download V0.5 --repo m5stack/M5PaperS3-UserDemo --pattern C139-PaperS3-FactoryTest-V0.5_0x0.bin`
- Flash address: `0x0` (the upstream asset is a merged image)
- Flash completed: 2026-07-14T19:33:09Z

## Why this is the control

The official V0.5 README pins ESP-IDF 5.3.3. Its `repos.json` selects M5GFX 0.2.15 and M5Unified 0.2.10. Flashing M5Stack's published merged image avoids substituting the locally installed ESP-IDF 5.3.4/5.4.2 or the modified sibling M5GFX checkout.

The release is treated as a black-box factory control. The release page associates the binary with tag V0.5, but the binary itself does not independently prove every source dependency SHA.

## Built-in optical sequence

At tag V0.5, `main/main.cpp::boot_display_test()` performs this sequence on every boot:

1. `FactoryTest: V0.5` in QUALITY mode; 1,000 ms hold.
2. Full-screen `TFT_BLACK` in QUALITY mode; 2,000 ms hold.
3. Full-screen `TFT_WHITE` in QUALITY mode; 2,000 ms hold.
4. Sixteen grayscale bars from `0xffffff` through `0x000000` in QUALITY mode; 2,000 ms hold.
5. The normal factory dashboard.

This is precisely the unmodified black/white/grayscale baseline needed before local LUT experimentation.

## Verification and flash notes

`esptool image_info` detected a valid ESP32-S3 image with a valid checksum and validation hash. The first invocation used the wrong esptool v4 spelling, `image-info`, and failed with:

```text
esptool: error: argument operation: invalid choice: 'image-info'
```

The corrected operation was `image_info`.

The Cell D monitor was exited with `Ctrl-]` before flashing. `fuser` then reported no owner for the serial device. Flashing used esptool v4.11.0 at 115200 baud and completed with `Hash of data verified.` See `01-flash-transcript.txt`.
