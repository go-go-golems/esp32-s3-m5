#!/usr/bin/env bash
set -euo pipefail

# Build a FATFS image for the `storage` partition (see partitions.csv) containing GIF files.
#
# By default this encodes ./assets into the FATFS root. The firmware will look in:
# - /storage/gifs (preferred)
# - /storage      (fallback)
#
# Requires ESP-IDF environment (IDF_PATH) to be active.

PROJ_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INPUT_DIR="${1:-$PROJ_DIR/assets}"
OUT_FILE="${2:-$PROJ_DIR/storage.bin}"
PARTITION_SIZE_BYTES="${3:-1048576}" # 1MiB (matches partitions.csv default)

if [[ -z "${IDF_PATH:-}" ]]; then
  echo "ERROR: IDF_PATH is not set. Run: source ~/esp/esp-idf-5.4.1/export.sh" >&2
  exit 1
fi

FATFSGEN="$IDF_PATH/components/fatfs/fatfsgen.py"
if [[ ! -f "$FATFSGEN" ]]; then
  echo "ERROR: fatfsgen.py not found at $FATFSGEN" >&2
  exit 1
fi

echo "make_storage_fatfs: input=$INPUT_DIR out=$OUT_FILE size=$PARTITION_SIZE_BYTES"
python "$FATFSGEN" \
  --output_file "$OUT_FILE" \
  --partition_size "$PARTITION_SIZE_BYTES" \
  --long_name_support \
  "$INPUT_DIR"

ls -lh "$OUT_FILE"


