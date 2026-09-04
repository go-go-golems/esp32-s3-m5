#!/usr/bin/env python3
"""ESP-55 experiment 1: trial-split pulp.js by its section markers and
compile every section with the host pulpjsc to see per-app bytecode sizes.

The split is purely mechanical (each `// ---- name --` banner starts a
section); nothing is refactored. Sections compile standalone because the
bytecode compiler only needs the stdlib atoms, not the helpers (enter,
chrome, ...) that the section references at run time.

usage: 01-trial-split-bytecode-sizes.py [--out DIR]
Prints a markdown table: section, source bytes, bytecode bytes, ratio.
"""
import argparse, os, re, subprocess, sys, tempfile

ROOT = "/home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0114-papers3-pulp-os"
PULP = os.path.join(ROOT, "tools/js/apps/pulp.js")
JSC = os.path.join(ROOT, "tools/js/host/pulpjsc")
BANNER = re.compile(r"^// -{4,}\s*(\S+)\s*-{2,}\s*$")

def split(src):
    sections, name, buf = [], "prelude", []
    for line in src.splitlines(keepends=True):
        m = BANNER.match(line.rstrip("\n"))
        if m:
            sections.append((name, "".join(buf)))
            name, buf = m.group(1), [line]
        else:
            buf.append(line)
    sections.append((name, "".join(buf)))
    return sections

def bc_size(header_path):
    with open(header_path) as f:
        return len(re.findall(r"0x[0-9a-fA-F]{2}", f.read()))

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=None)
    a = ap.parse_args()
    out = a.out or tempfile.mkdtemp(prefix="esp55-split-")
    os.makedirs(out, exist_ok=True)
    src = open(PULP).read()
    rows, tot_s, tot_b = [], 0, 0
    for name, body in split(src):
        js = os.path.join(out, f"{name}.js")
        hdr = os.path.join(out, f"js_{name}.h")
        open(js, "w").write(body)
        r = subprocess.run([JSC, js, hdr, f"kJsBytecode_{name}"],
                           capture_output=True, text=True)
        if r.returncode != 0:
            print(f"{name}: compile FAILED\n{r.stderr}", file=sys.stderr)
            continue
        b = bc_size(hdr); s = len(body.encode())
        tot_s += s; tot_b += b
        rows.append((name, s, b))
    print("| section | source bytes | bytecode bytes | bc/src |")
    print("|---|---:|---:|---:|")
    for n, s, b in rows:
        print(f"| {n} | {s} | {b} | {b/s:.2f} |")
    print(f"| **sum of sections** | {tot_s} | {tot_b} | {tot_b/tot_s:.2f} |")
    # Whole-file baseline for comparison.
    hdr = os.path.join(out, "js_pulp_all.h")
    subprocess.run([JSC, PULP, hdr, "kJsBytecode_all"], check=True,
                   capture_output=True)
    print(f"| whole pulp.js (one image) | {len(src.encode())} | {bc_size(hdr)} | "
          f"{bc_size(hdr)/len(src.encode()):.2f} |")
    print(f"\nsplit files written to {out}")

if __name__ == "__main__":
    main()
