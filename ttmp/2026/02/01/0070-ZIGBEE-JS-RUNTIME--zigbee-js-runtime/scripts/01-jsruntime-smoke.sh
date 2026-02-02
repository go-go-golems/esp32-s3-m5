#!/usr/bin/env bash
set -euo pipefail

# Smoke test for zigctl JS runtime.
# Assumes Zigbee2MQTT broker is running at mqtt://localhost:1884.

ROOT="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5"

cd "$ROOT/zigctl"

# Run a join/watch script (adjust broker/baseTopic in the JS file if needed).
go run ./ js run "$ROOT/zigctl/testdata/jsruntime/join-watch.js"
