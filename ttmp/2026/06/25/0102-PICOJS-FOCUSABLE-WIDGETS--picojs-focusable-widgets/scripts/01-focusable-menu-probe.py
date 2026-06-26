#!/usr/bin/env python3
"""Probe focusable PicoJS menu movement and onPick launching."""
from __future__ import annotations
import argparse, os, select, sys, termios, time
DEFAULT_PORT='/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00'; PROMPT='0102>'; BAUDS={115200:termios.B115200}
def cfg(fd):
 old=termios.tcgetattr(fd); a=termios.tcgetattr(fd); a[0]=0; a[1]=0; a[2]=termios.CS8|termios.CREAD|termios.CLOCAL; a[3]=0; a[4]=a[5]=termios.B115200; a[6][termios.VMIN]=0; a[6][termios.VTIME]=1; termios.tcsetattr(fd,termios.TCSANOW,a); return old
def read(fd,t):
 end=time.monotonic()+t; b=bytearray()
 while time.monotonic()<end:
  r,_,_=select.select([fd],[],[],0.2)
  if r:
   b.extend(os.read(fd,4096))
   if PROMPT.encode() in b: break
 return b.decode('utf-8','replace')
def send(fd,c,t):
 os.write(fd,(c+'\n').encode()); out=read(fd,t); print('---',c); print(out.rstrip()); return out
def main():
 ap=argparse.ArgumentParser(); ap.add_argument('--port',default=DEFAULT_PORT); ap.add_argument('--timeout',type=float,default=8); args=ap.parse_args()
 if '/dev/tty' in args.port: print('use by-id port',file=sys.stderr); return 2
 fd=os.open(args.port,os.O_RDWR|os.O_NOCTTY|os.O_NONBLOCK); old=cfg(fd)
 try:
  os.write(fd,b'\n'); boot=read(fd,10)
  if PROMPT not in boot: print(boot); return 1
  launch=send(fd,'picoos launcher',args.timeout); home0=send(fd,'picojs dump',args.timeout)
  right=send(fd,'picoos key right',args.timeout); home1=send(fd,'picojs dump',args.timeout)
  enter=send(fd,'picoos key enter',args.timeout); st1=send(fd,'picoos status',args.timeout); hello=send(fd,'picojs dump',args.timeout)
  send(fd,'picoos launcher',args.timeout); send(fd,'picoos key down',args.timeout); send(fd,'picoos key down',args.timeout); enter2=send(fd,'picoos key enter',args.timeout); st2=send(fd,'picoos status',args.timeout)
  checks=['select repl' in home0,'select hello' in home1,'picoos key: ESP_OK token=enter' in enter,'active=hello' in st1,'HELLO DEVICE' in hello,'picoos key: ESP_OK token=enter' in enter2,'active=calc' in st2]
  ok=all(checks); print('FOCUSABLE_MENU_PROBE','PASS' if ok else 'FAIL',checks); return 0 if ok else 1
 finally:
  termios.tcsetattr(fd,termios.TCSANOW,old); os.close(fd)
if __name__=='__main__': raise SystemExit(main())
