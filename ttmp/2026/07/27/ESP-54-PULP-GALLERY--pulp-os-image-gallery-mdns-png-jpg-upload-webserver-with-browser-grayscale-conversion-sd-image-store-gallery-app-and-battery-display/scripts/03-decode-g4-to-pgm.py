---
Title: "03-decode-g4-to-pgm"
Ticket: ESP-54-PULP-GALLERY
Status: active
Topics: [papers3, esp32s3, image-upload, tooling]
DocType: reference
Summary: "ESP-54 helper script"
---
import struct, sys
# Decode a .g4 to a PNG (or PPM) so we can SEE what the image content is.
def decode(path, out):
    with open(path,'rb') as f:
        data=f.read()
    magic=data[:4]; w,h=struct.unpack('<HH',data[4:8]); depth=data[8]; ver=data[9]
    assert magic==b'G4IM', magic
    rowB=(w+1)//2
    # Write PPM (easy, no deps)
    with open(out,'wb') as o:
        o.write(f'P5\n{w} {h}\n255\n'.encode())
        for y in range(h):
            row=data[12+y*rowB:12+y*rowB+rowB]
            for x in range(w):
                nib = (row[x>>1]>>4) if (x&1)==0 else (row[x>>1]&0x0F)
                o.write(bytes([(nib*255)//15]))
    print(f'{path}: {w}x{h} depth={depth} ver={ver} -> {out}')
decode('test.g4','test.pgm')
