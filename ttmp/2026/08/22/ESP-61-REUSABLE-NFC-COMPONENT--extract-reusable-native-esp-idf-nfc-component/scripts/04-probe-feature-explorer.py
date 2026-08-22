#!/usr/bin/env python3
"""Validate all nfc_feature_explorer reader commands on hardware."""
from __future__ import annotations
import argparse, pathlib, time, serial

PROMPT = b"nfc-explorer> "
CMDS = [
    "nfc-capabilities",
    "nfc-scan 1000",
    "nfc-info",
    "nfc-raw-read 0",
    "nfc-ndef-read",
    "nfc-dump",
    "nfc-ndef-write-demo --confirm REPLACE-NDEF",
    "nfc-ndef-read",
]

def read_prompt(port, timeout=30):
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
    raise TimeoutError(f"prompt not seen; {len(data)} bytes")

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--port", required=True)
    p.add_argument("--output", required=True, type=pathlib.Path)
    a = p.parse_args()
    out = bytearray()
    with serial.Serial(a.port, 115200, timeout=0.1, write_timeout=2) as port:
        port.dtr = False; port.rts = False; time.sleep(0.5)
        port.reset_input_buffer()
        port.write(b"\n"); port.flush()
        out.extend(read_prompt(port))
        for cmd in CMDS:
            out.extend(f"\n=== CMD: {cmd} ===\n".encode())
            port.write(cmd.encode() + b"\n"); port.flush()
            out.extend(read_prompt(port, 60))
    normalized = bytes(out).replace(b"\r\n", b"\n").replace(b"\r", b"\n")
    normalized = b"\n".join(line.rstrip(b" \t") for line in normalized.split(b"\n"))
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_bytes(normalized)
    print(f"wrote {a.output} ({len(normalized)} bytes)")

if __name__ == "__main__":
    raise SystemExit(main())
