# quickjs_native

`quickjs_native` vendors the core upstream QuickJS engine sources for native ESP-IDF builds.

## Source

- Upstream: Fabrice Bellard / Charlie Gordon QuickJS.
- Version: see `quickjs/VERSION`.
- Local source used for this first vendor pass: `0100-esp32-p4-quickjs-wasm/wasm-src/quickjs/`.
- License: see `quickjs/LICENSE` (MIT-style QuickJS license text).

## Included source set

The component includes the minimal engine source set that was already proven in the 0100 QuickJS-WASM host build:

- `quickjs.c`
- `quickjs.h`
- `quickjs-atom.h`
- `quickjs-opcode.h`
- `cutils.c` / `cutils.h`
- `dtoa.c` / `dtoa.h`
- `libregexp.c` / `libregexp.h`
- `libunicode.c` / `libunicode.h`
- `unicode_gen_def.h`
- `list.h`
- `libregexp-opcode.h`
- `libunicode-table.h`

## ESP-IDF compatibility notes

The component uses `quickjs_espidf_compat.h` as a forced include to declare `malloc_usable_size()`, which ESP-IDF provides but does not expose to QuickJS by default in this build.

The vendored `quickjs.c` has one local ESP-IDF portability patch: the timezone-offset helper uses the same `gmtime`/`localtime`/`mktime` fallback path as Windows when `ESP_PLATFORM` is defined, because ESP-IDF/newlib's `struct tm` does not provide `tm_gmtoff`.

ESP-IDF's default `-Werror=all` turns upstream QuickJS's `int`/`int32_t` pointer warnings into errors on this RISC-V/newlib configuration, so the component locally disables `-Werror=incompatible-pointer-types` while still preserving the warning output.

## Excluded for milestone 1

`quickjs-libc.c` is intentionally excluded. Firmware APIs should be explicit (`print`, `millis`, GPIO, display, keyboard, timers) instead of importing QuickJS's command-line `std`/`os` layer by default.

## Update procedure

1. Replace files under `quickjs/` from the selected upstream QuickJS release.
2. Keep `CMakeLists.txt` source list aligned with upstream requirements.
3. Build `0101-esp32-p4-native-quickjs`.
4. Run native smoke tests on ESP32-P4.
5. Record version and any source-list changes in the ticket diary.
