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
    - Path: esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/various/logs/20260104-185815-cores3-host-only-retest-script.log
      Note: Short host-only capture using script/PTY; shows TTY requirement and a transient stall
    - Path: esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/various/logs/20260104-185952-cores3-host-only-retest-script-120s.log
      Note: Host-only retest after H2 enclosure closed; shows full ZNSP exchange + network formed + permit-join open
    - Path: esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/various/logs/20260104-235331-cores3-host-only.log
      Note: Step 14 CoreS3-only validation (H2 not USB-attached)
    - Path: esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/index.md
      Note: Bugreport ticket created in Step 12
    - Path: esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/various/logs/20260104-234318-cores3-host.log
      Note: Step 13 paired capture (host) shows handshake + formation
    - Path: esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/various/logs/20260104-234318-h2-ncp.log
      Note: Step 13 paired capture (ncp) shows pattern detect + formation/steering
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

## Step 10: Build + flash + run `esp_zigbee_host` on CoreS3 (NCP host) and hit host↔NCP framing errors

This step attempted to run the NCP architecture “host firmware” (`esp_zigbee_host`) on the CoreS3 and talk to the Unit Gateway H2 (running `esp_zigbee_ncp`) over UART (the user connected the Unit Gateway H2 to CoreS3 “port C”). The firmware booted, but the host task aborted with `ESP_ZNSP_FRAME: Invalid packet format` / `ESP_ZNSP_MAIN: Process event fail`, indicating that the bytes arriving on the host UART did not decode into a complete host-frame + CRC payload.

### What I did
- Detected the CoreS3 USB-Serial/JTAG port as `/dev/ttyACM0` and verified the chip is `ESP32-S3` via `esptool.py chip_id`.
- Built and flashed `~/esp/esp-zigbee-sdk/examples/esp_zigbee_host` to `/dev/ttyACM0`.
- Ran monitor for ~25 seconds via `script` to provide a TTY.

### Why
- Validate the M5Stack `zigbee_ncp` tutorial end-to-end: CoreS3 host firmware should connect to the H2 NCP firmware and then form a Zigbee network.

### What worked
- CoreS3 flash succeeded on `/dev/ttyACM0` and the program boots.
- The earlier hard crash (`assert failed: xStreamBufferGenericCreate stream_buffer.c:355`) was resolved by patching SLIP buffer creation in:
  - `~/esp/esp-zigbee-sdk/examples/esp_zigbee_host/components/src/slip.c`
  - `~/esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/slip.c`

### What didn't work
- The host still aborts with:
  - `E (...) ESP_ZNSP_FRAME: Invalid packet format`
  - `E (...) ESP_ZNSP_MAIN: Process event fail`

### What I learned
- The host bus input path feeds each `UART_DATA` chunk directly into `slip_decode()` / `esp_host_frame_input()`; if a SLIP frame arrives split across multiple UART events, decode will see an incomplete frame and report `Invalid packet format`. This likely needs a higher-level “buffer until SLIP_END” reassembly in the host bus layer, or UART pattern/timeout configuration so reads align with complete SLIP frames.

### What warrants a second pair of eyes
- Confirm whether the CoreS3↔H2 UART wiring on this hardware revision actually matches the tutorial pinout (host RX=18/TX=17; NCP RX=23/TX=24) and whether the H2 is powered/enabled correctly from port C.
- Confirm whether the upstream `esp_zigbee_host` implementation is expected to work without SLIP frame reassembly (i.e., whether UART events typically align with full frames on the reference hardware).

### Technical details
Chip detection:
```bash
source ~/esp/esp-5.4.1/export.sh
python ~/esp/esp-5.4.1/components/esptool_py/esptool/esptool.py --port /dev/ttyACM0 chip_id
```

Build/flash/run:
```bash
source ~/esp/esp-5.4.1/export.sh
cd ~/esp/esp-zigbee-sdk/examples/esp_zigbee_host
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash
timeout 25s script -q -c "idf.py -p /dev/ttyACM0 monitor" /dev/null
```

## Step 11: Confirm host UART TX, but H2 sees no UART RX (likely wiring/connection issue)

This step focused on separating “software framing bugs” from “no electrical link”. I fixed the H2 console output so it no longer pollutes the NCP UART pins, updated the CoreS3 host bus to forward only complete SLIP frames (pattern detection), and then added instrumentation to prove that the host actually writes SLIP frames out on the configured UART pins. The host does transmit, but the H2 shows no `UART_DATA` events at all, strongly suggesting that the CoreS3→H2 direction is not physically connected (or the H2 isn’t actually on port C / isn’t sharing ground).

### What I did
- Regenerated `~/esp/esp-zigbee-sdk/examples/esp_zigbee_ncp/sdkconfig` from `sdkconfig.defaults` so:
  - the H2 console uses `USB-Serial/JTAG` (to keep GPIO23/24 quiet), and
  - bootloader log level is `NONE`.
