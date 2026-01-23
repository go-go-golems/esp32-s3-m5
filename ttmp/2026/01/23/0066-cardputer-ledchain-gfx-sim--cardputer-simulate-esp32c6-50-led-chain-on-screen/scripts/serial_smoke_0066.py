#!/usr/bin/env python3

import argparse
import datetime as dt
import os
import sys
import time

import serial


def now_stamp() -> str:
    return dt.datetime.now().strftime("%Y%m%d-%H%M%S")


def write_line(ser: serial.Serial, s: str) -> None:
    ser.write((s.rstrip("\n") + "\n").encode("utf-8"))


def read_for(ser: serial.Serial, seconds: float) -> bytes:
    end = time.time() + seconds
    buf = bytearray()
    while time.time() < end:
        chunk = ser.read(4096)
        if chunk:
            buf += chunk
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
        # Give some time after reset/boot.
        time.sleep(0.4)

        # Best-effort: elicit a prompt and prove the REPL parses our command.
        write_line(ser, "")
        write_line(ser, "help")
        write_line(ser, "sim help")
        write_line(ser, "sim status")

        write_line(ser, "sim pattern set rainbow")
        write_line(ser, "sim rainbow set --speed 5 --sat 100 --spread 10 --bri 25")
        write_line(ser, "sim status")

        write_line(ser, "sim pattern set chase")
        write_line(
            ser,
            "sim chase set --speed 30 --tail 5 --gap 10 --trains 1 --fg #FFFFFF --bg #000000 --dir forward --fade 1 --bri 25",
        )
        write_line(ser, "sim status")

        data = read_for(ser, args.read_seconds)
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
