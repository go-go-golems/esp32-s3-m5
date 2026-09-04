#!/usr/bin/env bash
# ESP-57: install the whole demo suite over HTTP. Every demo except the
# index installs hidden (the Demos app fronts them); titles/subtitles ride
# the upload query and land in the device-written manifest.
#   02-push-demos.sh [--host pulp.local]
set -euo pipefail
HOST=pulp.local
[ "${1:-}" = "--host" ] && HOST="$2"
DIR="$(cd "$(dirname "$0")/../../../../../../0114-papers3-pulp-os/tools/js/demos" && pwd)"
# NOTE: the upload route sanitizes but does NOT urldecode; keep titles
# to [A-Za-z0-9 _.-] (no & or %).
urlenc() {
  # percent-encode everything a query value needs (& especially)
  /usr/bin/python3 -c 'import sys,urllib.parse;print(urllib.parse.quote_plus(sys.argv[1]))' "$1"
}
push() {
  local id="$1" title="$2" sub="$3" hidden="$4"
  local q="name=$id&title=$(urlenc "$title")&subtitle=$(urlenc "$sub")"
  [ "$hidden" = 1 ] && q="$q&hidden=1"
  curl -fsS -m 15 -T "$DIR/$id.js" "http://$HOST/apps/upload?$q" >/dev/null
  echo "pushed $id"
  sleep 1
}
push demos     "Demos"          "the JS API, live"             0
push d-widgets "Type and Widgets" "the builder specimen"         1
push d-canvas  "Canvas"         "line disc ring box paint"     1
push d-touch   "Touch Lab"      "gestures and hit regions"     1
push d-ticker  "Ticker"         "every ms and dyn text"        1
push d-storage "Files and Store"  "the async files chain"        1
push d-net     "Network"        "wifi and http.get"            1
push d-serve   "Web Server"     "JS routes from an app"        1
push d-sound   "Sound"          "tone beep melody"             1
push d-power   "Power and mDNS"   "battery and pulp.local"       1
push d-books   "Books"          "the reader API"               1
push d-sysinfo "System"         "abi clocks catalog"           1
echo "suite installed - open Demos on the launcher"