- Updated the CoreS3 host bus UART reader to use SLIP `0xC0` pattern detection (so it only forwards complete SLIP frames to the host frame decoder).
- Fixed a startup race where `esp_host_send_event()` could drop early events because `dev->queue` was created after the bus task started (and the same issue on the NCP side).
- Added minimal debug logs:
  - Host prints `ESP_ZNSP_FRAME: TX ...` and `ESP_ZNSP_BUS: uart write ...` when it transmits a SLIP frame.
  - NCP prints `ESP_NCP_BUS: UART_DATA: ...` whenever *any* UART bytes arrive on the NCP bus (even if they’re not SLIP framed).
- Used the tmux helper script to capture paired logs:
  - `scripts/run_ncp_host_and_h2_ncp_tmux.sh` (captures into `various/logs/`).

### What worked
- The CoreS3 host reliably transmits a SLIP frame early in startup:
  - example: `ESP_ZNSP_FRAME: TX id=0 ... slip=11` + `ESP_ZNSP_BUS: uart write 11 bytes`
  - captured in `various/logs/20260104-230020-cores3-host.log`
- With H2 console moved to USB, host decoding is no longer polluted by H2 boot log ASCII.

### What didn't work
- The H2 never logs `ESP_NCP_BUS: UART_DATA: ...`, even though the host logs confirm it is writing bytes out to the configured UART.
  - example capture: `various/logs/20260104-230245-h2-ncp.log` (no `UART_DATA`)

### What I learned
- At this point the blocker is no longer “SLIP framing” or “host aborts on garbage input”; it’s that the H2 is not seeing *any* bytes on its NCP UART RX pin during host transmission. That points to a physical link problem (cable/port/ground), not firmware.

### What was tricky to build
- `sdkconfig.defaults` changes don’t apply retroactively; to move the H2 console off UART pins I had to remove the existing `sdkconfig`/`build` and reconfigure so defaults were actually applied.
- The host stack can block before app-level logs appear (it sends an early request and then waits for a response), so I had to add explicit TX/write logs to prove bytes are leaving the host.

### What warrants a second pair of eyes
- Confirm the physical setup matches the tutorial:
  - CoreS3 port C is actually connected to the Unit Gateway H2 via the Grove cable.
  - Both sides share ground (if H2 is USB-powered, the Grove GND still needs to be connected).
  - There isn’t a “straight-through vs crossed” UART wiring mismatch for this specific cable/adapter.

### What should be done in the future
- If the wiring is correct, add a dedicated “UART smoke test” firmware for both boards (send a known byte pattern and confirm reception) to rule out hardware/port issues before bringing up the full Zigbee host/NCP protocol. N/A otherwise.

### Code review instructions
- Host UART framing changes: `~/esp/esp-zigbee-sdk/examples/esp_zigbee_host/components/src/esp_host_bus.c`
- Host startup race fix: `~/esp/esp-zigbee-sdk/examples/esp_zigbee_host/components/src/esp_host_main.c`
- H2 console config: `~/esp/esp-zigbee-sdk/examples/esp_zigbee_ncp/sdkconfig.defaults`
- H2 bus RX instrumentation: `~/esp/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_bus.c`

### Technical details
Used tmux capture:
```bash
chmod +x esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/scripts/run_ncp_host_and_h2_ncp_tmux.sh
DURATION_SECONDS=30 CORES3_PORT=/dev/ttyACM0 H2_PORT=/dev/ttyACM1 \
  esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/scripts/run_ncp_host_and_h2_ncp_tmux.sh
```

## Step 12: Confirm the physical UART link was the issue (cable reseat) via a dedicated smoke test ticket

This step created a dedicated bugreport ticket and minimal UART smoke-test firmware to conclusively separate “protocol issues” from “wiring issues”. After reseating the Grove cable more firmly, the smoke test showed stable bidirectional `PING`/`PONG` exchange over the expected pins (CoreS3 UART1 TX=17 RX=18 ↔ H2 UART1 TX=24 RX=23), confirming that the earlier “H2 sees zero bytes” behavior was extremely likely a bad cable seating or incomplete connection.

### What I did
- Created a new bugreport ticket: `002-ZIGBEE-NCP-UART-LINK`.
- Wrote a bugreport doc summarizing deltas vs the M5Stack guide and the evidence collected.
- Added two minimal ESP-IDF projects under the new ticket:
  - CoreS3: periodically sends `PING n\n` on UART1 pins 17/18 and logs RX.
  - H2: logs received lines and replies `PONG n\n` on UART1 pins 24/23.
- Flashed both smoke-test firmwares and captured paired logs with tmux.

### What worked
- The UART link now clearly works in both directions with the smoke test:
  - CoreS3 logs show `TX -> PING ...` and `RX <- PONG ...`
  - H2 logs show `RX <- PING ...` and `TX -> PONG ...`

