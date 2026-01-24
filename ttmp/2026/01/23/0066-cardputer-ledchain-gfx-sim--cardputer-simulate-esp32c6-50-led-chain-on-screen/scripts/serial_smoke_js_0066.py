#!/usr/bin/env python3
import argparse
import datetime as dt
import pathlib
import sys
import time

import serial  # pyserial


def write_line(ser: serial.Serial, line: str) -> None:
    ser.write((line.rstrip("\n") + "\n").encode("utf-8"))
    ser.flush()


def read_until(ser: serial.Serial, needle: bytes, timeout_s: float) -> bytes:
    deadline = time.time() + timeout_s
    buf = bytearray()
    while time.time() < deadline:
        b = ser.read(1024)
        if b:
            buf.extend(b)
            if needle in buf:
                break
        else:
            time.sleep(0.01)
    return bytes(buf)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="/dev/ttyACM0")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--timeout", type=float, default=0.2)
    args = ap.parse_args()

    ticket_dir = pathlib.Path(
        "ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen"
    ).resolve()
    out_dir = ticket_dir / "various"
    out_dir.mkdir(parents=True, exist_ok=True)
    out_path = out_dir / f"serial-smoke-js-{dt.datetime.now().strftime('%Y%m%d-%H%M%S')}.log"

    transcript: list[bytes] = []

    with serial.Serial(args.port, args.baud, timeout=args.timeout) as ser:
        ser.reset_input_buffer()
        ser.reset_output_buffer()

        # Give the device a moment to boot after USB/JTAG reset.
        time.sleep(0.8)
        transcript.append(read_until(ser, b"sim> ", 6.0))

        def run(cmd: str) -> None:
            write_line(ser, cmd)
            time.sleep(0.05)
            transcript.append(read_until(ser, b"sim> ", 2.0))

        run("help")
        run("wifi status")
        run("sim status")
        run("js help")
        run("js eval sim.status()")
        run("js eval sim.statusJson()")
        run('js eval sim.setBrightness(100)')
        run('js eval sim.setPattern("rainbow")')
        run("js eval sim.setRainbow(5,100,10)")
        run("js eval sim.status()")
        run('js eval sim.setPattern("chase")')
        run('js eval sim.setChase(30,5,10,1,"#FFFFFF","#000000",0,1,100)')
        run("js eval sim.status()")

        # Timers: setTimeout + every/cancel.
        run("js eval var __t=0; setTimeout(function(){ __t = 123; }, 100); 'timeout scheduled'")
        time.sleep(0.25)
        run("js eval __t")

        run("js eval var __c=0; var __h=null; __h=every(50,function(){ __c++; if(__c>=5) cancel(__h); }); 'every scheduled'")
        time.sleep(0.5)
        run("js eval __c")

        # GPIO: best-effort exercise (no external validation here).
        run("js eval gpio.write('G3',0); gpio.write('G4',0); 'gpio init ok'")
        run("js eval gpio.toggle('G3'); gpio.toggle('G4'); 'gpio toggle ok'")

    out_path.write_bytes(b"".join(transcript))
    print(out_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
