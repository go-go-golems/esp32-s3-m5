#!/usr/bin/env bash
set -euo pipefail

# Focused capture harness for ESP-06 bottom-bar search mode.
# Produces deterministic 120x40 pane captures (txt + optional png).

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TICKET_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPO_ROOT="$(git -C "${TICKET_DIR}" rev-parse --show-toplevel)"

RUN_TUI="${REPO_ROOT}/ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/05-run-esper-tui.sh"

STAMP="${STAMP:-$(date +%Y%m%d-%H%M%S)}"
OUTDIR="${TICKET_DIR}/various/screenshots/${STAMP}"
mkdir -p "${OUTDIR}"

USE_VIRTUAL_PTY="${USE_VIRTUAL_PTY:-0}"
ESPPORT="${ESPPORT:-}"

have_convert() {
  command -v convert >/dev/null 2>&1
}

render_png() {
  local in_txt="$1"
  local out_png="$2"
  local cols="$3"
  local rows="$4"
  if ! have_convert; then
    return 0
  fi

  # Avoid ImageMagick @file reads (often blocked by security policy).
  local text
  text="$(cat "${in_txt}")"

  local char_w=9
  local char_h=18
  local pad=20
  local px_w=$((cols * char_w + pad * 2))
  local px_h=$((rows * char_h + pad * 2))

  convert \
    -size "${px_w}x${px_h}" \
    xc:'#0f111a' \
    -fill '#e6e6e6' \
    -font 'DejaVu-Sans-Mono' \
    -pointsize 14 \
    -gravity NorthWest \
    -annotate "+${pad}+${pad}" "${text}" \
    "${out_png}"
}

capture() {
  local session="$1"
  local w="$2"
  local h="$3"
  local name="$4"

  local base="${OUTDIR}/${w}x${h}-${name}"
  local out_txt="${base}.txt"
  local out_png="${base}.png"

  tmux capture-pane -pN -t "${session}:0.0" >"${out_txt}"
  render_png "${out_txt}" "${out_png}" "${w}" "${h}"
  echo "captured: ${out_txt}"
  if have_convert; then
    echo "captured: ${out_png}"
  fi
}

send_keys() {
  local session="$1"
  shift
  tmux send-keys -t "${session}:0.0" "$@"
}

start_virtual_pty() {
  # Creates two linked PTYs for deterministic "serial" screenshots:
  # - ESPPORT points to the esper-connected side
  # - feeder writes to the other side
  local pty_a="${OUTDIR}/esper-pty-a"
  local pty_b="${OUTDIR}/esper-pty-b"

  rm -f "${pty_a}" "${pty_b}"

  socat -d -d \
    "pty,raw,echo=0,link=${pty_a}" \
    "pty,raw,echo=0,link=${pty_b}" \
    >/dev/null 2>&1 &
  SOCAT_PID=$!

  # Wait for the PTY links to appear.
  for _ in $(seq 1 30); do
    if [[ -e "${pty_a}" && -e "${pty_b}" ]]; then
      ESPPORT="${pty_a}"
      FEED_PORT="${pty_b}"
      return 0
    fi
    sleep 0.1
  done

  echo "ERROR: socat PTYs did not appear" >&2
  return 1
}

start_feeder() {
  local feed_port="$1"
  (
    # Sample log stream with repeated "wifi_init" matches for search.
    for i in $(seq 1 220); do
      printf 'I (1523) wifi: wifi driver task: %d\\n' "${i}"
      printf 'I (1556) [wifi_init]: rx ba win: %d\\n' "${i}"
      if (( i % 5 == 0 )); then
        printf 'W (1550) wifi: Warning: NVS partition low on space\\n'
      fi
      sleep 0.01
    done
  ) >"${feed_port}" &
  FEED_PID=$!
}

wait_for_monitor() {
  local session="$1"
  local tries="${2:-80}"

  for _ in $(seq 1 "${tries}"); do
    local pane
    pane="$(tmux capture-pane -pN -t "${session}:0.0")"

    if echo "${pane}" | rg -q "Connected:"; then
      return 0
    fi

    if echo "${pane}" | rg -q "Connection failed|Serial port busy|permission denied"; then
      echo "ERROR: connect failed (see pane capture below)" >&2
      echo "${pane}" >&2
      return 1
    fi

    # If we're in the port picker, press Enter to connect to the selected port.
    if echo "${pane}" | rg -q "Select Serial Port"; then
      send_keys "${session}" Enter
    fi

    sleep 0.3
  done

  echo "ERROR: timed out waiting for monitor screen" >&2
  return 1
}

SOCAT_PID=""
FEED_PID=""
FEED_PORT=""

cleanup() {
  if [[ -n "${FEED_PID}" ]]; then
    kill "${FEED_PID}" >/dev/null 2>&1 || true
  fi
  if [[ -n "${SOCAT_PID}" ]]; then
    kill "${SOCAT_PID}" >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT

if [[ -z "${ESPPORT}" && "${USE_VIRTUAL_PTY}" == "1" ]]; then
  start_virtual_pty
  start_feeder "${FEED_PORT}"
fi

w=120
h=40
session="esp06_search_${w}x${h}_$$"

tmux new-session -d -x "${w}" -y "${h}" -s "${session}" -c "${REPO_ROOT}" "env ESPPORT='${ESPPORT}' '${RUN_TUI}'"
sleep 2
wait_for_monitor "${session}"

# Enter HOST mode.
send_keys "${session}" C-t
sleep 0.2
capture "${session}" "${w}" "${h}" "01-host"

# Search mode (HOST): / then type "wifi_init".
send_keys "${session}" /
sleep 0.1
send_keys "${session}" w i f i _ i n i t
sleep 0.2
capture "${session}" "${w}" "${h}" "02-search"

# Navigate and jump.
send_keys "${session}" n
sleep 0.1
capture "${session}" "${w}" "${h}" "03-search-next"

send_keys "${session}" Enter
sleep 0.2
capture "${session}" "${w}" "${h}" "04-search-jump"

# Exit cleanly (avoid orphaned go-run binaries holding the serial port).
send_keys "${session}" C-c
sleep 0.2
tmux kill-session -t "${session}" >/dev/null 2>&1 || true

echo "outputs: ${OUTDIR}"