### What I learned
- The “host TX confirmed but H2 sees no UART RX bytes” symptom is exactly what you get from a partially seated Grove cable; the smoke test is a fast way to validate physical connectivity before debugging higher-level ZNSP/SLIP protocol.

### Technical details
Bugreport ticket docs and code live at:
- `esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/index.md`

Evidence logs (smoke test):
- `esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/various/logs/20260104-232342-cores3-host.log`
- `esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/various/logs/20260104-232342-h2-ncp.log`

## Step 13: NCP host↔NCP protocol handshake succeeds (network formed + steering started)

This step revisited the actual `esp_zigbee_host` + `esp_zigbee_ncp` setup after confirming the physical UART link and fixing a missing ESP-IDF UART pattern queue reset. With `uart_pattern_queue_reset()` added on both the host and NCP bus layers, `UART_PATTERN_DET` events fire as expected, SLIP frames are reassembled, and the ZNSP protocol handshake completes. Both sides then proceed to Zigbee network formation and start network steering, matching the M5Stack guide’s expected “Normal Running Log Content”.

### What worked
- H2 NCP shows `ESP_NCP_ZB: Initialize Zigbee stack`, `Formed network successfully`, and `Network steering started`.
- CoreS3 host shows decoded ZNSP frames and logs `Formed network successfully ...` and `Network ... is open for 180 seconds`.

### Technical details
Paired capture logs:
- `esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/various/logs/20260104-234318-cores3-host.log`
- `esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/various/logs/20260104-234318-h2-ncp.log`

## Step 14: Validate CoreS3 host behavior with H2 closed up (H2 not USB-attached)

This step validated the “CoreS3 side” in the physical configuration you’d actually use: the Unit Gateway H2 is closed up and not connected to the computer over USB (no `/dev/ttyACM1`), but it is still powered and connected to the CoreS3 via Grove Port C. The CoreS3 host firmware still connects to the NCP over UART, initializes Zigbee, forms the network, and opens permit-join for 180 seconds.

### What worked
- CoreS3 host shows ZNSP request/response traffic and logs:
  - `Initialize Zigbee stack`
  - `Formed network successfully ...`
  - `Network(0x....) is open for 180 seconds`

### Technical details
Host-only capture:
- `esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/various/logs/20260104-235331-cores3-host-only.log`

## Step 15: Re-test CoreS3 host after H2 was physically closed (sanity check)

This step re-validated that “the S3 side” is still healthy now that the Unit Gateway H2 is fully closed up (no USB access) and connected only via Grove Port C. The key validation is that the CoreS3 can still exchange ZNSP/SLIP frames with the H2 NCP and reach the expected steady state (network formed + permit-join open).

### What I did
- Confirmed only `/dev/ttyACM0` is present (CoreS3) and the H2 is not USB-attached.
- Captured a new host-only boot + run log from `esp_zigbee_host` using `script` (because `idf.py monitor` requires an interactive TTY).

### What worked
- CoreS3 still receives valid ZNSP responses from the enclosed H2 NCP and reaches:
  - `Formed network successfully ...`
  - `Network(0x....) is open for 180 seconds`

### What didn't work
- Running `idf.py monitor` without a TTY fails with: `Error: Monitor requires standard input to be attached to TTY.` (fixed by using `script`).
- One short capture showed a transient stall after `TX id=4 ...` with no immediate response (likely a transient link/readiness issue); the subsequent longer capture was fully successful.

### What I learned
- For “headless” captures in this environment, `script -q -c "idf.py monitor"` is the simplest way to satisfy `idf_monitor`’s TTY requirement.
- The NCP can remain fully operational without USB attached; the CoreS3-only view is enough to validate the end-to-end host↔NCP UART path.

### What was tricky to build
- `idf.py monitor` is fundamentally interactive; piping output breaks it. The workaround needs a pseudo-tty.

### What warrants a second pair of eyes
- If we ever see the “stall after `TX id=4`” pattern again, confirm whether it correlates with cable strain/port seating after the H2 enclosure is closed.

### What should be done in the future
- If this becomes intermittent, add a “host-side UART heartbeat” (periodic stats: RX bytes, frames decoded, last response time) so the failure mode is visible without NCP logs. N/A otherwise.

### Code review instructions
- N/A (no code changes in this step).

### Technical details
Host-only captures:
- `esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/various/logs/20260104-185815-cores3-host-only-retest-script.log`
- `esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/various/logs/20260104-185952-cores3-host-only-retest-script-120s.log`

<!-- Provide background context needed to use this reference -->

## Quick Reference

<!-- Provide copy/paste-ready content, API contracts, or quick-look tables -->

## Usage Examples

<!-- Show how to use this reference in practice -->

## Related

<!-- Link to related documents or resources -->
