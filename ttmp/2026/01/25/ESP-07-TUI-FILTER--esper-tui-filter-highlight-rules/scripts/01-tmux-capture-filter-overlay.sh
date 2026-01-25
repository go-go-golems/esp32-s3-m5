#!/usr/bin/env bash
set -euo pipefail

# Focused capture harness for ESP-07 filter overlay (levels + regex + highlight rules).
# Produces deterministic 120x40 captures (txt + optional png).

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TICKET_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPO_ROOT="$(git -C "${TICKET_DIR}" rev-parse --show-toplevel)"

RUN_TUI="${REPO_ROOT}/ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/05-run-esper-tui.sh"

STAMP="${STAMP:-$(date +%Y%m%d-%H%M%S)}"
OUTDIR="${TICKET_DIR}/various/screenshots/${STAMP}"
mkdir -p "${OUTDIR}"

USE_VIRTUAL_PTY="${USE_VIRTUAL_PTY:-1}"
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
  local pty_a="${OUTDIR}/esper-pty-a"
  local pty_b="${OUTDIR}/esper-pty-b"

  rm -f "${pty_a}" "${pty_b}"

  socat -d -d \
    "pty,raw,echo=0,link=${pty_a}" \
    "pty,raw,echo=0,link=${pty_b}" \
    >/dev/null 2>&1 &
  SOCAT_PID=$!

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
    for i in $(seq 1 260); do
      printf 'I (12) wifi: connected ssid=lab-%03d\n' "${i}"
      if (( i % 7 == 0 )); then
        printf 'W (34) http: slow response %dms\n' "$((100 + i))"
      fi
      if (( i % 23 == 0 )); then
        printf 'E (56) sensor: read failed code=%d\n' "${i}"
      fi
      if (( i % 17 == 0 )); then
        printf 'D (78) wifi: debug trace i=%d\n' "${i}"
      fi
      if (( i % 19 == 0 )); then
        printf 'V (90) wifi: verbose trace i=%d\n' "${i}"
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
session="esp07_filter_${w}x${h}_$$"

tmux new-session -d -x "${w}" -y "${h}" -s "${session}" -c "${REPO_ROOT}" "env ESPPORT='${ESPPORT}' '${RUN_TUI}'"
sleep 2
wait_for_monitor "${session}"

# Enter HOST mode.
send_keys "${session}" C-t
sleep 0.2

# Open filter overlay.
send_keys "${session}" f
sleep 0.2

# Tab to include field (levels: E,W,I,D,V) then include.
send_keys "${session}" Tab Tab Tab Tab Tab
sleep 0.05
send_keys "${session}" w i f i
sleep 0.05

# Tab to highlight rules pattern field, add a rule, type a pattern, and cycle style.
send_keys "${session}" Tab Tab
sleep 0.05
send_keys "${session}" a
sleep 0.05
send_keys "${session}" e r r o r '|' f a i l
sleep 0.05
send_keys "${session}" Tab
sleep 0.05
send_keys "${session}" Space
sleep 0.2

capture "${session}" "${w}" "${h}" "01-filter-overlay"

# Exit cleanly.
send_keys "${session}" Escape
sleep 0.1
send_keys "${session}" C-c
sleep 0.2
tmux kill-session -t "${session}" >/dev/null 2>&1 || true

echo "outputs: ${OUTDIR}"

