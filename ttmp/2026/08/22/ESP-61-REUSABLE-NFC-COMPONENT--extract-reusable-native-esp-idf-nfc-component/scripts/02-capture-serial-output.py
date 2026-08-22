#!/usr/bin/env python3
"""Capture USB Serial/JTAG output from a flashed ESP32-S3 for a few seconds."""

from __future__ import annotations

import argparse
import pathlib
import time

import serial


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--port", required=True)
    p.add_argument("--output", required=True, type=pathlib.Path)
    p.add_argument("--seconds", type=float, default=4.0)
    p.add_argument("--baud", default=115200, type=int)
    a = p.parse_args()

    buf = bytearray()
    with serial.Serial(a.port, a.baud, timeout=0.1, write_timeout=2) as port:
        port.dtr = False
        port.rts = False
        time.sleep(0.2)
        port.reset_input_buffer()
        deadline = time.monotonic() + a.seconds
        while time.monotonic() < deadline:
            chunk = port.read(port.in_waiting or 64)
            if chunk:
                buf.extend(chunk)
            else:
                time.sleep(0.05)

    normalized = bytes(buf).replace(b"\r\n", b"\n").replace(b"\r", b"\n")
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_bytes(normalized)
    print(f"wrote {a.output} ({len(normalized)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
