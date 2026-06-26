#!/usr/bin/env python3
"""Reproduce visual-REPL /launch flows while preserving crash/panic logs.

This was originally written as a one-off /tmp script after a user reported that
/launch snake/hello/home crashed when typed on the physical device. It is kept in
the ticket scripts directory so future sessions can rerun the same reproduction
sequence and compare logs.
"""

from __future__ import annotations

import argparse
import os
import select
import sys
import termios
import time

DEFAULT_PORT = "/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00"
PROMPT = b"0102>"
BAUDS = {115200: termios.B115200, 460800: termios.B460800, 921600: termios.B921600}
DEFAULT_COMMANDS = [
    "screen eval /launch home",
    "screen eval /launch hello",
    "screen eval /launch snake",
]


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


def read_for(fd: int, seconds: float, *, stop_on_prompt: bool = False) -> bytes:
    end = time.monotonic() + seconds
    buf = bytearray()
    while time.monotonic() < end:
        r, _, _ = select.select([fd], [], [], 0.1)
        if not r:
            continue
        data = os.read(fd, 4096)
        if not data:
            continue
        buf.extend(data)
        sys.stdout.write(data.decode("utf-8", errors="replace"))
        sys.stdout.flush()
        if stop_on_prompt and PROMPT in buf:
            break
    return bytes(buf)


def panic_seen(blob: bytes) -> bool:
    lower = blob.lower()
    return b"guru meditation" in lower or b"panic" in lower or b"abort" in lower or b"stack canary" in lower


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--port", default=DEFAULT_PORT)
    ap.add_argument("--baud", type=int, default=115200, choices=sorted(BAUDS))
    ap.add_argument("--timeout", type=float, default=8.0)
    ap.add_argument("--tail-seconds", type=float, default=8.0)
    ap.add_argument("commands", nargs="*", help="Commands to send; defaults to REPL /launch home/hello/snake repro")
    args = ap.parse_args()

    if "/dev/ttyACM" in args.port or "/dev/ttyUSB" in args.port:
        print("Refusing unstable tty name; use /dev/serial/by-id/...", file=sys.stderr)
        return 2

    commands = args.commands or DEFAULT_COMMANDS
    fd = os.open(args.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    old = configure(fd, args.baud)
    try:
        print("--- repro start", time.strftime("%F %T"), "---", flush=True)
        os.write(fd, b"\n")
        read_for(fd, 5, stop_on_prompt=True)
        for command in commands:
            print("\n--- SEND", command, "---", flush=True)
            os.write(fd, (command + "\n").encode())
            out = read_for(fd, args.timeout, stop_on_prompt=True)
            tail = read_for(fd, 2, stop_on_prompt=False)
            if panic_seen(out + tail):
                print("--- panic marker seen; stopping command sequence ---", flush=True)
                break
        print("\n--- final tail ---", flush=True)
        read_for(fd, args.tail_seconds, stop_on_prompt=False)
        return 0
    finally:
        termios.tcsetattr(fd, termios.TCSANOW, old)
        os.close(fd)


if __name__ == "__main__":
    raise SystemExit(main())
