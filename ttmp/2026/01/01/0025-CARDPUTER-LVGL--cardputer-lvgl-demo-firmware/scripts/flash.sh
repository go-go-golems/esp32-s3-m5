#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-/dev/ttyACM0}"

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../../../" && pwd)"
PROJECT="${REPO_ROOT}/0025-cardputer-lvgl-demo"

cd "${PROJECT}"
./build.sh build
./build.sh -p "${PORT}" flash

