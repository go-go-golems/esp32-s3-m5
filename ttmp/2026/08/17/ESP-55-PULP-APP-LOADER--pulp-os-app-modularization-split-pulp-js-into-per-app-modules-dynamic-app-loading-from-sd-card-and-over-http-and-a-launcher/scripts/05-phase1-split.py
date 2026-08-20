#!/usr/bin/env python3
"""ESP-55 Phase 1: one-off mechanical split of tools/js/apps/pulp.js into
tools/js/os/*.js (kernel, launcher, boot) + tools/js/apps/<id>.js (verbatim
sections, cut at the `// ---- name --` banners). No code is changed; the
build re-concatenates everything into one image (build_bytecode_apps.sh).
"""
import os, re, sys

ROOT = "/home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0114-papers3-pulp-os"
PULP = os.path.join(ROOT, "tools/js/apps/pulp.js")
BANNER = re.compile(r"^// -{4,}\s*(\S+)\s*-{2,}\s*$")
DEST = {
    "prelude": "os/00-kernel.js", "home": "os/40-launcher.js",
    "boot": "os/90-boot.js",
}  # everything else -> apps/<name>.js

def main():
    src = open(PULP).read()
    sections, name, buf = {}, "prelude", []
    for line in src.splitlines(keepends=True):
        m = BANNER.match(line.rstrip("\n"))
        if m:
            sections[name] = "".join(buf)
            name, buf = m.group(1), [line]
        else:
            buf.append(line)
    sections[name] = "".join(buf)
    os.makedirs(os.path.join(ROOT, "tools/js/os"), exist_ok=True)
    for name, body in sections.items():
        rel = DEST.get(name, f"apps/{name}.js")
        path = os.path.join(ROOT, "tools/js", rel)
        open(path, "w").write(body)
        print(f"{rel}: {len(body)} bytes")
    os.remove(PULP)
    print("removed tools/js/apps/pulp.js")

if __name__ == "__main__":
    main()
