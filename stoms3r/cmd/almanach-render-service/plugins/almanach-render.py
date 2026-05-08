#!/usr/bin/env python3
"""
Almanach Render Service — devctl plugin

Manages the Go render server + Chrome headless for almanac generation.

Services:
  - render: the Go HTTP server (almanach-render-service)
  - chrome: headless Chrome (chromedp allocator, managed by the Go server)

Commands:
  - devctl print      — render and print an almanac page
  - devctl render     — render an almanac page to PNG
  - devctl health     — check service health
"""

import json
import os
import shutil
import subprocess
import sys
import time


# ── helpers ──────────────────────────────────────────────────────────────────

def emit(obj):
    sys.stdout.write(json.dumps(obj) + "\n")
    sys.stdout.flush()

def log(msg):
    sys.stderr.write("[almanach-render] " + msg + "\n")
    sys.stderr.flush()

def run(cmd, **kw):
    """Run a command, return CompletedProcess."""
    log(f"run: {' '.join(cmd)}")
    return subprocess.run(cmd, capture_output=True, text=True, **kw)


# ── config ─────────────────────────────────────────────────────────────────

PLUGIN_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_DIR = os.path.abspath(os.path.join(PLUGIN_DIR, ".."))

DEFAULT_PORT = 8199
DEFAULT_PRINTER_IP = ""  # must be set by user


# ── handshake ───────────────────────────────────────────────────────────────

emit({
    "type": "handshake",
    "protocol_version": "v2",
    "plugin_name": "almanach-render",
    "capabilities": {
        "ops": ["config.mutate", "validate.run", "build.run", "launch.plan", "command.run"],
        "commands": [
            {"name": "print", "help": "Render and print an almanac page to the thermal printer"},
            {"name": "render", "help": "Render an almanac page to a PNG file"},
            {"name": "health", "help": "Check render service health"},
        ],
    },
})


# ── main loop ───────────────────────────────────────────────────────────────

