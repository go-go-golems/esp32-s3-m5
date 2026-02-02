#!/usr/bin/env bash
set -euo pipefail

HOST="localhost"
PORT="1884"
BASE="zigbee2mqtt"
OUT_DIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/scripts/validation"
TS=$(date -u +%Y%m%dT%H%M%SZ)
LOG="${OUT_DIR}/bridge-validation-${TS}.log"

exec > >(tee -a "${LOG}") 2>&1

echo "# Bridge validation run @ ${TS}"

echo "## docker ps"
docker ps --format 'table {{.Names}}\t{{.Image}}\t{{.Status}}'

run_sub_pub() {
  local label=$1
  local sub_topic=$2
  local pub_topic=$3
  local payload=$4
  local expect=${5:-response}
  local timeout=${6:-5}

  echo "## ${label}"
  echo "- sub: ${sub_topic}"
  echo "- pub: ${pub_topic}"
  echo "- payload: ${payload}"
  echo "- expect: ${expect}"

  set +e
  mosquitto_sub -h "${HOST}" -p "${PORT}" -t "${sub_topic}" -C 1 -W "${timeout}" &
  local sub_pid=$!
  sleep 0.2
  mosquitto_pub -h "${HOST}" -p "${PORT}" -t "${pub_topic}" -m "${payload}"
  wait ${sub_pid}
  local sub_status=$?
  set -e

  if [[ "${expect}" == "none" ]]; then
    if [[ ${sub_status} -eq 0 ]]; then
      echo "[WARN] unexpected response received on ${sub_topic}"
    else
      echo "[OK] no response within ${timeout}s (expected)"
    fi
  else
    if [[ ${sub_status} -eq 0 ]]; then
      echo "[OK] response received"
    else
      echo "[FAIL] no response within ${timeout}s"
      return 1
    fi
  fi
}

echo "## mqtt sanity"
run_sub_pub "mqtt sanity" "test" "test" "hello" "response" 5

run_sub_pub "permit_join" "${BASE}/bridge/response/permit_join" "${BASE}/bridge/request/permit_join" '{"time": 1}' "response" 8

run_sub_pub "info" "${BASE}/bridge/info" "${BASE}/bridge/request/info" '{}' "response" 8

run_sub_pub "devices" "${BASE}/bridge/devices" "${BASE}/bridge/request/devices" '{}' "response" 8

run_sub_pub "definitions" "${BASE}/bridge/definitions" "${BASE}/bridge/request/definitions" '{}' "response" 12

run_sub_pub "health_check (expected no response)" "${BASE}/bridge/response/health_check" "${BASE}/bridge/request/health_check" '{}' "none" 6

run_sub_pub "logging (expected no response)" "${BASE}/bridge/response/logging" "${BASE}/bridge/request/logging" '{"level":"info"}' "none" 6

echo "# done"
