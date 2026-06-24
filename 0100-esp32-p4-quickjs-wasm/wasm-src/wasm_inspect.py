#!/usr/bin/env python3
"""Minimal WebAssembly section parser (dependency-free; replaces wabt/wasm-objdump).

Lists a .wasm module's imports and exports so we can verify that quickjs.wasm:
  - imports env.host_print / env.host_millis / env.host_gpio_write  (WAMR natives)
  - imports wasi_snapshot_preview1.*  (libc, satisfied by WAMR WASI)
  - exports qjs_init / qjs_eval        (the entry points WAMR will call)

Usage: wasm_inspect.py <file.wasm>
"""
import struct, sys

def read_leb(p, i):
    r = s = 0
    while True:
        b = p[i]; i += 1
        r |= (b & 0x7f) << s
        if not (b & 0x80): break
        s += 7
    return r, i

def read_name(p, i):
    n, i = read_leb(p, i)
    s = p[i:i+n].decode('utf-8', 'replace'); i += n
    return s, i

KIND = {0:'func',1:'table',2:'mem',3:'global'}
EKIND = {0:'func',1:'table',2:'mem',3:'global',4:'tag'}

def skip_limits(p, i):
    flags = p[i]; i += 1
    _, i = read_leb(p, i)            # min
    if flags & 1:
        _, i = read_leb(p, i)        # max
    return i

def main(path):
    data = open(path,'rb').read()
    assert data[:4] == b'\x00asm', "not a wasm magic"
    ver = struct.unpack('<I', data[4:8])[0]
    print(f"wasm version {ver}, {len(data)} bytes")
    i = 8
    imports, exports = [], []
    while i < len(data):
        sid = data[i]; i += 1
        slen, i = read_leb(data, i)
        end = i + slen
        if sid == 2:                      # import section
            count, i = read_leb(data, i)
            for _ in range(count):
                mod, i = read_name(data, i)
                fld, i = read_name(data, i)
                k = data[i]; i += 1
                if k == 0:                # func -> type index
                    _, i = read_leb(data, i)
                elif k == 1:              # table -> reftype + limits
                    i += 1; i = skip_limits(data, i)
                elif k == 2:              # mem -> limits
                    i = skip_limits(data, i)
                elif k == 3:              # global -> valtype + mut
                    i += 2
                imports.append((mod, fld, KIND.get(k, k)))
        elif sid == 7:                    # export section
            count, i = read_leb(data, i)
            for _ in range(count):
                nm, i = read_name(data, i)
                k = data[i]; i += 1
                idx, i = read_leb(data, i)
                exports.append((nm, EKIND.get(k, k), idx))
        i = end
    print(f"\n== imports ({len(imports)}) ==")
    for mod, fld, k in imports: print(f"  {k:7} {mod}.{fld}")
    print(f"\n== exports ({len(exports)}) ==")
    for nm, k, idx in exports: print(f"  {k:7} {nm}  (idx {idx})")

if __name__ == '__main__':
    main(sys.argv[1])
