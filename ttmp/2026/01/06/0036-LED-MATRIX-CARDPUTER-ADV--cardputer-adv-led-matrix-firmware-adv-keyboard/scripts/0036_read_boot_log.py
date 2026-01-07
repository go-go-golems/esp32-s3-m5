#!/usr/bin/env python3
import argparse
import sys
import time

import serial


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Read a few seconds of boot logs from a serial port (USB Serial/JTAG).")
    p.add_argument("--port", default="/dev/ttyACM0", help="Serial port (default: /dev/ttyACM0)")
    p.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    p.add_argument("--seconds", type=float, default=3.0, help="How long to read (default: 3.0)")
    p.add_argument("--reset", action="store_true", help="Toggle DTR/RTS to attempt a reset (best-effort)")
    return p.parse_args()


def main() -> int:
    args = parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=0.1)
    try:
        if args.reset:
            try:
                ser.dtr = False
                ser.rts = False
                time.sleep(0.1)
                ser.dtr = True
                ser.rts = True
                time.sleep(0.1)
            except Exception:
                pass

        deadline = time.time() + args.seconds
        while time.time() < deadline:
            b = ser.read(4096)
            if b:
                sys.stdout.buffer.write(b)
                sys.stdout.flush()
        return 0
    finally:
        ser.close()


if __name__ == "__main__":
    raise SystemExit(main())

