#!/usr/bin/env python3
"""Small reusable UART console helper for the ESP32-P4 PicoCalc 0102 firmware.

This intentionally uses only the Python standard library (termios/select/os),
because pyserial is not installed in the current development environment.

Examples:

  tools/picocalc_console.py status
  tools/picocalc_console.py "lcd init" "screen demo" status
  tools/picocalc_console.py "picojs load interactive" "picojs run 2 1000" "picojs key left" "picojs dump"
  tools/picocalc_console.py --expect "ESP_OK" --expect "0102 status" status
  tools/picocalc_console.py --commands commands.txt
"""

from __future__ import annotations

import argparse
import os
import select
import sys
import termios
import time
from dataclasses import dataclass
from pathlib import Path

DEFAULT_PORT = "/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00"
DEFAULT_PROMPT = "0102>"
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
class ReadResult:
    text: str
    saw_prompt: bool


class PicoCalcConsole:
    def __init__(self, port: str, baud: int, prompt: str, timeout: float):
        if baud not in BAUDS:
            raise ValueError(f"unsupported baud {baud}; choose one of {sorted(BAUDS)}")
        self.port = port
        self.baud = baud
        self.prompt = prompt
        self.timeout = timeout
        self.fd: int | None = None
        self.old_attrs = None

    def __enter__(self) -> "PicoCalcConsole":
        self.fd = os.open(self.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        self.old_attrs = termios.tcgetattr(self.fd)
        attrs = termios.tcgetattr(self.fd)
        attrs[0] = 0
        attrs[1] = 0
        attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
        attrs[3] = 0
        attrs[4] = BAUDS[self.baud]
        attrs[5] = BAUDS[self.baud]
        attrs[6][termios.VMIN] = 0
        attrs[6][termios.VTIME] = 1
        termios.tcsetattr(self.fd, termios.TCSANOW, attrs)
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        if self.fd is not None and self.old_attrs is not None:
            termios.tcsetattr(self.fd, termios.TCSANOW, self.old_attrs)
        if self.fd is not None:
            os.close(self.fd)
        self.fd = None
        self.old_attrs = None

    def write_line(self, line: str) -> None:
        if self.fd is None:
            raise RuntimeError("console is not open")
        os.write(self.fd, (line + "\n").encode("utf-8"))

    def read_until_prompt(self, timeout: float | None = None) -> ReadResult:
        if self.fd is None:
            raise RuntimeError("console is not open")
        timeout = self.timeout if timeout is None else timeout
        deadline = time.monotonic() + timeout
        buf = bytearray()
        prompt_bytes = self.prompt.encode("utf-8")
        saw_prompt = False
        while time.monotonic() < deadline:
            readable, _, _ = select.select([self.fd], [], [], 0.1)
            if not readable:
                continue
            chunk = os.read(self.fd, 4096)
            if not chunk:
                continue
            buf.extend(chunk)
            if prompt_bytes in buf:
                saw_prompt = True
                break
        return ReadResult(buf.decode("utf-8", errors="replace"), saw_prompt)

    def wake(self, timeout: float | None = None) -> ReadResult:
        self.write_line("")
        return self.read_until_prompt(timeout)

    def command(self, command: str, timeout: float | None = None) -> ReadResult:
        self.write_line(command)
        return self.read_until_prompt(timeout)


def load_command_file(path: Path) -> list[str]:
    commands: list[str] = []
    for raw in path.read_text().splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        commands.append(line)
    return commands


def parse_args(argv: list[str]) -> argparse.Namespace:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("commands", nargs="*", help="console commands to run; quote commands containing spaces")
    ap.add_argument("--commands", dest="command_file", type=Path, help="file containing one command per line; # comments and blanks ignored")
    ap.add_argument("--port", default=DEFAULT_PORT, help=f"serial by-id port (default: {DEFAULT_PORT})")
    ap.add_argument("--baud", type=int, default=115200, choices=sorted(BAUDS), help="UART baud rate")
    ap.add_argument("--prompt", default=DEFAULT_PROMPT, help=f"console prompt to wait for (default: {DEFAULT_PROMPT!r})")
    ap.add_argument("--timeout", type=float, default=10.0, help="seconds to wait for each prompt")
    ap.add_argument("--wake-timeout", type=float, default=12.0, help="seconds to wait for the initial prompt")
    ap.add_argument("--no-wake", action="store_true", help="do not send an initial blank line before commands")
    ap.add_argument("--expect", action="append", default=[], help="substring that must appear in the combined output; repeatable")
    ap.add_argument("--quiet-wake", action="store_true", help="do not print initial wake output")
    ap.add_argument("--allow-unstable-port", action="store_true", help="allow /dev/ttyACM* or /dev/ttyUSB* instead of requiring /dev/serial/by-id")
    return ap.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)

    if not args.allow_unstable_port and ("/dev/ttyACM" in args.port or "/dev/ttyUSB" in args.port):
        print("Refusing unstable tty name; use /dev/serial/by-id/... or pass --allow-unstable-port.", file=sys.stderr)
        return 2

    commands: list[str] = []
    if args.command_file:
        commands.extend(load_command_file(args.command_file))
    commands.extend(args.commands)
    if not commands and not args.expect:
        commands = ["status"]

    combined: list[str] = []
    try:
        with PicoCalcConsole(args.port, args.baud, args.prompt, args.timeout) as console:
            if not args.no_wake:
                wake = console.wake(args.wake_timeout)
                combined.append(wake.text)
                if not args.quiet_wake and wake.text:
                    print("--- wake")
                    print(wake.text.rstrip())
                if not wake.saw_prompt:
                    print("Prompt not seen; press reset or check the by-id port.", file=sys.stderr)
                    return 1

            for command in commands:
                result = console.command(command, args.timeout)
                combined.append(result.text)
                print(f"--- {command}")
                print(result.text.rstrip())
                if not result.saw_prompt:
                    print(f"Prompt not seen after command: {command}", file=sys.stderr)
                    return 1
    except OSError as e:
        print(f"serial error on {args.port}: {e}", file=sys.stderr)
        return 1

    all_output = "".join(combined)
    missing = [needle for needle in args.expect if needle not in all_output]
    if missing:
        for needle in missing:
            print(f"missing expected substring: {needle!r}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
