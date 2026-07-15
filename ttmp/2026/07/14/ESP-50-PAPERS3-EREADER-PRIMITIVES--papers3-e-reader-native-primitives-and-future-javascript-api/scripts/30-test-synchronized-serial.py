#!/usr/bin/env python3
"""Pseudo-terminal integration test for synchronized Printalyzer raw capture."""

from __future__ import annotations

import errno
import json
import os
import pty
import select
import subprocess
import tempfile
import threading
import time
import tty
from pathlib import Path

SCRIPT = Path(__file__).with_name("29-capture-synchronized-serial.py")


def main() -> None:
    master, slave = pty.openpty()
    tty.setraw(master)
    tty.setraw(slave)
    slave_path = os.ttyname(slave)
    os.close(slave)
    stop = threading.Event()
    commands: list[str] = []

    def fake_printalyzer() -> None:
        pending = bytearray()
        streaming = False
        last_reading = time.monotonic()
        while not stop.is_set():
            readable, _, _ = select.select([master], [], [], 0.01)
            if readable:
                try:
                    chunk = os.read(master, 4096)
                except OSError as exc:
                    # PTY masters return EIO while no slave endpoint is open.
                    if exc.errno == errno.EIO:
                        time.sleep(0.01)
                        continue
                    raise
                pending.extend(chunk)
                while b"\n" in pending:
                    raw, remainder = pending.split(b"\n", 1)
                    pending = bytearray(remainder)
                    command = raw.rstrip(b"\r").decode("ascii")
                    commands.append(command)
                    if command == "GC LIGHT":
                        response = "GC LIGHT,128,122"
                    elif command == "GC GAIN":
                        response = "GC GAIN,3F800000,3F800000,41C414E0,41CD2F71,43CC95C1,43CDF800,46141D28,46257619"
                    elif command == "GC SLOPE":
                        response = "GC SLOPE,3E1F077D,3F785719,BC24228A"
                    elif command == "GC REFL":
                        response = "GC REFL,3D75C28F,42D6DDBA,3FC00000,409478EF"
                    elif command == "IS REMOTE,1":
                        response = "IS REMOTE,1"
                    elif command == "IS REMOTE,0":
                        response = "IS REMOTE,0"
                    elif command.startswith("SD S,"):
                        response = "SD S,OK"
                    elif command.startswith("SD LR,"):
                        response = "SD LR,OK"
                    elif command == "ID S,START":
                        response = "ID S,OK"
                        streaming = True
                    elif command == "ID S,STOP":
                        response = "ID S,OK"
                        streaming = False
                    else:
                        response = f"{command},OK"
                    os.write(master, (response + "\r\n").encode("ascii"))
            if streaming and time.monotonic() - last_reading >= 0.03:
                os.write(master, b"GD S,1234,56,2,0\r\n")
                last_reading = time.monotonic()

    fake_thread = threading.Thread(target=fake_printalyzer, daemon=True)
    fake_thread.start()
    with tempfile.TemporaryDirectory(prefix="esp50-sync-test-") as temp_dir:
        output = Path(temp_dir) / "capture.jsonl"
        result = subprocess.run(
            [
                str(SCRIPT),
                "--execute",
                "--no-firmware",
                "--dens-port",
                slave_path,
                "--dens-raw-stream",
                "--confirm",
                "ENABLE-DENS-RAW-STREAM",
                "--duration",
                "0.2",
                "--output",
                str(output),
            ],
            check=False,
            text=True,
            capture_output=True,
        )
        stop.set()
        fake_thread.join(timeout=2)
        os.close(master)
        if result.returncode != 0:
            raise SystemExit(result.stdout + result.stderr)

        records = [json.loads(line) for line in output.read_text().splitlines()]
        raw_records = [
            record
            for record in records
            if record.get("parsed", {}).get("kind") == "raw_sensor"
        ]
        expected_commands = [
            "GC LIGHT",
            "GC GAIN",
            "GC SLOPE",
            "GC REFL",
            "IS REMOTE,1",
            "SD S,CFG,2,0",
            "SD LR,128",
            "ID S,START",
            "ID S,STOP",
            "SD LR,0",
            "IS REMOTE,0",
        ]
        assert commands == expected_commands, commands
        assert raw_records, "no parsed raw-sensor records"
        assert all(
            record["parsed"]["derived"]["density_estimate_valid"]
            for record in raw_records
        )
        assert records[-1]["event"] == "capture_end"
        assert records[-1]["result"] == "ok"
        assert [record["sequence"] for record in records] == list(range(len(records)))
        print(
            f"fake_raw_stream_test=PASS records={len(records)} raw={len(raw_records)}"
        )
        print("commands=" + repr(commands))


if __name__ == "__main__":
    main()
