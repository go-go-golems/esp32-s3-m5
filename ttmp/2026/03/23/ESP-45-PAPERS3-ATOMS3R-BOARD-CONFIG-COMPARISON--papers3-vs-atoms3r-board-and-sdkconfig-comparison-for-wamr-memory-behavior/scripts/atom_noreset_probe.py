#!/usr/bin/env python3
import argparse
import os
import select
import sys
import termios
import time


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Probe an Atom-style USB Serial/JTAG console without pyserial DTR/RTS toggling."
    )
    parser.add_argument("--port", default="/dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--boot-wait-seconds", type=float, default=12.0)
    parser.add_argument("--inter-command-delay-seconds", type=float, default=0.8)
    parser.add_argument("--capture-seconds", type=float, default=8.0)
    parser.add_argument("--command", action="append", required=True)
    return parser.parse_args()


def configure_port(fd: int, baud: int) -> None:
    baud_map = {
        115200: termios.B115200,
        460800: termios.B460800,
    }
    attrs = termios.tcgetattr(fd)
    attrs[0] = 0
    attrs[1] = 0
    attrs[2] = termios.CLOCAL | termios.CREAD | termios.CS8
    attrs[3] = 0
    attrs[4] = baud_map[baud]
    attrs[5] = baud_map[baud]
    termios.tcsetattr(fd, termios.TCSANOW, attrs)


def wait_for_port(port: str, timeout_seconds: float = 15.0) -> None:
    deadline = time.time() + timeout_seconds
    while not os.path.exists(port):
        if time.time() >= deadline:
            raise FileNotFoundError(port)
        time.sleep(0.1)


def read_for(fd: int, output: bytearray, seconds: float) -> None:
    deadline = time.time() + seconds
    while time.time() < deadline:
        rlist, _, _ = select.select([fd], [], [], 0.1)
        if not rlist:
            continue
        try:
            data = os.read(fd, 4096)
        except BlockingIOError:
            continue
        if data:
            output.extend(data)


def write_with_retry(fd: int, payload: bytes, retry_seconds: float = 6.0) -> None:
    deadline = time.time() + retry_seconds
    while True:
        try:
            os.write(fd, payload)
            return
        except OSError as exc:
            if exc.errno != 5 or time.time() >= deadline:
                raise
            time.sleep(0.1)


def main() -> int:
    args = parse_args()
    output = bytearray()
    wait_for_port(args.port)
    fd = os.open(args.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    try:
        configure_port(fd, args.baud)
        read_for(fd, output, args.boot_wait_seconds)
        write_with_retry(fd, b"\n")
        read_for(fd, output, 0.5)

        for command in args.command:
            write_with_retry(fd, (command + "\r").encode("utf-8"))
            read_for(fd, output, args.inter_command_delay_seconds)

        read_for(fd, output, args.capture_seconds)
    finally:
        os.close(fd)

    sys.stdout.buffer.write(bytes(output))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
