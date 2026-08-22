#!/usr/bin/env python3
"""Prompt-aware, single-owner probe for 0117 native NFC feature explorer.

Runs read-only commands only. It intentionally excludes every command requiring a
confirmation token (raw write test, NDEF replacement, wallet mutation, mode switch).
"""

from __future__ import annotations

import argparse
import datetime as dt
import pathlib
import sys
import time

import serial

PROMPT = b"nfc-explorer> "
COMMANDS = [
    ("nfc-mode", 10.0),
    ("nfc-capabilities", 10.0),
    ("nfc-scan 1000", 20.0),
    ("nfc-info", 20.0),
    ("nfc-raw-read 0", 20.0),
    ("nfc-ndef-read", 30.0),
    ("nfc-dump", 45.0),
]


def read_to_prompt(port: serial.Serial, timeout: float) -> bytes:
    deadline = time.monotonic() + timeout
    data = bytearray()
    while time.monotonic() < deadline:
        chunk = port.read(port.in_waiting or 1)
        if chunk:
            data.extend(chunk)
            if PROMPT in data:
                return bytes(data)
        else:
            time.sleep(0.01)
    raise TimeoutError(f"prompt not seen after {timeout:.1f}s; received {len(data)} bytes")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True)
    parser.add_argument("--output", required=True, type=pathlib.Path)
    parser.add_argument("--baud", type=int, default=115200)
    args = parser.parse_args()

    transcript = bytearray()
    header = (
        f"# 0117 read-only NFC feature explorer probe\n"
        f"# captured={dt.datetime.now(dt.timezone.utc).isoformat()}\n"
        f"# port={args.port}\n"
        f"# baud={args.baud}\n"
        f"# commands={','.join(command for command, _ in COMMANDS)}\n\n"
    ).encode()
    transcript.extend(header)

    try:
        with serial.Serial(args.port, args.baud, timeout=0.1, write_timeout=2) as port:
            port.dtr = False
            port.rts = False
            time.sleep(0.5)
            port.reset_input_buffer()
            port.write(b"\n")
            port.flush()
            boot = read_to_prompt(port, 20.0)
            transcript.extend(b"=== BOOT/PROMPT ===\n")
            transcript.extend(boot)

            for command, timeout in COMMANDS:
                transcript.extend(f"\n=== CMD: {command} ===\n".encode())
                port.write(command.encode() + b"\n")
                port.flush()
                transcript.extend(read_to_prompt(port, timeout))
    except Exception as exc:
        transcript.extend(f"\n=== PROBE ERROR ===\n{type(exc).__name__}: {exc}\n".encode())
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_bytes(bytes(transcript).replace(b"\r\n", b"\n").replace(b"\r", b"\n"))
        print(f"probe failed: {exc}", file=sys.stderr)
        return 1

    normalized = bytes(transcript).replace(b"\r\n", b"\n").replace(b"\r", b"\n")
    normalized = b"\n".join(line.rstrip(b" \t") for line in normalized.split(b"\n"))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(normalized)
    print(f"wrote {args.output} ({len(normalized)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
