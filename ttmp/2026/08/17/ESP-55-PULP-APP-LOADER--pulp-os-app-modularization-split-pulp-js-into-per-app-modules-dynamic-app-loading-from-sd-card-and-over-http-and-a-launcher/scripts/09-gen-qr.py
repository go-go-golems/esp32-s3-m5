#!/usr/bin/env python3
"""ESP-55 P10: regenerate the QR row-bitmask literal embedded in
tools/js/apps/settings.js (webScreen). The encoded string is constant
(http://pulp.local/apps — mDNS makes it IP-independent), so the matrix is
precomputed here rather than encoded on-device.

usage: 09-gen-qr.py [text]     (default: http://pulp.local/apps)
Prints QR_N and the QR_ROWS ES5 literal; paste into settings.js.
"""
import sys
import qrcode

text = sys.argv[1] if len(sys.argv) > 1 else "http://pulp.local/apps"
qr = qrcode.QRCode(border=0, error_correction=qrcode.constants.ERROR_CORRECT_L)
qr.add_data(text)
qr.make()
m = qr.modules
n = len(m)
rows = []
for row in m:
    v = 0
    for c, bit in enumerate(row):
        if bit:
            v |= 1 << c
    rows.append(v)
print(f"// {text} — version with {n} modules")
print(f"var QR_N = {n};")
body = ",\n  ".join(", ".join(str(x) for x in rows[i:i+5])
                    for i in range(0, n, 5))
print(f"var QR_ROWS = [{body}];")
runs = sum(1 for row in m for i, b in enumerate(row)
           if b and (i == 0 or not row[i-1]))
print(f"// total row-runs: {runs} (canvas cap is 96/slot -> split rows)")