for line in sys.stdin:
    line = line.strip()
    if not line:
        continue

    req = json.loads(line)
    rid = req.get("request_id", "")
    op = req.get("op", "")
    ctx = req.get("ctx", {}) or {}
    inp = req.get("input", {}) or {}

    dry_run = bool(ctx.get("dry_run", False))
    repo_root = ctx.get("repo_root", "") or REPO_DIR

    try:
        # ── config.mutate ──────────────────────────────────────────────
        if op == "config.mutate":
            port = os.environ.get("ALMANACH_PORT", str(DEFAULT_PORT))
            printer_ip = os.environ.get("ALMANACH_PRINTER_IP", DEFAULT_PRINTER_IP)

            emit({
                "type": "response",
                "request_id": rid,
                "ok": True,
                "output": {
                    "config_patch": {
                        "set": {
                            "services.render.port": int(port),
                            "services.render.url": f"http://127.0.0.1:{port}",
                            "env.ALMANACH_PORT": port,
                            "env.ALMANACH_PRINTER_IP": printer_ip,
                        },
                        "unset": [],
                    }
                },
            })

        # ── validate.run ──────────────────────────────────────────────
        elif op == "validate.run":
            errors = []
            warnings = []

            # Check Go toolchain
            if not shutil.which("go"):
                errors.append({
                    "code": "E_MISSING_TOOL",
                    "message": "Go toolchain not found (go not in PATH)",
                })

            # Check Chrome/Chromium
            chrome_path = os.environ.get("ALMANACH_CHROME_PATH", "")
            if not chrome_path:
                for candidate in ["chromium-browser", "chromium", "google-chrome", "chrome"]:
                    if shutil.which(candidate):
                        chrome_path = candidate
                        break
            if not chrome_path:
                warnings.append({
                    "code": "W_NO_CHROME",
                    "message": "Chrome/Chromium not found. Set ALMANACH_CHROME_PATH or install chromium-browser.",
                })

            # Check printer IP
            if not os.environ.get("ALMANACH_PRINTER_IP"):
                warnings.append({
                    "code": "W_NO_PRINTER_IP",
                    "message": "ALMANACH_PRINTER_IP not set. Print commands will fail.",
                })

            emit({
                "type": "response",
                "request_id": rid,
                "ok": True,
                "output": {
                    "valid": len(errors) == 0,
                    "errors": errors,
                    "warnings": warnings,
                },
            })

        # ── build.run ────────────────────────────────────────────────
        elif op == "build.run":
            steps = []

            # Step 1: Build the Go binary
            steps.append({
                "name": "build-go-binary",
                "command": ["go", "build", "-o", "almanach-render-service", "."],
                "cwd": repo_root,
            })

            # Step 2: Build the SPA bundle (if npm is available)
            web_dir = os.path.join(repo_root, "web", "almanach")
            if os.path.isdir(web_dir) and shutil.which("npm"):
                steps.append({
                    "name": "build-spa-bundle",
                    "command": ["bash", "-lc", f"cd {web_dir} && npm install && npm run build"],
                    "cwd": web_dir,
                })

            emit({
                "type": "response",
                "request_id": rid,
                "ok": True,
                "output": {"steps": steps},
            })

        # ── launch.plan ───────────────────────────────────────────────
        elif op == "launch.plan":
            port = os.environ.get("ALMANACH_PORT", str(DEFAULT_PORT))
            chrome_path = os.environ.get("ALMANACH_CHROME_PATH", "")
            if not chrome_path:
                for candidate in ["chromium-browser", "chromium", "google-chrome", "chrome"]:
                    if shutil.which(candidate):
                        chrome_path = candidate
                        break

            web_dir = os.path.join(repo_root, "web", "almanach", "dist")
            binary = os.path.join(repo_root, "almanach-render-service")

            env = {
                "ALMANACH_PORT": port,
                "ALMANACH_WEB_DIR": web_dir,
                "ALMANACH_PRINTER_IP": os.environ.get("ALMANACH_PRINTER_IP", ""),
            }
            if chrome_path:
                env["ALMANACH_CHROME_PATH"] = chrome_path

            services = [
                {
                    "name": "render",
                    "cwd": repo_root,
                    "command": [binary, "serve"],
                    "env": env,
                    "health": {
                        "type": "http",
                        "url": f"http://127.0.0.1:{port}/health",
                        "timeout_ms": 15000,
                    },
                },
            ]

            emit({
                "type": "response",
                "request_id": rid,
                "ok": True,
                "output": {"services": services},
            })

        # ── command.run ───────────────────────────────────────────────
        elif op == "command.run":
            name = inp.get("name", "")
            argv = inp.get("argv", [])
            port = os.environ.get("ALMANACH_PORT", str(DEFAULT_PORT))
            base_url = f"http://127.0.0.1:{port}"
            binary = os.path.join(repo_root, "almanach-render-service")
            web_dir = os.path.join(repo_root, "web", "almanach", "dist")
            printer_ip = os.environ.get("ALMANACH_PRINTER_IP", DEFAULT_PRINTER_IP)

            if name == "health":
                r = run(["curl", "-s", f"{base_url}/health"])
                ok = r.returncode == 0 and '"ok":true' in r.stdout
                log(f"health: {r.stdout.strip()}")
                if not ok:
                    log(f"health check failed (exit {r.returncode})")
                emit({
                    "type": "response",
                    "request_id": rid,
                    "ok": True,
                    "output": {"exit_code": 0 if ok else 1},
                })

            elif name == "render":
                out_path = "/tmp/almanach-render.png"
                layout_path = ""
                if argv:
                    if argv[0].endswith((".json", ".yaml", ".yml")):
                        layout_path = argv[0]
                        if len(argv) > 1:
                            out_path = argv[1]
                    else:
                        out_path = argv[0]
                        if len(argv) > 1:
                            layout_path = argv[1]
                cmd = [binary, "render", "--out", out_path, "--web-dir", web_dir]
                if layout_path:
                    cmd.extend(["--layout", layout_path])
                r = run(cmd)
                log(f"render: saved to {out_path} (exit {r.returncode})")
                emit({
                    "type": "response",
                    "request_id": rid,
                    "ok": True,
                    "output": {"exit_code": r.returncode, "artifacts": {"render-png": out_path}},
                })

            elif name == "print":
                layout_path = argv[0] if argv else ""
                cmd = [binary, "print", "--web-dir", web_dir]
                if printer_ip:
                    cmd.extend(["--printer-ip", printer_ip])
                if layout_path:
                    cmd.extend(["--layout", layout_path])
                r = run(cmd)
                log(f"print: {r.stdout.strip()}")
                emit({
                    "type": "response",
                    "request_id": rid,
                    "ok": True,
                    "output": {"exit_code": r.returncode},
                })

            else:
                emit({
                    "type": "response",
                    "request_id": rid,
                    "ok": False,
                    "error": {"code": "E_UNKNOWN_COMMAND", "message": f"unknown command: {name}"},
                })

        # ── unsupported ───────────────────────────────────────────────
        else:
            emit({
                "type": "response",
                "request_id": rid,
                "ok": False,
                "error": {"code": "E_UNSUPPORTED", "message": f"unsupported op: {op}"},
            })

    except Exception as e:
        log(f"error: {e}")
        emit({
            "type": "response",
            "request_id": rid,
            "ok": False,
            "error": {"code": "E_INTERNAL", "message": str(e)},
        })
