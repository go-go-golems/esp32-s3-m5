---
Title: "04-gen-test-jpg"
Ticket: ESP-54-PULP-GALLERY
Status: active
Topics: [papers3, esp32s3, image-upload, tooling]
DocType: reference
Summary: "ESP-54 helper script"
---
# Generate a simple test JPEG (a colored gradient).
try:
    from PIL import Image
    img = Image.new('RGB', (800, 1200))
    for y in range(1200):
        for x in range(800):
            img.putpixel((x, y), ((x*255)//800, (y*255)//1200, 128))
    img.save('test.jpg', 'JPEG', quality=85)
    import os
    print(f"wrote test.jpg: {os.path.getsize('test.jpg')} bytes")
except ImportError:
    # No PIL: generate a minimal valid JPEG is complex; fall back to PNG via struct
    # Actually, let's just generate a PNG which the browser also accepts.
    import struct, zlib
    W, H = 800, 1200
    raw = bytearray()
    for y in range(H):
        raw.append(0)  # filter byte
        for x in range(W):
            raw += bytes([(x*255)//W, (y*255)//H, 128])
    def chunk(t, d):
        c = t + d
        return struct.pack('>I', len(d)) + c + struct.pack('>I', zlib.crc32(c) & 0xffffffff)
    sig = b'\x89PNG\r\n\x1a\n'
    ihdr = struct.pack('>IIBBBBB', W, H, 8, 2, 0, 0, 0)
    idat = zlib.compress(bytes(raw))
    png = sig + chunk(b'IHDR', ihdr) + chunk(b'IDAT', idat) + chunk(b'IEND', b'')
    open('test.png', 'wb').write(png)
    import os
    print(f"wrote test.png: {os.path.getsize('test.png')} bytes (no PIL)")
