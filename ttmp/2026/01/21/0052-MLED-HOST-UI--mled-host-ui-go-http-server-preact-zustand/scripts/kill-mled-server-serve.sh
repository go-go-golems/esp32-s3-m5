#!/usr/bin/env bash
set -euo pipefail

# Kills any running "mled-server serve" processes (including ones started via `go run`).
# Prefer `tmux-stop-mled-server.sh` if you're managing the server via a tmux session.

gather_pids() {
  (pgrep -f "go run ./cmd/mled-server serve" || true
   pgrep -f "mled-server serve" || true) | sort -n | uniq
}

pids="$(gather_pids | tr '\n' ' ')"
if [[ -z "${pids// /}" ]]; then
  echo "No mled-server serve processes found."
  exit 0
fi

echo "Killing mled-server serve pids: $pids"

for pid in $pids; do
  if [[ "$pid" == "$$" ]]; then
    continue
  fi
  kill -TERM "$pid" 2>/dev/null || true
done

sleep 0.7

remaining="$(gather_pids | tr '\n' ' ')"
if [[ -z "${remaining// /}" ]]; then
  echo "Stopped."
  exit 0
fi

echo "Still running after TERM, sending KILL: $remaining"
for pid in $remaining; do
  if [[ "$pid" == "$$" ]]; then
    continue
  fi
  kill -KILL "$pid" 2>/dev/null || true
done

echo "Done."
