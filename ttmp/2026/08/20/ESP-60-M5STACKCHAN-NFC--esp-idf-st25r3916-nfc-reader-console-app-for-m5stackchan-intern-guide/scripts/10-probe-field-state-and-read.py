#!/usr/bin/env python3
"""Probe ST25R3916 field state before and after reads.

Reusable single-owner serial workflow. It explicitly calls nfc-field on, verifies
registers, runs one and then several reads, and verifies registers again. Waits
for the actual esp_console prompt between commands.
"""
from __future__ import annotations
import argparse
import serial
import time


def read_prompt(ser: serial.Serial, timeout_s: float) -> str:
    data = bytearray()
    deadline = time.monotonic() + timeout_s
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

        boot = read_prompt(ser, 8.0)
        log.write(f"=== BOOT ===\n{boot}\n")
        run("nfc-i2c-debug off", 3.0)
        run("nfc-trace clear", 3.0)
        run("nfc-regs", 4.0)
        run("nfc-field on", 4.0)
        run("nfc-regs", 4.0)
        run("nfc-read --attempts 1", 10.0)
        run("nfc-regs", 4.0)
        run(f"nfc-read --attempts {args.attempts}", max(20.0, args.attempts * 3.0))
        run("nfc-regs", 4.0)
        run("nfc-trace status", 3.0)
        run("nfc-trace first-error", 5.0)
    ser.close()


if __name__ == "__main__":
    main()
