#!/usr/bin/env bash
set -euo pipefail

TICKET_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
REPO_ROOT=$(git -C "$TICKET_ROOT" rev-parse --show-toplevel)
PROJECT="$REPO_ROOT/0108-papers3-m5gfx-runtime-trace"
COMPONENTS="$PROJECT/components"
IDF_ROOT='/home/manuel/esp/esp-idf-5.4.2'
OUTPUT="$TICKET_ROOT/scripts/output"
STAMP=$(date -u +%Y%m%dT%H%M%SZ)
GENERATED=$(date -u +%Y-%m-%dT%H:%M:%SZ)
REPORT="$OUTPUT/19-m5gfx-runtime-trace-build-latest.md"
mkdir -p "$OUTPUT"

[[ -f "$IDF_ROOT/export.sh" ]] || { echo "error: ESP-IDF 5.4.2 missing at $IDF_ROOT" >&2; exit 2; }
echo '[build] preparing pinned M5GFX/M5Unified sources and trace patch'
"$TICKET_ROOT/scripts/18-prepare-m5gfx-runtime-trace.sh" >/dev/null
export PAPERS3_COMPONENTS_DIR="$COMPONENTS"
# shellcheck disable=SC1091
source "$IDF_ROOT/export.sh" >/dev/null 2>&1
[[ "$(idf.py --version)" == 'ESP-IDF v5.4.2' ]] || { echo 'error: wrong ESP-IDF' >&2; exit 2; }

build_variant() {
  local variant=$1
  local build="$PROJECT/build-$variant"
  local sdkconfig="$PROJECT/sdkconfig.$variant.generated"
  local defaults="$PROJECT/sdkconfig.defaults;$PROJECT/sdkconfig.$variant.defaults"
  local log="$OUTPUT/19-m5gfx-runtime-trace-$variant-$STAMP.log"
  rm -rf "$build"
  rm -f "$sdkconfig"
  echo "[build] variant=$variant full output -> $log"
  set +e
  {
    printf 'variant=%s\nbuild_utc=%s\nidf=%s\ncomponents=%s\n' "$variant" "$STAMP" "$(idf.py --version)" "$COMPONENTS"
    idf.py -C "$PROJECT" -B "$build" -D "SDKCONFIG=$sdkconfig" -D "SDKCONFIG_DEFAULTS=$defaults" set-target esp32s3 &&
      idf.py -C "$PROJECT" -B "$build" -D "SDKCONFIG=$sdkconfig" build &&
      idf.py -C "$PROJECT" -B "$build" -D "SDKCONFIG=$sdkconfig" size
  } >"$log" 2>&1
  local status=$?
  set -e
  perl -pi -e 's/[ \t]+$//' "$log"
  if [[ $status -ne 0 ]]; then
    echo "error: $variant build failed (exit $status); filtered tail of $log:" >&2
    tail -n 80 "$log" | cut -c1-500 >&2
    exit "$status"
  fi
  if rg -n 'warning:' "$log"; then
    echo "error: $variant build emitted warnings; see $log" >&2
    exit 4
  fi
  local expected='n'
  [[ "$variant" == trace ]] && expected='y'
  if [[ "$expected" == y ]]; then
    grep -q '^CONFIG_PAPERS3_M5GFX_RUNTIME_TRACE=y$' "$sdkconfig" || { echo 'error: trace variant disabled' >&2; exit 4; }
    grep -q '^CONFIG_PAPERS3_M5GFX_TRACE_CAPACITY=512$' "$sdkconfig" || { echo 'error: wrong trace capacity' >&2; exit 4; }
  else
    grep -q '^# CONFIG_PAPERS3_M5GFX_RUNTIME_TRACE is not set$' "$sdkconfig" || { echo 'error: off variant enabled' >&2; exit 4; }
  fi
  grep -q '^CONFIG_FREERTOS_HZ=100$' "$sdkconfig" || { echo 'error: M5GFX control must preserve 100 Hz baseline tick' >&2; exit 4; }
  grep -q '^CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y$' "$sdkconfig" || { echo 'error: wrong console' >&2; exit 4; }
}

build_variant off
build_variant trace

