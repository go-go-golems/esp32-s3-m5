#!/usr/bin/env python3
"""
Read serial output from ATOMS3R ESP-IDF device
Created: 2026-04-22
"""

import os
import time
import select
import fcntl
import termios
import struct
import sys

BAUD = 115200
PORT = "/dev/ttyUSB0"
TIMEOUT = 10

def read_serial(port=PORT, baud=BAUD, timeout=TIMEOUT):
    fd = os.open(port, os.O_RDWR | os.O_NOCTTY)
    
    # Set baud rate
    attrs = termios.tcgetattr(fd)
    attrs[4] = getattr(termios, f"B{baud}")
    attrs[5] = getattr(termios, f"B{baud}")
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    
    # Reset device via DTR/RTS
    for attr in [termios.TIOCM_DTR, termios.TIOCM_RTS]:
        fcntl.ioctl(fd, termios.TIOCMBIC, struct.pack('I', attr))
    time.sleep(0.1)
    for attr in [termios.TIOCM_DTR, termios.TIOCM_RTS]:
        fcntl.ioctl(fd, termios.TIOCMBIS, struct.pack('I', attr))
    time.sleep(0.5)
    
    start = time.time()
    buf = b''
    while time.time() - start < timeout:
        ready, _, _ = select.select([fd], [], [], 0.5)
        if ready:
            chunk = os.read(fd, 4096)
            if chunk:
                buf += chunk
    
    os.close(fd)
    return buf

if __name__ == "__main__":
    print(f"Reading from {PORT} at {BAUD} baud for {TIMEOUT} seconds...")
    data = read_serial()
    print(f"\nReceived {len(data)} bytes:")
    print(data.decode('utf-8', errors='replace'))
