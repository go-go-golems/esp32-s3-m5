#!/usr/bin/env python3
import argparse
import sys
import time

import serial


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--port", default="/dev/serial/by-id/usb-1a86_USB_Single_Serial_575E072431-if00")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--seconds", type=float, default=20.0)
    p.add_argument("--outfile", default="")
    args = p.parse_args()

    out = open(args.outfile, "w", encoding="utf-8") if args.outfile else None
    deadline = time.time() + args.seconds

    try:
        with serial.Serial(args.port, args.baud, timeout=0.2) as ser:
            while time.time() < deadline:
                chunk = ser.read(4096)
                if not chunk:
                    continue
                text = chunk.decode("utf-8", errors="replace")
                sys.stdout.write(text)
                sys.stdout.flush()
                if out:
                    out.write(text)
                    out.flush()
    finally:
        if out:
            out.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