for variant in off trace; do
  build="$PROJECT/build-$variant"
  app="$build/papers3_m5gfx_runtime_trace.bin"
  elf="$build/papers3_m5gfx_runtime_trace.elf"
  archive="$build/esp-idf/M5GFX/libM5GFX.a"
  [[ -f "$app" && -f "$elf" && -f "$archive" ]] || { echo "error: missing $variant artifact" >&2; exit 5; }
  eval "${variant}_app_sha=$(sha256sum "$app" | awk '{print $1}')"
  eval "${variant}_elf_sha=$(sha256sum "$elf" | awk '{print $1}')"
  eval "${variant}_app_size=$(stat -c %s "$app")"
  eval "${variant}_elf_size=$(stat -c %s "$elf")"
  eval "${variant}_archive_size=$(stat -c %s "$archive")"
  eval "${variant}_sdk_sha=$(sha256sum "$PROJECT/sdkconfig.$variant.generated" | awk '{print $1}')"
done

PATCH_SHA=$(sha256sum "$TICKET_ROOT/scripts/patches/18-m5gfx-runtime-trace-hooks.patch" | awk '{print $1}')
OFF_TRACE_SYMBOLS=$(xtensa-esp32s3-elf-nm -C "$PROJECT/build-off/papers3_m5gfx_runtime_trace.elf" | grep -c 'lgfx_epd_trace_emit' || true)
ON_TRACE_SYMBOLS=$(xtensa-esp32s3-elf-nm -C "$PROJECT/build-trace/papers3_m5gfx_runtime_trace.elf" | grep -c 'lgfx_epd_trace_emit' || true)
OFF_TRACE_SYMBOLS=${OFF_TRACE_SYMBOLS:-0}
ON_TRACE_SYMBOLS=${ON_TRACE_SYMBOLS:-0}
APP_DELTA=$((trace_app_size - off_app_size))
ELF_DELTA=$((trace_elf_size - off_elf_size))
ARCHIVE_DELTA=$((trace_archive_size - off_archive_size))

cat > "$REPORT" <<EOF
---
Title: M5GFX Runtime Trace Variant Build Evidence
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
Summary: "Warning-free trace-off and fixed-ring trace-timing builds from the same pinned M5GFX source and configuration."
LastUpdated: $GENERATED
WhatFor: "Compare compile-time and memory observer effects before any instrumented firmware is flashed."
WhenToUse: "Use when reviewing runtime trace implementation, binary identity, or authorization for a physical timing experiment."
---

# M5GFX runtime trace variant build evidence

- Build UTC: \`$STAMP\`
- ESP-IDF: \`ESP-IDF v5.4.2\`
- M5GFX: \`ad9b814264d4e2000e9f30070002310bbccaffc9\` (0.2.25)
- M5Unified: \`b1ffcc677014ed8bd01e5a1f240736ae654bfe12\` (0.2.18)
- Trace patch SHA-256: \`$PATCH_SHA\`
- Canonical LUT SHA-256: \`d24b2df188e4261d5891a0884e2510567ea45c38bcaebeb66ade1d4f4b979af3\`
- FreeRTOS tick: \`100 Hz\` (preserves the qualification/factory-family M5GFX baseline)
- Hardware modified: **no**

| Artifact | Trace off | Trace timing | Delta |
|---|---:|---:|---:|
| application bytes | $off_app_size | $trace_app_size | $APP_DELTA |
| ELF bytes | $off_elf_size | $trace_elf_size | $ELF_DELTA |
| M5GFX archive bytes | $off_archive_size | $trace_archive_size | $ARCHIVE_DELTA |
| linked trace symbols | $OFF_TRACE_SYMBOLS | $ON_TRACE_SYMBOLS | $((ON_TRACE_SYMBOLS - OFF_TRACE_SYMBOLS)) |

## Artifact identities

- Off application SHA-256: \`$off_app_sha\`
- Off ELF SHA-256: \`$off_elf_sha\`
- Off sdkconfig SHA-256: \`$off_sdk_sha\`
- Trace application SHA-256: \`$trace_app_sha\`
- Trace ELF SHA-256: \`$trace_elf_sha\`
- Trace sdkconfig SHA-256: \`$trace_sdk_sha\`

## Build dispositions

- Both variants built from clean state with zero compiler warnings.
- Both variants use the same patched source tree; the Kconfig boolean is the only trace-mode selection.
- The off variant contains no linked \`lgfx_epd_trace_emit\` symbol.
- The timing variant links the fixed-ring hook and retains no per-row drive-code counting.
- Neither build command included flash or monitor actions.
EOF

printf '[build] complete: off_app=%s trace_app=%s delta=%s\nreport=%s\noff_sha256=%s\ntrace_sha256=%s\nhardware_modified=no\n' \
  "$off_app_size" "$trace_app_size" "$APP_DELTA" "$REPORT" "$off_app_sha" "$trace_app_sha"
