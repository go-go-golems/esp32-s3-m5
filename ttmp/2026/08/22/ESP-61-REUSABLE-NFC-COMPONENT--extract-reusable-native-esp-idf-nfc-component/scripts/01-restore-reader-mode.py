#!/usr/bin/env python3
"""Restore 0117 reader mode with one serial owner and preserve a transcript."""

from __future__ import annotations

import argparse
import datetime as dt
import pathlib
import time

import serial

PROMPT = b"nfc-explorer> "


def read_until(port: serial.Serial, marker: bytes, timeout: float) -> bytes:
    deadline = time.monotonic() + timeout
    data = bytearray()
    while time.monotonic() < deadline:
        chunk = port.read(port.in_waiting or 1)
        if chunk:
            data.extend(chunk)
            if marker in data:
                return bytes(data)
        else:
            time.sleep(0.01)
    raise TimeoutError(f"marker {marker!r} not seen; received {len(data)} bytes")


def open_prompt(port_name: str, baud: int, timeout: float = 25) -> tuple[serial.Serial, bytes]:
    deadline = time.monotonic() + timeout
    last_error: Exception | None = None
    while time.monotonic() < deadline:
        port: serial.Serial | None = None
        try:
            port = serial.Serial(port_name, baud, timeout=0.1, write_timeout=2)
            port.dtr = False
            port.rts = False
            time.sleep(0.3)
            port.write(b"\n")
            port.flush()
            return port, read_until(port, PROMPT, 15)
        except Exception as exc:
            last_error = exc
            if port is not None:
                port.close()
            time.sleep(0.4)
    raise RuntimeError(f"could not open console: {last_error}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True)
    parser.add_argument("--output", required=True, type=pathlib.Path)
    parser.add_argument("--baud", default=115200, type=int)
    args = parser.parse_args()

    transcript = bytearray(
        (
            "# ESP-61 Phase 0 restore reader mode\n"
            f"# captured={dt.datetime.now(dt.timezone.utc).isoformat()}\n"
            f"# port={args.port}\n"
        ).encode()
    )

    port, boot = open_prompt(args.port, args.baud)
    transcript.extend("\n=== INITIAL BOOT ===\n".encode())
    transcript.extend(boot)
    transcript.extend("\n=== CMD: nfc-mode reader --confirm REBOOT ===\n".encode())
    port.write(b"nfc-mode reader --confirm REBOOT\n")
    port.flush()
    time.sleep(1)
    while port.in_waiting:
        transcript.extend(port.read(port.in_waiting))
    port.close()

    port, boot = open_prompt(args.port, args.baud)
    transcript.extend("\n=== READER BOOT ===\n".encode())
    transcript.extend(boot)
    port.write(b"nfc-mode\n")
    port.flush()
    transcript.extend("\n=== CMD: nfc-mode ===\n".encode())
    mode_output = read_until(port, PROMPT, 10)
    transcript.extend(mode_output)
    port.close()

    normalized = bytes(transcript).replace(b"\r\n", b"\n").replace(b"\r", b"\n")
    normalized = b"\n".join(line.rstrip(b" \t") for line in normalized.split(b"\n"))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(normalized)
    print(f"wrote {args.output} ({len(normalized)} bytes)")
    if b"current=reader" not in normalized:
        raise RuntimeError("reader mode was not confirmed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
