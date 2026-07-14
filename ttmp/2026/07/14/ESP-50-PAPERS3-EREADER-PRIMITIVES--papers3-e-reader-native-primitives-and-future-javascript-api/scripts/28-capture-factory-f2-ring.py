#!/usr/bin/env python3
"""Capture the post-idle F2 JSONL ring without sending serial input."""
from __future__ import annotations

import argparse
import fcntl
import json
import os
import time
from datetime import datetime, timezone
from pathlib import Path

import serial

TICKET = Path(__file__).resolve().parents[1]
EXP = TICKET / "scripts/experiments/EXP-20260714-003-factory-v05-source-f2-trace"
PORT = Path("/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00")


def utcstamp() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--execute", action="store_true")
    args = ap.parse_args()
    if args.check == args.execute:
        ap.error("choose exactly one of --check or --execute")
    if not EXP.is_dir() or not PORT.exists():
        raise SystemExit("missing F2 preregistration or serial port")
    real = PORT.resolve()
    print(f"experiment={EXP.name}\nport={PORT}\nreal_port={real}")
    if args.check:
        print("mode=check\nhardware_modified=no")
        return

    stamp = utcstamp()
    transcript = EXP / f"serial-{stamp}.log"
    jsonl = EXP / f"ring-{stamp}.jsonl"
    deadline = time.monotonic() + 45
    ser = None
    while time.monotonic() < deadline and ser is None:
        try:
            candidate = serial.Serial(str(PORT), 115200, timeout=0.25, exclusive=True)
            fcntl.flock(candidate.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
            ser = candidate
        except (OSError, serial.SerialException, BlockingIOError):
            time.sleep(0.2)
    if ser is None:
        raise SystemExit("could not acquire serial port within capture deadline")

    raw = bytearray()
    marker_seen_at = None
    try:
        while time.monotonic() < deadline:
            chunk = ser.read(4096)
            if chunk:
                raw.extend(chunk)
                if b"FACTORY_TRACE_DUMP_END" in raw and marker_seen_at is None:
                    marker_seen_at = time.monotonic()
            if marker_seen_at is not None and time.monotonic() - marker_seen_at >= 2:
                break
    finally:
        ser.close()

    normalized = bytes(raw).replace(b"\r", b"").replace(b"\x00", b"")
    transcript.write_bytes(normalized)
    lines = normalized.decode("utf-8", "replace").splitlines()
    records = []
    for line in lines:
        if line.startswith("{"):
            try:
                item = json.loads(line)
            except json.JSONDecodeError:
                continue
            if item.get("schema") == 1:
                records.append(item)
    jsonl.write_text("".join(json.dumps(item, sort_keys=True) + "\n" for item in records))

    if not any("FACTORY_TRACE_DUMP_BEGIN" in line for line in lines):
        raise SystemExit(f"capture incomplete: begin marker absent; transcript={transcript}")
    if not any("FACTORY_TRACE_DUMP_END" in line for line in lines):
        raise SystemExit(f"capture incomplete: end marker absent; transcript={transcript}")
    if not records or any(item.get("valid") is False for item in records):
        raise SystemExit(f"capture invalid or empty; transcript={transcript}")
    seq = [int(item["sequence"]) for item in records]
    if seq != list(range(seq[0], seq[0] + len(seq))):
        raise SystemExit("ring sequence is not contiguous")
    names = [item.get("name") for item in records]
    for required in ("POWER_ON_BEGIN", "FRAME_BEGIN", "FRAME_END", "POWER_OFF_END", "DISPLAY_IDLE"):
        if required not in names:
            raise SystemExit(f"required event absent: {required}")

    print(f"mode=execute\ntranscript={transcript}\nring_jsonl={jsonl}\nrecords={len(records)}\nserial_input_sent=no\nhardware_modified=no")

if __name__ == "__main__":
    main()
