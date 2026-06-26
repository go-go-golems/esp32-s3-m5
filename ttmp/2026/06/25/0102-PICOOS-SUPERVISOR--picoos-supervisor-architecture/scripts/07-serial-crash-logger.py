#!/usr/bin/env python3
"""Continuous serial crash logger for the ESP32-P4 PicoCalc console.

Run directly when you want to capture logs around physical key presses or typed
LCD-REPL actions. Prefer launching it through `08-start-serial-crash-logger-tmux.sh`
so it survives while another terminal is used for notes.
"""

from __future__ import annotations

import argparse
import os
import select
import sys
import termios
import time

DEFAULT_PORT = "/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00"
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


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--port", default=DEFAULT_PORT)
    ap.add_argument("--baud", type=int, default=115200, choices=sorted(BAUDS))
    ap.add_argument("--wake", action="store_true", help="Send an initial newline to wake the console prompt")
    args = ap.parse_args()

    if "/dev/ttyACM" in args.port or "/dev/ttyUSB" in args.port:
        print("Refusing unstable tty name; use /dev/serial/by-id/...", file=sys.stderr)
        return 2

    fd = os.open(args.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    old = configure(fd, args.baud)
    print("--- serial crash logger started", time.strftime("%F %T"), "port=", args.port, "---", flush=True)
    try:
        if args.wake:
            os.write(fd, b"\n")
        while True:
            r, _, _ = select.select([fd], [], [], 0.2)
            if not r:
                continue
            data = os.read(fd, 4096)
            if not data:
                continue
            sys.stdout.write(data.decode("utf-8", errors="replace"))
            sys.stdout.flush()
    finally:
        termios.tcsetattr(fd, termios.TCSANOW, old)
        os.close(fd)


if __name__ == "__main__":
    raise SystemExit(main())
