#!/usr/bin/env python3
"""Capture PaperS3 USB Serial/JTAG across a physical reset without modem control.

This utility deliberately avoids pyserial, termios setup, all writes, and every
DTR/RTS ioctl.  It reopens only an O_RDONLY/O_NOCTTY/O_NONBLOCK descriptor if
the USB CDC device disconnects during a physical reset.
"""

from __future__ import annotations

import argparse
import fcntl
import json
import os
import select
import time
from datetime import datetime, timezone
from pathlib import Path

DEFAULT_PORT = Path(
    "/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00"
)


def stamp() -> dict[str, int | str]:
    now = time.time_ns()
    return {
        "host_monotonic_ns": time.monotonic_ns(),
        "host_utc_ns": now,
        "host_utc": datetime.fromtimestamp(now / 1_000_000_000, timezone.utc)
        .isoformat()
        .replace("+00:00", "Z"),
    }


def emit(stream, sequence: int, event: str, **fields) -> int:
    stream.write(
        json.dumps(
            {
                "schema": "esp50.papers3-readonly-reconnect.v1",
                "sequence": sequence,
                "source": "firmware",
                "event": event,
                **stamp(),
                **fields,
            },
            sort_keys=True,
            separators=(",", ":"),
        )
        + "\n"
    )
    stream.flush()
    return sequence + 1


def open_read_only(port: Path) -> int:
    fd = os.open(port, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
    try:
        fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except Exception:
        os.close(fd)
        raise
    return fd


def close(fd: int) -> None:
    try:
        fcntl.flock(fd, fcntl.LOCK_UN)
    finally:
        os.close(fd)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--check", action="store_true")
    mode.add_argument("--execute", action="store_true")
    parser.add_argument("--confirm")
    parser.add_argument("--port", type=Path, default=DEFAULT_PORT)
    parser.add_argument("--duration", type=float, default=75.0)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.duration <= 0:
        parser.error("--duration must be positive")
    if args.output.exists():
        parser.error(f"refusing to replace evidence: {args.output}")
    if args.check:
        print(
            f"mode=check\nport={args.port}\noutput={args.output}\nhardware_modified=no"
        )
        return
    if args.confirm != "CAPTURE-F2-READONLY-RESET":
        parser.error("--execute requires --confirm CAPTURE-F2-READONLY-RESET")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    sequence = 0
    fd: int | None = None
    pending = bytearray()
    deadline = time.monotonic() + args.duration
    with args.output.open("x", encoding="utf-8") as stream:
        sequence = emit(
            stream,
            sequence,
            "capture_begin",
            port=str(args.port),
            modem_control_issued=False,
            writes_issued=False,
        )
        while time.monotonic() < deadline:
            if fd is None:
                try:
                    fd = open_read_only(args.port)
                    sequence = emit(
                        stream,
                        sequence,
                        "source_open",
                        port=str(args.port),
                        real_port=str(args.port.resolve()),
                        open_mode="read-only-os-reconnect",
                        modem_control_issued=False,
                    )
                except (FileNotFoundError, BlockingIOError, OSError):
                    time.sleep(0.05)
                    continue
            try:
                readable, _, _ = select.select([fd], [], [], 0.05)
                if not readable:
                    continue
                chunk = os.read(fd, 4096)
                if not chunk:
                    raise OSError("EOF/hangup")
                pending.extend(chunk)
                while b"\n" in pending:
                    raw, rest = pending.split(b"\n", 1)
                    pending = bytearray(rest)
                    sequence = emit(
                        stream,
                        sequence,
                        "rx_line",
                        line=raw.rstrip(b"\r\x00").decode("utf-8", "replace"),
                        raw_hex=bytes(raw).hex(),
                    )
            except (OSError, ValueError) as exc:
                sequence = emit(stream, sequence, "source_disconnect", error=repr(exc))
                close(fd)
                fd = None
                time.sleep(0.05)
        if pending:
            sequence = emit(
                stream,
                sequence,
                "rx_partial",
                raw_hex=bytes(pending).hex(),
                text=bytes(pending).decode("utf-8", "replace"),
            )
        if fd is not None:
            close(fd)
            sequence = emit(stream, sequence, "source_close", port=str(args.port))
        emit(
            stream,
            sequence,
            "capture_end",
            result="ok",
            modem_control_issued=False,
            writes_issued=False,
        )
    print(f"result=ok\noutput={args.output}\nhardware_modified=no")


if __name__ == "__main__":
    main()
