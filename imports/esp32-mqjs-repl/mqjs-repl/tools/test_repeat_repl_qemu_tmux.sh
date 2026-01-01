#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
test_repeat_repl_qemu_tmux.sh

Runs `idf.py qemu monitor` in a tmux session, sends a few REPL inputs, and checks output.

Usage:
  ./tools/test_repeat_repl_qemu_tmux.sh [--session NAME] [--timeout SECONDS] [--keep-session]

Notes:
  - This uses `tmux send-keys`, so it exercises the same stdin path as a human in the monitor.
  - If QEMU/monitor input is broken (ticket 0015), this will fail and print the captured console output.
EOF
}

SESSION="mqjs-repl-qemu-test-$$"
TIMEOUT_SECS="45"
KEEP_SESSION="0"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --session)
      SESSION="$2"
      shift 2
      ;;
    --timeout)
      TIMEOUT_SECS="$2"
      shift 2
      ;;
    --keep-session)
      KEEP_SESSION="1"
      shift 1
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown arg: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

if ! command -v tmux >/dev/null 2>&1; then
  echo "tmux is required" >&2
  exit 1
fi

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${PROJECT_DIR}"
LOG_DIR="${PROJECT_DIR}/build"
mkdir -p "${LOG_DIR}"
LOG_PATH="${LOG_DIR}/repeat_repl_qemu_tmux_${SESSION}.log"

TMUX_SOCKET=".tmuxsock-${SESSION}"

tmux_cmd() {
  command tmux -S "${TMUX_SOCKET}" "$@"
}

cleanup() {
  if [[ "${KEEP_SESSION}" == "1" ]]; then
    echo "Keeping tmux session: ${SESSION}" >&2
    return
  fi
  tmux_cmd kill-session -t "${SESSION}" >/dev/null 2>&1 || true
  rm -f "${TMUX_SOCKET}" >/dev/null 2>&1 || true
}
trap cleanup EXIT

tmux_cmd kill-session -t "${SESSION}" >/dev/null 2>&1 || true

tmux_cmd new-session -d -s "${SESSION}" -c "${PROJECT_DIR}" \
  "unset ESP_IDF_VERSION; ./build.sh qemu monitor"

capture() {
  tmux_cmd capture-pane -pt "${SESSION}" -S -5000
}

wait_for_fixed() {
  local needle="$1"
  local start
  start="$(date +%s)"
  while true; do
    if capture | grep -Fq -- "${needle}"; then
      return 0
    fi
    if (( "$(date +%s)" - start > TIMEOUT_SECS )); then
      return 1
    fi
    sleep 0.2
  done
}

send_line() {
  local line="$1"
  tmux_cmd send-keys -t "${SESSION}" -- "${line}" Enter
}

echo "Waiting for REPL prompt in QEMU..." >&2
if ! wait_for_fixed "repeat> "; then
  capture >"${LOG_PATH}"
  echo "Timed out waiting for prompt; captured output at: ${LOG_PATH}" >&2
  exit 1
fi

send_line ":mode"
if ! wait_for_fixed "mode: repeat"; then
  capture >"${LOG_PATH}"
  echo "Did not see expected ':mode' output; captured output at: ${LOG_PATH}" >&2
  exit 1
fi

send_line "hello-qemu"
if ! wait_for_fixed "hello-qemu"; then
  capture >"${LOG_PATH}"
  echo "Did not see expected echo output; captured output at: ${LOG_PATH}" >&2
  exit 1
fi

send_line ":prompt test> "
if ! wait_for_fixed "test> "; then
  capture >"${LOG_PATH}"
  echo "Did not see updated prompt; captured output at: ${LOG_PATH}" >&2
  exit 1
fi

capture >"${LOG_PATH}"
echo "OK: repeat REPL QEMU smoke test passed (log: ${LOG_PATH})" >&2
