---
Title: Diary
Ticket: 001-ZIGBEE-GATEWAY
Status: active
Topics:
    - zigbee
    - esp-idf
    - esp32
    - esp32h2
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../esp/esp-5.4.1/examples/openthread/ot_rcp/build/esp_ot_rcp.bin
      Note: Built RCP firmware artifact (ESP32-H2)
    - Path: ../../../../../../../../../../esp/esp-5.4.1/examples/openthread/ot_rcp/sdkconfig
      Note: Confirms OPENTHREAD_NCP_VENDOR_HOOK enabled for the ESP32-H2 RCP build
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/examples/zigbee/esp_zigbee_gateway/build/esp_zigbee_gateway.bin
      Note: ESP-IDF Zigbee gateway build artifact
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/examples/esp_zigbee_gateway/build/esp_zigbee_gateway.bin
      Note: Built gateway firmware artifact (ESP32-S3)
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/examples/esp_zigbee_gateway/main/esp_zigbee_gateway.c
      Note: Upstream example modified to enable CoreS3 Grove power (AW9523 I2C write)
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/examples/esp_zigbee_gateway/sdkconfig.defaults
      Note: Upstream example defaults adjusted for CoreS3↔RCP UART pins and disabling auto-update
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/examples/esp_zigbee_ncp/build/esp_zigbee_ncp.bin
      Note: Built and flashed NCP firmware artifact
    - Path: ../../../../../../../../../../esp/esp-zigbee-sdk/examples/esp_zigbee_ncp/sdkconfig.defaults
      Note: Set H2 NCP UART pins (RX=23
    - Path: TX
      Note: 24) per M5Stack guide
    - Path: esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/index.md
      Note: Ticket index and file relationships
    - Path: esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/playbook/01-esp-zigbee-gateway-unit-gateway-h2-cores3.md
      Note: Primary bring-up playbook for the gateway setup
    - Path: esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/reference/02-rcp-spinel-vs-ncp-znp-style-on-unit-gateway-h2.md
      Note: Doc written in Step 6 (RCP+Spinel vs NCP/ZNP comparison)
    - Path: esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/sources/m5stack_zigbee_gateway.txt
      Note: Upstream guide snapshot used as source material
    - Path: esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/sources/m5stack_zigbee_ncp.txt
      Note: Archived source referenced in Step 6
ExternalSources:
    - https://docs.m5stack.com/en/esp_idf/zigbee/unit_gateway_h2/zigbee_gateway:M5Stack guide (Unit Gateway H2 + CoreS3, ESP Zigbee Gateway)
    - https://github.com/espressif/esp-zigbee-sdk/tree/master/examples/esp_zigbee_gateway:Upstream example and expected logs
Summary: Backfilled implementation diary for bringing up the ESP Zigbee Gateway reference setup (CoreS3 + Unit Gateway H2) using ESP-IDF 5.4.1.
LastUpdated: 2026-01-04T21:05:00Z
WhatFor: Capture the exact commands, decisions, and gotchas while bringing up Zigbee gateway firmware.
WhenToUse: When repeating bring-up, debugging boot/flash issues, or reviewing why specific config choices were made.
---




# Diary

## Goal

Backfill the work done so far for ticket `001-ZIGBEE-GATEWAY`, and continue recording build/flash steps as we bring up the gateway.

## Step 1: Create the ticket workspace + playbook skeleton

This step established the documentation workspace and recorded a repeatable bring-up procedure based on the M5Stack guide, but adapted to the local requirement of using ESP-IDF 5.4.1. I also archived the upstream guide content inside the ticket so future work can refer to a stable snapshot even if the web page changes.

### What I did
- Read `/home/manuel/.cursor/commands/docmgr.md` and `/home/manuel/.cursor/commands/diary.md` to follow local documentation workflows.
- Added `zigbee` and `esp32h2` topics to docmgr vocabulary.
- Created ticket `001-ZIGBEE-GATEWAY`.
- Downloaded the upstream page HTML and produced a text dump using `lynx -dump`.
- Created the playbook doc and filled it with the bring-up steps.

### Why
- Standardize the bring-up commands and keep them close to the repo.
- Keep a local snapshot of upstream docs for reproducibility.

### What worked
- `docmgr ticket create-ticket` created the expected workspace under `esp32-s3-m5/ttmp/...`.
- The archived sources (`.html` + `.txt`) were successfully stored in `sources/`.
- `docmgr doctor` passed for the ticket.

### What didn't work
- N/A

