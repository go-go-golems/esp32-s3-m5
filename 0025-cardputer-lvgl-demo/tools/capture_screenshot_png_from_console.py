#!/usr/bin/env python3

import argparse
import re
import time

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
    ap.add_argument("--cmd", default="screenshot", help="Console command to send (default: screenshot)")
    ap.add_argument("--timeout-s", type=float, default=15.0)
    args = ap.parse_args()

    deadline = time.time() + args.timeout_s

    with serial.Serial(args.port, args.baud, timeout=0.2) as ser:
        # Best effort: clear any buffered output before we issue the command.
        ser.reset_input_buffer()
        ser.reset_output_buffer()

        ser.write((args.cmd + "\r\n").encode("utf-8"))
        ser.flush()

        while True:
            if time.time() > deadline:
                raise TimeoutError("timed out waiting for PNG_BEGIN")

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
                if time.time() > deadline:
                    raise TimeoutError("timed out waiting for PNG_END")
                l2 = ser.readline()
                if b"PNG_END" in l2:
                    break

            with open(args.out, "wb") as f:
                f.write(data)
            print(f"wrote {args.out} ({len(data)} bytes)")
            return 0


if __name__ == "__main__":
    raise SystemExit(main())

