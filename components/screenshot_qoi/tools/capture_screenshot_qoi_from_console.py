#!/usr/bin/env python3

import argparse
import re
import struct
import time
from typing import Tuple

import serial  # type: ignore

HEADER_RE = re.compile(rb"^QOI_BEGIN (\d+)\n$")
QOI_MAGIC = b"qoif"
QOI_END = b"\x00\x00\x00\x00\x00\x00\x00\x01"


def read_exact(ser: serial.Serial, n: int) -> bytes:
    buf = bytearray()
    while len(buf) < n:
        chunk = ser.read(n - len(buf))
        if not chunk:
            continue
        buf.extend(chunk)
    return bytes(buf)


def qoi_hash(r: int, g: int, b: int, a: int) -> int:
    return (r * 3 + g * 5 + b * 7 + a * 11) % 64


def decode_qoi(data: bytes) -> Tuple[int, int, bytes]:
    if len(data) < 14 + 8:
        raise ValueError("qoi payload too short")
    if data[:4] != QOI_MAGIC:
        raise ValueError(f"unexpected qoi magic: {data[:4]!r}")
    if data[-8:] != QOI_END:
        raise ValueError("missing qoi end marker")

    w = int.from_bytes(data[4:8], byteorder="big", signed=False)
    h = int.from_bytes(data[8:12], byteorder="big", signed=False)
    channels = data[12]
    if channels not in (3, 4):
        raise ValueError(f"unsupported channels={channels}")
    if w <= 0 or h <= 0:
        raise ValueError(f"invalid dimensions {w}x{h}")

    stream = data[14:-8]
    p = 0
    px_count = w * h
    out = bytearray(px_count * 3)

    index = [(0, 0, 0, 0) for _ in range(64)]
    pr, pg, pb, pa = 0, 0, 0, 255
    run = 0

    for i in range(px_count):
        if run > 0:
            run -= 1
        else:
            if p >= len(stream):
                raise ValueError("unexpected end of qoi stream")
            b1 = stream[p]
            p += 1

            if b1 == 0xFE:  # RGB
                if p + 3 > len(stream):
                    raise ValueError("truncated qoi rgb op")
                pr, pg, pb = stream[p], stream[p + 1], stream[p + 2]
                p += 3
            elif b1 == 0xFF:  # RGBA
                if p + 4 > len(stream):
                    raise ValueError("truncated qoi rgba op")
                pr, pg, pb, pa = stream[p], stream[p + 1], stream[p + 2], stream[p + 3]
                p += 4
            else:
                tag = b1 & 0xC0
                if tag == 0x00:  # INDEX
                    idx = b1 & 0x3F
                    pr, pg, pb, pa = index[idx]
                elif tag == 0x40:  # DIFF
                    pr = (pr + ((b1 >> 4) & 0x03) - 2) & 0xFF
                    pg = (pg + ((b1 >> 2) & 0x03) - 2) & 0xFF
                    pb = (pb + (b1 & 0x03) - 2) & 0xFF
                elif tag == 0x80:  # LUMA
                    if p >= len(stream):
                        raise ValueError("truncated qoi luma op")
                    b2 = stream[p]
                    p += 1
                    dg = (b1 & 0x3F) - 32
                    dr_dg = ((b2 >> 4) & 0x0F) - 8
                    db_dg = (b2 & 0x0F) - 8
                    pr = (pr + dg + dr_dg) & 0xFF
                    pg = (pg + dg) & 0xFF
                    pb = (pb + dg + db_dg) & 0xFF
                else:  # RUN
                    run = b1 & 0x3F

        index[qoi_hash(pr, pg, pb, pa)] = (pr, pg, pb, pa)
        out[i * 3 + 0] = pr
        out[i * 3 + 1] = pg
        out[i * 3 + 2] = pb

    return w, h, bytes(out)


def write_ppm(path: str, w: int, h: int, rgb: bytes) -> None:
    with open(path, "wb") as f:
        f.write(f"P6\n{w} {h}\n255\n".encode("ascii"))
        f.write(rgb)


def write_bmp(path: str, w: int, h: int, rgb: bytes) -> None:
    row_raw = w * 3
    row_stride = (row_raw + 3) & ~3
    pixel_bytes = row_stride * h
    file_size = 54 + pixel_bytes

    with open(path, "wb") as f:
        # BITMAPFILEHEADER
        f.write(b"BM")
        f.write(struct.pack("<IHHI", file_size, 0, 0, 54))
        # BITMAPINFOHEADER
        f.write(struct.pack("<IIIHHIIIIII", 40, w, h, 1, 24, 0, pixel_bytes, 2835, 2835, 0, 0))

        pad = b"\x00" * (row_stride - row_raw)
        for y in range(h - 1, -1, -1):
            row = rgb[y * row_raw : (y + 1) * row_raw]
            bgr = bytearray(row_raw)
            for x in range(w):
                r = row[x * 3 + 0]
                g = row[x * 3 + 1]
                b = row[x * 3 + 2]
                bgr[x * 3 + 0] = b
                bgr[x * 3 + 1] = g
                bgr[x * 3 + 2] = r
            f.write(bgr)
            if pad:
                f.write(pad)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("port", help="Serial port (prefer /dev/serial/by-id/...)")
    ap.add_argument("out", help="Output QOI path")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--cmd", default="screenshot", help="Console command to send")
    ap.add_argument("--timeout-s", type=float, default=20.0)
    ap.add_argument("--ppm-out", default="", help="Optional decoded PPM output path")
    ap.add_argument("--bmp-out", default="", help="Optional decoded BMP output path")
    args = ap.parse_args()

    deadline = time.time() + args.timeout_s

    with serial.Serial(args.port, args.baud, timeout=0.2) as ser:
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        ser.write((args.cmd + "\r\n").encode("utf-8"))
        ser.flush()

        while True:
            if time.time() > deadline:
                raise TimeoutError("timed out waiting for QOI_BEGIN")

            line = ser.readline()
            if not line:
                continue
            m = HEADER_RE.match(line)
            if not m:
                continue

            length = int(m.group(1))
            if length <= 0:
                raise ValueError(f"invalid qoi length: {length}")

            data = read_exact(ser, length)

            while True:
                if time.time() > deadline:
                    raise TimeoutError("timed out waiting for QOI_END")
                trailer = ser.readline()
                if b"QOI_END" in trailer:
                    break

            with open(args.out, "wb") as f:
                f.write(data)
            print(f"wrote {args.out} ({len(data)} bytes)")

            if args.ppm_out or args.bmp_out:
                w, h, rgb = decode_qoi(data)
                if args.ppm_out:
                    write_ppm(args.ppm_out, w, h, rgb)
                    print(f"wrote {args.ppm_out} ({w}x{h})")
                if args.bmp_out:
                    write_bmp(args.bmp_out, w, h, rgb)
                    print(f"wrote {args.bmp_out} ({w}x{h})")

            return 0


if __name__ == "__main__":
    raise SystemExit(main())