### What I learned
- The docmgr workspace root is `esp32-s3-m5/ttmp` (per `.ttmp.yaml`), not repo root.

### What was tricky to build
- Converting a Nuxt-rendered docs page into readable text; `lynx -dump` was the most reliable approach.

### What warrants a second pair of eyes
- Confirm the playbook’s `menuconfig` guidance matches CoreS3 + Unit Gateway H2 wiring for this hardware revision.

### What should be done in the future
- If M5Stack changes their guide, re-download the page and diff against `sources/m5stack_zigbee_gateway.txt`. N/A otherwise.

### Code review instructions
- Start in the playbook: `.../playbook/01-esp-zigbee-gateway-unit-gateway-h2-cores3.md`.
- Confirm the external source links and related files on the ticket index.

### Technical details
Commands used (high level):
```bash
docmgr vocab add --category topics --slug zigbee --description "..."
docmgr vocab add --category topics --slug esp32h2 --description "..."
docmgr ticket create-ticket --ticket 001-ZIGBEE-GATEWAY --title "..." --topics zigbee,esp-idf,esp32,esp32h2
curl -L https://docs.m5stack.com/en/esp_idf/zigbee/unit_gateway_h2/zigbee_gateway -o .../sources/m5stack_zigbee_gateway.html
lynx -dump -nolist .../sources/m5stack_zigbee_gateway.html > .../sources/m5stack_zigbee_gateway.txt
docmgr doc add --ticket 001-ZIGBEE-GATEWAY --doc-type playbook --title "ESP Zigbee Gateway (Unit Gateway H2 + CoreS3)"
```

### What I'd do differently next time
- Capture the exact `curl` timestamp/etag in the sources metadata if we care about long-term provenance.

## Step 2: Prepare local toolchain inputs (ESP-IDF path + esp-zigbee-sdk)

This step ensured the exact ESP-IDF path requested by the ticket exists and cloned the SDK repo that contains the gateway example. I also inspected the example’s build behavior to understand what it expects (especially the optional auto-RCP-update flow).

### What I did
- Verified ESP-IDF 5.4.1 exists at `/home/manuel/esp/esp-idf-5.4.1`.
- Created a compatibility symlink `/home/manuel/esp/esp-5.4.1 -> /home/manuel/esp/esp-idf-5.4.1` (ticket requirement).
- Cloned `https://github.com/espressif/esp-zigbee-sdk.git` into `/home/manuel/esp/esp-zigbee-sdk`.
- Read `examples/esp_zigbee_gateway/README.md` and `main/CMakeLists.txt`.

### Why
- The ticket explicitly wants ESP-IDF 5.4.1 at `~/esp/esp-5.4.1`.
- The gateway firmware is built from `esp-zigbee-sdk/examples/esp_zigbee_gateway`.

### What worked
- `esp-zigbee-sdk` clone succeeded and the example project was present.
- The example’s build is compatible with ESP-IDF 5.4.1 and pulls dependencies via `idf.py`.

### What didn't work
- N/A

### What I learned
- `main/CMakeLists.txt` only generates the embedded RCP firmware image if `CONFIG_ZIGBEE_GW_AUTO_UPDATE_RCP=y`; otherwise the gateway build does not require a prebuilt `ot_rcp` image.

### What was tricky to build
- Keeping paths consistent between the playbook (docs) and the actual disk layout (ESP-IDF installation naming).

### What warrants a second pair of eyes
- Whether we should avoid modifying upstream `esp-zigbee-sdk` files and instead maintain a local copy under the workspace (tradeoff: reproducibility vs upstream drift).

### What should be done in the future
- If we decide to keep upstream pristine, fork/patch with a dedicated overlay directory and reference it from the playbook.

### Code review instructions
- Confirm the symlink is correct: `/home/manuel/esp/esp-5.4.1`.
- Review the playbook’s assumptions about `esp-zigbee-sdk` location.

### Technical details
```bash
ln -s ~/esp/esp-idf-5.4.1 ~/esp/esp-5.4.1
git clone --recursive https://github.com/espressif/esp-zigbee-sdk.git ~/esp/esp-zigbee-sdk
```

### What I'd do differently next time
- Capture the exact `esp-zigbee-sdk` commit hash when cloning (for strict reproducibility).

## Step 3: Build the CoreS3 gateway firmware (ESP32-S3) and hit a port mismatch

This step built the `esp_zigbee_gateway` firmware successfully for `esp32s3`. When trying to proceed to flashing, I discovered the only attached device was actually the ESP32-H2 (RCP) presenting as USB-Serial/JTAG on `/dev/ttyACM0`, so the gateway flash could not proceed until the CoreS3 port is available.

