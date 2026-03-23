#!/usr/bin/env python3
import argparse
import sys
import time

import serial


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Send esp_console commands over USB Serial/JTAG without prompt gating."
    )
    parser.add_argument("--port", default="/dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--command", action="append", required=True)
    parser.add_argument("--boot-settle-seconds", type=float, default=2.5)
    parser.add_argument("--inter-command-delay-seconds", type=float, default=0.5)
    parser.add_argument("--char-delay-ms", type=float, default=30.0)
    parser.add_argument("--capture-seconds", type=float, default=10.0)
    return parser.parse_args()


def read_for(ser: serial.Serial, output: bytearray, seconds: float) -> None:
    deadline = time.time() + seconds
    while time.time() < deadline:
        data = ser.read(4096)
        if data:
            output.extend(data)
        else:
            time.sleep(0.05)


def send_command(ser: serial.Serial, output: bytearray, command: str, char_delay: float) -> None:
    for ch in (command + "\r").encode():
        ser.write(bytes([ch]))
        time.sleep(char_delay)
        data = ser.read(256)
        if data:
            output.extend(data)


def main() -> int:
    args = parse_args()
    output = bytearray()
    char_delay = args.char_delay_ms / 1000.0

    with serial.Serial(args.port, args.baud, timeout=0.1, dsrdtr=False, rtscts=False) as ser:
        ser.dtr = False
        ser.rts = False

        read_for(ser, output, args.boot_settle_seconds)
        ser.write(b"\n")
        read_for(ser, output, 0.5)

        for command in args.command:
            send_command(ser, output, command, char_delay)
            read_for(ser, output, args.inter_command_delay_seconds)

        read_for(ser, output, args.capture_seconds)

    sys.stdout.buffer.write(bytes(output))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
