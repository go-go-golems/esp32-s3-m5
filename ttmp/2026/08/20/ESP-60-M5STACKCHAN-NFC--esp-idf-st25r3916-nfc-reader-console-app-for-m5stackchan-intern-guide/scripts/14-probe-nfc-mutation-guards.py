#!/usr/bin/env python3
"""Verify 0117 mutation commands refuse execution without exact confirmation."""

from __future__ import annotations

import argparse
import datetime as dt
import pathlib
import time

import serial

PROMPT = b"nfc-explorer> "
COMMANDS = [
    "nfc-value-inspect",
    "nfc-write-test 4",
    "nfc-write-test 4 --confirm WRONG",
    "nfc-ndef-write-demo",
    "nfc-ndef-write-demo --confirm WRONG",
    "nfc-wallet-demo 62 rechargeable",
    "nfc-wallet-demo 62 rechargeable --confirm WRONG",
]


def read_prompt(port: serial.Serial, timeout: float = 20) -> bytes:
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
    raise TimeoutError(f"prompt not seen; received {len(data)} bytes")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True)
    parser.add_argument("--output", required=True, type=pathlib.Path)
    args = parser.parse_args()

    out = bytearray(
        (
            "# 0117 NFC mutation guard probe\n"
            f"# captured={dt.datetime.now(dt.timezone.utc).isoformat()}\n"
            "# no valid mutation confirmation token is sent\n"
        ).encode()
    )
    with serial.Serial(args.port, 115200, timeout=0.1, write_timeout=2) as port:
        port.dtr = False
        port.rts = False
        time.sleep(0.5)
        port.reset_input_buffer()
        port.write(b"\n")
        port.flush()
        out.extend(read_prompt(port))
        for text in COMMANDS:
            out.extend(f"\n=== CMD: {text} ===\n".encode())
            port.write(text.encode() + b"\n")
            port.flush()
            out.extend(read_prompt(port))

    normalized = bytes(out).replace(b"\r\n", b"\n").replace(b"\r", b"\n")
    normalized = b"\n".join(line.rstrip(b" \t") for line in normalized.split(b"\n"))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(normalized)
    print(f"wrote {args.output} ({len(normalized)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