### What I did
- Enumerated serial ports under `/dev/serial/by-id/` and `/dev/ttyACM*`.
- Confirmed chip type on `/dev/ttyACM0` via `esptool.py chip_id` (ESP32-H2).
- Updated `esp_zigbee_gateway` example defaults to match the M5 guide’s CoreS3 pin settings:
  - Disable auto RCP update (since reset/boot pins set to `-1`)
  - Set UART pins to TX=18 / RX=17
- Added CoreS3 Grove power enable helper (`fix_aw9523_p0_pull_up()`) into `app_main()` (per M5 guide guidance).
- Built the gateway firmware for `esp32s3` with ESP-IDF 5.4.1.

### Why
- Pin config must match the CoreS3 ↔ Unit Gateway H2 wiring.
- The M5 guide explicitly calls out the Grove power enable function as required for CoreS3.

### What worked
- `idf.py set-target esp32s3 && idf.py build` succeeded for the gateway.
- Chip detection showed `/dev/ttyACM0` is ESP32-H2, explaining why flashing the ESP32-S3 gateway to that port would be wrong.

### What didn't work
- Could not flash the gateway firmware because the CoreS3 (ESP32-S3) serial port was not present; only the ESP32-H2 port was attached.

### What I learned
- USB-Serial/JTAG presents as a stable `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_...` symlink; it’s the preferred target for flashing/monitor.

### What was tricky to build
- Ensuring we don’t accidentally flash an ESP32-S3 image to the ESP32-H2 device (wrong target).

### What warrants a second pair of eyes
- The decision to modify upstream `esp-zigbee-sdk` in-place (vs. a local project copy).
- The correctness of the AW9523 I2C write for this CoreS3 hardware revision.

### What should be done in the future
- When the CoreS3 is attached, confirm chip type is `ESP32-S3` before flashing.

### Code review instructions
- Review changes in:
  - `/home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_gateway/main/esp_zigbee_gateway.c`
  - `/home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_gateway/sdkconfig.defaults`
- Build output should contain `build/esp_zigbee_gateway.bin`.

### Technical details
Chip detection:
```bash
source ~/esp/esp-5.4.1/export.sh
python ~/esp/esp-5.4.1/components/esptool_py/esptool/esptool.py --port /dev/ttyACM0 chip_id
```

Gateway build:
```bash
source ~/esp/esp-5.4.1/export.sh
cd ~/esp/esp-zigbee-sdk/examples/esp_zigbee_gateway
idf.py set-target esp32s3
idf.py build
```

### What I'd do differently next time
- Standardize ports as env vars in a local `scripts/` helper (like other tickets), to reduce operator error.

## Step 4: Build the Unit Gateway H2 firmware (ESP32-H2 / ot_rcp)

This step switches focus to the Unit Gateway H2 itself. The attached device enumerates as ESP32-H2 on `/dev/ttyACM0`, so I built the ESP-IDF `ot_rcp` example for `esp32h2` using ESP-IDF 5.4.1.

### What I did
- Built `~/esp/esp-5.4.1/examples/openthread/ot_rcp` for `esp32h2` using `idf.py set-target esp32h2` and `idf.py build`.
- Verified that `CONFIG_OPENTHREAD_NCP_VENDOR_HOOK=y` ended up enabled in the generated `sdkconfig`.
- Confirmed the default UART baud rate remains `460800` in `main/esp_ot_config.h` (no change applied yet).

### Why
- The Zigbee gateway uses an 802.15.4 RCP on ESP32-H2; we need a built image before flashing.

### What worked
- Build completed successfully and produced `build/esp_ot_rcp.bin`.
- `CONFIG_OPENTHREAD_NCP_VENDOR_HOOK` is enabled (required by the gateway example).

### What didn't work
- N/A

### What I learned
- `CONFIG_OPENTHREAD_NCP_VENDOR_HOOK` is `default y` in ESP-IDF 5.4.1 and does end up enabled in `ot_rcp`’s `sdkconfig` by default.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- Whether to change the default UART baud rate (460800) to 230400 preemptively; M5 recommends lowering only if long cable issues appear.

### What should be done in the future
- If we see UART instability between CoreS3 and H2, apply the baud rate change in `main/esp_ot_config.h` and rebuild.

### Code review instructions
- Verify build steps and resulting artifacts under `~/esp/esp-5.4.1/examples/openthread/ot_rcp/build`.

