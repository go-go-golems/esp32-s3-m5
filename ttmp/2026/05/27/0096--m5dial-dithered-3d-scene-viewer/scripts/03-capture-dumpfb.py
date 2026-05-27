#!/usr/bin/env python3
"""Capture M5Dial firmware screenshots over USB Serial/JTAG.

This script talks to the esp_console prompt, sends `dumpfb`, captures the 2-bit
framebuffer transcript, and writes PNG files using the sibling
`02-dumpfb-to-png.py` decoder.

Examples:
  python3 03-capture-dumpfb.py --port /dev/ttyACM0 --out artifacts/current.png

  python3 03-capture-dumpfb.py --port /dev/ttyACM0 \
      --setup "scene planet" --setup "debug off" --setup "pixel 2" \
      --out artifacts/planet.png --transcript artifacts/planet.txt

  python3 03-capture-dumpfb.py --port /dev/ttyACM0 --all-scenes --out-dir artifacts/scenes

The serial device must be single-owner. Stop `idf.py monitor` before running.
Opening the USB Serial/JTAG port may reset the board; the script waits for the
`3d>` prompt before sending commands.
"""

from __future__ import annotations

import argparse
import importlib.util
import sys
import time
from pathlib import Path

try:
    import serial  # type: ignore
except ImportError as exc:  # pragma: no cover - host environment issue
    raise SystemExit("pyserial is required for serial capture: python3 -m pip install pyserial") from exc

SCENES = ["terrain", "torus", "ocean", "planet", "tunnel"]
PROMPT = b"3d>"


def load_png_decoder():
    decoder_path = Path(__file__).with_name("02-dumpfb-to-png.py")
    spec = importlib.util.spec_from_file_location("dumpfb_to_png", decoder_path)
    if spec is None or spec.loader is None:
        raise SystemExit(f"could not load decoder script: {decoder_path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class ConsoleSerial:
    def __init__(self, port: str, baud: int, verbose: bool):
        self.port = port
        self.baud = baud
        self.verbose = verbose
        self.ser: serial.Serial | None = None

    def __enter__(self):
        self.ser = serial.Serial(self.port, self.baud, timeout=0.05, write_timeout=2.0)
        # These levels match the manual pyserial smoke test on USB Serial/JTAG.
        # Opening the port can reset the board; wait_for_prompt handles that.
        self.ser.dtr = False
        self.ser.rts = False
        self.ser.reset_input_buffer()
        return self

    def __exit__(self, exc_type, exc, tb):
        if self.ser is not None:
            self.ser.close()
            self.ser = None

    def write_command(self, command: str) -> None:
        assert self.ser is not None
        self.ser.write(command.encode("utf-8") + b"\r\n")
        self.ser.flush()

    def read_until(self, marker: bytes, timeout: float) -> bytes:
        assert self.ser is not None
        end = time.monotonic() + timeout
        chunks: list[bytes] = []
        joined = b""
        while time.monotonic() < end:
            data = self.ser.read(4096)
            if not data:
                continue
            chunks.append(data)
            joined += data
            if marker in joined:
                return joined
        raise TimeoutError(f"timed out waiting for {marker!r}; captured {len(joined)} bytes")

    def wait_for_prompt(self, timeout: float) -> bytes:
        # Send a newline in case the app is already at the prompt. If opening the
        # port reset the board, boot logs will arrive before the prompt.
        self.write_command("")
        return self.read_until(PROMPT, timeout)

    def run_command_wait_prompt(self, command: str, timeout: float) -> bytes:
        if self.verbose:
            print(f"3d> {command}", file=sys.stderr)
        self.write_command(command)
        return self.read_until(PROMPT, timeout)


def extract_dump_block(text: str) -> str:
    begin = text.find("DUMPFB_BEGIN")
    end = text.find("DUMPFB_END")
    if begin < 0 or end < 0:
        raise SystemExit("capture did not contain a complete DUMPFB block")
    return text[begin : end + len("DUMPFB_END")] + "\n"


def capture_once(
    con: ConsoleSerial,
    decoder,
    setup: list[str],
    out_png: Path,
    transcript_path: Path | None,
    command_timeout: float,
    dump_timeout: float,
    verbose: bool,
) -> None:
    for command in setup:
        con.run_command_wait_prompt(command, command_timeout)

    if verbose:
        print("3d> dumpfb", file=sys.stderr)
    con.write_command("dumpfb")
    raw = con.read_until(b"DUMPFB_END", dump_timeout)
    # Consume the trailing prompt when it arrives, but do not require it. This
    # keeps the next command from seeing leftover prompt bytes.
    try:
        raw += con.read_until(PROMPT, 1.0)
    except TimeoutError:
        pass

    dump_text = extract_dump_block(raw.decode("utf-8", errors="replace"))

    if transcript_path:
        transcript_path.parent.mkdir(parents=True, exist_ok=True)
        transcript_path.write_text(dump_text)

    width, height, palette, rows = decoder.parse_dump(dump_text)
    png = decoder.rows_to_png(width, height, palette, rows)
    out_png.parent.mkdir(parents=True, exist_ok=True)
    out_png.write_bytes(png)
    if verbose:
        print(f"wrote {out_png} ({len(png)} bytes)", file=sys.stderr)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default="/dev/ttyACM0", help="serial port (default: /dev/ttyACM0)")
    parser.add_argument("--baud", type=int, default=115200, help="serial baud (default: 115200)")
    parser.add_argument("--setup", action="append", default=[], help="setup command before dumpfb; may repeat")
    parser.add_argument("--out", type=Path, help="output PNG for single capture")
    parser.add_argument("--transcript", type=Path, help="optional dump transcript for single capture")
    parser.add_argument("--all-scenes", action="store_true", help="capture terrain, torus, ocean, planet, tunnel")
    parser.add_argument("--out-dir", type=Path, help="output directory for --all-scenes")
    parser.add_argument("--prompt-timeout", type=float, default=12.0, help="seconds to wait for boot prompt")
    parser.add_argument("--command-timeout", type=float, default=3.0, help="seconds to wait for command prompt")
    parser.add_argument("--dump-timeout", type=float, default=20.0, help="seconds to wait for dumpfb")
    parser.add_argument("--quiet", action="store_true", help="suppress progress logs")
    args = parser.parse_args()

    verbose = not args.quiet
    decoder = load_png_decoder()

    with ConsoleSerial(args.port, args.baud, verbose) as con:
        if verbose:
            print(f"waiting for prompt on {args.port}...", file=sys.stderr)
        con.wait_for_prompt(args.prompt_timeout)

        if args.all_scenes:
            out_dir = args.out_dir or Path("artifacts/dumpfb-scenes")
            for scene in SCENES:
                setup = [*args.setup, f"scene {scene}"]
                png = out_dir / f"{scene}.png"
                txt = out_dir / f"{scene}.txt"
                capture_once(con, decoder, setup, png, txt, args.command_timeout, args.dump_timeout, verbose)
        else:
            out_png = args.out or Path("dumpfb.png")
            capture_once(con, decoder, args.setup, out_png, args.transcript, args.command_timeout, args.dump_timeout, verbose)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
