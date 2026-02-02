#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/scripts/validation"
TS=$(date -u +%Y%m%dT%H%M%SZ)
LOG="${OUT_DIR}/zigctl-validation-${TS}.log"

BROKER="mqtt://localhost:1884"
BASE="zigbee2mqtt"

exec > >(tee -a "${LOG}") 2>&1

echo "# zigctl validation run @ ${TS}"

echo "## zigctl bridge info"
( cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl && \
  go run ./ bridge info --broker "${BROKER}" --base-topic "${BASE}" --output json )

echo "## zigctl bridge devices"
( cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl && \
  go run ./ bridge devices --broker "${BROKER}" --base-topic "${BASE}" --output json )

echo "## zigctl bridge permit-join"
( cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl && \
  go run ./ bridge permit-join --broker "${BROKER}" --base-topic "${BASE}" --seconds 1 --output json )

echo "## zigctl mqtt pub/sub"
set +e
( cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl && \
  timeout 6s go run ./ mqtt sub --broker "${BROKER}" --topic "${BASE}/test/zigctl" --output json ) &
SUB_PID=$!
sleep 0.3
( cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl && \
  go run ./ mqtt pub --broker "${BROKER}" --topic "${BASE}/test/zigctl" --message '{"hello":"zigctl"}' --output json )
wait ${SUB_PID}
SUB_STATUS=$?
set -e

if [[ ${SUB_STATUS} -ne 0 ]]; then
  echo "[WARN] mqtt sub exited with status ${SUB_STATUS} (expected if timeout)"
else
  echo "[OK] mqtt sub received message"
fi

echo "# done"
