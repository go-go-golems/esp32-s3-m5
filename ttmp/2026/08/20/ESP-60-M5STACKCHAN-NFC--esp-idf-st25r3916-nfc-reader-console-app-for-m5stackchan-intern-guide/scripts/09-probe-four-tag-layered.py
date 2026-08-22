#!/usr/bin/env python3
"""ESP-60 single-owner four-tag layer probe.

Waits for the real esp_console prompt between commands. This avoids the fixed
settle-window interleaving seen in older probes. No driver DEBUG during the
measured read, so serial logging does not perturb the hot I2C path.
"""
from __future__ import annotations
import argparse
import serial
import time


def read_until_prompt(ser: serial.Serial, timeout_s: float) -> str:
    deadline = time.monotonic() + timeout_s
    data = bytearray()
    while time.monotonic() < deadline:
        n = ser.in_waiting
        if n:
            data.extend(ser.read(n))
            if b"nfc> " in data:
                break
        else:
            time.sleep(0.02)
    return data.decode("utf-8", "replace")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("port")
    ap.add_argument("output")
    args = ap.parse_args()

    ser = serial.Serial(args.port, 115200, timeout=0.2)
    ser.dtr = False
    ser.rts = False
    time.sleep(0.2)
    ser.reset_input_buffer()

    with open(args.output, "w", encoding="utf-8") as log:
        def section(name: str, text: str) -> None:
            block = f"\n=== {name} ===\n{text}\n"
            print(block, end="")
            log.write(block)
            log.flush()

        section("BOOT", read_until_prompt(ser, 8.0))
        commands = [
            ("nfc-i2c-debug off", 3.0),
            ("nfc-trace clear", 3.0),
            ("nfc-regs", 5.0),
            ("nfc-read --attempts 1", 8.0),
            ("nfc-trace status", 3.0),
            ("nfc-trace dump --last 96", 8.0),
            ("nfc-trace first-error", 5.0),
        ]
        for command, timeout_s in commands:
            ser.write((command + "\n").encode())
            ser.flush()
            section(f"CMD: {command}", read_until_prompt(ser, timeout_s))
    ser.close()


if __name__ == "__main__":
    main()
