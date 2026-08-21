#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# ESP-60 Phase 4: confirm I2C_EVENT_NACK via the driver DEBUG log.
# Enables nfc-i2c-debug, runs a few reads, and captures the driver's
# "I2C transaction unexpected nack detected" / "timeout detected" lines.
import sys, time, serial, argparse

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("port")
    ap.add_argument("output")
    ap.add_argument("--attempts", type=int, default=10)
    args = ap.parse_args()

    ser = serial.Serial(args.port, 115200, timeout=0.2)
    ser.dtr = False; ser.rts = False
    time.sleep(0.2); ser.reset_input_buffer()
    log = open(args.output, "w")
    def writeln(s=""):
        print(s); log.write(s + "\n"); log.flush()
    def read_for(seconds):
        end = time.time() + seconds; buf = ""
        while time.time() < end:
            n = ser.in_waiting
            if n: buf += ser.read(n).decode("utf-8", "replace")
            else: time.sleep(0.03)
        return buf
    def send(cmd, settle=1.5):
        ser.write((cmd + "\n").encode()); ser.flush()
        return read_for(settle)

    writeln("=== BOOT ==="); writeln(read_for(4.0))
    seq = ["nfc-i2c-debug on", "nfc-trace clear",
           f"nfc-read --attempts {args.attempts}",
           "nfc-i2c-debug off",
           "nfc-trace annotate nack",
           "nfc-trace status",
           "nfc-trace first-error"]
    for c in seq:
        writeln(f"\n=== CMD: {c} ===")
        writeln(send(c, settle=4.0 if "read" in c else 1.5))
    writeln("\n=== DONE ==="); log.close(); ser.close()

if __name__ == "__main__":
    main()
