#!/usr/bin/env bash
set -euo pipefail

TICKET_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
REPO_ROOT=$(git -C "$TICKET_ROOT" rev-parse --show-toplevel)
COMMIT='753c521da8aef59756df07c1a4eb88f1c64c8227'
UPSTREAM="$TICKET_ROOT/sources/code/epd-painter-$COMMIT"
PATCH_FILE="$TICKET_ROOT/scripts/patches/11-epd-painter-pure-idf-hardening.patch"
PROJECT="$REPO_ROOT/0107-papers3-epd-painter-control"
DEST="$PROJECT/components/epd_painter"

for required in "$UPSTREAM/MANIFEST.txt" "$UPSTREAM/src/EPD_Painter.cpp" "$PATCH_FILE"; do
  if [[ ! -f "$required" ]]; then
    echo "error: missing required input: $required" >&2
    exit 2
  fi
done

# Verify the ticket-owned upstream evidence before deriving firmware source.
(
  cd "$UPSTREAM"
  sha256sum --strict --check MANIFEST.txt
)

rm -rf "$DEST"
mkdir -p "$DEST"
cp -a "$UPSTREAM/src" "$DEST/src"
cp "$UPSTREAM/README.md" "$UPSTREAM/How_It_Works.md" "$UPSTREAM/reference_manual.md" "$DEST/"

patch --batch --fuzz=0 --strip=1 --directory="$DEST" < "$PATCH_FILE"

# The independent experiment must retain upstream waveform bytes exactly.
cmp "$UPSTREAM/src/EPD_Painter_presets.h" "$DEST/src/EPD_Painter_presets.h"

cat > "$DEST/CMakeLists.txt" <<'CMAKE'
idf_component_register(
    SRCS
        "src/EPD_Painter.cpp"
        "src/EPD_Painter.S"
        "src/epd_painter_powerctl.cpp"
        "src/epd_pin_driver.cpp"
    INCLUDE_DIRS "src"
    REQUIRES
        esp_driver_gpio
        esp_hw_support
        esp_timer
        esp_system
        freertos
        heap
        log
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
target_compile_definitions(${COMPONENT_LIB} PUBLIC
    EPD_PAINTER_PRESET_M5PAPER_S3=1
    EPD_PAINTER_DISABLE_BOOTCTL=1
)
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -Wno-missing-field-initializers
    -Werror=return-type
    -Werror=uninitialized
)
CMAKE

PATCH_SHA=$(sha256sum "$PATCH_FILE" | awk '{print $1}')
cat > "$DEST/PROVENANCE.md" <<EOF_PROVENANCE
# EPD_Painter component provenance

- Upstream repository: https://github.com/tonywestonuk/EPD_Painter
- Upstream commit: \`$COMMIT\`
- Ticket snapshot: \`${UPSTREAM#$REPO_ROOT/}\`
- Local patch: \`${PATCH_FILE#$REPO_ROOT/}\`
- Local patch SHA-256: \`$PATCH_SHA\`
- Build mode: pure ESP-IDF; M5PaperS3 preset; automatic boot/shutdown controller excluded

Run \`${BASH_SOURCE[0]#$REPO_ROOT/}\` from any directory to reconstruct this component. The script verifies the ticket snapshot, applies the patch with zero fuzz, and proves that \`EPD_Painter_presets.h\` remains byte-identical to upstream.
EOF_PROVENANCE

(
  cd "$DEST"
  {
    echo "# upstream_commit=$COMMIT"
    echo "# patch_sha256=$PATCH_SHA"
    find src -type f -print0 | sort -z | xargs -0 sha256sum
    sha256sum CMakeLists.txt PROVENANCE.md
  } > PREPARED_MANIFEST.txt
)

# Static postconditions for the eight-blocker gate. The Adafruit binding is
# intentionally present only as reference source and is not compiled.
rg -q 'epd_gpio_func_sel\(pin\)' "$DEST/src/EPD_Painter.cpp"
rg -q 'epd_gpio_func_sel\(_config.pin_cl\)' "$DEST/src/EPD_Painter.cpp"
! rg -q 'epd_gpio_func_sel\(GPIO_PIN_MUX_REG' "$DEST/src/EPD_Painter.cpp"
! rg -q 'log_w\(' "$DEST/src/EPD_Painter.cpp"
rg -q 'memset\(packed_screenbuffer, 0, packed_size\)' "$DEST/src/EPD_Painter.cpp"
rg -q 'bool EPD_Painter::waitIdle' "$DEST/src/EPD_Painter.cpp"
rg -q 'PanelPowerGuard::initialize' "$DEST/src/EPD_Painter.cpp"
rg -q 'EPD_PAINTER_DISABLE_BOOTCTL=1' "$DEST/CMakeLists.txt"

printf 'prepared=%s\nupstream=%s\npatch_sha256=%s\n' "$DEST" "$COMMIT" "$PATCH_SHA"
