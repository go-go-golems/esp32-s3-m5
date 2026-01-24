#!/usr/bin/env python3
import argparse
import sys
import time


def main() -> int:
    ap = argparse.ArgumentParser(description="Read raw serial output for N seconds (non-interactive).")
    ap.add_argument("--port", default="/dev/ttyACM0")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--seconds", type=float, default=5.0)
    ap.add_argument("--reset", action="store_true", help="Toggle DTR/RTS to try to reset the device.")
    args = ap.parse_args()

    try:
        import serial  # type: ignore
    except Exception as e:
        print(f"ERROR: pyserial not available: {e}", file=sys.stderr)
        return 2

    try:
        ser = serial.Serial(args.port, args.baud, timeout=0.1)
    except Exception as e:
        print(f"ERROR: failed to open {args.port}: {e}", file=sys.stderr)
        return 2

    with ser:
        if args.reset:
            try:
                ser.dtr = False
                ser.rts = True
                time.sleep(0.05)
                ser.rts = False
                time.sleep(0.05)
                ser.dtr = True
            except Exception:
                pass

        deadline = time.time() + args.seconds
        buf = bytearray()
        while time.time() < deadline:
            try:
                chunk = ser.read(4096)
            except Exception as e:
                print(f"ERROR: read failed: {e}", file=sys.stderr)
                return 2
            if chunk:
                buf.extend(chunk)

        sys.stdout.buffer.write(bytes(buf))
        if buf and buf[-1] != 0x0A:
            sys.stdout.write("\n")
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
