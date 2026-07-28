---
Title: "02-gen-diagnostic-g4"
Ticket: ESP-54-PULP-GALLERY
Status: active
Topics: [papers3, esp32s3, image-upload, tooling]
DocType: reference
Summary: "ESP-54 helper script"
---
# Clean diagnostic .g4: horizontal bands (vary by Y only) + a solid block.
# If horizontal bands appear diagonal on the panel, the stride is wrong
# (a real shear bug). If they appear horizontal, the rasterizer is correct
# and the earlier "diagonal stripes" were just the (x+y)%16 test image.
import struct
W, H = 540, 960
row_bytes = (W + 1) // 2
header = struct.pack('<4sHHBBH', b'G4IM', W, H, 4, 1, 0)
pixels = bytearray(row_bytes * H)
for y in range(H):
    # 16 horizontal bands, each 60 px tall, full width.
    band = (y // 60) % 16
    for x in range(W):
        g = band
        # Solid square in the top-left corner at gray 0 (black) to anchor
        # orientation: 0..199 x, 0..199 y.
        if x < 200 and y < 200:
            g = 0
        # White cross to confirm orientation.
        if (260 <= x <= 280 and y < 400) or (260 <= y <= 280 and x < 400):
            g = 15
        off = y * row_bytes + (x >> 1)
        if x & 1:
            pixels[off] |= g
        else:
            pixels[off] = g << 4
with open('clean.g4', 'wb') as f:
    f.write(header)
    f.write(pixels)
import os
print(f"wrote clean.g4: {os.path.getsize('clean.g4')} bytes")
