#!/usr/bin/env python3
"""Run one PaperS3 EPD qualification corpus through one serial session."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import pathlib
import sys
import time
from dataclasses import dataclass

try:
    import serial
except ImportError as exc:  # pragma: no cover - environment error
    raise SystemExit(
        "pyserial is required; source the selected ESP-IDF export.sh before running this tool"
    ) from exc


PROMPT = b"epd-qual> "


@dataclass(frozen=True)
class TestCommand:
    name: str
    command: str
    expects_pass_marker: bool = True


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Capture the automatic half of a PaperS3 EPD matrix-cell qualification"
    )
    parser.add_argument("--port", required=True, help="stable /dev/serial/by-id path")
    parser.add_argument("--cell", required=True, choices=list("ABCD"))
    parser.add_argument("--output-dir", required=True, type=pathlib.Path)
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--command-timeout", type=float, default=1800.0)
    parser.add_argument("--text-iterations", type=int, default=1000)
    parser.add_argument("--mixed-iterations", type=int, default=1000)
    parser.add_argument(
        "--smoke",
        action="store_true",
        help="run 10 text and 25 mixed updates instead of the requested soak counts",
    )
    return parser.parse_args()


def command_corpus(args: argparse.Namespace) -> list[TestCommand]:
    text_iterations = 10 if args.smoke else args.text_iterations
    mixed_iterations = 25 if args.smoke else args.mixed_iterations
    return [
        TestCommand("initial-status", "epd status", expects_pass_marker=False),
        TestCommand("white", "epd scene white"),
        TestCommand("black", "epd scene black"),
        TestCommand("grayscale", "epd scene gray"),
        TestCommand("checkerboard", "epd scene checker"),
        TestCommand("text", "epd scene text"),
        TestCommand("issue-181-boundaries", "epd boundary all"),
        TestCommand("text-soak", f"epd text-soak {text_iterations}"),
        TestCommand("mixed-refresh-soak", f"epd soak {mixed_iterations}"),
        TestCommand("display-sleep-wake", "epd cycle-sleep 2000"),
        TestCommand("final-status", "epd status", expects_pass_marker=False),
    ]


def read_to_prompt(port: serial.Serial, timeout_seconds: float) -> bytes:
    deadline = time.monotonic() + timeout_seconds
    received = bytearray()
    while time.monotonic() < deadline:
        chunk = port.read(port.in_waiting or 1)
        if chunk:
            received.extend(chunk)
            if PROMPT in received:
                return bytes(received)
    raise TimeoutError(
        f"did not receive {PROMPT.decode()!r} within {timeout_seconds:.0f}s; "
        "keep this session open and press the PaperS3 reset button if the board is waiting in ROM mode"
    )


def run_command(
    port: serial.Serial, test: TestCommand, timeout_seconds: float
) -> tuple[bytes, bool]:
    wire_command = (test.command + "\n").encode()
    port.write(wire_command)
    port.flush()
    response = read_to_prompt(port, timeout_seconds)
    passed = not test.expects_pass_marker or b"command.result=pass" in response
    return response, passed


def write_operator_checklist(path: pathlib.Path, cell: str) -> None:
    path.write_text(
        f"""# PaperS3 matrix cell {cell}: operator checklist

Automatic serial checks cannot establish display quality. Review the panel and attach photographs before marking this cell qualified.

- [ ] White scene is uniform and has no unexpected black edge pixels.
- [ ] Black scene is uniform and reaches all four physical corners.
- [ ] Sixteen grayscale bars are ordered, distinct where the panel permits, and free of severe band corruption.
- [ ] Checkerboard geometry is aligned in every region.
- [ ] Text scene is legible, correctly rotated, and not clipped.
- [ ] Boundary corpus reached every rotation without reboot, corruption, or lost prompt.
- [ ] The 1,000-update text soak completed without worsening localized artifacts beyond expected EPD ghosting.
- [ ] Mixed full/partial refresh soak completed with stable heap diagnostics.
- [ ] Display sleep/wake restored a complete, correctly rotated text scene.
- [ ] Final panel condition and serial transcript have been reviewed together.

## Evidence

- Transcript: `transcript.txt`
- Machine result: `result.json`
- Build metadata: copy `build-cell-{cell}/qualification-build.txt` here as `qualification-build.txt`
- Photographs: add paths or links here.
- Operator:
- Date:
- Final disposition: pending / pass / fail
- Notes:
""",
        encoding="utf-8",
    )


def main() -> int:
    args = parse_args()
    if not 1 <= args.text_iterations <= 10000:
        raise SystemExit("--text-iterations must be 1..10000")
    if not 1 <= args.mixed_iterations <= 10000:
        raise SystemExit("--mixed-iterations must be 1..10000")

    args.output_dir.mkdir(parents=True, exist_ok=True)
    transcript_path = args.output_dir / "transcript.txt"
    result_path = args.output_dir / "result.json"
    checklist_path = args.output_dir / "operator-checklist.md"
    started = dt.datetime.now(dt.timezone.utc)
    outcomes: list[dict[str, object]] = []
    automatic_pass = True

    print(f"Opening {args.port} exclusively; do not start idf.py monitor in parallel.")
    with serial.Serial(
        args.port,
        args.baud,
        timeout=0.25,
        write_timeout=5,
        exclusive=True,
    ) as port, transcript_path.open("wb") as transcript:
        # A newline makes a prompt observable even when the initial boot prompt preceded attach.
        port.write(b"\n")
        port.flush()
        try:
            initial = read_to_prompt(port, 15)
        except TimeoutError as exc:
            print(str(exc), file=sys.stderr)
            return 2
        transcript.write(initial)

        for test in command_corpus(args):
            print(f"[{test.name}] {test.command}", flush=True)
            command_started = time.monotonic()
            transcript.write(f"\n\n### {test.name}: {test.command}\n".encode())
            try:
                response, passed = run_command(port, test, args.command_timeout)
                transcript.write(response)
                transcript.flush()
                duration = time.monotonic() - command_started
                outcomes.append(
                    {
                        "name": test.name,
                        "command": test.command,
                        "passed": passed,
                        "duration_seconds": round(duration, 3),
                    }
                )
                automatic_pass = automatic_pass and passed
                print(f"  {'PASS' if passed else 'FAIL'} ({duration:.1f}s)")
                if not passed:
                    break
            except (TimeoutError, serial.SerialException) as exc:
                duration = time.monotonic() - command_started
                outcomes.append(
                    {
                        "name": test.name,
                        "command": test.command,
                        "passed": False,
                        "duration_seconds": round(duration, 3),
                        "error": str(exc),
                    }
                )
                automatic_pass = False
                print(f"  FAIL: {exc}", file=sys.stderr)
                break

    finished = dt.datetime.now(dt.timezone.utc)
    result = {
        "schema_version": 1,
        "cell": args.cell,
        "port": args.port,
        "smoke": args.smoke,
        "started_at": started.isoformat(),
        "finished_at": finished.isoformat(),
        "automatic_pass": automatic_pass,
        "visual_disposition": "pending",
        "commands": outcomes,
    }
    result_path.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    write_operator_checklist(checklist_path, args.cell)
    print(f"Transcript: {transcript_path}")
    print(f"Result: {result_path}")
    print(f"Operator checklist: {checklist_path}")
    print("Automatic checks do not qualify a cell until the operator checklist and photographs pass.")
    return 0 if automatic_pass else 1


if __name__ == "__main__":
    raise SystemExit(main())
