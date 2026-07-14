#!/usr/bin/env bash
set -euo pipefail

TICKET_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
REPO_ROOT=$(git -C "$TICKET_ROOT" rev-parse --show-toplevel)
PROJECT="$REPO_ROOT/0109-papers3-factory-v0.5-runtime-trace"
IDF='/home/manuel/esp/esp-idf-5.3.3'
OUTPUT="$TICKET_ROOT/scripts/output"
STAMP=$(date -u +%Y%m%dT%H%M%SZ)
REPORT="$OUTPUT/24-factory-v0.5-trace-build-latest.md"
mkdir -p "$OUTPUT"

echo '[build] preparing exact FactoryTest V0.5 component lineage'
"$TICKET_ROOT/scripts/23-prepare-factory-v0.5-trace-components.sh" >/dev/null
unset IDF_PATH IDF_VERSION IDF_PYTHON_ENV_PATH
# shellcheck disable=SC1091
source "$IDF/export.sh" >/dev/null 2>&1
[[ "$(idf.py --version)" == 'ESP-IDF v5.3.3' ]] || { echo 'error: exact IDF 5.3.3 is not active' >&2; exit 2; }

build_variant() {
  local variant=$1 profile defaults build sdkconfig log
  profile=trace
  [[ "$variant" == clean ]] && profile=clean
  defaults="$PROJECT/sdkconfig.defaults;$PROJECT/sdkconfig.$variant.defaults"
  build="$PROJECT/build-$variant"
  sdkconfig="$PROJECT/sdkconfig.$variant.generated"
  log="$OUTPUT/24-factory-v0.5-$variant-$STAMP.log"
  rm -rf "$build"; rm -f "$sdkconfig"
  export PAPERS3_FACTORY_COMPONENTS_DIR="$PROJECT/.components/$profile"
  echo "[build] variant=$variant profile=$profile; full output -> $log"
  set +e
  {
    printf 'variant=%s\nprofile=%s\nidf=%s\n' "$variant" "$profile" "$(idf.py --version)"
    idf.py -C "$PROJECT" -B "$build" -D "SDKCONFIG=$sdkconfig" -D "SDKCONFIG_DEFAULTS=$defaults" set-target esp32s3 &&
      idf.py -C "$PROJECT" -B "$build" -D "SDKCONFIG=$sdkconfig" build &&
      idf.py -C "$PROJECT" -B "$build" -D "SDKCONFIG=$sdkconfig" size
  } >"$log" 2>&1
  status=$?
  set -e
  perl -pi -e 's/[ \t]+$//' "$log"
  if [[ $status -ne 0 ]]; then
    echo "error: $variant build failed (exit $status); filtered tail:" >&2
    tail -n 80 "$log" | cut -c1-500 >&2
    exit "$status"
  fi
  warnings=$(rg -c 'warning:' "$log" || true); warnings=${warnings:-0}
  warning_sha=$(rg 'warning:' "$log" | sed 's/^.*warning:/warning:/' | sort | sha256sum | awk '{print $1}')
  eval "${variant}_warning_count=$warnings"
  eval "${variant}_warning_sha=$warning_sha"
  grep -q '^CONFIG_FREERTOS_HZ=100$' "$sdkconfig" || { echo 'error: wrong tick rate' >&2; exit 4; }
  if [[ "$variant" == trace ]]; then
    grep -q '^CONFIG_PAPERS3_FACTORY_RUNTIME_TRACE=y$' "$sdkconfig" || { echo 'error: trace disabled' >&2; exit 4; }
    grep -q '^CONFIG_PAPERS3_FACTORY_TRACE_CAPACITY=1024$' "$sdkconfig" || { echo 'error: wrong ring capacity' >&2; exit 4; }
  else
    grep -q '^# CONFIG_PAPERS3_FACTORY_RUNTIME_TRACE is not set$' "$sdkconfig" || { echo 'error: trace enabled in control' >&2; exit 4; }
  fi
}

build_variant clean
build_variant off
build_variant trace

for variant in clean off trace; do
  build="$PROJECT/build-$variant"
  app="$build/papers3_factory_v05_trace.bin"; elf="$build/papers3_factory_v05_trace.elf"
  [[ -f "$app" && -f "$elf" ]] || { echo "error: missing $variant artifact" >&2; exit 5; }
  eval "${variant}_app_size=$(stat -c %s "$app")"
  eval "${variant}_app_sha=$(sha256sum "$app" | awk '{print $1}')"
  eval "${variant}_elf_sha=$(sha256sum "$elf" | awk '{print $1}')"
  eval "${variant}_sdk_sha=$(sha256sum "$PROJECT/sdkconfig.$variant.generated" | awk '{print $1}')"
done
[[ "$clean_warning_count" == "$off_warning_count" && "$off_warning_count" == "$trace_warning_count" ]] || {
  echo 'error: warning counts differ across clean/off/trace variants' >&2; exit 4;
}
[[ "$clean_warning_sha" == "$off_warning_sha" && "$off_warning_sha" == "$trace_warning_sha" ]] || {
  echo 'error: warning sets differ across clean/off/trace variants' >&2; exit 4;
}
PATCH_SHA=$(sha256sum "$TICKET_ROOT/scripts/patches/23-m5gfx-0.2.15-factory-runtime-trace.patch" | awk '{print $1}')
cat > "$REPORT" <<EOF
---
Title: FactoryTest V0.5 Stock-Source Trace Build Evidence
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
Summary: "Exact-IDF clean, trace-off, and trace-on FactoryTest V0.5 source-lineage builds without hardware access."
LastUpdated: $(date -u +%Y-%m-%dT%H:%M:%SZ)
WhatFor: "Establish F1/F2 source controls before replaying or instrumenting the physical factory sequence."
WhenToUse: "Review before any 0109 flash or interpretation of FactoryTest-derived runtime events."
---

# FactoryTest V0.5 stock-source trace build evidence

- Factory source: \`V0.5\` / \`5e275ad4b70abb85f7193fda137844730e64c4db\`
- ESP-IDF: \`v5.3.3\` / \`6db3dc25df7325c1c81b7cd7d4e42babff7a818e\`
- M5GFX: \`0.2.15\` / \`c6f92dc03226cdc04d67c705a2020f62ad21ad01\`
- M5Unified: \`0.2.10\` / \`5580ff6923a868cc71d5b30c962186bde2c85b67\`
- Trace patch SHA-256: \`$PATCH_SHA\`
- FreeRTOS tick: \`100 Hz\`
- Hardware modified: **no**

| Variant | M5GFX source | Trace | Application bytes | Application SHA-256 | ELF SHA-256 |
|---|---|---|---:|---|---|
| clean | unpatched | off | $clean_app_size | \`$clean_app_sha\` | \`$clean_elf_sha\` |
| F1/off | patched | off | $off_app_size | \`$off_app_sha\` | \`$off_elf_sha\` |
| F2/trace | patched | 1024 × 48-byte ring | $trace_app_size | \`$trace_app_sha\` | \`$trace_elf_sha\` |

All three builds completed from clean build directories. Each preserves the same $clean_warning_count upstream FactoryTest/IDF 5.3.3 warnings, normalized warning-set SHA-256 \`$clean_warning_sha\`; tracing introduced no new warning. Build commands contain no flash or monitor action.
EOF
printf '[build] complete clean=%s off=%s trace=%s\nreport=%s\ntrace_sha256=%s\nhardware_modified=no\n' \
 "$clean_app_size" "$off_app_size" "$trace_app_size" "$REPORT" "$trace_app_sha"
