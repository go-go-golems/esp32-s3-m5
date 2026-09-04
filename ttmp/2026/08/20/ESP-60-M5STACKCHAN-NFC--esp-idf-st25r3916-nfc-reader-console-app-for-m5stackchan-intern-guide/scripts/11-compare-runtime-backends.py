#!/usr/bin/env python3
"""Run idf-high and idf-defined ST25R3916 backends in one firmware boot.

For each backend: select backend (clears trace), apply the same NFC-A config via
that backend, verify registers, run one-tag reads, and export status/tail/first
error. One serial owner; waits for real esp_console prompts.
"""
from __future__ import annotations
import argparse
import serial
import time


def read_prompt(ser: serial.Serial, timeout_s: float) -> str:
    deadline = time.monotonic() + timeout_s
    data = bytearray()
    while time.monotonic() < deadline:
        if ser.in_waiting:
            data.extend(ser.read(ser.in_waiting))
            if b"nfc> " in data:
                break
        else:
            time.sleep(0.02)
    return data.decode("utf-8", "replace")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("port")
    ap.add_argument("output")
    ap.add_argument("--attempts", type=int, default=5)
    args = ap.parse_args()

    ser = serial.Serial(args.port, 115200, timeout=0.2)
    ser.dtr = False
    ser.rts = False
    time.sleep(0.2)
    ser.reset_input_buffer()

    with open(args.output, "w", encoding="utf-8") as log:
        def run(command: str, timeout_s: float = 8.0) -> str:
            ser.write((command + "\n").encode())
            ser.flush()
            out = read_prompt(ser, timeout_s)
            block = f"\n=== CMD: {command} ===\n{out}\n"
            print(block, end="")
            log.write(block)
            log.flush()
            return out

        log.write(f"=== BOOT ===\n{read_prompt(ser, 8.0)}\n")
        run("nfc-i2c-debug off", 3.0)
        for backend in ("idf-high", "idf-defined"):
            log.write(f"\n######## BACKEND {backend} ########\n")
            run(f"nfc-backend {backend}", 3.0)
            run("nfc-configure", 8.0)
            run("nfc-regs", 4.0)
            run(f"nfc-read --attempts {args.attempts}", max(20.0, args.attempts * 3.0))
            run("nfc-regs", 4.0)
            run("nfc-trace status", 3.0)
            run("nfc-trace dump --last 160", 12.0)
            run("nfc-trace first-error", 6.0)
    ser.close()


if __name__ == "__main__":
    main()
