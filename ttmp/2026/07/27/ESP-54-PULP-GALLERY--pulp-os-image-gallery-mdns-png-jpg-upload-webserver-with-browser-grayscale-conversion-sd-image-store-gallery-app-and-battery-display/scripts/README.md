---
Title: "ESP-54 helper scripts"
Ticket: ESP-54-PULP-GALLERY
Status: active
Topics:
    - papers3
    - esp32s3
    - image-upload
    - tooling
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Host-side test image generators and .g4 decoder for the ESP-54 gallery upload + bitmap-blit pipeline."
LastUpdated: 2026-07-27T23:26:00.000000000-04:00
WhatFor: Generating and inspecting .g4 test frames for the gallery upload pipeline.
WhenToUse: Run from a host on the PaperS3 WiFi to test upload + display.
---

# ESP-54 PULP Gallery — scripts

Helper scripts for the ESP-54 ticket. Run from a host on the same WiFi as the
PaperS3 (reach it at `http://pulp.local`). Numbered prefixes preserve
execution order.

## Test image generators (host-side, produce .g4 / .jpg files)

- `01-gen-gradient-g4.py` — generates `test.g4`, a 540×960 4-bit grayscale
  frame where `gray = (x + y) % 16`. This is the *diagonal* gradient used to
  exercise the upload + bitmap-blit pipeline. (Its diagonal appearance is by
  design — it is NOT a rasterizer bug; see `02` for the orientation test.)
- `02-gen-diagnostic-g4.py` — generates `clean.g4`: 16 **horizontal** bands,
  a black square top-left, and a white cross. If the bands appear diagonal on
  the panel, the stride is wrong (a real shear bug). If they appear
  horizontal, the rasterizer is correct.
- `04-gen-test-jpg.py` — generates a `test.jpg` (PIL if available, else PNG)
  for exercising the browser decode → quantize → POST path.

## Decoders (host-side, inspect a .g4 file)

- `03-decode-g4-to-pgm.py` — decodes a `.g4` to a PGM/PNG so you can SEE the
  image content on the host (verifies the format and the stored pixels).

## Usage

```bash
# 1. Generate a test frame and upload it via the browser UI
python3 01-gen-gradient-g4.py          # -> test.g4
curl -X POST -H 'Content-Type: application/octet-stream' \
     --data-binary @test.g4 http://pulp.local/images/upload

# 2. Verify the rasterizer has no shear bug
python3 02-gen-diagnostic-g4.py        # -> clean.g4
curl -X POST --data-binary @clean.g4 http://pulp.local/images/upload
# then on the device: open Gallery, view the clean image — bands must be HORIZONTAL

# 3. Inspect a stored .g4 on the host (copy it off the SD or re-download)
python3 03-decode-g4-to-pgm.py path/to/img.g4 out.pgm
```

## The .g4 format

12-byte header + packed 4-bit grayscale pixels (2 px/byte, high nibble first,
row-major, width padded to an even byte count):

```
offset  size  field
0       4     magic "G4IM"
4       2     width  (uint16 LE; 540 for a full frame)
6       2     height (uint16 LE; 960 for a full frame)
8       1     depth  (4)
9       1     version (1)
10      2     reserved (0)
12      ...   pixel data (ceil(width/2) * height bytes)
```

A full 540×960 frame is 259,200 bytes of pixel data (~253 KiB total).
