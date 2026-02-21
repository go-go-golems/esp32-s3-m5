#!/usr/bin/env python3
import argparse
import sys
import time

import serial


def read_until_prompt(ser: serial.Serial, timeout_s: float = 4.0) -> str:
    end = time.time() + timeout_s
    chunks: list[str] = []
    while time.time() < end:
        b = ser.read(512)
        if b:
            t = b.decode("utf-8", errors="replace")
            chunks.append(t)
            if "c3m> " in t:
                break
            if "c3m> " in "".join(chunks):
                break
    return "".join(chunks)


def run_cmd(ser: serial.Serial, cmd: str) -> str:
    ser.write((cmd + "\n").encode("utf-8"))
    ser.flush()
    return read_until_prompt(ser)


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--port", default="/dev/serial/by-id/usb-1a86_USB_Single_Serial_575E072431-if00")
    p.add_argument("--baud", type=int, default=115200)
    args = p.parse_args()

    commands = [
        "js examples",
        "js status",
        "js eval 2+3",
        "js reset",
        "js eval 4+5",
        "js reset hard",
        "js eval 6+7",
    ]

    with serial.Serial(args.port, args.baud, timeout=0.2) as ser:
        # drain startup output and wait for prompt once
        boot = read_until_prompt(ser, timeout_s=8.0)
        if boot:
            sys.stdout.write("== boot/prompt ==\n")
            sys.stdout.write(boot)
            sys.stdout.write("\n")

        for cmd in commands:
            sys.stdout.write(f"== {cmd} ==\n")
            out = run_cmd(ser, cmd)
            sys.stdout.write(out)
            if not out.endswith("\n"):
                sys.stdout.write("\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
