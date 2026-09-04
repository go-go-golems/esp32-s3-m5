#!/usr/bin/env python3
"""PULP app-index server (ESP-58 P3).

Serves a directory of .js app modules over HTTP and advertises itself on
the local network as _pulp-apps._tcp so a PULP device can discover it
with mdns.browse() instead of a hand-typed URL.

Contract (ESP-58 guide section 5):
  - mDNS service type _pulp-apps._tcp, TXT records:
      path=<index path>   (default /pulp/index.json)
      name=<human label>  (shown in the store's server list)
  - GET /pulp/index.json -> {"v":1,"name":...,"apps":[{id,title,subtitle,url}]}
  - GET /apps/<id>.js    -> module source
  - ids must match [a-z0-9_-]{1,24}; titles/subtitles plain ASCII with no
    &<>\" (the device-side upload/manifest path never urldecodes — the
    ESP-57 encoding lesson, enforced here at startup, fail-fast).

Metadata: for each <id>.js an optional sidecar <id>.json may carry
{"title": ..., "subtitle": ...}; otherwise title falls back to the id.
The PULP demo suite's descriptor headers are NOT parsed — sidecars keep
this script dumb and the contract explicit.

Usage:
  ./01-app-index-server.py --dir ../../../../0114-papers3-pulp-os/tools/js/demos \
      --port 8123 --name "Demo Shelf"

Requires: python3-zeroconf (pip install zeroconf). Ctrl-C to stop; the
service is withdrawn on exit so no stale record lingers.
"""

import argparse
import json
import os
import re
import socket
import sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

from zeroconf import ServiceInfo, Zeroconf

ID_RE = re.compile(r"^[a-z0-9_-]{1,24}$")
BAD_CHARS = set('&<>"')


def plain_ascii(s: str) -> bool:
    return all(32 <= ord(c) < 127 and c not in BAD_CHARS for c in s)


def local_ip() -> str:
    # UDP connect trick: no packet is sent; the kernel picks the outbound
    # interface address, which is what LAN peers must dial.
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("192.0.2.1", 9))  # TEST-NET-1, never routed
        return s.getsockname()[0]
    finally:
        s.close()


def build_index(directory: str, name: str, base_url: str):
    """Scans the directory once per request: drop a new .js in, it shows up."""
    apps = []
    for fn in sorted(os.listdir(directory)):
        if not fn.endswith(".js"):
            continue
        app_id = fn[:-3]
        if not ID_RE.match(app_id):
            print(f"skip {fn}: id must match [a-z0-9_-]{{1,24}}", file=sys.stderr)
            continue
        title, subtitle = app_id, ""
        sidecar = os.path.join(directory, app_id + ".json")
        if os.path.exists(sidecar):
            try:
                with open(sidecar) as f:
                    meta = json.load(f)
                title = str(meta.get("title", title))
                subtitle = str(meta.get("subtitle", subtitle))
            except (json.JSONDecodeError, OSError) as e:
                print(f"skip sidecar {sidecar}: {e}", file=sys.stderr)
        for label, value in (("title", title), ("subtitle", subtitle)):
            if not plain_ascii(value):
                raise SystemExit(
                    f"{app_id}: {label} {value!r} is not plain ASCII "
                    f"(no &<>\") — fix the sidecar; the device never urldecodes")
        apps.append({
            "id": app_id,
            "title": title,
            "subtitle": subtitle,
            "url": f"{base_url}/apps/{fn}",
        })
    return {"v": 1, "name": name, "apps": apps}


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--dir", required=True, help="directory of .js modules")
    ap.add_argument("--port", type=int, default=8123)
    ap.add_argument("--name", default="App Shelf", help="server label (TXT name)")
    ap.add_argument("--index-path", default="/pulp/index.json")
    ap.add_argument("--no-advertise", action="store_true",
                    help="HTTP only (contract tests without mDNS)")
    args = ap.parse_args()

    directory = os.path.abspath(args.dir)
    if not os.path.isdir(directory):
        raise SystemExit(f"not a directory: {directory}")
    if not plain_ascii(args.name):
        raise SystemExit("--name must be plain ASCII with no &<>\"")

    ip = local_ip()
    base_url = f"http://{ip}:{args.port}"
    # Fail fast on bad metadata before advertising anything.
    index = build_index(directory, args.name, base_url)
    print(f"serving {len(index['apps'])} app(s) from {directory}")
    print(f"index: {base_url}{args.index_path}")

    class Handler(BaseHTTPRequestHandler):
        def do_GET(self):
            if self.path == args.index_path:
                body = json.dumps(
                    build_index(directory, args.name, base_url)).encode()
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)
                return
            m = re.match(r"^/apps/([a-z0-9_-]{1,24})\.js$", self.path)
            if m and os.path.exists(os.path.join(directory, m.group(1) + ".js")):
                with open(os.path.join(directory, m.group(1) + ".js"), "rb") as f:
                    body = f.read()
                self.send_response(200)
                self.send_header("Content-Type", "application/javascript")
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)
                return
            self.send_response(404)
            self.end_headers()

        def log_message(self, fmt, *a):
            print(f"http: {self.address_string()} {fmt % a}")

    httpd = ThreadingHTTPServer(("", args.port), Handler)

    zc = None
    if not args.no_advertise:
        # Bind only the outbound interface: joining the mDNS multicast
        # group on every interface can hit the kernel's
        # igmp_max_memberships cap (ENOBUFS) on hosts with many virtual
        # interfaces (docker etc).
        zc = Zeroconf(interfaces=[ip])
        info = ServiceInfo(
            "_pulp-apps._tcp.local.",
            f"{args.name}._pulp-apps._tcp.local.",
            addresses=[socket.inet_aton(ip)],
            port=args.port,
            properties={"path": args.index_path, "name": args.name},
        )
        zc.register_service(info)
        print(f"advertising _pulp-apps._tcp \"{args.name}\" at {ip}:{args.port}")

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        if zc is not None:
            zc.unregister_service(info)
            zc.close()
            print("service withdrawn")


if __name__ == "__main__":
    main()
