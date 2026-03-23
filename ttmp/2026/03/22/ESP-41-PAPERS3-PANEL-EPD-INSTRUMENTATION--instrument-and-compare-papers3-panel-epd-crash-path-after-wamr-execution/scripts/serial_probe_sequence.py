#!/usr/bin/env python3
import argparse
import sys
import time

import serial


PROMPT = b"paper>"
CURSOR_QUERY = b"\x1b[6n"
CURSOR_RESPONSE = b"\x1b[1;1R"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Send one or more esp_console commands over USB Serial/JTAG in a single boot session."
    )
    parser.add_argument("--port", default="/dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--command", action="append", required=True,
                        help="Command to send. Repeat --command for multi-step probes.")
    parser.add_argument("--char-delay-ms", type=float, default=30.0)
    parser.add_argument("--boot-settle-seconds", type=float, default=1.5)
    parser.add_argument("--prompt-timeout", type=float, default=4.0)
    parser.add_argument("--capture-seconds", type=float, default=8.0)
    parser.add_argument("--inter-command-delay-seconds", type=float, default=0.2)
    return parser.parse_args()


def filter_cursor_queries(ser: serial.Serial, output: bytearray) -> None:
    if CURSOR_QUERY in output:
        ser.write(CURSOR_RESPONSE)
        while CURSOR_QUERY in output:
            output[:] = output.replace(CURSOR_QUERY, b"")


def read_until(ser: serial.Serial, output: bytearray, deadline: float, needle: bytes | None = None) -> bool:
    while time.time() < deadline:
        data = ser.read(4096)
        if not data:
            time.sleep(0.05)
            continue

        output.extend(data)
        filter_cursor_queries(ser, output)
        if needle is not None and needle in output:
            return True

    return needle is None


def send_command(ser: serial.Serial, output: bytearray, command: str, char_delay: float) -> None:
    for ch in (command + "\r").encode():
        ser.write(bytes([ch]))
        time.sleep(char_delay)
        data = ser.read(256)
        if data:
            output.extend(data)
            filter_cursor_queries(ser, output)


def main() -> int:
    args = parse_args()
    char_delay = args.char_delay_ms / 1000.0
    output = bytearray()

    with serial.Serial(args.port, args.baud, timeout=0.1, dsrdtr=False, rtscts=False) as ser:
        ser.dtr = False
        ser.rts = False

        settle_deadline = time.time() + args.boot_settle_seconds
        read_until(ser, output, settle_deadline)

        if PROMPT not in output:
            ser.write(b"\n")

        prompt_deadline = time.time() + args.prompt_timeout
        if not read_until(ser, output, prompt_deadline, PROMPT):
            sys.stdout.buffer.write(bytes(output))
            print("\nserial_probe_sequence: prompt not observed before timeout", file=sys.stderr)
            return 1

        for index, command in enumerate(args.command):
            time.sleep(args.inter_command_delay_seconds)
            send_command(ser, output, command, char_delay)
            if index == len(args.command) - 1:
                capture_deadline = time.time() + args.capture_seconds
                read_until(ser, output, capture_deadline)
            else:
                next_prompt_deadline = time.time() + args.prompt_timeout + args.capture_seconds
                if not read_until(ser, output, next_prompt_deadline, PROMPT):
                    sys.stdout.buffer.write(bytes(output))
                    print(f"\nserial_probe_sequence: prompt not observed after command {index + 1}", file=sys.stderr)
                    return 1

    sys.stdout.buffer.write(bytes(output))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
