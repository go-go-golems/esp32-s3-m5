#!/usr/bin/env bash
set -euo pipefail

# Demo for ESP-02: device registry + nickname resolution.
#
# Requires:
# - Go toolchain
# - an ESP device connected (USB Serial/JTAG or similar)
#
# Usage:
#   USB_SERIAL="D8:85:AC:A4:FB:7C" NICKNAME="atoms3r" ./scripts/01-demo-device-registry.sh

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
workspace_root="$(cd "$here/../../../../../../" && pwd)"
esper_root="$workspace_root/esper"

USB_SERIAL="${USB_SERIAL:-}"
NICKNAME="${NICKNAME:-}"

if [[ -z "$USB_SERIAL" || -z "$NICKNAME" ]]; then
  echo "missing USB_SERIAL or NICKNAME env var" >&2
  exit 2
fi

(cd "$esper_root" && go run ./cmd/esper devices path)

echo
echo "# scan (show serials)"
(cd "$esper_root" && go run ./cmd/esper scan --output table --fields serial,preferred_path,chip_description)

echo
echo "# set nickname"
(cd "$esper_root" && go run ./cmd/esper devices set \
  --usb-serial "$USB_SERIAL" \
  --nickname "$NICKNAME" \
  --name "$NICKNAME" \
  --description "demo entry")

echo
echo "# list registry"
(cd "$esper_root" && go run ./cmd/esper devices list --output table --fields usb_serial,nickname,name,description,preferred_path)

echo
echo "# scan annotated"
(cd "$esper_root" && go run ./cmd/esper scan --output table --fields preferred_path,serial,nickname,device_name,device_description)

echo
echo "# resolve nickname -> port"
(cd "$esper_root" && go run ./cmd/esper devices resolve --nickname "$NICKNAME")

