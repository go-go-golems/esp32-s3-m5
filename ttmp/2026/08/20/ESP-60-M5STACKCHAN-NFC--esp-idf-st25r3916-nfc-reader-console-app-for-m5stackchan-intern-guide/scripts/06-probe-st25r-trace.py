#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# ESP-60: single-owner serial probe for the standalone 0115 trace firmware.
# Opens /dev/serial/by-id/... once, captures a fresh boot, then exercises the
# nfc-trace / nfc-probe / nfc-read commands and saves all output to a file.
#
# Usage: probe_st25r_trace.py <by-id-port> <output-file> [--attempts N]
import sys, time, serial, argparse

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("port")
    ap.add_argument("output")
    ap.add_argument("--attempts", type=int, default=25)
    args = ap.parse_args()

    ser = serial.Serial(args.port, 115200, timeout=0.2)
    # Avoid toggling reset lines aggressively; a re-reset is acceptable (fresh init).
    ser.dtr = False
    ser.rts = False
    time.sleep(0.2)
    ser.reset_input_buffer()

    log = open(args.output, "w")
    def writeln(s=""):
        print(s)
        log.write(s + "\n"); log.flush()

    def read_for(seconds):
        end = time.time() + seconds
        buf = ""
        while time.time() < end:
            n = ser.in_waiting
            if n:
                chunk = ser.read(n).decode("utf-8", "replace")
                buf += chunk
            else:
                time.sleep(0.05)
        return buf

    def send_cmd(cmd, settle=1.5):
        ser.write((cmd + "\n").encode())
        ser.flush()
        return read_for(settle)

    writeln("=== BOOT (fresh init, trace ring populated) ===")
    boot = read_for(4.0)
    writeln(boot)

    cmds = [
        "nfc-trace status",
        "nfc-probe",
        "nfc-regs",
        f"nfc-read --attempts {args.attempts}",
        "nfc-trace status",
        "nfc-trace dump --last 40",
        "nfc-trace first-error",
    ]
    for c in cmds:
        writeln(f"\n=== CMD: {c} ===")
        out = send_cmd(c, settle=2.5 if "read" in c or "dump" in c else 1.5)
        writeln(out)

    writeln("\n=== DONE ===")
    log.close()
    ser.close()

if __name__ == "__main__":
    main()
