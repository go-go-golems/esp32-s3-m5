### What’s going on (why `SHA_SIZE` is undefined)

`esp_insights` **v1.2.0** is published from commit **`6e3bca50a15dfb04fcd71151dede0dd63edcf609`**. ([components.espressif.com][1])
In its **dependency constraints**, it allows (among others):

* `espressif/esp_diag_data_store ~1.0` (i.e., *not pinned to a single minor/patch*) ([components.espressif.com][2])
* `espressif/esp_diagnostics >=1.2.0` ([components.espressif.com][2])

That means the component solver can legally pull in **newer** `esp_diag_data_store` / `esp_diagnostics` releases whose headers no longer provide the legacy identifiers that `esp_insights` v1.2.0 expects. This manifests exactly as:

* `error: 'SHA_SIZE' undeclared ... char sha_sum[2 * SHA_SIZE + 1];`
* plus the related struct/field mismatch (`rtc_store_non_critical_data_hdr_t` missing `dg`) ([GitHub][3])

This is also why a downstream workaround people used was to **pin** `esp_diag_data_store` back to **1.0.1**. ([GitHub][4])

---

### Which macro *should* be used in `esp_insights_cbor_encoder.c`?

From Espressif’s own API docs for **ESP Diagnostics**, there are two relevant macros:

* `DIAG_SHA_SIZE`
* `DIAG_HEX_SHA_SIZE` ([Espressif Systems][5])

Given the code pattern you cited:

```c
char sha_sum[2 * SHA_SIZE + 1];
```

…the intent is clearly: **hex string** (`2 * bytes`) + **NUL**. The cleanest “modern” replacement is therefore:

* **Prefer** `DIAG_HEX_SHA_SIZE` for the buffer size (since it already represents the hex length), i.e.:

  * `char sha_sum[DIAG_HEX_SHA_SIZE + 1];` ([Espressif Systems][5])
* Or equivalently (and closest to the old expression), use `DIAG_SHA_SIZE`:

  * `char sha_sum[2 * DIAG_SHA_SIZE + 1];` ([Espressif Systems][5])

Between your three candidates:

* **Not a “missing SHA_SIZE define”** (it’s a compatibility break due to dependency drift) ([GitHub][3])
* **`DIAG_SHA_SIZE` / `DIAG_HEX_SHA_SIZE` are the right public macros to key off** (documented and intended for this purpose). ([Espressif Systems][5])
* `RTC_STORE_SHA_SIZE` *may* exist in some `rtc_store.h` variants, but I couldn’t verify it directly from the upstream source view in this session (GitHub file browsing for those paths wasn’t reliably accessible via the web tool). So I can’t honestly “confirm” it the way I can confirm `DIAG_SHA_SIZE`. ([Espressif Systems][5])

---

### Expected `sha_sum` length in `rtc_store_meta_header_t`

Even without direct header access, the sizing expression `2 * SHA_SIZE + 1` strongly implies:

* `SHA_SIZE` is the **binary digest length** (bytes)
* the printable hex string is **2× that** (characters), plus NUL

And Espressif’s Diagnostics docs define exactly that split via `DIAG_SHA_SIZE` vs `DIAG_HEX_SHA_SIZE`. ([Espressif Systems][5])

So the expectation is:

* `rtc_store_meta_header_t.sha_sum` (binary) is **`DIAG_SHA_SIZE` bytes**
* when rendered as hex, you need **`DIAG_HEX_SHA_SIZE` chars** (+1 for `\0`) ([Espressif Systems][5])

(Practically, this is almost certainly SHA-256 → 32 bytes / 64 hex chars, but the *documented* confirmation we can point to here is the existence of the two macros, not the numeric literal.) ([Espressif Systems][5])

---

### Upstream signals (commits / versions) that show the “intended” direction

Two concrete, linkable facts from the component registry:

* `esp_insights` **v1.2.0** commit: **`6e3bca50a15d...`** ([components.espressif.com][1])
* `esp_insights` **v1.2.2** commit: **`36c8bfc71268...`** ([components.espressif.com][6])

And `esp_insights` **v1.2.2** pins dependencies much more tightly (notably `esp_diag_data_store 1.0.2` and `esp_diagnostics 1.2.1`), which is consistent with “don’t let the solver pull newer headers where these identifiers changed”. ([components.espressif.com][7])

Also, `esp_diag_data_store` **v1.1.0** is published from commit **`8828e662c153714a9b2a3389bf6f1c10d354ef41`**. ([components.espressif.com][8])
That’s the kind of newer dependency that `~1.0` can accidentally select, triggering the breakage seen in the esp-matter build logs. ([components.espressif.com][2])

---

### What I would do in your build (practical fixes)

1. **Best fix (no patching):** move from `esp_insights` **1.2.0 → 1.2.2+** so your dependency set is consistent. ([components.espressif.com][6])

2. **If you must stay on 1.2.0:** add explicit constraints in your project (like people did) to keep a compatible `esp_diag_data_store` / `esp_diagnostics` combination (e.g. pin `esp_diag_data_store` to 1.0.1 / 1.0.2). ([GitHub][4])

3. **If you want a one-line code patch:** replace `SHA_SIZE` usage with the documented macro:

   * `DIAG_HEX_SHA_SIZE` / `DIAG_SHA_SIZE` (recommended) ([Espressif Systems][5])

If you paste the ~30 lines around the `SHA_SIZE` usage (and where `rtc_store_meta_header_t` is referenced) I can give you the exact minimal patch that matches the struct field type you actually have in your build.

[1]: https://components.espressif.com/components/espressif/esp_insights/versions/1.2.0/readme?language= "
    espressif/esp_insights • v1.2.0
•
    ESP Component Registry"
[2]: https://components.espressif.com/components/espressif/esp_insights/versions/1.2.0/dependencies?language=en "
    espressif/esp_insights - 1.2.0 Dependencies
•
    ESP Component Registry"
[3]: https://github.com/espressif/esp-matter/issues/1115 "Build Failure : 'SHA_SIZE' and 'rtc_store_non_critical_data_hdr_t' Errors (Due to recent esp-Insights Release) (CON-1373) · Issue #1115 · espressif/esp-matter · GitHub"
[4]: https://github.com/espressif/esp-matter/issues/1114 "Build fails due to recent release of esp_insights (CON-1372) · Issue #1114 · espressif/esp-matter · GitHub"
[5]: https://docs.espressif.com/projects/esp-insights/en/main/esp32/esp_diagnostics.html?utm_source=chatgpt.com "ESP Diagnostics - ESP32"
[6]: https://components.espressif.com/components/espressif/esp_insights/versions/1.2.2/readme?language=en "
    espressif/esp_insights • v1.2.2
•
    ESP Component Registry"
[7]: https://components.espressif.com/components/espressif/esp_insights/versions/1.2.2/dependencies?language=en&utm_source=chatgpt.com "espressif/esp_insights - 1.2.2 Dependencies"
[8]: https://components.espressif.com/components/espressif/esp_diag_data_store "
    espressif/esp_diag_data_store • v1.1.0
•
    ESP Component Registry"

