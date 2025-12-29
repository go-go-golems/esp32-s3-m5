---
Title: 'Bug report: SPIFFS mount/format + autoload JS parse errors'
Ticket: 0016-SPIFFS-AUTOLOAD
Status: active
Topics:
    - esp32s3
    - esp-idf
    - qemu
    - spiffs
    - flash
    - filesystem
    - javascript
    - microquickjs
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/main.c
      Note: SPIFFS init + create_example_libraries + load_js_file(JS_Eval) are the likely sources of the issue
    - Path: imports/esp32-mqjs-repl/mqjs-repl/partitions.csv
      Note: Defines the SPIFFS partition 'storage' referenced by init_spiffs
    - Path: imports/qemu_storage_repl.txt
      Note: Reference QEMU log showing the same first-boot formatting + JS parse errors
    - Path: ttmp/2025/12/29/0016-SPIFFS-AUTOLOAD--bug-spiffs-first-boot-format-autoload-js-parse-errors/sources/qemu-spiffs-autoload-snippet.txt
      Note: Condensed log snippet used in the bug report
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-29T14:05:01.574230242-05:00
WhatFor: ""
WhenToUse: ""
---


# Bug report: SPIFFS mount/format + autoload JS parse errors

## Summary

On QEMU boot, SPIFFS mount fails and triggers a format, then example JS libraries are created, but **autoload library loading fails** with MicroQuickJS parse errors:

- `SPIFFS: mount failed, -10025. formatting...`
- `Error loading /spiffs/autoload/*.js: [mtag]: expecting ';'`

This is likely two issues:

1) **First boot behavior**: SPIFFS is expected to format on first boot (empty/uninitialized partition). We need to confirm whether `-10025` is “expected first boot” or indicates a deeper partition/config mismatch.
2) **JS library content incompatibility**: The firmware writes `math.js`, `string.js`, `array.js` to `/spiffs/autoload/`, but the MicroQuickJS parser rejects them at `1:5` with `expecting ';'`.

## Environment

- **ESP-IDF**: 5.4.1
- **Target**: esp32s3
- **Run mode**: `idf.py qemu monitor` (QEMU + idf_monitor over `socket://localhost:5555`)
- **Firmware**: `imports/esp32-mqjs-repl/mqjs-repl`

## Evidence (log excerpt)

```text
I (...) mqjs-repl: Starting ESP32-S3 MicroQuickJS REPL with Storage
I (...) mqjs-repl: Initializing SPIFFS
W (...) SPIFFS: mount failed, -10025. formatting...
I (...) mqjs-repl: SPIFFS: total=896321 bytes, used=0 bytes
I (...) mqjs-repl: First boot - creating example libraries
I (...) mqjs-repl: Creating example JavaScript libraries
I (...) mqjs-repl: Created math.js
I (...) mqjs-repl: Created string.js
I (...) mqjs-repl: Created array.js
I (...) mqjs-repl: Initializing JavaScript engine with 65536 bytes
I (...) mqjs-repl: JavaScript engine initialized successfully
I (...) mqjs-repl: Loading libraries from /spiffs/autoload
I (...) mqjs-repl: Loading JavaScript file: /spiffs/autoload/math.js
Error loading /spiffs/autoload/math.js: [mtag]: expecting ';'
    at /spiffs/autoload/math.js:1:5
```

## Code points involved

### SPIFFS init / format behavior

File: `imports/esp32-mqjs-repl/mqjs-repl/main/main.c`

- `init_spiffs()` uses:
  - `partition_label = "storage"`
  - `format_if_mount_failed = true`
  - calls `esp_vfs_spiffs_register(&conf)`

This means a mount failure will **format automatically**, which explains why we see “formatting…” and then a clean filesystem.

### Autoload directory + JS loader

File: `imports/esp32-mqjs-repl/mqjs-repl/main/main.c`

- `create_example_libraries()` writes three files into `/spiffs/autoload/*.js`
- `load_autoload_libraries()` scans that directory and calls `load_js_file()`
- `load_js_file()` reads content and calls:
  - `JS_Eval(js_ctx, content, size, filepath, 0)`

## Expected behavior

- On first boot in QEMU, it’s acceptable/expected that SPIFFS is uninitialized and gets formatted.
- After example libraries are created, they should load cleanly (or at least one minimal library should load) so the banner’s suggested calls actually work.

## Actual behavior

- SPIFFS mount fails and formats (may be fine, but needs confirmation).
- All three example libraries fail to parse with the same `[mtag]: expecting ';'` at `1:5`.
- The REPL banner still prints “Loaded libraries: …”, but this appears to be static text (not based on successful load).

## Hypotheses

### SPIFFS mount/format

1) **Expected**: `-10025` corresponds to “not a valid SPIFFS filesystem yet” and is normal for an empty partition; formatting is the correct behavior.
2) **Partition mismatch**: The `storage` partition may be absent/misaligned in some configs (this was previously observed when the build used `partitions_singleapp.csv`). If partition config regresses, mount failure becomes “real” not “expected”.

### JS parse errors

1) **MicroQuickJS parser differs from upstream QuickJS** in ways that reject our “simple” ES5.1-looking library syntax.
2) **File contents on disk differ from what we think** (e.g. missing/garbled bytes, encoding/line endings, partial writes). We should read back the file and log its first ~64 bytes for confirmation.
3) **`JS_Eval` flags / context configuration**: maybe the minimal stdlib or build options affect parsing. (Still: `var X = {};` should parse.)

## Next debugging steps (fresh session)

### SPIFFS behavior

1) Identify what `-10025` maps to in SPIFFS error codes (ESP-IDF) and document whether it’s expected for first boot.
2) Confirm the generated partition table includes `storage` and matches `partition_label="storage"` consistently in QEMU runs.

### JS autoload parse errors

1) Dump the first line / first bytes of each created file right after write (and again after reopen) to confirm actual content.
2) Create a minimal test file (e.g. `/spiffs/autoload/test.js` with `1+2;`) and see if it parses.
3) Compare `JS_Eval` error object output to see what `[mtag]` indicates (is it a token tag? a module tag?).
4) If needed, reduce library syntax until it parses, then build back up.