### Technical details
Command sequence used:
```bash
source ~/esp/esp-5.4.1/export.sh
cd ~/esp/esp-5.4.1/examples/openthread/ot_rcp
idf.py set-target esp32h2
idf.py build
```

Build artifacts:
- `~/esp/esp-5.4.1/examples/openthread/ot_rcp/build/esp_ot_rcp.bin`

Config checks:
```bash
rg -n "CONFIG_OPENTHREAD_NCP_VENDOR_HOOK" ~/esp/esp-5.4.1/examples/openthread/ot_rcp/sdkconfig
rg -n "baud_rate" ~/esp/esp-5.4.1/examples/openthread/ot_rcp/main/esp_ot_config.h
```

Next step (not done in this step): flash to the H2 unit:
```bash
source ~/esp/esp-5.4.1/export.sh
cd ~/esp/esp-5.4.1/examples/openthread/ot_rcp
idf.py -p /dev/ttyACM0 flash
```

## Step 5: Flash `ot_rcp` to the Unit Gateway H2 (ESP32-H2)

This step flashed the built `ot_rcp` image to the attached Unit Gateway H2. The device was reachable via USB-Serial/JTAG at `/dev/ttyACM0` (also visible as `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_48:31:B7:CA:45:5B-if00`).

### What I did
- Flashed the RCP firmware using `idf.py -p /dev/ttyACM0 flash` from the `ot_rcp` project directory.
- Tried to run `idf.py monitor` to confirm runtime logs, but `idf_monitor` failed because stdin wasn’t a TTY in this execution environment.

### Why
- Program the ESP32-H2 RCP so the CoreS3 gateway can talk to it over UART via Spinel.

### What worked
- Flash succeeded and verified hashes.

Key flash output (high signal):
```text
Serial port /dev/ttyACM0
Chip is ESP32-H2 (revision v0.1)
USB mode: USB-Serial/JTAG
BASE MAC: 48:31:b7:ca:45:5b
Wrote 203264 bytes (136576 compressed) at 0x00010000 ... Hash of data verified.
Leaving... Hard resetting via RTS pin... Done
```

### What didn't work
- `idf.py -p /dev/ttyACM0 monitor` failed with:
  - `Error: Monitor requires standard input to be attached to TTY. Try using a different terminal.`

### What I learned
- `idf_monitor` needs an interactive terminal (TTY). For non-interactive runs, use a real terminal session or capture UART output with another tool.

### What was tricky to build
- Avoiding target/port confusion: `/dev/ttyACM0` is the ESP32-H2 (not the CoreS3 / ESP32-S3).

### What warrants a second pair of eyes
- Confirm that the Unit Gateway H2 is indeed the device on `/dev/ttyACM0` (ESP32-H2) and not a misidentified attachment during later bring-up.

### What should be done in the future
- When doing an interactive session, run monitor from a real terminal:
  - `source ~/esp/esp-5.4.1/export.sh && cd ~/esp/esp-5.4.1/examples/openthread/ot_rcp && idf.py -p /dev/ttyACM0 monitor`

### Code review instructions
- N/A (no repo code changes in this step; this is a device flash action).

### Technical details
Command used:
```bash
source ~/esp/esp-5.4.1/export.sh
cd ~/esp/esp-5.4.1/examples/openthread/ot_rcp
idf.py set-target esp32h2
idf.py -p /dev/ttyACM0 flash
```

Monitor attempt logs were written by `idf.py` to:
- `~/esp/esp-5.4.1/examples/openthread/ot_rcp/build/log/idf_py_stdout_output_2440206`
- `~/esp/esp-5.4.1/examples/openthread/ot_rcp/build/log/idf_py_stderr_output_2440206`

## Step 6: Research and document RCP+Spinel vs NCP (ZNP-style), and compare M5Stack guides

This step clarified the conceptual confusion around “RCP/Spinel” versus “NCP/ZNP” by reading both M5Stack guides and the upstream Espressif example READMEs. The deliverable is a reference document that explains what runs where, what protocol goes over UART, and how the two M5Stack tutorials differ (they are not two variants of the same thing; they are two different architectures).

### What I did
- Downloaded and archived the M5Stack Zigbee NCP page (`zigbee_ncp`) into the ticket `sources/` directory (HTML + `lynx -dump` text).
- Read the upstream Espressif READMEs:
  - `esp_zigbee_gateway` (RCP-based gateway)
  - `esp_zigbee_ncp` (NCP firmware)
  - `esp_zigbee_host` (host firmware)
