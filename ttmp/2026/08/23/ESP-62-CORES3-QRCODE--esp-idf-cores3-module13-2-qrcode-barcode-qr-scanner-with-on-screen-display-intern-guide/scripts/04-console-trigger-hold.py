#!/usr/bin/env python3
"""Hold Module13.2 hardware TRIG low through esp_console in one serial session."""

import argparse
import time

import serial


def drain(port: serial.Serial, output: bytearray, seconds: float) -> None:
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        chunk = port.read(port.in_waiting or 1)
        if chunk:
            output.extend(chunk)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True)
    parser.add_argument("--hold", type=float, default=3.0)
    parser.add_argument("--after", type=float, default=3.0)
    parser.add_argument("--out", required=True)
    args = parser.parse_args()

    captured = bytearray()
    with serial.Serial(args.port, 115200, timeout=0.05) as port:
        drain(port, captured, 0.5)
        port.write(b"qr trig low\r\n")
        port.flush()
        drain(port, captured, args.hold)
        port.write(b"qr trig high\r\n")
        port.flush()
        drain(port, captured, args.after)

    with open(args.out, "wb") as handle:
        handle.write(captured)
    print(captured.decode("utf-8", errors="replace"), end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
