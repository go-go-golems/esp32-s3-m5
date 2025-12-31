#!/usr/bin/env bash
set -euo pipefail

WIFI_SSID="${WIFI_SSID:-}"
WIFI_PASSWORD="${WIFI_PASSWORD:-}"

if [[ -z "${WIFI_SSID}" ]]; then
  echo "ERROR: set WIFI_SSID (and optionally WIFI_PASSWORD) in env" >&2
  echo "Example:" >&2
  echo "  WIFI_SSID='MyNetwork' WIFI_PASSWORD='secret' ./tools/set_wifi_sta_creds.sh" >&2
  exit 1
fi

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SDKCONFIG="${SDKCONFIG:-${PROJECT_DIR}/sdkconfig}"

if [[ ! -f "${SDKCONFIG}" ]]; then
  echo "ERROR: sdkconfig not found at: ${SDKCONFIG}" >&2
  echo "Run one of these first:" >&2
  echo "  ./build.sh set-target esp32s3" >&2
  echo "  ./build.sh build" >&2
  exit 1
fi

perl -0777 -pe 's/^CONFIG_CLINTS_MEMO_WIFI_STA_ENABLE=.*\n//mg; s/^CONFIG_CLINTS_MEMO_WIFI_STA_SSID=.*\n//mg; s/^CONFIG_CLINTS_MEMO_WIFI_STA_PASSWORD=.*\n//mg;' -i "${SDKCONFIG}"

{
  printf '\n'
  printf 'CONFIG_CLINTS_MEMO_WIFI_STA_ENABLE=y\n'
  printf 'CONFIG_CLINTS_MEMO_WIFI_STA_SSID="%s"\n' "${WIFI_SSID}"
  printf 'CONFIG_CLINTS_MEMO_WIFI_STA_PASSWORD="%s"\n' "${WIFI_PASSWORD}"
} >> "${SDKCONFIG}"

echo "Updated WiFi STA credentials in: ${SDKCONFIG}"
echo "NOTE: sdkconfig is ignored by git; do not commit WiFi secrets."

