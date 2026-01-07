#!/usr/bin/env python3
import argparse
import sys
import time

import serial


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Send a small command sequence to the 0036 matrix esp_console REPL.")
    p.add_argument("--port", default="/dev/ttyACM0", help="Serial port (default: /dev/ttyACM0)")
    p.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    p.add_argument("--reverse", action="store_true", help="Send `matrix reverse on` before patterns")
    p.add_argument("--read-seconds", type=float, default=2.5, help="How long to read back after sending (default: 2.5)")
    return p.parse_args()


def write_line(ser: serial.Serial, line: str) -> None:
    ser.write((line + "\n").encode("utf-8"))
    ser.flush()
    time.sleep(0.15)


def main() -> int:
    args = parse_args()
    ser = serial.Serial(args.port, args.baud, timeout=0.1)
    try:
        time.sleep(0.2)

        write_line(ser, "")
        write_line(ser, "matrix help")
        write_line(ser, "matrix init")
        if args.reverse:
            write_line(ser, "matrix reverse on")
        write_line(ser, "matrix pattern ids")
        write_line(ser, "matrix px 0 0 1")
        write_line(ser, "matrix px 31 7 1")

        deadline = time.time() + args.read_seconds
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

