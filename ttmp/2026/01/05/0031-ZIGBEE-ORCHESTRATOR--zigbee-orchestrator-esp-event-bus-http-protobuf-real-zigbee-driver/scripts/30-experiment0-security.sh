#!/usr/bin/env bash
set -euo pipefail

# Experiment 0: Confirm join-loop diagnosis (TCLK/authorization timeout).
#
# This script can optionally flash both devices, then drives the Cardputer
# console to apply the default Trust Center Link Key and toggle link-key
# exchange requirement.
#
# Usage:
#   ./scripts/30-experiment0-security.sh
#
# Optional:
#   FLASH=1 H2_PORT=... HOST_PORT=... ./scripts/30-experiment0-security.sh
#
# Then put your device into pairing mode and watch logs in another terminal:
#   ./scripts/10-tmux-dual-monitor.sh

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"

HOST_PROJ="${ROOT_DIR}/0031-zigbee-orchestrator"
H2_PROJ="${H2_PROJ:-${ROOT_DIR}/thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp}"

H2_PORT="${H2_PORT:-/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_48:31:B7:CA:45:5B-if00}"
HOST_PORT="${HOST_PORT:-/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00}"

FLASH="${FLASH:-0}"

if [[ "$FLASH" == "1" ]]; then
  echo "[exp0] flashing H2..."
  idf.py -C "$H2_PROJ" set-target esp32h2
  idf.py -C "$H2_PROJ" -p "$H2_PORT" flash

  echo "[exp0] flashing host..."
  idf.py -C "$HOST_PROJ" -p "$HOST_PORT" flash
fi

python3 - <<PY
import os, sys, time, re
import serial

port = os.environ.get("HOST_PORT", "${HOST_PORT}")

prompt_re = re.compile(rb"\\bgw>\\s*$", re.MULTILINE)

def open_serial():
    s = serial.Serial(port, 115200, timeout=0.1)
    s.reset_input_buffer()
    return s

def read_for(s, seconds):
    end = time.time() + seconds
    buf = bytearray()
    while time.time() < end:
        chunk = s.read(4096)
        if chunk:
            buf.extend(chunk)
        else:
            time.sleep(0.02)
    return bytes(buf)

def read_until_prompt(s, timeout_s=8.0):
    end = time.time() + timeout_s
    buf = bytearray()
    while time.time() < end:
        chunk = s.read(4096)
        if chunk:
            buf.extend(chunk)
            if prompt_re.search(bytes(buf)):
                return bytes(buf)
        else:
            time.sleep(0.02)
    return bytes(buf)

def send_cmd(s, cmd):
    s.write(cmd.encode("utf-8") + b"\\r\\n")
    s.flush()
    out = read_until_prompt(s, timeout_s=10.0)
    sys.stdout.write(out.decode("utf-8", errors="replace"))
    if not prompt_re.search(out):
        sys.stdout.write("\\n[exp0] WARNING: prompt not seen after command: %r\\n" % cmd)

try:
    s = open_serial()
except Exception as e:
    print("[exp0] ERROR: failed to open serial %s: %r" % (port, e), file=sys.stderr)
    sys.exit(2)

print("[exp0] connected:", port)
sys.stdout.write(read_for(s, 0.6).decode("utf-8", errors="replace"))

# Ensure we have a prompt.
s.write(b"\\r\\n")
s.flush()
sys.stdout.write(read_until_prompt(s, timeout_s=6.0).decode("utf-8", errors="replace"))

print("\\n[exp0] 1) Set TCLK to ZigBeeAlliance09 and read it back")
send_cmd(s, "zb tclk set default")
send_cmd(s, "zb tclk get")

print("\\n[exp0] 2) Disable link-key exchange requirement temporarily (diagnostic)")
send_cmd(s, "zb lke off")
send_cmd(s, "zb lke status")

print("\\n[exp0] 3) Open permit join (pair your device now)")
send_cmd(s, "gw post permit_join 180")

print(
    """
[exp0] Next manual steps:
- Put plug into pairing mode.
- Watch H2 logs for 'Device authorized ... status=0x00' (success) or status=0x01 (timeout).
- If stable with lke=off, later try: 'zb lke on' and pair again.
"""
)
PY
