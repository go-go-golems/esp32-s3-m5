#!/usr/bin/env python3
"""01-probe-quickjs-console.py — single-owner serial probe for firmware 0100.

Resets the ESP32-P4, captures the boot log, then sends `js eval` and `js status`
commands and prints the responses. One process, one port session (AGENTS.md
serial-ownership). Usage: python 01-probe-quickjs-console.py [/dev/ttyACM0]"""
import serial, sys, time

PORT = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0"
BAUD = 115200


def reset(s: serial.Serial):
    # ESP32 auto-reset: DTR=GPIO0 (False = normal boot), RTS=EN (True = reset).
    s.dtr = False
    s.rts = True
    time.sleep(0.1)
    s.rts = False
    time.sleep(0.05)


def read_for(s: serial.Serial, seconds: float) -> str:
    end = time.time() + seconds
    buf = bytearray()
    while time.time() < end:
        n = s.in_waiting
        if n:
            buf += s.read(n)
        else:
            time.sleep(0.05)
    return buf.decode("utf-8", "replace")


def send(s: serial.Serial, line: str):
    s.write((line + "\r\n").encode())


def main():
    s = serial.Serial(PORT, BAUD, timeout=0.1)
    reset(s)
    print("=== boot log (~6s) ===")
    print(read_for(s, 6.0), end="")
    for cmd in ['js eval "print(1+2)"', 'js eval "for(let i=0;i<3;i++) print(i)"',
                'js eval "print(6*7)"', 'js status']:
        print(f"\n>>> {cmd}")
        send(s, cmd)
        print(read_for(s, 3.0), end="")
    s.close()


if __name__ == "__main__":
    main()
