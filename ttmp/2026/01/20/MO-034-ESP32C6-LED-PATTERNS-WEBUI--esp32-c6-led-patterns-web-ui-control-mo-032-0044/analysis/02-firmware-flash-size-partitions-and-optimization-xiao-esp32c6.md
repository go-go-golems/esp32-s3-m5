---
title: Firmware flash size, partitioning, and space optimization (XIAO ESP32C6 / 0046)
---

# Firmware flash size, partitioning, and space optimization (XIAO ESP32C6 / 0046)

This document is a “textbook-style” walkthrough of:

- What flash size you actually have on **Seeed Studio XIAO ESP32C6**
- Why the **app partition** is nearly full even though the board has more flash
- How **ESP-IDF partitions** work (bootloader / partition table / app / data)
- How to **measure** firmware size and identify what consumes flash/RAM
- Practical **ways to reclaim space** (and how to decide what to optimize)

It is written specifically with tutorial `0046-xiao-esp32c6-led-patterns-webui` in mind, but the concepts apply to any ESP-IDF project.

## 1) What flash size does the XIAO ESP32C6 have?

The ESP32-C6 is a chip; the **flash is external** (on the module/board). Different modules have different flash sizes.

For the **Seeed Studio XIAO ESP32C6**, Seeed’s own specs indicate:

- **Flash:** 4MB
- **SRAM:** 512KB

Source: Seeed wiki spec table (see “Resources / Specification”).

### Why it looks like you have “only 1MB”

In ESP-IDF, your firmware is constrained by the size of the **selected app partition**, not by the total flash size on the module.

In `0046`, the current configuration is:

- `CONFIG_ESPTOOLPY_FLASHSIZE="2MB"` (so the build assumes 2MB flash)
- `CONFIG_PARTITION_TABLE_SINGLE_APP=y` (single “factory” app partition layout)
- App partition size is **1MB** (more on that below)

That combination explains why you see:

> “Smallest app partition is 0x100000 bytes … 1% free space left”

It does **not** mean the hardware only has 1MB flash.

## 2) Flash layout: bootloader, partition table, and partitions

### 2.1 What gets written to flash?

An ESP-IDF flash typically contains (minimum):

1) **Bootloader** (small program that loads your app)
2) **Partition table** (describes what’s in flash and where)
3) **App partition(s)** (your firmware image)
4) **Data partitions** (NVS, PHY init data, SPIFFS/FATFS, OTA data, …)

For ESP32-C6, the build output in `0046` prints the typical flash command:

- Bootloader at `0x0`
- Partition table at `0x8000`
- App at `0x10000`

### 2.2 Partition table format (CSV)

ESP-IDF partitions are usually defined in a CSV like:

```
# Name,   Type, SubType, Offset, Size, Flags
nvs,      data, nvs,     ,      0x6000,
phy_init, data, phy,     ,      0x1000,
factory,  app,  factory, ,      1M,
```

Key concepts:

- **Offset**: where the partition begins in flash
- **Size**: how many bytes are allocated to that partition
- If you leave Offset blank, ESP-IDF auto-places partitions sequentially with alignment.

### 2.3 What partitions does 0046 currently have?

The generated partition table (decoded from `build/partition_table/partition-table.bin`) is:

```
# Name, Type, SubType, Offset, Size, Flags
nvs,data,nvs,0x9000,24K,
phy_init,data,phy,0xf000,4K,
factory,app,factory,0x10000,1M,
```

Important: there is **no SPIFFS partition** here, and no OTA partitions — just a single 1MB factory app.

## 3) Why the app partition is “nearly full” in 0046

### 3.1 The warning is about the app partition, not total flash

ESP-IDF runs a size check that compares your built `.bin` to your **smallest app partition**:

```
xiao_esp32c6_led_patterns_webui_0046.bin binary size 0xfdc30 bytes.
Smallest app partition is 0x100000 bytes.
0x23d0 bytes (1%) free.
```

Translated:

- App partition size: `0x100000` = 1,048,576 bytes
- Firmware image size: `0x0fdc30` = 1,039,408 bytes
- Free in that partition: `0x23d0` = 9,168 bytes

### 3.2 “Why is the binary so big?”

There are three big reasons in this project:

1) **Wi‑Fi stacks are large** (net80211, lwIP, WPA supplicant, PHY/PP)
2) This project includes “comfort features” (console, HTTP server, JSON, etc.)
3) **Optimization is currently set to debug**, which is significantly larger than `-Os`:
   - `CONFIG_COMPILER_OPTIMIZATION_DEBUG=y`
   - `CONFIG_COMPILER_OPTIMIZATION_SIZE` is not set

That last point is the biggest “easy win”: debug optimization is great for stepping through code, but it bloats `.text` and often `.rodata`.

## 4) Measuring size: what to run and how to interpret it

All commands below are run from:

`0046-xiao-esp32c6-led-patterns-webui/`

### 4.1 Top-level memory summary: `idf.py size`

Run:

```
./build.sh size
```

Example output (current 0046 build):

- **Total image size:** ~1,039,299 bytes (the `.bin` may be padded slightly)
- **Flash Code:** 911,628 bytes
  - `.text`: 711,196 bytes
  - `.rodata`: 200,176 bytes
- **DIRAM:** 150,487 bytes
  - IRAM `.text`: 111,722 bytes
  - `.bss`: 22,816 bytes
  - `.data`: 15,949 bytes

How to use it:

- If “Flash Code” is too big: focus on feature reduction / optimization flags / removing embedded assets.
- If “DIRAM” is too big: focus on IRAM attribution, static allocations, and stack sizes.
- `.rodata` tends to be strings/tables; logging and embedded web bundles show up here.

### 4.2 Which libraries (“archives”) are largest: `idf.py size-components`

Run:

```
./build.sh size-components
```

Then look at the largest “Per-archive contributions to ELF file”.

Key rows in current 0046:

- `libnet80211.a`: ~190,878 bytes
- `liblwip.a`: ~114,050 bytes
- `libpp.a`: ~110,730 bytes
- `libwpa_supplicant.a`: ~78,931 bytes
- `libesp_http_server.a`: ~11,507 bytes
- `libmain.a` (your code + embedded assets): ~93,599 bytes

This confirms: Wi‑Fi and TCP/IP are the bulk, and your `main/` is not dominating.

### 4.3 Which object files are largest: `idf.py size-files`

Run:

```
./build.sh size-files
```

This can help you spot:

- Embedded assets (e.g. `app.js.S.obj`, `app.js.map.gz.S.obj`, `index.html.S.obj`)
- Large tables/strings in your own files
- When logging strings dominate (`.rodata.str1.4` sections)

Tip: treat this output as an attribution tool. Padding/alignment and linker script “dummy” sections can make some rows look odd; always cross-check against `.map` for suspicious outliers.

### 4.4 Inspecting the linker map file directly (`.map`)

The map file is the ground truth for layout and can answer questions like:

- “Which symbols are in `.flash.rodata`?”
- “Why is `.rodata` huge?”
- “Where did this padding come from?”

Location:

`0046-xiao-esp32c6-led-patterns-webui/build/xiao_esp32c6_led_patterns_webui_0046.map`

Good searches:

- `.flash.text`
- `.flash.rodata`
- `.rodata.str1.4`
- `*(.rodata_desc*)`
- asset objects like `app.js.S.obj`

## 5) Embedded web bundle: how much space does it actually consume?

The embedded web assets used by `0046` (current build outputs):

- `main/assets/index.html`: 379 bytes
- `main/assets/assets/app.js`: 33,501 bytes (~32.7 KiB)
- `main/assets/assets/app.css`: 498 bytes
- `main/assets/assets/app.js.map.gz`: 25,928 bytes (~25.3 KiB)

So the “web bundle payload” is only ~59 KiB total.

### Practical takeaway

If you’re just trying to get away from the “1% free” warning:

- Removing the sourcemap (`app.js.map.gz`) buys ~25 KiB immediately.

But the real fix is: **stop using a 1MB app partition** (and stop pretending flash is 2MB if the board is 4MB).

## 6) How to use the full 4MB flash on XIAO ESP32C6

There are two knobs:

1) **Flash size setting** (what the build/flasher assumes)
2) **Partition table** (how you allocate that flash)

### 6.1 Flash size setting

In ESP-IDF menuconfig:

- `Serial flasher config` → `Flash size` → set **4MB**