- Read the host↔NCP frame header definition `esp_host_frame.h` to confirm this is not Spinel (it’s an EGSP-like frame format: version/type/id/seq/len).
- Wrote a full reference document explaining:
  - RCP + Spinel data/control flow and failure modes
  - NCP host-protocol data/control flow and how it matches the classic “ZNP” pattern
  - A direct comparison of the M5Stack `zigbee_gateway` vs `zigbee_ncp` tutorials (pins, firmware roles, protocols)

### Why
- We flashed `ot_rcp` to the H2 (RCP architecture). If we accidentally follow the M5Stack NCP guide, we’ll end up flashing different firmware (`esp_zigbee_ncp`) and the host would need to run `esp_zigbee_host` instead of `esp_zigbee_gateway`.
- “Stock firmware” on the H2 is only useful if it implements the exact UART protocol the host expects (Spinel vs Espressif NCP frames).

### What worked
- `curl` + `lynx -dump` produced a stable local snapshot of the M5Stack NCP guide content.
- The reference doc ended up with concrete, source-backed differences (pin configs and which examples to use).

### What didn't work
- N/A

### What I learned
- The two M5Stack guides correspond to two different architectures:
  - `zigbee_gateway`: RCP (`ot_rcp`) on H2 + gateway (`esp_zigbee_gateway`) on CoreS3, with Spinel over UART.
  - `zigbee_ncp`: NCP (`esp_zigbee_ncp`) on H2 + host (`esp_zigbee_host`) on CoreS3, with a custom host↔NCP protocol over UART.
- Espressif’s NCP wire protocol is explicitly framed and command-ID driven (more like ZNP/EZSP patterns than Spinel).

### What was tricky to build
- Terminology collision: “NCP” exists in Thread (often Spinel-based) and in Zigbee (often ZNP-like), but the M5Stack “Zigbee NCP” guide is clearly the “stack-on-coprocessor” model with a custom host protocol.

### What warrants a second pair of eyes
- Confirm the pin numbers in the M5Stack guides match our physical wiring and the specific CoreS3/Unit Gateway H2 revision:
  - NCP guide: H2 UART RX=23 / TX=24, host UART RX=18 / TX=17
  - Gateway guide: host RCP TX=18 / RX=17

### What should be done in the future
- If we decide to switch architectures (RCP ↔ NCP), explicitly update the playbook and re-flash the H2 accordingly; don’t mix-and-match firmware from the two guides.

### Code review instructions
- Read the new reference doc:
  - `.../reference/02-rcp-spinel-vs-ncp-znp-style-on-unit-gateway-h2.md`

### Technical details
Sources captured:
- `.../sources/m5stack_zigbee_ncp.html`
- `.../sources/m5stack_zigbee_ncp.txt`

## Step 7: Upload ticket docs to reMarkable

This step published the key ticket documents to the reMarkable tablet as PDFs so they’re easy to read and annotate. I used the standard uploader script (`remarkable_upload.py`) with “mirror ticket structure” enabled to avoid filename collisions under the date folder.

### What I did
- Ran a dry-run to confirm remote destination and filenames.
- Uploaded:
  - Ticket index
  - RCP gateway playbook
  - Diary
  - RCP vs NCP reference doc

### Why
- Make the docs readable on the device and keep the workflow consistent with other tickets.

### What worked
- Uploads succeeded with `rmapi put`.

### What didn't work
- N/A

### What I learned
- Using `--mirror-ticket-structure` keeps docs organized under `ai/YYYY/MM/DD/<ticket-root>/...`.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- If any of these PDFs are annotated on-device, avoid `--force` overwrites unless you’ve downloaded/exported annotations first.

### Code review instructions
- N/A

### Technical details
Command used:
```bash
TICKET_DIR="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2"
python3 /home/manuel/.local/bin/remarkable_upload.py \
  --ticket-dir "$TICKET_DIR" \
  --mirror-ticket-structure \
  "$TICKET_DIR/index.md" \
  "$TICKET_DIR/playbook/01-esp-zigbee-gateway-unit-gateway-h2-cores3.md" \
  "$TICKET_DIR/reference/01-diary.md" \
  "$TICKET_DIR/reference/02-rcp-spinel-vs-ncp-znp-style-on-unit-gateway-h2.md"
```

Remote destination:
- `ai/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/`

## Step 8: Build + flash the Zigbee NCP firmware (`esp_zigbee_ncp`) to the H2

