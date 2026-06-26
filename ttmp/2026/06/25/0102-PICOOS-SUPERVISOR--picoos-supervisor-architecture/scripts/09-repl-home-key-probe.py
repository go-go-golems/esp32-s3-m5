#!/usr/bin/env python3
"""Probe that the semantic Home key opens PicoOS Home from REPL surface."""
from __future__ import annotations
import argparse, os, select, sys, termios, time
DEFAULT_PORT='/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00'; PROMPT='0102>'
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
  repl=send(fd,'picoos repl',args.timeout)+send(fd,'picoos status',args.timeout)
  home=send(fd,'picoos key home',args.timeout)+send(fd,'picoos status',args.timeout)+send(fd,'picojs dump',args.timeout)
  checks=['surface=repl' in repl,'picoos key: ESP_OK token=home' in home,'surface=app active=home' in home,'PicoOS' in home,'Apps' in home]
  ok=all(checks); print('REPL_HOME_KEY_PROBE','PASS' if ok else 'FAIL',checks); return 0 if ok else 1
 finally:
  termios.tcsetattr(fd,termios.TCSANOW,old); os.close(fd)
if __name__=='__main__': raise SystemExit(main())
