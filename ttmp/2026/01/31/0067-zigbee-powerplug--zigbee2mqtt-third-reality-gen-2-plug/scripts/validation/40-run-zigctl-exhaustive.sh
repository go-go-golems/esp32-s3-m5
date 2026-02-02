#!/usr/bin/env bash
set -euo pipefail

WORKDIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5"
ZIGCTL_DIR="${WORKDIR}/zigctl"
OUT_DIR="${WORKDIR}/ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/scripts/validation"
TS=$(date -u +%Y%m%dT%H%M%SZ)
LOG="${OUT_DIR}/zigctl-exhaustive-${TS}.log"

BROKER="mqtt://localhost:1884"
BASE="zigbee2mqtt"
TEST_DEVICE="test_device"
TEST_TOPIC="${BASE}/test/zigctl"

exec > >(tee -a "${LOG}") 2>&1

echo "# zigctl exhaustive validation run @ ${TS}"

echo "## ensure broker stack is running"
"${WORKDIR}/ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/scripts/validation/10-start-broker-tmux.sh" || true
sleep 2

docker ps --format 'table {{.Names}}\t{{.Image}}\t{{.Status}}'

echo "## zigctl bridge info"
( cd "${ZIGCTL_DIR}" && go run ./ bridge info --broker "${BROKER}" --base-topic "${BASE}" --output json )

echo "## zigctl bridge devices"
( cd "${ZIGCTL_DIR}" && go run ./ bridge devices --broker "${BROKER}" --base-topic "${BASE}" --output json )

echo "## zigctl bridge permit-join"
( cd "${ZIGCTL_DIR}" && go run ./ bridge permit-join --broker "${BROKER}" --base-topic "${BASE}" --seconds 1 --output json )

echo "## zigctl mqtt pub"
( cd "${ZIGCTL_DIR}" && go run ./ mqtt pub --broker "${BROKER}" --topic "${TEST_TOPIC}" --message '{"hello":"zigctl"}' --output json )

echo "## zigctl mqtt sub (timeout expected)"
set +e
( cd "${ZIGCTL_DIR}" && timeout 6s go run ./ mqtt sub --broker "${BROKER}" --topic "${TEST_TOPIC}" --output json ) &
SUB_PID=$!
sleep 0.3
( cd "${ZIGCTL_DIR}" && go run ./ mqtt pub --broker "${BROKER}" --topic "${TEST_TOPIC}" --message '{"hello":"zigctl-sub"}' --output json )
wait ${SUB_PID}
SUB_STATUS=$?
set -e
if [[ ${SUB_STATUS} -eq 124 ]]; then
  echo "[OK] mqtt sub terminated due to timeout (expected)"
elif [[ ${SUB_STATUS} -eq 0 ]]; then
  echo "[OK] mqtt sub exited cleanly"
else
  echo "[WARN] mqtt sub exited with status ${SUB_STATUS}"
fi

echo "## zigctl listen raw (timeout expected)"
set +e
( cd "${ZIGCTL_DIR}" && timeout 6s go run ./ listen raw --broker "${BROKER}" --topic "${BASE}/#" --output json ) &
RAW_PID=$!
sleep 0.3
mosquitto_pub -h localhost -p 1884 -t "${BASE}/${TEST_DEVICE}" -m '{"state":"ON","source":"raw-listen"}'
wait ${RAW_PID}
RAW_STATUS=$?
set -e
if [[ ${RAW_STATUS} -eq 124 ]]; then
  echo "[OK] listen raw terminated due to timeout (expected)"
elif [[ ${RAW_STATUS} -eq 0 ]]; then
  echo "[OK] listen raw exited cleanly"
else
  echo "[WARN] listen raw exited with status ${RAW_STATUS}"
fi

echo "## zigctl listen state --device ${TEST_DEVICE} (timeout expected)"
set +e
( cd "${ZIGCTL_DIR}" && timeout 6s go run ./ listen state --broker "${BROKER}" --base-topic "${BASE}" --device "${TEST_DEVICE}" --output json ) &
STATE_PID=$!
sleep 0.3
mosquitto_pub -h localhost -p 1884 -t "${BASE}/${TEST_DEVICE}" -m '{"state":"OFF","source":"state-listen"}'
wait ${STATE_PID}
STATE_STATUS=$?
set -e
if [[ ${STATE_STATUS} -eq 124 ]]; then
  echo "[OK] listen state terminated due to timeout (expected)"
elif [[ ${STATE_STATUS} -eq 0 ]]; then
  echo "[OK] listen state exited cleanly"
else
  echo "[WARN] listen state exited with status ${STATE_STATUS}"
fi

echo "# done"
