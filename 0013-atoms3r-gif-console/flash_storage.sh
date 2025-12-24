#!/usr/bin/env bash
set -euo pipefail

# Flash a prebuilt FATFS image into the `storage` partition using parttool.py.
#
# Usage:
#   ./flash_storage.sh /dev/ttyACM0 storage.bin
#
# Requires ESP-IDF environment (parttool.py on PATH) to be active.

PORT="${1:-/dev/ttyACM0}"
IMAGE="${2:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/storage.bin}"

echo "flash_storage: port=$PORT image=$IMAGE"
parttool.py --port "$PORT" write_partition --partition-name=storage --input "$IMAGE"


