---
Title: Analysis Design and Implementation Guide
Ticket: ATOMS3R-M12-QUICKJS-STORAGE
Status: active
Topics:
    - atoms3r
    - esp32s3
    - quickjs
    - javascript
    - firmware
    - storage
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0103-atoms3r-m12-native-quickjs/main/app_main.cpp
      Note: Mounts storage and installs namespace at startup
    - Path: 0103-atoms3r-m12-native-quickjs/main/js_command.cpp
      Note: Reinstalls storage namespace after js reset
    - Path: 0103-atoms3r-m12-native-quickjs/main/storage_namespace.cpp
      Note: Implemented bounded virtual-rooted FatFs console and QuickJS storage namespace
    - Path: 0103-atoms3r-m12-native-quickjs/main/storage_namespace.h
      Note: Storage startup/install/console API
    - Path: 0103-atoms3r-m12-native-quickjs/partitions.csv
      Note: Defines the 3 MiB FatFs storage partition
ExternalSources: []
Summary: Intern-facing guide for the AtomS3R M12 QuickJS bounded storage namespace.
LastUpdated: 2026-06-25T23:30:00-07:00
WhatFor: Use when reviewing, extending, or validating the `storage` console and QuickJS namespace in `0103-atoms3r-m12-native-quickjs`.
WhenToUse: Read before changing `storage_namespace.cpp`, adding `js run`, changing storage limits, or exposing additional filesystem operations to JavaScript.
---


# AtomS3R M12 QuickJS Storage Namespace — Analysis, Design, and Implementation Guide

## Executive summary

The `0103-atoms3r-m12-native-quickjs` firmware runs native QuickJS on an AtomS3R M12 / ESP32-S3 board with 8 MB PSRAM. The storage namespace adds a small persistent filesystem surface to that JavaScript runtime. It uses the existing flash partition named `storage`, mounts it as a FatFs filesystem at `/storage`, and exposes only three virtual roots to JavaScript: `/scripts`, `/data`, and `/tmp`.

The design is intentionally bounded. JavaScript cannot open arbitrary native paths, cannot keep file handles across garbage collection or `js reset`, and cannot read or write more than 16 KiB per call. This keeps the feature compatible with the 1 MiB QuickJS heap cap that was validated earlier on the AtomS3R M12.

The implementation already exists in commit `521d5a2`:

- `0103-atoms3r-m12-native-quickjs/main/storage_namespace.cpp`
- `0103-atoms3r-m12-native-quickjs/main/storage_namespace.h`

It also modifies:

- `0103-atoms3r-m12-native-quickjs/main/app_main.cpp`
- `0103-atoms3r-m12-native-quickjs/main/js_command.cpp`
- `0103-atoms3r-m12-native-quickjs/main/CMakeLists.txt`
- `0103-atoms3r-m12-native-quickjs/README.md`

## Current system context

The firmware is built around a native QuickJS service:

```text
USB Serial/JTAG console
        |
        v
esp_console commands
        |
        +--> js status/eval/reset/bench
        |       |
        |       v
        |   qjs_service queue
        |       |
        |       v
        |   QuickJS owner task owns JSRuntime and JSContext
        |
        +--> storage status/mount/list/read/write
                |
                v
            FatFs VFS at /storage
                |
                v
            flash partition: storage, data, fat, 3M
```

QuickJS itself must be accessed only on the owner task. Native firmware code that mutates the JavaScript global object uses `qjs_service_run()` so that all `JSContext*` operations happen on the owner task. The storage module follows that rule when it installs the `storage` namespace.

The filesystem operations are firmware-owned and protected by a FreeRTOS mutex. They are currently synchronous and bounded. The synchronous design is acceptable for the first milestone because reads and writes are limited to 16 KiB. If future writes become larger or long-running, move them to a separate storage worker and expose request/status APIs.

## Partition and mount model

The partition table reserves a 3 MiB data partition:

```csv
# Name,   Type, SubType, Offset,   Size, Flags
storage,  data, fat,     ,         3M,
```

The firmware mounts it at `/storage` with ESP-IDF's wear-levelled FatFs helper:

```c
esp_vfs_fat_spiflash_mount_rw_wl(
    "/storage",
    "storage",
    &mount_config,
    &s_wl_handle);
```

Use ESP-IDF component dependencies:

- `fatfs` for `esp_vfs_fat.h` and FatFs VFS helpers.
- `wear_levelling` for `wl_handle_t` and the flash wear-levelled driver.

Do not add a component named `esp_vfs_fat`. That is an API/header name, not an ESP-IDF component.

## First-boot policy

The firmware does not auto-format on startup. That is deliberate.

A blank storage partition produces:

```text
f_mount failed (13)
storage mount skipped/failed: ESP_FAIL
```

The operator must explicitly run:

```text
storage mount format
```

This protects user scripts from being erased by a transient mount error. It also makes test runs reproducible: formatting is a visible operator decision, not hidden startup behavior.

## JavaScript API

The namespace is installed after boot and after `js reset`.

```js
storage.status()
storage.list(path)
storage.stat(path)
storage.readText(path)
storage.writeText(path, text)
```

### `storage.status()`

Returns metadata and limits.

```js
storage.status()
// {
//   mounted: true,
//   mountPoint: "/storage",
//   partition: "storage",
//   lastMountError: "ESP_OK",
//   maxReadBytes: 16384,
//   maxWriteBytes: 16384,
//   maxListEntries: 64
// }
```

### `storage.writeText(path, text)`

Writes a complete text payload to a virtual-rooted path.

```js
storage.writeText('/scripts/demo.js', 'print(123)')
// { ok: true, bytes: 10 }
```

### `storage.readText(path)`

Reads a complete text payload, capped at 16 KiB.

```js
storage.readText('/scripts/demo.js')
// "print(123)"
```

### `storage.list(path)`

Lists up to 64 entries.

```js
storage.list('/scripts')
// [{ name: 'DEMO.JS', type: 'file', size: 10 }]
```

FatFs may return short filenames in uppercase. Do not assume directory listing preserves input casing.

### `storage.stat(path)`

Returns type and size.

```js
storage.stat('/scripts/demo.js')
// { type: 'file', size: 10 }
```

## Console API

The console command is for diagnostics and recovery:

```text
storage status
storage mount [format]
storage list /scripts
storage read /scripts/demo.js
storage write /scripts/demo.js "print('hi')"
```

The console path matters because it remains available even if JavaScript scripts are broken.

## Virtual path rules

Only these roots are accepted:

| JavaScript root | Native path | Purpose |
|---|---|---|
| `/scripts` | `/storage/scripts` | User scripts and examples. |
| `/data` | `/storage/data` | JSON state, small logs, app data. |
| `/tmp` | `/storage/tmp` | Scratch data that firmware may clear. |

Rejected path features:

- missing leading `/`
- roots outside `/scripts`, `/data`, `/tmp`
- `..`
- `.` segments
- repeated separators used to escape roots
- backslashes
- colons
- native absolute paths such as `/storage/...`

Pseudocode:

```c
validate(path):
    if path does not start with "/": reject
    if len(path) > 127: reject
    if root not in {"/scripts", "/data", "/tmp"}: reject
    if contains "..", "/./", "//", "\\", ":": reject
    return OK

native_path(path):
    validate(path)
    return "/storage" + path
```

## Memory and timing limits

| Limit | Current value | Reason |
|---|---:|---|
| Read text | 16 KiB | Keeps QuickJS strings small under 1 MiB heap cap. |
| Write text | 16 KiB | Bounds temporary buffers and flash write time. |
| Directory entries | 64 | Bounds JavaScript arrays and per-entry objects. |
| Path length | 127 bytes | Keeps fixed buffers small and simple. |
| Open handles | none | Avoids handle lifetime bugs across `js reset` and GC. |

The first hardware smoke showed a 10-byte write took 288 ms from JavaScript. Flash writes can be slow even when bounded. Large writes should become asynchronous before increasing limits.

## Implementation walkthrough

### Startup

`app_main.cpp` starts storage before QuickJS:

```c
storage_err = storage_namespace_start(false);
log_memory_baseline("after_storage");
svc = start_quickjs_service();
install_system_namespace(svc);
install_storage_namespace(svc);
```

Startup uses `format_if_mount_failed=false`. If the partition is blank, the firmware logs the failure and continues. QuickJS still starts, and `storage.status().mounted` reports `false`.

### Reset

`js reset` recreates the QuickJS runtime. Any firmware-provided globals disappear unless they are reinstalled.

`js_command.cpp` therefore does:

```c
qjs_service_reset(g_svc, timeout);
install_system_namespace(g_svc);
install_storage_namespace(g_svc);
```

### QuickJS namespace installation

`install_storage_namespace()` creates a `qjs_job_t` and calls `qjs_service_run()`. The job runs on the QuickJS owner task, creates the `storage` object, attaches C functions, calls `JS_PreventExtensions()`, and defines the global property.

```c
storage = JS_NewObject(ctx);
set_function(ctx, storage, "status", js_storage_status, 0);
set_function(ctx, storage, "list", js_storage_list, 1);
set_function(ctx, storage, "stat", js_storage_stat, 1);
set_function(ctx, storage, "readText", js_storage_read_text, 1);
set_function(ctx, storage, "writeText", js_storage_write_text, 2);
JS_PreventExtensions(ctx, storage);
JS_DefinePropertyValueStr(ctx, global, "storage", storage, JS_PROP_ENUMERABLE);
```

## Validation checklist

Build:

```bash
source /home/manuel/esp/esp-idf-5.4.2/export.sh
cd 0103-atoms3r-m12-native-quickjs
idf.py --no-hints build
```

Flash/monitor:

```bash
PORT=/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_B4:3A:45:BE:16:80-if00
idf.py --no-hints -p "$PORT" flash monitor
```

Smoke:

```text
storage status
storage mount format        # only on a blank development partition
storage write /scripts/demo.js print123
storage read /scripts/demo.js
js eval "storage.writeText('/scripts/jsdemo.js','print(456)').bytes"
js eval "storage.readText('/scripts/jsdemo.js')"
js eval "JSON.stringify(storage.list('/scripts'))"
js eval "JSON.stringify(storage.stat('/scripts/jsdemo.js'))"
js reset
js eval "storage.readText('/scripts/jsdemo.js')"
```

Expected result: read/write/list/stat succeed, reset preserves the namespace, and board reset preserves the file.

## Decision records

### Decision: Use FatFs on the existing flash partition

- **Context:** `0103` already reserves a `data,fat` partition.
- **Decision:** Use wear-levelled FatFs at `/storage`.
- **Rationale:** FatFs supports directories and is appropriate for a small script/data tree. SPIFFS directory semantics would be weaker.
- **Status:** accepted.

### Decision: Do not auto-format at startup

- **Context:** A blank partition cannot mount until formatted.
- **Decision:** Startup mounts without formatting; operators use `storage mount format` explicitly.
- **Rationale:** Prevents accidental script/data loss after a mount failure.
- **Status:** accepted.

### Decision: No JavaScript file handles in milestone 1

- **Context:** Handles need lifetime management across GC and reset.
- **Decision:** Expose whole-file bounded functions only.
- **Rationale:** Simpler and safer under a 1 MiB QuickJS cap.
- **Status:** accepted.

## Future work

- Add `js run <virtual-path>` with filename-aware eval errors.
- Add an optional read-only asset root for bundled flash assets.
- Add asynchronous write jobs before increasing the 16 KiB limit.
- Normalize or document FatFs uppercase short names in UI code.
- Add storage quota/status fields if the HTTP server needs upload support.

## References

- `0103-atoms3r-m12-native-quickjs/main/storage_namespace.cpp`
- `0103-atoms3r-m12-native-quickjs/main/storage_namespace.h`
- `0103-atoms3r-m12-native-quickjs/main/app_main.cpp`
- `0103-atoms3r-m12-native-quickjs/main/js_command.cpp`
- `0103-atoms3r-m12-native-quickjs/partitions.csv`
- `0103-atoms3r-m12-native-quickjs/README.md`
