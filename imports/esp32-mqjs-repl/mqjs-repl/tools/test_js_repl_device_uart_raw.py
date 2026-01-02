#!/usr/bin/env python3

import argparse
import sys
import time


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Direct UART smoke test for JsEvaluator REPL (no idf_monitor)."
    )
    parser.add_argument(
        "--port",
        default="auto",
        help="Serial port (e.g. /dev/ttyACM0) or 'auto' (default: auto)",
    )
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--timeout", type=float, default=25.0, help="Timeout seconds")
    args = parser.parse_args()

    try:
        import serial  # type: ignore
    except Exception as e:
        print(f"pyserial is required (`python -m pip install pyserial`): {e}", file=sys.stderr)
        return 2

    start = time.time()
    buf = ""

    def deadline() -> bool:
        return (time.time() - start) > args.timeout

    port = args.port
    if port == "auto":
        import glob

        candidates = sorted(glob.glob("/dev/serial/by-id/*Espressif*USB_JTAG*"))
        if len(candidates) == 1:
            port = candidates[0]
        elif len(candidates) > 1:
            print(
                "Multiple Espressif USB_JTAG ports found; pass --port explicitly:\n"
                + "\n".join(candidates),
                file=sys.stderr,
            )
            return 2
        else:
            port = "/dev/ttyACM0"

    with serial.Serial(port, args.baud, timeout=0.2) as ser:
        ser.reset_input_buffer()
        ser.reset_output_buffer()

        # Don't rely on seeing the initial prompt: it may have printed before we opened the port
        # (and we reset the input buffer). Instead, trigger output by issuing commands.
        ser.write(b":mode js\r\n")
        ser.flush()

        while "mode: js" not in buf:
            if deadline():
                print("Timed out waiting for 'mode: js' response", file=sys.stderr)
                return 1
            chunk = ser.read(4096)
            if chunk:
                buf += chunk.decode("utf-8", errors="replace")

        ser.write(b"var x = 1 + 2; x\r\n")
        ser.flush()

        while "\n3\n" not in buf and "3\n" not in buf:
            if deadline():
                print("Timed out waiting for JS eval output containing '3'", file=sys.stderr)
                return 1
            chunk = ser.read(4096)
            if chunk:
                buf += chunk.decode("utf-8", errors="replace")

        ser.write(b":stats\r\n")
        ser.flush()

        while "heap_free=" not in buf:
            if deadline():
                print("Timed out waiting for ':stats' output (missing 'heap_free=')", file=sys.stderr)
                return 1
            chunk = ser.read(4096)
            if chunk:
                buf += chunk.decode("utf-8", errors="replace")

        ser.write(b":autoload --format\r\n")
        ser.flush()

        while "autoload:" not in buf:
            if deadline():
                print("Timed out waiting for ':autoload' output (missing 'autoload:')", file=sys.stderr)
                return 1
            chunk = ser.read(4096)
            if chunk:
                buf += chunk.decode("utf-8", errors="replace")

        ser.write(b"globalThis.AUTOLOAD_SEED\r\n")
        ser.flush()

        while "\n123\n" not in buf and "123\n" not in buf:
            if deadline():
                print("Timed out waiting for autoload seed value (expected '123')", file=sys.stderr)
                return 1
            chunk = ser.read(4096)
            if chunk:
                buf += chunk.decode("utf-8", errors="replace")

    print("OK: device JS UART raw smoke test passed", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
