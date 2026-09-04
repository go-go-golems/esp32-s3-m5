---
Title: "01-gen-gradient-g4"
Ticket: ESP-54-PULP-GALLERY
Status: active
Topics: [papers3, esp32s3, image-upload, tooling]
DocType: reference
Summary: "ESP-54 helper script"
---
# Generate a valid 540x960 4-bit grayscale .g4 frame (a gradient).
import struct
W, H = 540, 960
row_bytes = (W + 1) // 2  # 270
header = struct.pack('<4sHHBBH', b'G4IM', W, H, 4, 1, 0)
pixels = bytearray(row_bytes * H)
for y in range(H):
    for x in range(W):
        # diagonal gradient 0..15
        g = (x + y) % 16
        off = y * row_bytes + (x >> 1)
        if x & 1:
            pixels[off] |= g
        else:
            pixels[off] = g << 4
with open('test.g4', 'wb') as f:
    f.write(header)
    f.write(pixels)
import os
print(f"wrote test.g4: {os.path.getsize('test.g4')} bytes")
