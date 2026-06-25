#!/usr/bin/env python3
"""Probe the PicoJS layout/widgets slice over the ESP32-P4 UART."""

from __future__ import annotations

import argparse
import os
import select
import sys
import termios
import time

DEFAULT_PORT = "/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00"
PROMPT = "0102>"
BAUDS = {115200: termios.B115200, 460800: termios.B460800, 921600: termios.B921600}


def configure(fd: int, baud: int):
    old = termios.tcgetattr(fd)
    attrs = termios.tcgetattr(fd)
    attrs[0] = 0
    attrs[1] = 0
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    attrs[3] = 0
    attrs[4] = BAUDS[baud]
    attrs[5] = BAUDS[baud]
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 1
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    return old


def read_until(fd: int, timeout: float) -> str:
    deadline = time.monotonic() + timeout
    buf = bytearray()
    while time.monotonic() < deadline:
        r, _, _ = select.select([fd], [], [], 0.2)
        if r:
            buf.extend(os.read(fd, 4096))
            if PROMPT.encode() in buf:
                break
    return buf.decode("utf-8", errors="replace")


def send(fd: int, command: str, timeout: float) -> str:
    os.write(fd, (command + "\n").encode())
    out = read_until(fd, timeout)
    print(f"--- {command}")
    print(out.rstrip())
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--port", default=DEFAULT_PORT)
    ap.add_argument("--baud", type=int, default=115200, choices=sorted(BAUDS))
    ap.add_argument("--timeout", type=float, default=10.0)
    args = ap.parse_args()

    if "/dev/ttyACM" in args.port or "/dev/ttyUSB" in args.port:
        print("Refusing unstable tty name; use /dev/serial/by-id/...", file=sys.stderr)
        return 2

    fd = os.open(args.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    old = configure(fd, args.baud)
    try:
        os.write(fd, b"\n")
        boot = read_until(fd, max(args.timeout, 12.0))
        if PROMPT not in boot:
            print(boot, end="")
            print("Prompt not seen; press reset and retry if the board is in download mode.", file=sys.stderr)
            return 1
        print("--- prompt")
        print(boot.rstrip())

        load = send(fd, "picojs load dashboard", args.timeout)
        frame1 = send(fd, "picojs frame 1000", args.timeout)
        dump1 = send(fd, "picojs dump", args.timeout)
        frame2 = send(fd, "picojs frame 1000", args.timeout)
        dump2 = send(fd, "picojs dump", args.timeout)

        checks = [
            "picojs load dashboard: ESP_OK ok=1" in load,
            "picojs frame: ESP_OK dt_ms=1000" in frame1,
            "PicoJS Dashboard" in dump1,
            "ESP32-P4 native DSL" in dump1,
            "batt [" in dump1 and "heap [" in dump1,
            "dashboard native picojs" in dump1,
            "frame: ESP_OK" in frame2,
            dump1 != dump2 and "batt [" in dump2,
        ]
        ok = all(checks)
        print("LAYOUT_WIDGETS_PROBE", "PASS" if ok else "FAIL", checks)
        return 0 if ok else 1
    finally:
        termios.tcsetattr(fd, termios.TCSANOW, old)
        os.close(fd)


if __name__ == "__main__":
    raise SystemExit(main())
