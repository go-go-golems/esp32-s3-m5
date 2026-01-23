#!/usr/bin/env python3
"""
Generate a 0066-specific MicroQuickJS stdlib header that adds a `sim` global
object for controlling the LED-chain simulator.

Why this exists:
- MicroQuickJS (mquickjs) is table-driven: to add new native-callable
  functions, we must regenerate the stdlib table so it contains entries for
  those functions.
- We keep generator inputs in one place (the upstream mqjs-repl generator),
  and patch it programmatically to avoid forking the whole generator tree.

Outputs:
- 0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/esp32_stdlib.h

This script intentionally does NOT overwrite the shared mquickjs_atom.h in the
imports tree. For 0066 we only need the stdlib header to include the additional
 `sim` entries; the predefined atom indices remain stable as long as we only
 append new atoms.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import subprocess
import sys
import tempfile


SIM_OBJ_BLOCK = r"""

// --- 0066 additions: LED-chain simulator control API ---
static const JSPropDef js_sim[] = {
    JS_CFUNC_DEF("status", 0, js_sim_status),
    JS_CFUNC_DEF("setFrameMs", 1, js_sim_setFrameMs),
    JS_CFUNC_DEF("setBrightness", 1, js_sim_setBrightness),
    JS_CFUNC_DEF("setPattern", 1, js_sim_setPattern),

    JS_CFUNC_DEF("setRainbow", 3, js_sim_setRainbow),
    JS_CFUNC_DEF("setChase", 9, js_sim_setChase),
    JS_CFUNC_DEF("setBreathing", 5, js_sim_setBreathing),
    JS_CFUNC_DEF("setSparkle", 6, js_sim_setSparkle),
    JS_PROP_END,
};

static const JSClassDef js_sim_obj =
    JS_OBJECT_DEF("sim", js_sim);
"""


def patch_mqjs_stdlib(src: str) -> str:
    # Insert js_sim definitions before the global object definition.
    marker = "static const JSPropDef js_global_object[] = {"
    if marker not in src:
        raise RuntimeError("mqjs_stdlib.c patch point not found (js_global_object)")

    if "js_sim_obj" in src:
        raise RuntimeError("mqjs_stdlib.c already appears to include sim additions")

    parts = src.split(marker, 1)
    head = parts[0]
    tail = marker + parts[1]

    # Add sim object block near other object defs.
    patched = head + SIM_OBJ_BLOCK + "\n" + tail

    # Add global property entry.
    # Place it near other "other objects" (console/performance/gpio/i2c).
    prop_re = re.compile(r'(\s*JS_PROP_CLASS_DEF\("performance",\s*&js_performance_obj\),\s*\n)')
    m = prop_re.search(patched)
    if not m:
        raise RuntimeError("mqjs_stdlib.c patch point not found (performance object in global)")

    insert = m.group(1) + '    JS_PROP_CLASS_DEF("sim", &js_sim_obj),\n'
    patched = patched[: m.start(1)] + insert + patched[m.end(1) :]

    return patched


def run(cmd: list[str]) -> None:
    subprocess.check_call(cmd)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo-root", required=True)
    args = ap.parse_args()

    repo_root = pathlib.Path(args.repo_root).resolve()
    src_mqjs_stdlib = repo_root / "imports/esp32-mqjs-repl/mqjs-repl/tools/esp_stdlib_gen/mqjs_stdlib.c"
    mquickjs_dir = repo_root / "imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs"
    mquickjs_build_c = mquickjs_dir / "mquickjs_build.c"

    out_header = repo_root / "0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/esp32_stdlib.h"

    if not src_mqjs_stdlib.exists():
        print(f"error: missing source: {src_mqjs_stdlib}", file=sys.stderr)
        return 2
    if not mquickjs_build_c.exists():
        print(f"error: missing mquickjs_build.c: {mquickjs_build_c}", file=sys.stderr)
        return 2

    src = src_mqjs_stdlib.read_text(encoding="utf-8")
    patched = patch_mqjs_stdlib(src)

    out_header.parent.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="mqjs_stdlib_0066_") as td:
        td_path = pathlib.Path(td)
        patched_c = td_path / "mqjs_stdlib_0066.c"
        gen_bin = td_path / "esp_stdlib_gen_0066"

        patched_c.write_text(patched, encoding="utf-8")

        # Build generator (host tool).
        run(
            [
                "cc",
                "-O2",
                "-std=c99",
                "-D_POSIX_C_SOURCE=200809L",
                "-I",
                str(mquickjs_dir),
                str(patched_c),
                str(mquickjs_build_c),
                "-o",
                str(gen_bin),
            ]
        )

        # Emit 32-bit stdlib header.
        header = subprocess.check_output([str(gen_bin), "-m32"], text=True)
        if "static const uint32_t" not in header:
            print("error: generator output did not look like a 32-bit stdlib header", file=sys.stderr)
            return 2
        out_header.write_text(header, encoding="utf-8")

    print(f"wrote {out_header}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
