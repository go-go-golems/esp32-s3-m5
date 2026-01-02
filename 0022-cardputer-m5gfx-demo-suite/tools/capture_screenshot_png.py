#!/usr/bin/env python3

import argparse
import re
import sys

import serial  # type: ignore


HEADER_RE = re.compile(rb"^PNG_BEGIN (\d+)\n$")


def read_exact(ser: serial.Serial, n: int) -> bytes:
    buf = bytearray()
    while len(buf) < n:
        chunk = ser.read(n - len(buf))
        if not chunk:
            continue
        buf.extend(chunk)
    return bytes(buf)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("port", help="Serial port (prefer /dev/serial/by-id/...)")
    ap.add_argument("out", help="Output PNG path")
    ap.add_argument("--baud", type=int, default=115200)
    args = ap.parse_args()

    with serial.Serial(args.port, args.baud, timeout=0.2) as ser:
        while True:
            line = ser.readline()
            if not line:
                continue
            m = HEADER_RE.match(line)
            if not m:
                continue

            length = int(m.group(1))
            data = read_exact(ser, length) if length > 0 else b""

            # Consume until footer line; tolerate any stray newlines.
            while True:
                l2 = ser.readline()
                if b"PNG_END" in l2:
                    break

            with open(args.out, "wb") as f:
                f.write(data)
            print(f"wrote {args.out} ({len(data)} bytes)")
            return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        raise SystemExit(130)
