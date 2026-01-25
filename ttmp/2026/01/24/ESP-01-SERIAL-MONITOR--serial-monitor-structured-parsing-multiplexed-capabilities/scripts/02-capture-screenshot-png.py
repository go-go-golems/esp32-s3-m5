#!/usr/bin/env python3

"""
Capture a framed PNG screenshot from an ESP32 device over serial.

Supports the two variants used in this repo:

1) Fixed-length payload:
   PNG_BEGIN <len>\n<raw png bytes len=<len>>\nPNG_END\n

2) Streamed payload (len==0):
   PNG_BEGIN 0\n<raw png bytes streamed>\nPNG_END\n
   In this mode, we parse the PNG format itself (signature + chunks until IEND)
   to know exactly where the PNG ends, then we consume the PNG_END footer to
   resynchronize.
"""

from __future__ import annotations

import argparse
import glob
import re
import sys
import time
from pathlib import Path

try:
    import serial  # type: ignore
except Exception as e:  # pragma: no cover
    raise SystemExit(f"pyserial is required (try: python3 -m pip install pyserial): {e}")


HEADER_RE = re.compile(rb"^PNG_BEGIN (\d+)\n$")
PNG_SIG = b"\x89PNG\r\n\x1a\n"


def pick_port(pattern: str) -> str:
    if any(ch in pattern for ch in "*?[]"):
        matches = sorted(glob.glob(pattern))
        if not matches:
            raise FileNotFoundError(f"no serial ports match pattern: {pattern!r}")
        if len(matches) > 1:
            # Deterministic choice, but let the user know.
            sys.stderr.write(f"note: multiple ports matched; using: {matches[0]}\n")
        return matches[0]
    return pattern


def read_exact(ser: serial.Serial, n: int, deadline: float | None) -> bytes:
    buf = bytearray()
    while len(buf) < n:
        if deadline is not None and time.time() > deadline:
            raise TimeoutError(f"timed out reading {n} bytes (got {len(buf)})")
        chunk = ser.read(n - len(buf))
        if not chunk:
            continue
        buf.extend(chunk)
    return bytes(buf)


def read_png_stream(ser: serial.Serial, deadline: float | None) -> bytes:
    sig = read_exact(ser, 8, deadline)
    if sig != PNG_SIG:
        raise ValueError(f"unexpected PNG signature: {sig!r}")

    out = bytearray(sig)
    while True:
        hdr = read_exact(ser, 8, deadline)
        out.extend(hdr)
        chunk_len = int.from_bytes(hdr[0:4], byteorder="big", signed=False)
        chunk_type = hdr[4:8]
        out.extend(read_exact(ser, chunk_len + 4, deadline))  # data + CRC
        if chunk_type == b"IEND":
            break
    return bytes(out)


def consume_until_footer(ser: serial.Serial, deadline: float | None) -> None:
    # Consume until footer line; tolerate any stray newlines/bytes.
    while True:
        if deadline is not None and time.time() > deadline:
            raise TimeoutError("timed out waiting for PNG_END")
        line = ser.readline()
        if not line:
            continue
        if b"PNG_END" in line:
            return


def capture_one(ser: serial.Serial, out_path: Path, deadline: float | None) -> int:
    while True:
        if deadline is not None and time.time() > deadline:
            raise TimeoutError("timed out waiting for PNG_BEGIN")
        line = ser.readline()
        if not line:
            continue
        m = HEADER_RE.match(line)
        if not m:
            continue

        length = int(m.group(1))
        if length > 0:
            data = read_exact(ser, length, deadline)
        else:
            data = read_png_stream(ser, deadline)

        consume_until_footer(ser, deadline)
        out_path.write_bytes(data)
        return len(data)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("port", help="Serial port (or glob; prefer /dev/serial/by-id/...)")
    ap.add_argument("out", help="Output PNG path")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument(
        "--cmd",
        default="",
        help="If set, write this command (plus CRLF) to the device before capturing (e.g. screenshot)",
    )
    ap.add_argument("--timeout-s", type=float, default=30.0)
    ap.add_argument("--no-reset-buffers", action="store_true", help="Do not reset serial input/output buffers")
    args = ap.parse_args()

    port = pick_port(args.port)
    out_path = Path(args.out)
    deadline = time.time() + args.timeout_s if args.timeout_s > 0 else None

    with serial.Serial(port, args.baud, timeout=0.2) as ser:
        if not args.no_reset_buffers:
            # Best effort: clear any buffered output before we issue the command.
            ser.reset_input_buffer()
            ser.reset_output_buffer()

        if args.cmd:
            ser.write((args.cmd + "\r\n").encode("utf-8"))
            ser.flush()

        n = capture_one(ser, out_path, deadline)
        sys.stdout.write(f"wrote {out_path} ({n} bytes)\n")
        return 0


if __name__ == "__main__":
    raise SystemExit(main())

