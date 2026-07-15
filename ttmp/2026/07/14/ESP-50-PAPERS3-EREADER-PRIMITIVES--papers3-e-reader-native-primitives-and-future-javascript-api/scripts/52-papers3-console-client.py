#!/usr/bin/env python3
"""Interactive PaperS3 USB Serial/JTAG console client without modem control.

Sends scripted console commands to the reader-primitives firmware and records
the transcript. Unlike pyserial, this opens the tty with plain os.open and
only uses tcsetattr (raw mode, no echo); it never issues DTR/RTS ioctls, so it
cannot reset the PaperS3 into ROM download mode.

Usage:
  52-papers3-console-client.py --output transcript.log \
      --cmd status --cmd "stress 500" --cmd events
"""

from __future__ import annotations

import argparse
import fcntl
import os
import select
import sys
import termios
import time
from pathlib import Path

DEFAULT_PORT = Path(
    "/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00"
)


def open_rw_raw(port: Path) -> int:
    fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    try:
        fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        # Raw mode: no echo, no canonical processing, no CR/NL translation.
        # tcsetattr does not touch modem-control lines (DTR/RTS).
        attrs = termios.tcgetattr(fd)
        attrs[0] = 0  # iflag
        attrs[1] = 0  # oflag
        attrs[3] = 0  # lflag
        attrs[6][termios.VMIN] = 0
        attrs[6][termios.VTIME] = 0
        termios.tcsetattr(fd, termios.TCSANOW, attrs)
        return fd
    except Exception:
        os.close(fd)
        raise


def drain(fd: int, duration: float, sink) -> None:
    deadline = time.monotonic() + duration
    while time.monotonic() < deadline:
        readable, _, _ = select.select([fd], [], [], 0.05)
        if not readable:
            continue
        try:
            chunk = os.read(fd, 4096)
        except OSError:
            break
        if chunk:
            sink.write(chunk.decode("utf-8", "replace"))
            sink.flush()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=Path, default=DEFAULT_PORT)
    parser.add_argument("--cmd", action="append", default=[],
                        help="console command to send (repeatable, in order)")
    parser.add_argument("--settle", type=float, default=1.5,
                        help="seconds to capture output after each command")
    parser.add_argument("--output", type=Path,
                        help="optional transcript file (also echoed to stdout)")
    args = parser.parse_args()
    if not args.cmd:
        parser.error("at least one --cmd is required")

    class Tee:
        def __init__(self, *streams):
            self.streams = streams

        def write(self, text):
            for s in self.streams:
                s.write(text)

        def flush(self):
            for s in self.streams:
                s.flush()

    out_file = args.output.open("w", encoding="utf-8") if args.output else None
    sink = Tee(sys.stdout, out_file) if out_file else sys.stdout

    fd = open_rw_raw(args.port)
    try:
        sink.write(f"# port={args.port} modem_control=none\n")
        # Flush any pending boot output before the first command.
        drain(fd, 0.5, sink)
        for cmd in args.cmd:
            sink.write(f"\n# >>> {cmd}\n")
            os.write(fd, cmd.encode() + b"\n")
            # stress runs can take a while; give long-running commands time.
            settle = args.settle
            if cmd.split()[0] in ("stress",):
                settle = max(settle, 30.0)
            drain(fd, settle, sink)
        sink.write("\n# done\n")
    finally:
        fcntl.flock(fd, fcntl.LOCK_UN)
        os.close(fd)
        if out_file:
            out_file.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