This step switched the H2 from the RCP firmware (`ot_rcp`) to the Zigbee NCP firmware (`esp_zigbee_ncp`). This overwrites the RCP image, which is expected if we are following the M5Stack `zigbee_ncp` tutorial (NCP architecture).

### What I did
- Updated `~/esp/esp-zigbee-sdk/examples/esp_zigbee_ncp/sdkconfig.defaults` to set the UART pins per the M5Stack NCP guide:
  - `CONFIG_NCP_BUS_UART_RX_PIN=23`
  - `CONFIG_NCP_BUS_UART_TX_PIN=24`
- Built the NCP firmware for `esp32h2`.
- Erased and flashed the NCP firmware to the H2 on `/dev/ttyACM0`.

### Why
- The NCP architecture requires the H2 to run the full Zigbee stack + NCP bus/protocol, not Spinel RCP firmware.

### What worked
- Build succeeded and produced `build/esp_zigbee_ncp.bin`.
- `idf.py -p /dev/ttyACM0 erase-flash` and `idf.py -p /dev/ttyACM0 flash` succeeded.

### What didn't work
- N/A

### What I learned
- The NCP example’s UART pin config lives under `CONFIG_NCP_BUS_UART_{RX,TX}_PIN` (from `components/esp-zigbee-ncp/Kconfig`), so setting those in `sdkconfig.defaults` is the simplest non-interactive way to match the M5Stack guide.

### What was tricky to build
- Ensuring we don’t mix architectures: once the H2 is flashed with NCP firmware, the CoreS3 must run `esp_zigbee_host` (not `esp_zigbee_gateway`).

### What warrants a second pair of eyes
- Confirm that GPIO23/GPIO24 are indeed the correct CoreS3↔H2 UART pins for the Unit Gateway H2 hardware revision (per M5Stack docs).

### What should be done in the future
- If we want to return to the RCP gateway architecture, re-flash `ot_rcp` to the H2 and switch the CoreS3 firmware accordingly.

### Code review instructions
- N/A (build/flash actions; small config change in upstream example defaults).

### Technical details
Build:
```bash
source ~/esp/esp-5.4.1/export.sh
cd ~/esp/esp-zigbee-sdk/examples/esp_zigbee_ncp
idf.py set-target esp32h2
idf.py build
```

Flash:
```bash
source ~/esp/esp-5.4.1/export.sh
cd ~/esp/esp-zigbee-sdk/examples/esp_zigbee_ncp
idf.py -p /dev/ttyACM0 erase-flash
idf.py -p /dev/ttyACM0 flash
```

## Step 9: Build the ESP-IDF Zigbee gateway example (5.4.1)

This step built Espressif’s Zigbee gateway example that ships inside ESP-IDF 5.4.1 (`examples/zigbee/esp_zigbee_gateway`). ESP-IDF 5.4.1 does not ship a Zigbee “NCP firmware example” equivalent to `esp_zigbee_ncp`; that NCP firmware example is in `esp-zigbee-sdk`.

### What I did
- Built `~/esp/esp-5.4.1/examples/zigbee/esp_zigbee_gateway` for `esp32s3`.

### Why
- Provide a baseline “ESP-IDF built-in” Zigbee gateway build artifact and confirm compatibility with our installed ESP-IDF toolchain.

### What worked
- Build succeeded and produced `build/esp_zigbee_gateway.bin`.

### What didn't work
- N/A

### What I learned
- The ESP-IDF Zigbee gateway example pulls `esp-zboss-lib` + `esp-zigbee-lib` via component manager, similar to the SDK example.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- If the intent was specifically “ESP-IDF Zigbee NCP firmware”, we need to clarify which project you want built, since ESP-IDF 5.4.1 doesn’t include an `esp_zigbee_ncp` example under `examples/zigbee/`.

### What should be done in the future
- If the CoreS3 is attached, we can flash and compare runtime logs between:
  - ESP-IDF `examples/zigbee/esp_zigbee_gateway`
  - esp-zigbee-sdk `examples/esp_zigbee_gateway`

### Technical details
```bash
source ~/esp/esp-5.4.1/export.sh
cd ~/esp/esp-5.4.1/examples/zigbee/esp_zigbee_gateway
idf.py set-target esp32s3
idf.py build
```

<!-- Provide background context needed to use this reference -->

## Quick Reference

<!-- Provide copy/paste-ready content, API contracts, or quick-look tables -->

## Usage Examples

<!-- Show how to use this reference in practice -->

## Related

<!-- Link to related documents or resources -->
