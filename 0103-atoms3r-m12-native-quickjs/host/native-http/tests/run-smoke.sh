#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
HOST_DIR=$(cd "$SCRIPT_DIR/.." && pwd)
ROOT=$(cd "$HOST_DIR/../../.." && pwd)
BIN="$HOST_DIR/build/qjs-http-host"

make -C "$HOST_DIR" all >/tmp/qjs-http-host-build.log

route_out=$(
  "$BIN" "$HOST_DIR/examples/server.js" --dispatch /api/hello
)
printf '%s\n' "$route_out"
[[ "$route_out" == *"routes=1 mounts=1"* ]]
[[ "$route_out" == *"DISPATCH status=200 content-type=application/json; charset=utf-8"* ]]
[[ "$route_out" == *'"ok":true'* ]]
[[ "$route_out" == *'"path":"/api/hello"'* ]]

python3 - <<'PY' &
from http.server import BaseHTTPRequestHandler, HTTPServer
class H(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/healthz':
            body = b'ok\n'
            self.send_response(200)
            self.send_header('content-type', 'text/plain; charset=utf-8')
            self.send_header('content-length', str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        else:
            self.send_response(404)
            self.end_headers()
    def log_message(self, fmt, *args):
        pass
HTTPServer(('127.0.0.1', 18080), H).serve_forever()
PY
server_pid=$!
trap 'kill "$server_pid" 2>/dev/null || true' EXIT
sleep 0.3

fetch_out=$(
  "$BIN" "$HOST_DIR/examples/fetch.js"
)
printf '%s\n' "$fetch_out"
[[ "$fetch_out" == *"fetch status=200 ok=true"* ]]
[[ "$fetch_out" == *"fetch body=ok"* ]]

echo "PASS native-http host smoke"