In `sdkconfig`, that corresponds to:

- `CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y`
- `CONFIG_ESPTOOLPY_FLASHSIZE="4MB"`

If you leave it at 2MB, you’re artificially constraining partition choices and may get confusing size behavior.

### 6.2 Choose a bigger app partition layout

In menuconfig:

- `Partition Table` → pick a layout that fits your needs:

Options to consider:

1) **Single app (large)**: one big factory app partition (no OTA)
   - Uses `partitions_singleapp_large.csv` (factory app is 1500K).
2) **Two OTA (large)**: two OTA slots for firmware updates
   - Uses `partitions_two_ota_large.csv` (two 1700K app partitions).
3) **Custom**: define your own CSV to allocate app + data as you want.

Rule of thumb:

- If you care about OTA updates: pick **two OTA**.
- If you don’t: pick **single app large** and allocate the rest to data partitions as needed.

## 7) Space optimization playbook (what to do first)

Optimization on embedded systems is about picking the highest ROI changes first.

### 7.1 Highest ROI: switch from debug optimization to size optimization

Current `0046` config shows:

- `CONFIG_COMPILER_OPTIMIZATION_DEBUG=y`
- `CONFIG_LOG_DEFAULT_LEVEL_INFO=y`

For a production-ish firmware, consider:

- `CONFIG_COMPILER_OPTIMIZATION_SIZE=y`
- Lower default logging level to WARN/ERROR as appropriate

This often yields substantial flash savings without changing features.

### 7.2 Medium ROI: remove or gate the sourcemap

Sourcemaps are great for debugging, but they’re not needed on-device most of the time.

Two patterns:

1) **Remove** embedding of `app.js.map.gz` entirely (always off).
2) **Gate** it behind a Kconfig flag, e.g. `CONFIG_MO033_HTTP_SERVE_SOURCEMAP`.

This saves ~25 KiB of app partition usage.

### 7.3 Medium ROI: reduce logging strings

Logging impacts flash in two ways:

- Code for logging paths
- `.rodata` strings (format strings and tags)

Knobs:

- Lower default log level (`CONFIG_LOG_DEFAULT_LEVEL_*`)
- Lower maximum log level (`CONFIG_LOG_MAXIMUM_LEVEL_*`)
- Remove very chatty logs in hot code paths

### 7.4 Targeted feature cuts (only if needed)

If you still need more room:

- Audit which ESP-IDF components you truly use.
- Disable unused features in menuconfig (Wi‑Fi extras, protocols, TLS features, etc.).
- Consider whether you really need:
  - `cJSON` vs a tiny fixed JSON parser
  - `esp_console` features (line editing/history/completions)
  - HTTPS / TLS / mbedTLS (if you only serve HTTP on LAN)

This is highly project-dependent and can be time-consuming; do it only after the high ROI steps.

## 8) Partitioning strategy for “web UI on device”

You have two broad approaches:

### A) Embed web assets in the app image (what 0046 does now)

Pros:

- Simple: one firmware image contains everything
- No separate filesystem to mount/manage

Cons:

- Web assets consume app partition flash
- Updating web assets requires reflashing firmware

### B) Store web assets in a data partition (SPIFFS/FATFS)

Pros:

- Web assets don’t consume app partition space
- Can update assets without changing the app (depending on update strategy)

Cons:

- More plumbing: mounting FS, file I/O, HTTP static file serving logic
- Adds partition complexity

On a 4MB XIAO ESP32C6, approach A is usually fine if you allocate a larger app partition.

## 9) Concrete recommendations for 0046 on XIAO ESP32C6

If the target is the XIAO ESP32C6 (4MB flash):

1) Set flash size to **4MB**
2) Switch partition layout to **single app large** (if no OTA), or **two OTA large** (if OTA is a goal)
3) Switch compiler optimization to **size** (`-Os`) for non-debug builds
4) Gate or remove sourcemap embedding for day-to-day builds

That should eliminate the “1% free” warning and give comfortable headroom for future features.

## 10) Sources

- Seeed Studio wiki: XIAO ESP32C6 getting started (spec table): https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/
- Store page (general board reference): https://www.seeedstudio.com/Seeed-Studio-XIAO-ESP32C6-p-5884.html
