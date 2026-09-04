#!/usr/bin/env python3
"""PaperS3 console client that survives cdc_acm's open-time DTR/RTS.

On this host (kernel 6.8, cdc_acm), *every* open of /dev/ttyACM0 asserts
DTR+RTS, and the resulting edges leave the ESP32-S3 USB-Serial-JTAG in ROM
download mode: the app never talks, the console looks dead (found in
ESP-55 Phase 0; esptool --before no_reset syncs => chip sits in the ROM).
The ESP-50 client avoided ioctls but cannot avoid the open-time assertion.

This client therefore opens the port ONCE, performs a deliberate, correct
reset-into-app through the held fd (DTR=0 + RTS=1 pulses EN low, RTS=0
releases it with GPIO0 high), then keeps the fd open for the whole command
list so no further modem edges occur. Use it instead of the ESP-50 client
whenever the console appears silent.

usage:
  04-papers3-console-hold.py [--port P] [--no-reset] [--boot-settle 9]
      [--settle 3] [--output F] --cmd "status" [--cmd ...]
"""
from __future__ import annotations
import argparse, fcntl, os, select, struct, sys, termios, time
from pathlib import Path

DEFAULT_PORT = Path(
    "/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_"
    "D0:CF:13:16:17:DC-if00")

def set_modem(fd, dtr, rts):
    TIOCMGET, TIOCMSET = 0x5415, 0x5418
    TIOCM_DTR, TIOCM_RTS = 0x002, 0x004
    bits = struct.unpack('I', fcntl.ioctl(fd, TIOCMGET, struct.pack('I', 0)))[0]
    bits = (bits | TIOCM_DTR) if dtr else (bits & ~TIOCM_DTR)
    bits = (bits | TIOCM_RTS) if rts else (bits & ~TIOCM_RTS)
    fcntl.ioctl(fd, TIOCMSET, struct.pack('I', bits))

def drain(fd, duration, sink):
    deadline = time.monotonic() + duration
    while time.monotonic() < deadline:
        r, _, _ = select.select([fd], [], [], 0.05)
        if not r:
            continue
        try:
            chunk = os.read(fd, 4096)
        except OSError:
            break
        if chunk:
            sink.write(chunk.decode("utf-8", "replace"))
            sink.flush()

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--port", type=Path, default=DEFAULT_PORT)
    ap.add_argument("--cmd", action="append", default=[])
    ap.add_argument("--settle", type=float, default=3.0)
    ap.add_argument("--boot-settle", type=float, default=9.0,
                    help="seconds to capture boot output after the reset")
    ap.add_argument("--no-reset", action="store_true",
                    help="assume the app is already running")
    ap.add_argument("--output", type=Path)
    a = ap.parse_args()
    if not a.cmd:
        ap.error("at least one --cmd required")

    out = a.output.open("w") if a.output else None
    class Tee:
        def write(s, t):
            sys.stdout.write(t)
            if out: out.write(t)
        def flush(s):
            sys.stdout.flush()
            if out: out.flush()
    sink = Tee()

    fd = os.open(a.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
    attrs = termios.tcgetattr(fd)
    attrs[0] = attrs[1] = attrs[3] = 0
    attrs[6][termios.VMIN] = attrs[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, attrs)

    if not a.no_reset:
        # esptool HardReset for USB-Serial-JTAG, from a clean idle state:
        # the peripheral latches GPIO0 from DTR when EN falls, and cdc_acm
        # opened the port with DTR=RTS=1, so go through (0,0) first or the
        # chip re-enters the ROM downloader (observed: boot:0x0 DOWNLOAD).
        sink.write("# reset: idle(0,0) .. RTS=1 (EN low) .. RTS=0 (boot)\n")
        set_modem(fd, dtr=False, rts=False)
        time.sleep(0.2)
        set_modem(fd, dtr=False, rts=True)
        time.sleep(0.2)
        set_modem(fd, dtr=False, rts=False)
        drain(fd, a.boot_settle, sink)
    for cmd in a.cmd:
        sink.write(f"\n# >>> {cmd}\n")
        os.write(fd, cmd.encode() + b"\n")
        drain(fd, a.settle, sink)
    sink.write("\n# done\n")
    fcntl.flock(fd, fcntl.LOCK_UN)
    os.close(fd)
    return 0

if __name__ == "__main__":
    sys.exit(main())
