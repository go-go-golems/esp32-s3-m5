#!/usr/bin/env bash
set -euo pipefail

SERVER_URL="${SERVER_URL:-http://localhost:18765}"
TIMEOUT_MS="${TIMEOUT_MS:-5000}"

REPO_ROOT="/home/manuel/workspaces/2025-12-21/echo-base-documentation"
MLED_SERVER_DIR="$REPO_ROOT/esp32-s3-m5/mled-server"

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing required command: $1" >&2
    exit 1
  fi
}

require_cmd jq

cd "$MLED_SERVER_DIR"

echo "Testing against: $SERVER_URL"

json_or_empty_array() {
  local in
  in="$(cat)"
  if [[ -z "${in//[[:space:]]/}" ]]; then
    echo "[]"
  else
    echo "$in"
  fi
}

smoke_sse_events() {
  # SSE is a streaming endpoint; curl will commonly exit non-zero due to --max-time.
  # Treat that as OK as long as we got a 200 and the initial ": ok" line.
  local out
  out="$(curl -i -N "$SERVER_URL/api/events" --max-time 1 2>/dev/null || true)"
  echo "$out" | grep -q "HTTP/1.1 200" || { echo "SSE /api/events did not return 200" >&2; return 1; }
  echo "$out" | grep -q "^: ok" || { echo "SSE /api/events did not include initial ': ok'" >&2; return 1; }
}

# Basic read-only verbs
go run ./cmd/mled-server status --server "$SERVER_URL" --timeout-ms "$TIMEOUT_MS" --output json \
  | jq -e '.[0].running == true' >/dev/null

smoke_sse_events

go run ./cmd/mled-server nodes --server "$SERVER_URL" --timeout-ms "$TIMEOUT_MS" --output json \
  | json_or_empty_array \
  | jq -e 'type=="array"' >/dev/null

go run ./cmd/mled-server presets --server "$SERVER_URL" --timeout-ms "$TIMEOUT_MS" --output json \
  | json_or_empty_array \
  | jq -e 'type=="array"' >/dev/null

go run ./cmd/mled-server preset list --server "$SERVER_URL" --timeout-ms "$TIMEOUT_MS" --output json \
  | json_or_empty_array \
  | jq -e 'type=="array"' >/dev/null

go run ./cmd/mled-server settings get --server "$SERVER_URL" --timeout-ms "$TIMEOUT_MS" --output json \
  | jq -e '.[0].multicast_group != null' >/dev/null

# Preset CRUD (create/update/delete)
PRESET_JSON="$(go run ./cmd/mled-server preset add \
  --server "$SERVER_URL" --timeout-ms "$TIMEOUT_MS" \
  --name "TestPreset" --icon "✨" \
  --type solid --param color=#112233 --brightness 55 \
  --output json)"

PRESET_ID="$(echo "$PRESET_JSON" | jq -r '.[0].id')"
if [[ -z "$PRESET_ID" || "$PRESET_ID" == "null" ]]; then
  echo "Failed to create preset (no id returned)" >&2
  echo "$PRESET_JSON" >&2
  exit 1
fi

go run ./cmd/mled-server preset update \
  --server "$SERVER_URL" --timeout-ms "$TIMEOUT_MS" \
  --id "$PRESET_ID" --name "TestPresetUpdated" \
  --output json \
  | jq -e '.[0].name=="TestPresetUpdated"' >/dev/null

go run ./cmd/mled-server preset delete \
  --server "$SERVER_URL" --timeout-ms "$TIMEOUT_MS" \
  --id "$PRESET_ID" \
  --output json \
  | jq -e '.[0].ok==true' >/dev/null

go run ./cmd/mled-server preset list --server "$SERVER_URL" --timeout-ms "$TIMEOUT_MS" --output json \
  | json_or_empty_array \
  | jq -e --arg id "$PRESET_ID" 'map(select(.id==$id)) | length == 0' >/dev/null

# Apply verbs (should succeed even if no targets exist)
go run ./cmd/mled-server apply --server "$SERVER_URL" --timeout-ms "$TIMEOUT_MS" --type off --all --output json \
  | jq -e 'type=="array"' >/dev/null

go run ./cmd/mled-server apply-solid --server "$SERVER_URL" --timeout-ms "$TIMEOUT_MS" --color "#FF00FF" --brightness 30 --all --output json \
  | jq -e 'type=="array"' >/dev/null

go run ./cmd/mled-server apply-rainbow --server "$SERVER_URL" --timeout-ms "$TIMEOUT_MS" --speed 25 --brightness 70 --all --output json \
  | jq -e 'type=="array"' >/dev/null

# Settings set roundtrip (safe: modifies only the server's data-dir settings.json)
OLD="$(go run ./cmd/mled-server settings get --server "$SERVER_URL" --timeout-ms "$TIMEOUT_MS" --output json)"
OLD_OFF="$(echo "$OLD" | jq -r '.[0].offline_threshold_s')"
if [[ "$OLD_OFF" == "null" || -z "$OLD_OFF" ]]; then
  echo "Unexpected settings get output:" >&2
  echo "$OLD" >&2
  exit 1
fi

NEW_OFF="$((OLD_OFF + 1))"
go run ./cmd/mled-server settings set --server "$SERVER_URL" --timeout-ms "$TIMEOUT_MS" --offline-threshold-s "$NEW_OFF" --output json \
  | jq -e --argjson v "$NEW_OFF" '.[0].offline_threshold_s == $v' >/dev/null

echo "OK: exercised REST verbs against $SERVER_URL"
