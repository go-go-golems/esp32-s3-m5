#!/usr/bin/env bash
set -euo pipefail

echo "mled-server-related processes:"
ps aux | rg -n "(/|\\s)mled-server(\\s|$)|go run ./cmd/mled-server|tmux new-session -d -s mled-server|tmux: server" || true
