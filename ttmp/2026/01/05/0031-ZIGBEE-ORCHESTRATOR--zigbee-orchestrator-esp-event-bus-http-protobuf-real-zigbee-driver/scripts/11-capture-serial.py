#!/usr/bin/env python3
import argparse
import sys
import time

import serial


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Capture raw serial output to a file (no TTY required)."
    )
    ap.add_argument("--port", required=True, help="Serial port (e.g. /dev/ttyACM0)")
    ap.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    ap.add_argument(
        "--seconds", type=float, default=30.0, help="Capture duration (default: 30)"
    )
    ap.add_argument("--out", required=True, help="Output file path")
    ap.add_argument(
        "--append", action="store_true", help="Append to output file instead of overwrite"
    )
    args = ap.parse_args()

    mode = "ab" if args.append else "wb"
    start = time.time()
    end = start + args.seconds

    sys.stderr.write(f"[capture] port={args.port} baud={args.baud} seconds={args.seconds}\n")
    sys.stderr.write(f"[capture] out={args.out} append={args.append}\n")

    with serial.Serial(args.port, args.baud, timeout=0.1) as s:
        # Best effort: avoid asserting DTR/RTS (can reset some boards).
        try:
            s.dtr = False
            s.rts = False
        except Exception:
            pass
        s.reset_input_buffer()
        with open(args.out, mode) as f:
            while time.time() < end:
                chunk = s.read(4096)
                if chunk:
                    f.write(chunk)
                    f.flush()
                else:
                    time.sleep(0.02)

    sys.stderr.write("[capture] done\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
