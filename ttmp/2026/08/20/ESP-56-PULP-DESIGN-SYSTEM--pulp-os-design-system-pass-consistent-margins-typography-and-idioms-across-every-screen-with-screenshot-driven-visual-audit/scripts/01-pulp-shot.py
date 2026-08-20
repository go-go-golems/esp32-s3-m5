#!/usr/bin/env python3
"""ESP-56: screenshot client for the PULP `shot` console command.

Holds the port open (cdc_acm-safe, no modem-control edges beyond the
open), optionally runs console commands first (taps to reach a screen),
then sends `shot`, captures the QOI_BEGIN <len> framed stream, decodes it
(pure-python QOI, from components/screenshot_qoi/tools) and saves a PNG.

usage:
  01-pulp-shot.py --out screen.png [--cmd "js tap 270 235"]... [--settle 3]
"""
from __future__ import annotations
import argparse, fcntl, os, re, select, sys, termios, time
from pathlib import Path
from PIL import Image

DEFAULT_PORT = Path(
    "/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_"
    "D0:CF:13:16:17:DC-if00")
HEADER_RE = re.compile(rb"QOI_BEGIN (\d+)\n")

def decode_qoi(data: bytes):
    if data[:4] != b"qoif":
        raise ValueError("bad magic")
    w = int.from_bytes(data[4:8], "big")
    h = int.from_bytes(data[8:12], "big")
    stream = data[14:-8]
    p = 0
    out = bytearray(w * h * 3)
    index = [(0, 0, 0, 0)] * 64
    pr, pg, pb, pa = 0, 0, 0, 255
    run = 0
    for i in range(w * h):
        if run > 0:
            run -= 1
        else:
            b0 = stream[p]; p += 1
            if b0 == 0xFE:
                pr, pg, pb = stream[p], stream[p+1], stream[p+2]; p += 3
            elif b0 == 0xFF:
                pr, pg, pb, pa = stream[p:p+4]; p += 4
            elif b0 >> 6 == 0:
                pr, pg, pb, pa = index[b0 & 0x3F]
            elif b0 >> 6 == 1:
                pr = (pr + ((b0 >> 4) & 3) - 2) & 255
                pg = (pg + ((b0 >> 2) & 3) - 2) & 255
                pb = (pb + (b0 & 3) - 2) & 255
            elif b0 >> 6 == 2:
                b1 = stream[p]; p += 1
                dg = (b0 & 0x3F) - 32
                pr = (pr + dg + ((b1 >> 4) & 15) - 8) & 255
                pg = (pg + dg) & 255
                pb = (pb + dg + (b1 & 15) - 8) & 255
            else:
                run = b0 & 0x3F
        index[(pr*3 + pg*5 + pb*7 + pa*11) % 64] = (pr, pg, pb, pa)
        out[i*3:i*3+3] = bytes((pr, pg, pb))
    return w, h, bytes(out)

def drain(fd, dur):
    end = time.monotonic() + dur
    buf = bytearray()
    while time.monotonic() < end:
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            try:
                c = os.read(fd, 65536)
            except OSError:
                break
            buf += c
    return bytes(buf)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=Path, default=DEFAULT_PORT)
    ap.add_argument("--cmd", action="append", default=[],
                    help="console commands to run before the shot")
    ap.add_argument("--settle", type=float, default=3.0)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    fd = os.open(a.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
    at = termios.tcgetattr(fd)
    at[0] = at[1] = at[3] = 0
    at[6][termios.VMIN] = at[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, at)
    for cmd in a.cmd:
        os.write(fd, cmd.encode() + b"\n")
        drain(fd, a.settle)
    os.write(fd, b"shot\n")
    buf = bytearray()
    deadline = time.monotonic() + 40
    payload = None
    while time.monotonic() < deadline:
        buf += drain(fd, 0.4)
        m = HEADER_RE.search(bytes(buf))
        if m:
            need = m.end() + int(m.group(1))
            while len(buf) < need and time.monotonic() < deadline:
                buf += drain(fd, 0.4)
            payload = bytes(buf[m.end():need])
            break
    os.close(fd)
    if payload is None:
        print("no QOI stream captured", file=sys.stderr)
        return 1
    w, h, rgb = decode_qoi(payload)
    Image.frombytes("RGB", (w, h), rgb).save(a.out)
    print(f"wrote {a.out} ({w}x{h}, {len(payload)} qoi bytes)")
    return 0

if __name__ == "__main__":
    sys.exit(main())
