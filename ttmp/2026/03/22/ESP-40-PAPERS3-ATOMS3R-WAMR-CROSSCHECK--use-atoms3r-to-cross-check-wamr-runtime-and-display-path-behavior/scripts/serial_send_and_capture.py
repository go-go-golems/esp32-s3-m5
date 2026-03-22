#!/usr/bin/env python3
import argparse
import sys
import time

import serial


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Send one esp_console command over USB Serial/JTAG and capture the reply."
    )
    parser.add_argument("--port", default="/dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--command", required=True)
    parser.add_argument("--prompt", default="atom>")
    parser.add_argument("--char-delay-ms", type=float, default=30.0)
    parser.add_argument("--boot-settle-seconds", type=float, default=1.0)
    parser.add_argument("--prompt-timeout", type=float, default=2.0)
    parser.add_argument("--capture-seconds", type=float, default=8.0)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    char_delay = args.char_delay_ms / 1000.0
    prompt = args.prompt.encode()
    output = bytearray()

    with serial.Serial(args.port, args.baud, timeout=0.1, dsrdtr=False, rtscts=False) as ser:
        ser.dtr = False
        ser.rts = False

        settle_deadline = time.time() + args.boot_settle_seconds
        while time.time() < settle_deadline:
            data = ser.read(1024)
            if not data:
                time.sleep(0.05)
                continue

            output.extend(data)
            if b"\x1b[6n" in output:
                ser.write(b"\x1b[1;1R")
                output = output.replace(b"\x1b[6n", b"")

        if prompt not in output:
            ser.write(b"\n")

        prompt_deadline = time.time() + args.prompt_timeout
        while time.time() < prompt_deadline:
            data = ser.read(1024)
            if not data:
                time.sleep(0.05)
                continue

            output.extend(data)
            if b"\x1b[6n" in output:
                ser.write(b"\x1b[1;1R")
                output = output.replace(b"\x1b[6n", b"")
            if prompt in output:
                break

        time.sleep(0.2)
        for ch in (args.command + "\r").encode():
            ser.write(bytes([ch]))
            time.sleep(char_delay)
            data = ser.read(256)
            if data:
                output.extend(data)
                if b"\x1b[6n" in output:
                    ser.write(b"\x1b[1;1R")
                    output = output.replace(b"\x1b[6n", b"")

        capture_deadline = time.time() + args.capture_seconds
        while time.time() < capture_deadline:
            data = ser.read(4096)
            if not data:
                time.sleep(0.05)
                continue

            output.extend(data)
            if b"\x1b[6n" in output:
                ser.write(b"\x1b[1;1R")
                output = output.replace(b"\x1b[6n", b"")

    sys.stdout.buffer.write(bytes(output))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
