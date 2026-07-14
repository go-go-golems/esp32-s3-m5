#!/usr/bin/env bash
set -euo pipefail

TICKET_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
REPO_ROOT=$(git -C "$TICKET_ROOT" rev-parse --show-toplevel)
PROJECT="$REPO_ROOT/0107-papers3-epd-painter-control"
IDF_ROOT='/home/manuel/esp/esp-idf-5.4.2'
BUILD_DIR="$PROJECT/build-ticket"
SDKCONFIG="$PROJECT/sdkconfig.ticket"
OUTPUT_DIR="$TICKET_ROOT/scripts/output"
TIMESTAMP=$(date -u +%Y%m%dT%H%M%SZ)
GENERATED=$(date -u +%Y-%m-%dT%H:%M:%SZ)
LOG="$OUTPUT_DIR/12-epd-painter-build-$TIMESTAMP.log"
REPORT="$OUTPUT_DIR/12-epd-painter-build-latest.md"

if [[ ! -f "$IDF_ROOT/export.sh" ]]; then
  echo "error: exact ESP-IDF 5.4.2 is not installed at $IDF_ROOT" >&2
  exit 2
fi

mkdir -p "$OUTPUT_DIR"
"$TICKET_ROOT/scripts/11-prepare-epd-painter-control.sh"

# shellcheck disable=SC1091
source "$IDF_ROOT/export.sh" >/dev/null
IDF_VERSION=$(idf.py --version)
if [[ "$IDF_VERSION" != "ESP-IDF v5.4.2" ]]; then
  echo "error: expected 'ESP-IDF v5.4.2', got '$IDF_VERSION'" >&2
  exit 2
fi

# sdkconfig.defaults seeds only absent values. Recreate this evidence config on
# every run so tick rate, USB console, PSRAM, and partition settings cannot be
# inherited from an earlier build.
rm -f "$SDKCONFIG"
rm -rf "$BUILD_DIR"

{
  echo "build_utc=$TIMESTAMP"
  echo "idf=$IDF_VERSION"
  echo "project=$PROJECT"
  echo "command=idf.py -B $BUILD_DIR -D SDKCONFIG=$SDKCONFIG set-target esp32s3 build"
  (
    cd "$PROJECT"
    idf.py -B "$BUILD_DIR" -D "SDKCONFIG=$SDKCONFIG" set-target esp32s3
    idf.py -B "$BUILD_DIR" -D "SDKCONFIG=$SDKCONFIG" build
    idf.py -B "$BUILD_DIR" -D "SDKCONFIG=$SDKCONFIG" size
  )
} 2>&1 | tee "$LOG"
# CMake and size-table output can contain cosmetic trailing spaces. Normalize
# the committed evidence log without changing messages or command outcomes.
perl -pi -e 's/[ \t]+$//' "$LOG"

if rg -n 'warning:' "$LOG"; then
  echo "error: build completed with warnings; review $LOG" >&2
  exit 4
fi

APP_BIN="$BUILD_DIR/papers3_epd_painter_control.bin"
ELF="$BUILD_DIR/papers3_epd_painter_control.elf"
BOOT_BIN="$BUILD_DIR/bootloader/bootloader.bin"
PART_BIN="$BUILD_DIR/partition_table/partition-table.bin"
for artifact in "$APP_BIN" "$ELF" "$BOOT_BIN" "$PART_BIN"; do
  [[ -f "$artifact" ]] || { echo "error: missing build artifact $artifact" >&2; exit 3; }
done

APP_SHA=$(sha256sum "$APP_BIN" | awk '{print $1}')
ELF_SHA=$(sha256sum "$ELF" | awk '{print $1}')
BOOT_SHA=$(sha256sum "$BOOT_BIN" | awk '{print $1}')
PART_SHA=$(sha256sum "$PART_BIN" | awk '{print $1}')
PATCH_SHA=$(sha256sum "$TICKET_ROOT/scripts/patches/11-epd-painter-pure-idf-hardening.patch" | awk '{print $1}')
PREPARED_SHA=$(sha256sum "$PROJECT/components/epd_painter/PREPARED_MANIFEST.txt" | awk '{print $1}')
SDKCONFIG_SHA=$(sha256sum "$SDKCONFIG" | awk '{print $1}')
DEFAULTS_SHA=$(sha256sum "$PROJECT/sdkconfig.defaults" | awk '{print $1}')
APP_SIZE=$(stat -c %s "$APP_BIN")
ELF_SIZE=$(stat -c %s "$ELF")

cat > "$REPORT" <<EOF_REPORT
---
Title: EPD Painter Control Build Evidence
Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES
Status: active
Topics:
    - papers3
    - eink
    - esp-idf
    - hardware-qualification
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Latest reproducible no-flash build evidence for the hardened independent PaperS3 EPD control."
LastUpdated: $GENERATED
WhatFor: "Verify exact toolchain, patched source, configuration, binary identity, and size before any hardware flash."
WhenToUse: "Regenerate after any firmware, patch, SDK configuration, or toolchain change."
---

# EPD_Painter control build evidence

- Build UTC: \`$TIMESTAMP\`
- ESP-IDF: \`$IDF_VERSION\`
- Target: \`esp32s3\`
- Upstream EPD_Painter: \`753c521da8aef59756df07c1a4eb88f1c64c8227\`
- Local patch SHA-256: \`$PATCH_SHA\`
- Prepared manifest SHA-256: \`$PREPARED_SHA\`
- sdkconfig.defaults SHA-256: \`$DEFAULTS_SHA\`
- generated sdkconfig SHA-256: \`$SDKCONFIG_SHA\`
- Application BIN: \`$APP_SIZE bytes\`, SHA-256 \`$APP_SHA\`
- ELF: \`$ELF_SIZE bytes\`, SHA-256 \`$ELF_SHA\`
- Bootloader SHA-256: \`$BOOT_SHA\`
- Partition table SHA-256: \`$PART_SHA\`
- Full build log: \`${LOG#$REPO_ROOT/}\`
- Hardware modified: **no**

## Fixed safety configuration

~~~text
CONFIG_IDF_TARGET="esp32s3"
CONFIG_FREERTOS_HZ=1000
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
EPD_PAINTER_PRESET_M5PAPER_S3=1
EPD_PAINTER_DISABLE_BOOTCTL=1
~~~
EOF_REPORT

printf 'build_report=%s\napp_sha256=%s\nelf_sha256=%s\nhardware_modified=no\n' \
  "$REPORT" "$APP_SHA" "$ELF_SHA"
