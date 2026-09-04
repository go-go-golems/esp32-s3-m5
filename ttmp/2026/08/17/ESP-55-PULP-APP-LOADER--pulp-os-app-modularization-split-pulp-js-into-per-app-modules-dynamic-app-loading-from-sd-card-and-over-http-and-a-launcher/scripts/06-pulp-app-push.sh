#!/usr/bin/env bash
# ESP-55 P5: push an app module to a running PULP device and optionally
# hot-launch it. The developer inner loop:
#   06-pulp-app-push.sh tools/js/apps/dice.js [--host pulp.local] [--run]
# The file name (minus .js) is the app id; the device streams it to
# /sdcard/apps/<id>.js, rescans, and the launcher rebuilds. --run asks
# GET /apps/run?id=<id> (only honoured while the launcher is showing).
set -euo pipefail
HOST=pulp.local
RUN=0
FILE=""
while [ $# -gt 0 ]; do
  case "$1" in
    --host) HOST="$2"; shift 2 ;;
    --run) RUN=1; shift ;;
    *) FILE="$1"; shift ;;
  esac
done
[ -n "$FILE" ] || { echo "usage: $0 <app.js> [--host H] [--run]"; exit 2; }
ID="$(basename "$FILE" .js)"
echo "push $FILE -> http://$HOST/apps/upload?name=$ID"
curl -fsS -T "$FILE" "http://$HOST/apps/upload?name=$ID" && echo
if [ "$RUN" = 1 ]; then
  sleep 1
  curl -fsS "http://$HOST/apps/run?id=$ID" && echo
fi
