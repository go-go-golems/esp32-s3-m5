#!/usr/bin/env python3
"""Probe the 0102 ESP32-P4 UART console without pyserial.

Use a stable /dev/serial/by-id path. Do not pass /dev/ttyACM0 when multiple
boards are attached.
"""

from __future__ import annotations

import argparse
import os
import select
import sys
import termios
import time
from dataclasses import dataclass

DEFAULT_PORT = "/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00"
PROMPT = "0102>"

BAUDS = {
    9600: termios.B9600,
    19200: termios.B19200,
    38400: termios.B38400,
    57600: termios.B57600,
    115200: termios.B115200,
    230400: termios.B230400,
    460800: termios.B460800,
    921600: termios.B921600,
}


@dataclass
class ProbeCase:
    command: str
    required: tuple[str, ...]
    allow_fail_text: bool = False


def configure(fd: int, baud: int) -> list:
    old = termios.tcgetattr(fd)
    attrs = termios.tcgetattr(fd)
    speed = BAUDS[baud]
    attrs[0] = 0  # iflag
    attrs[1] = 0  # oflag
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    attrs[3] = 0  # lflag
    attrs[4] = speed
    attrs[5] = speed
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 1
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    return old


def read_until(fd: int, needle: str, timeout: float) -> str:
    deadline = time.monotonic() + timeout
    buf = bytearray()
    needle_b = needle.encode()
    while time.monotonic() < deadline:
        remaining = max(0.0, deadline - time.monotonic())
        r, _, _ = select.select([fd], [], [], min(0.2, remaining))
        if not r:
            continue
        chunk = os.read(fd, 4096)
        if not chunk:
            continue
        buf.extend(chunk)
        if needle_b in buf:
            break
    return buf.decode("utf-8", errors="replace")


def send_line(fd: int, line: str) -> None:
    os.write(fd, (line + "\n").encode())


def run_case(fd: int, case: ProbeCase, timeout: float) -> bool:
    send_line(fd, case.command)
    out = read_until(fd, PROMPT, timeout)
    ok = all(s in out for s in case.required)
    status = "PASS" if ok else "FAIL"
    print(f"[{status}] {case.command}")
    print(out.rstrip())
    print("---")
    return ok


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default=DEFAULT_PORT, help="stable /dev/serial/by-id path for the ESP32-P4")
    parser.add_argument("--baud", type=int, default=115200, choices=sorted(BAUDS))
    parser.add_argument("--timeout", type=float, default=8.0)
    parser.add_argument("--prompt-timeout", type=float, default=12.0)
    args = parser.parse_args()

    if "/dev/ttyACM" in args.port or "/dev/ttyUSB" in args.port:
        print("Refusing unstable tty name; use /dev/serial/by-id/...", file=sys.stderr)
        return 2

    fd = os.open(args.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    old_attrs = configure(fd, args.baud)
    try:
        # Wake the REPL and wait for a prompt. The first newline is harmless if
        # the firmware is already sitting at the prompt.
        send_line(fd, "")
        boot = read_until(fd, PROMPT, args.prompt_timeout)
        if PROMPT not in boot:
            print(boot, end="")
            print(f"Prompt {PROMPT!r} not seen on {args.port}. If the board is in download mode, press reset and rerun.", file=sys.stderr)
            return 1
        print("[PASS] prompt")
        print(boot.rstrip())
        print("---")

        cases = [
            ProbeCase("status", ("quickjs:", "visual:", "heap:")),
            ProbeCase("js smoke", ("js smoke", "PASS")),
            ProbeCase("js eval print('hello-device')", ("hello-device", "[eval] ok=1")),
            ProbeCase("js eval throw new Error('boom')", ("error:", "boom")),
            ProbeCase("screen demo", ("screen demo: ESP_OK",)),
            ProbeCase("screen dump", ("screen dump: ESP_OK", "[00]", "[19]")),
        ]
        ok = True
        for case in cases:
            ok = run_case(fd, case, args.timeout) and ok
        return 0 if ok else 1
    finally:
        termios.tcsetattr(fd, termios.TCSANOW, old_attrs)
        os.close(fd)


if __name__ == "__main__":
    raise SystemExit(main())
