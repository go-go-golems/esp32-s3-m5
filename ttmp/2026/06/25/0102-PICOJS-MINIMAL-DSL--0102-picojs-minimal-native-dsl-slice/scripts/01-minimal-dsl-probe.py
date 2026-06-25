#!/usr/bin/env python3
"""Probe the minimal PicoJS native DSL slice over the ESP32-P4 UART."""

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
        if not r:
            continue
        chunk = os.read(fd, 4096)
        if not chunk:
            continue
        buf.extend(chunk)
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
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default=DEFAULT_PORT)
    parser.add_argument("--baud", type=int, default=115200, choices=sorted(BAUDS))
    parser.add_argument("--timeout", type=float, default=10.0)
    args = parser.parse_args()

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
            print(f"Prompt {PROMPT!r} not seen; press reset and retry if the board is in download mode.", file=sys.stderr)
            return 1
        print("--- prompt")
        print(boot.rstrip())

        status1 = send(fd, "picojs status", args.timeout)
        load1 = send(fd, "picojs load hello", args.timeout)
        frame1 = send(fd, "picojs frame 0", args.timeout)
        dump1 = send(fd, "picojs dump", args.timeout)
        reset = send(fd, "js reset", args.timeout)
        status2 = send(fd, "picojs status", args.timeout)
        load2 = send(fd, "picojs load hello", args.timeout)
        dump2 = send(fd, "picojs dump", args.timeout)

        checks = [
            "js_installed=1" in status1,
            "picojs load hello: ESP_OK ok=1" in load1,
            "picojs frame: ESP_OK" in frame1,
            "HELLO DEVICE" in dump1 and "native picojs minimal" in dump1 and "hello" in dump1,
            "picojs_reinstall=ESP_OK" in reset,
            "js_installed=1" in status2,
            "picojs load hello: ESP_OK ok=1" in load2,
            "HELLO DEVICE" in dump2,
        ]
        ok = all(checks)
        print("MINIMAL_DSL_PROBE", "PASS" if ok else "FAIL", checks)
        return 0 if ok else 1
    finally:
        termios.tcsetattr(fd, termios.TCSANOW, old)
        os.close(fd)


if __name__ == "__main__":
    raise SystemExit(main())
