#!/usr/bin/env python3
"""Switch through 0117 emulation profiles and restore reader mode.

This validates NVS-selected initialization and local state reporting. It does not
claim RF interoperability; that requires a phone or second NFC reader.
"""

from __future__ import annotations

import argparse
import datetime as dt
import pathlib
import sys
import time

import serial

PROMPT = b"nfc-explorer> "


def open_prompt(port_name: str, baud: int, transcript: bytearray, label: str) -> serial.Serial:
    deadline = time.monotonic() + 20
    last_error: Exception | None = None
    while time.monotonic() < deadline:
        try:
            port = serial.Serial(port_name, baud, timeout=0.1, write_timeout=2)
            port.dtr = False
            port.rts = False
            time.sleep(0.3)
            port.write(b"\n")
            port.flush()
            data = read_until(port, PROMPT, 15)
            transcript.extend(f"\n=== BOOT: {label} ===\n".encode())
            transcript.extend(data)
            return port
        except Exception as exc:
            last_error = exc
            try:
                port.close()
            except Exception:
                pass
            time.sleep(0.4)
    raise RuntimeError(f"could not open prompt for {label}: {last_error}")


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


def command(port: serial.Serial, text: str, transcript: bytearray, expect_prompt: bool = True) -> None:
    transcript.extend(f"\n=== CMD: {text} ===\n".encode())
    port.write(text.encode() + b"\n")
    port.flush()
    if expect_prompt:
        transcript.extend(read_until(port, PROMPT, 10))
        return

    deadline = time.monotonic() + 3
    while time.monotonic() < deadline:
        try:
            chunk = port.read(port.in_waiting or 1)
            if chunk:
                transcript.extend(chunk)
            else:
                time.sleep(0.02)
        except serial.SerialException:
            break


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True)
    parser.add_argument("--output", required=True, type=pathlib.Path)
    parser.add_argument("--baud", default=115200, type=int)
    args = parser.parse_args()

    transcript = bytearray(
        (
            "# 0117 NFC emulation boot-mode probe\n"
            f"# captured={dt.datetime.now(dt.timezone.utc).isoformat()}\n"
            f"# port={args.port}\n"
            "# scope=local initialization and state only; no external RF reader\n"
        ).encode()
    )

    try:
        port = open_prompt(args.port, args.baud, transcript, "initial")
        command(port, "nfc-mode emulation-ultralight --confirm REBOOT", transcript, False)
        port.close()

        port = open_prompt(args.port, args.baud, transcript, "emulation-ultralight")
        command(port, "nfc-emulation-status", transcript)
        command(port, "nfc-mode emulation-ntag213 --confirm REBOOT", transcript, False)
        port.close()

        port = open_prompt(args.port, args.baud, transcript, "emulation-ntag213")
        command(port, "nfc-emulation-status", transcript)
        command(port, "nfc-mode reader --confirm REBOOT", transcript, False)
        port.close()

        port = open_prompt(args.port, args.baud, transcript, "reader-restored")
        command(port, "nfc-mode", transcript)
        command(port, "nfc-info", transcript)
        port.close()
    except Exception as exc:
        transcript.extend(f"\n=== PROBE ERROR ===\n{type(exc).__name__}: {exc}\n".encode())
        result = 1
    else:
        result = 0

    normalized = bytes(transcript).replace(b"\r\n", b"\n").replace(b"\r", b"\n")
    normalized = b"\n".join(line.rstrip(b" \t") for line in normalized.split(b"\n"))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(normalized)
    print(f"wrote {args.output} ({len(normalized)} bytes)")
    if result:
        print("probe failed; reader restoration may require rerunning nfc-mode reader", file=sys.stderr)
    return result


if __name__ == "__main__":
    raise SystemExit(main())
