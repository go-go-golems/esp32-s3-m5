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

promise_out=$(
  "$BIN" "$HOST_DIR/examples/async-routes.js" --dispatch /api/promise
)
printf '%s\n' "$promise_out"
[[ "$promise_out" == *"routes=3 mounts=0"* ]]
[[ "$promise_out" == *"DISPATCH status=200 content-type=application/json; charset=utf-8"* ]]
[[ "$promise_out" == *'"kind":"promise"'* ]]
[[ "$promise_out" == *'"path":"/api/promise"'* ]]

async_out=$(
  "$BIN" "$HOST_DIR/examples/async-routes.js" --dispatch /api/async
)
printf '%s\n' "$async_out"
[[ "$async_out" == *"DISPATCH status=200 content-type=application/json; charset=utf-8"* ]]
[[ "$async_out" == *'"kind":"async-value"'* ]]
[[ "$async_out" == *'"path":"/api/async"'* ]]

reject_out=$(
  "$BIN" "$HOST_DIR/examples/async-routes.js" --dispatch /api/reject
)
printf '%s\n' "$reject_out"
[[ "$reject_out" == *"DISPATCH status=500 content-type=text/plain; charset=utf-8"* ]]
[[ "$reject_out" == *"route promise rejected: Error: route boom"* ]]

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

fake_async_fetch_out=$(
  "$BIN" "$HOST_DIR/examples/fetch.js" --fake-async-fetch
)
printf '%s\n' "$fake_async_fetch_out"
[[ "$fake_async_fetch_out" == *"fetch status=200 ok=true"* ]]
[[ "$fake_async_fetch_out" == *"fetch body=ok"* ]]

echo "PASS native-http host smoke"
