#!/usr/bin/env python3

import argparse
import datetime as dt
import os
import sys
import time

import serial
from serial.serialutil import SerialException


def now_stamp() -> str:
    return dt.datetime.now().strftime("%Y%m%d-%H%M%S")


def write_line(ser: serial.Serial, s: str) -> None:
    ser.write((s.rstrip("\n") + "\n").encode("utf-8"))


def read_for(ser: serial.Serial, seconds: float) -> bytes:
    end = time.time() + seconds
    buf = bytearray()
    while time.time() < end:
        try:
            chunk = ser.read(4096)
        except SerialException as e:
            buf += f"\n[serial read error: {e}]\n".encode("utf-8", errors="replace")
            break
        if chunk:
            buf += chunk
            continue
        time.sleep(0.02)
    return bytes(buf)

def read_until(ser: serial.Serial, needle: bytes, timeout_s: float) -> bytes:
    end = time.time() + timeout_s
    buf = bytearray()
    while time.time() < end:
        try:
            chunk = ser.read(4096)
        except SerialException as e:
            buf += f"\n[serial read error: {e}]\n".encode("utf-8", errors="replace")
            break
        if chunk:
            buf += chunk
            if needle in buf:
                break
            continue
        time.sleep(0.02)
    return bytes(buf)


def main() -> int:
    ap = argparse.ArgumentParser(description="0066 Cardputer LED sim: non-interactive serial smoke test")
    ap.add_argument("--port", default="/dev/ttyACM0")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--timeout", type=float, default=0.2)
    ap.add_argument("--read-seconds", type=float, default=5.0)
    ap.add_argument("--out", default="")
    args = ap.parse_args()

    out_path = args.out
    if not out_path:
        ticket_dir = os.path.dirname(os.path.dirname(__file__))
        out_dir = os.path.join(ticket_dir, "various")
        os.makedirs(out_dir, exist_ok=True)
        out_path = os.path.join(out_dir, f"serial-smoke-0066-{now_stamp()}.log")

    try:
        ser = serial.Serial(args.port, args.baud, timeout=args.timeout)
    except Exception as e:
        print(f"ERROR: open {args.port}: {e}", file=sys.stderr)
        return 2

    try:
        try:
            ser.reset_input_buffer()
        except Exception:
            pass

        # Give some time after reset/boot.
        time.sleep(0.4)

        # Try to reset any partially-entered line in linenoise.
        ser.write(b"\x03")  # Ctrl-C
        ser.write(b"\x15")  # Ctrl-U (kill line)
        ser.write(b"\r\n")
        time.sleep(0.1)

        transcript = bytearray()
        transcript += read_until(ser, b"sim> ", 1.0)

        def run(cmd: str) -> None:
            write_line(ser, cmd)
            time.sleep(0.05)
            transcript.extend(read_until(ser, b"sim> ", 1.5))
            time.sleep(4)

        run("help")
        run("sim help")
        run("sim status")

        run("sim pattern set rainbow")
        run("sim rainbow set --speed 5 --sat 100 --spread 10 --bri 25")
        run("sim status")

        run("sim pattern set chase")
        run("sim chase set --speed 30 --tail 5 --gap 10 --trains 1 --fg #FFFFFF --bg #000000 --dir forward --fade 1 --bri 25")
        run("sim status")

        transcript.extend(read_for(ser, args.read_seconds))
        data = bytes(transcript)
    finally:
        try:
            ser.close()
        except Exception:
            pass

    text = data.decode("utf-8", errors="replace")
    with open(out_path, "w", encoding="utf-8") as f:
        f.write(text)

    print(out_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
