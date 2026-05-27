#!/usr/bin/env python3
"""Convert an M5Dial `dumpfb` console transcript to a PNG.

Usage:
  python3 02-dumpfb-to-png.py dumpfb.txt screenshot.png
  python3 02-dumpfb-to-png.py < dumpfb.txt > screenshot.png

The firmware emits a 2-bit packed framebuffer as 240 ROW lines plus an RGB565
palette. This script has no third-party dependencies; it writes a minimal PNG
with zlib-compressed RGB scanlines.
"""

from __future__ import annotations

import argparse
import binascii
import re
import struct
import sys
import zlib
from pathlib import Path

BEGIN_RE = re.compile(r"DUMPFB_BEGIN\s+width=(\d+)\s+height=(\d+)\s+bpp=(\d+)\s+bytes=(\d+)\s+palette=(.*)")
PALETTE_RE = re.compile(r"PALETTE\s+([0-9A-Fa-f]{4})\s+([0-9A-Fa-f]{4})\s+([0-9A-Fa-f]{4})\s+([0-9A-Fa-f]{4})")
ROW_RE = re.compile(r"ROW\s+(\d{3})\s+([0-9A-Fa-f]+)")


def rgb565_to_rgb888(value: int) -> tuple[int, int, int]:
    r5 = (value >> 11) & 0x1F
    g6 = (value >> 5) & 0x3F
    b5 = value & 0x1F
    # Bit replication maps full-scale 5/6-bit channels to 255 exactly.
    r = (r5 << 3) | (r5 >> 2)
    g = (g6 << 2) | (g6 >> 4)
    b = (b5 << 3) | (b5 >> 2)
    return r, g, b


def parse_dump(text: str) -> tuple[int, int, list[tuple[int, int, int]], list[bytes]]:
    width = height = bpp = expected_bytes = None
    palette: list[tuple[int, int, int]] | None = None
    rows: dict[int, bytes] = {}

    for line in text.splitlines():
        if match := BEGIN_RE.search(line):
            width = int(match.group(1))
            height = int(match.group(2))
            bpp = int(match.group(3))
            expected_bytes = int(match.group(4))
            continue
        if match := PALETTE_RE.search(line):
            palette = [rgb565_to_rgb888(int(match.group(i), 16)) for i in range(1, 5)]
            continue
        if match := ROW_RE.search(line):
            y = int(match.group(1))
            rows[y] = bytes.fromhex(match.group(2))

    if width is None or height is None or bpp is None or expected_bytes is None:
        raise SystemExit("missing DUMPFB_BEGIN line")
    if bpp != 2:
        raise SystemExit(f"unsupported bpp={bpp}; expected 2")
    if palette is None:
        raise SystemExit("missing PALETTE line")

    bytes_per_row = width // 4
    if expected_bytes != bytes_per_row * height:
        raise SystemExit(f"byte count mismatch: header={expected_bytes}, computed={bytes_per_row * height}")

    ordered_rows: list[bytes] = []
    for y in range(height):
        if y not in rows:
            raise SystemExit(f"missing row {y:03d}")
        if len(rows[y]) != bytes_per_row:
            raise SystemExit(f"row {y:03d} has {len(rows[y])} bytes; expected {bytes_per_row}")
        ordered_rows.append(rows[y])

    return width, height, palette, ordered_rows


def png_chunk(kind: bytes, payload: bytes) -> bytes:
    return (
        struct.pack(">I", len(payload))
        + kind
        + payload
        + struct.pack(">I", binascii.crc32(kind + payload) & 0xFFFFFFFF)
    )


def rows_to_png(width: int, height: int, palette: list[tuple[int, int, int]], rows: list[bytes]) -> bytes:
    raw = bytearray()
    for y in range(height):
        raw.append(0)  # PNG filter type 0: none
        row = rows[y]
        for x in range(width):
            packed = row[x >> 2]
            color_index = (packed >> ((x & 3) * 2)) & 0x03
            raw.extend(palette[color_index])

    signature = b"\x89PNG\r\n\x1a\n"
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)  # 8-bit RGB
    return signature + png_chunk(b"IHDR", ihdr) + png_chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + png_chunk(b"IEND", b"")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", nargs="?", help="dumpfb transcript; stdin if omitted or '-'")
    parser.add_argument("output", nargs="?", help="output PNG; stdout if omitted or '-'")
    args = parser.parse_args()

    if args.input and args.input != "-":
        text = Path(args.input).read_text()
    else:
        text = sys.stdin.read()

    width, height, palette, rows = parse_dump(text)
    png = rows_to_png(width, height, palette, rows)

    if args.output and args.output != "-":
        Path(args.output).write_bytes(png)
    else:
        sys.stdout.buffer.write(png)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
