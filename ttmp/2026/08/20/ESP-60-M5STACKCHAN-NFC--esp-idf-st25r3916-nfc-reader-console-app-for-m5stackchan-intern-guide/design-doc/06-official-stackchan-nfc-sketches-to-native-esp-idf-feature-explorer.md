---
Title: Official StackChan NFC Sketches to Native ESP-IDF Feature Explorer
Ticket: ESP-60-M5STACKCHAN-NFC
Status: active
Topics:
    - m5stackchan
    - nfc
    - st25r3916
    - esp32-s3
    - esp-idf
    - esp-console
    - intern-guide
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://0117-m5stackchan-nfc-feature-explorer/README.md
      Note: Operator guide, command matrix, build instructions, and mutation safety contract
    - Path: repo://0117-m5stackchan-nfc-feature-explorer/main/idf_component.yml
      Note: Pinned native ESP-IDF M5Unit-NFC dependency
    - Path: repo://0117-m5stackchan-nfc-feature-explorer/main/nfc_console.cpp
      Note: Console equivalents and exact confirmation-token guards
    - Path: repo://0117-m5stackchan-nfc-feature-explorer/main/nfc_explorer.cpp
      Note: Official M5Unit-NFC reader, NDEF, value-block, and emulation capability implementation
    - Path: repo://ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/sources/hardware/21-24-native-feature-explorer.provenance.md
      Note: Build, read-only NTAG215, emulation-mode, and mutation-guard evidence
    - Path: repo://ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/sources/web/04-m5stack-stackchan-nfc-full-official-doc.md
      Note: Vendor source for all six sketch families
ExternalSources:
    - https://docs.m5stack.com/en/arduino/stackchan/nfc
    - https://github.com/m5stack/M5Unit-NFC
Summary: Map all six official StackChan NFC Arduino sketch families to a safe, native ESP-IDF feature explorer using the official M5Unit-NFC protocol layer on an ESP-IDF I2C bus.
LastUpdated: 2026-08-22T14:02:31.16551581-04:00
WhatFor: Implement and validate the full documented NFC-A capability set after proving native ESP-IDF UID polling.
WhenToUse: Use when extending the M5StackChan ST25R3916 beyond UID polling into identification, memory access, NDEF, MIFARE Classic value blocks, and tag emulation.
---


# Official StackChan NFC Sketches to Native ESP-IDF Feature Explorer

## Executive Summary

The official StackChan NFC guide presents six NFC-A example families: quick scan and identification, complete memory dump, ST25R3916 tag emulation, direct card read/write, NDEF read/write, and MIFARE Classic value-block wallet operations. The completed `0115-m5stackchan-nfc-reader` proves that a minimal native C driver can identify the ST25R3916 and select an ISO/IEC 14443-A tag through ESP-IDF 5.5.4. Reimplementing every higher protocol in that diagnostic driver would duplicate substantial vendor code: MIFARE Classic Crypto1, Ultralight/NTAG identification, ISO-DEP, DESFire filesystem operations, NDEF TLV/record parsing, value-block access conditions, and ST25R3916 target emulation.

The proposed implementation is a separate pure ESP-IDF project, `0117-m5stackchan-nfc-feature-explorer`. It will create the CoreS3 internal I2C bus with ESP-IDF, pass the resulting `i2c_master_bus_handle_t` to the current native-ESP-IDF `M5UnitUnified` adapter, and use the official `M5Unit-NFC` protocol layer. It will expose capability-oriented `esp_console` commands rather than touch gestures. Reader mode will be the safe default. Tag emulation will be selected through a persistent boot mode because the ST25R3916 must be configured as poller or target before initialization.

The new explorer complements rather than replaces `0115`:

- `0115` remains the minimal transport, register, RF, anticollision, and backend A/B regression harness.
- `0117` becomes the broad card-family and application-feature harness.
- The future NFC LAB integration can choose tested capabilities from both projects.

All state-changing card commands require an explicit confirmation token. Read-only commands never mutate card memory. No format, access-bit, NDEF conversion, raw write, or value-block command will be run against a physical tag without a declared sacrificial test tag.

## Problem Statement

A UID proves only the ISO/IEC 14443-A discovery and selection path. It does not establish what a selected tag supports or exercise the application-visible functions in the M5Stack guide. The current native driver cannot yet answer:

- the exact tag subtype and usable memory size;
- whether the current tag stores valid NDEF data;
- the contents of Ultralight/NTAG pages or Classic sectors;
- whether a Classic key authenticates;
- whether a tag supports safe read, write, value-block, ISO-DEP, or DESFire operations;
- whether the ST25R3916 can emulate a readable Ultralight/NTAG NDEF tag under pure ESP-IDF.

The official page already defines working behavior and expected outputs. The implementation should preserve those semantics while adapting interaction and safety to a console diagnostic firmware.

## Evidence and Upstream Baseline

The vendor guide is preserved at:

```text
sources/web/04-m5stack-stackchan-nfc-full-official-doc.md
```

The current M5Unit-NFC repository was inspected at:

```text
commit 93745b547364f310cd64b5155a870103a7800a5d
```

That revision contains native ESP-IDF example entry points and an ESP-IDF component manifest. Its `M5UnitUnified` dependency supports attaching a unit directly to an existing ESP-IDF bus:

```cpp
bool UnitUnified::add(Component& unit, i2c_master_bus_handle_t bus);
```

This is the required integration boundary. The application owns bus creation and board pins; M5Unit-NFC owns ST25R3916 and NFC protocol behavior.

## Official Sketch Inventory

### 1. Quick Scan Identification

Vendor flow:

```text
detect one or many PICCs
identify each PICC
print UID, type, ATQA, SAK, user area, total size
deactivate
```

Native explorer equivalents:

```text
nfc-scan [--timeout-ms N]
nfc-info
```

`nfc-scan` enumerates multiple PICCs through `detect(std::vector<PICC>&)`. `nfc-info` selects one card, performs full identification and reactivation, prints the complete capability summary, then deactivates.

### 2. Complete Data Reading

Vendor flow:

```text
detect
identify
reactivate
dump card using default Classic Key A when relevant
deactivate
```

Native explorer equivalent:

```text
nfc-dump [--key-a FFFFFFFFFFFF]
```

The default key is visible in command output. The command is read-only but authentication may place a Classic PICC into HALT after a failed key; the implementation always deactivates or restores field state before returning.

### 3. Tag Emulation

Vendor flow:

```text
configure ST25R3916 target/emulation mode
construct Ultralight or NTAG213 PICC
embed seven-byte UID and BCC values in memory
serve an NDEF URI + text image
run the target state machine continuously
report Off/Idle/Ready/Active/Halt transitions
```

Native explorer equivalents:

```text
nfc-mode emulation-ultralight --confirm REBOOT
nfc-mode emulation-ntag213 --confirm REBOOT
nfc-emulation-status
nfc-mode reader --confirm REBOOT
```

The selected mode is stored in NVS and applied before unit initialization on reboot. This avoids runtime teardown assumptions and gives target mode an uninterrupted update loop.

### 4. Direct Card Reading and Writing

The current upstream example supports more families than the StackChan page: Classic block access, Ultralight/NTAG pages, MIFARE Plus SL3, DESFire, and ST25TA. The first explorer version exposes a common read interface and family-specific write validation.

Native explorer equivalents:

```text
nfc-raw-read --address N [--length N] [--key-a HEX]
nfc-raw-write --address N --hex HEX --confirm WRITE
nfc-write-verify-clear --address N --confirm ERASE-TEST-AREA
```

Safety requirements:

- resolve and print PICC type before choosing page or block semantics;
- reject manufacturer pages, lock/configuration pages, sector trailers, and non-user blocks by default;
- require a separate future `--unsafe` contract for protected metadata writes;
- read back every write;
- never report success before byte-for-byte verification;
- preserve the original bytes and restore them for a reversible test instead of clearing arbitrary user data.

### 5. NDEF Format Reading and Writing

Vendor functions include validity checking, TLV parsing, URI/text/MIME records, capacity-based message sizing, Ultralight conversion, DESFire formatting, and DESFire NDEF preparation.

Native explorer equivalents:

```text
nfc-ndef-read
nfc-ndef-write-demo --confirm REPLACE-NDEF
nfc-ndef-write-uri --uri URL --confirm REPLACE-NDEF
```

Version-one write scope is deliberately narrower than the vendor demonstration:

- support existing valid Type 2 NDEF tags first;
- do not silently convert a non-NDEF Ultralight tag;
- do not format DESFire automatically;
- reject messages larger than the reported user area;
- write a zero-length NDEF TLV before replacing payload when the underlying library requires atomic NDEF update semantics;
- read back and parse the resulting records.

Irreversible conversion and DESFire formatting, if added, must use separate command names with exact destructive confirmation strings.

### 6. MIFARE Classic E-Wallet

Vendor flow mutates sector access conditions and value blocks, tests decrement/increment permissions, copies a value through RESTORE/TRANSFER, and restores normal access. This is the highest-risk sketch because an incorrect key or access-bit write can make data inaccessible.

Native explorer equivalents:

```text
nfc-value-inspect [--key-a HEX]
nfc-wallet-demo --block N --mode non-rechargeable --confirm MUTATE-CLASSIC
nfc-wallet-demo --block N --mode rechargeable --confirm MUTATE-CLASSIC
nfc-value-restore --block N --confirm RESTORE-CLASSIC
```

`nfc-value-inspect` is read-only and may run without destructive consent. The demo commands must:

1. verify MIFARE Classic type;
2. verify the target and adjacent blocks are user blocks;
3. reject manufacturer block 0 and sector trailers;
4. authenticate and dump original target/trailer bytes;
5. print an explicit mutation plan;
6. apply value/access operations;
7. verify every transition;
8. restore original data and access bytes;
9. report any restoration failure as a high-severity result.

## Proposed Architecture

```text
0117-m5stackchan-nfc-feature-explorer/
├── CMakeLists.txt
├── sdkconfig.defaults
├── partitions.csv (only if dependency size requires it)
├── README.md
└── main/
    ├── CMakeLists.txt
    ├── idf_component.yml
    ├── app_main.cpp
    ├── nfc_explorer.hpp/.cpp
    ├── nfc_console.hpp/.cpp
    ├── nfc_ndef_print.hpp/.cpp
    ├── nfc_emulation.hpp/.cpp
    └── Kconfig.projbuild (only for compile-time limits)
```

### Runtime ownership

One `NfcExplorer` object owns:

- one ESP-IDF I2C master bus;
- one `m5::unit::UnitUnified` manager;
- one `m5::unit::UnitNFC` ST25R3916 instance;
- one reader or emulation protocol layer according to boot mode;
- one mutex protecting all unit operations.

The console REPL is the only reader-operation caller. Emulation mode runs its update state machine from the main task; console status and reboot commands only inspect atomic status or modify NVS.

### Bus configuration

The explorer uses the established StackChan internal bus:

```text
I2C controller: 1
SDA: GPIO12
SCL: GPIO11
frequency: 400 kHz
ST25R3916 address: 0x50
```

USB Serial/JTAG remains the console. No UART console is allowed because StackChan peripheral pins may be shared.

### Dependency policy

`main/idf_component.yml` declares dependencies. M5Unit-NFC is pinned to a known Git revision or resolved through committed `dependencies.lock`. Build artifacts and `managed_components/` remain ignored. The upstream dependency version and resolved commit are printed by the README and recorded in the ticket.

## Safety Contract

Commands are classified before implementation:

| Class | Examples | Confirmation |
|---|---|---|
| read-only | scan, info, dump, raw read, NDEF read, value inspect | none |
| reversible write test | save/write/read/restore same user page/block | `RESTORE-AFTER-TEST` |
| replacement write | replace NDEF payload | `REPLACE-NDEF` |
| structural mutation | access bits, value block, Classic keys | `MUTATE-CLASSIC` |
| irreversible/destructive | Ultralight NDEF conversion, DESFire format | separate command; not version one |
| firmware mode switch | reader to target emulation | `REBOOT` |

The console must print that confirmation tokens protect against accidental command invocation, not against choosing the wrong physical tag. A UID allow-list option should be added before any write test.

## Design Decisions

### Decision 1: Preserve the minimal C driver

**Status:** accepted.

`0115` has observer-safe trace instrumentation and two controllable ESP-IDF transport backends. Pulling C++ protocol dependencies into it would weaken its value as a minimal regression target. New feature work belongs in `0117`.

### Decision 2: Reuse M5Unit-NFC under native ESP-IDF

**Status:** accepted.

The official library already implements the protocol families used by all six sketches and now ships pure ESP-IDF examples. Independent reimplementation would add substantial correctness and security risk without improving the diagnostic objective.

### Decision 3: Attach to an application-created ESP-IDF bus

**Status:** accepted.

The application must control the CoreS3 internal pins and console/runtime policy. `M5UnitUnified::add(unit, i2c_master_bus_handle_t)` is preferable to a hidden second bus or Arduino `Wire` compatibility.

### Decision 4: Reboot between reader and target modes

**Status:** accepted for version one.

ST25R3916 reader and target configuration differ before `begin()`. NVS-selected reboot gives deterministic initialization and avoids relying on incomplete runtime teardown behavior.

### Decision 5: Safe read-only delivery before write tests

**Status:** accepted.

The current physical tag is identified as an Ultralight/NTAG-family PICC but has not been declared sacrificial. The explorer will be built and hardware-tested first with scan, info, dump, raw read, and NDEF read only.

## Alternatives Considered

### Port all higher protocols into the C driver

Rejected for this phase. Crypto1, NDEF, DESFire, ISO-DEP, value access bits, and emulation are mature upstream implementations. Porting them would create a second protocol library and increase the test matrix considerably.

### Copy six independent example projects

Rejected as the primary interface. It would reproduce the vendor build matrix but require full flashing to move between ordinary reader functions and duplicate initialization code. A console explorer makes card capability testing faster. Reader versus emulation still requires reboot, but not a different source tree.

### Use Arduino as the final feature runtime

Rejected. Arduino remains an important control, but the project objective is a native ESP-IDF system that can later integrate with Mooncake and shared-bus ownership.

### Integrate features directly into NFC LAB

Deferred. NFC LAB adds display, touch, LVGL, and application lifecycle variables. The feature explorer should first establish card-family behavior in a standalone environment.

## Implementation Plan

### Phase 1: Source and build proof

1. Create `0117` with ESP-IDF 5.5.4 and target `esp32s3`.
2. Pin M5Unit-NFC native component and dependencies in `main/idf_component.yml`.
3. Create bus 1 on GPIO12/11.
4. Attach `UnitNFC` to the existing `i2c_master_bus_handle_t`.
5. Build and print unit debug information.

### Phase 2: Read-only explorer

1. Add prompt-aware USB Serial/JTAG console.
2. Implement scan, info, dump, raw read, NDEF read, and value inspect.
3. Ensure every selected PICC is deactivated on all exit paths.
4. Print stable machine-readable result prefixes.
5. Full-flash and validate the current tag without changing it.

### Phase 3: Emulation

1. Add NVS boot mode.
2. Add official Ultralight and NTAG213 memory templates with embedded UID/BCC.
3. Run `EmulationLayerA::update()` continuously.
4. Print state transitions.
5. Validate with a separate NFC reader or smartphone.

### Phase 4: Guarded writes

1. Add UID allow-list and exact confirmation tokens.
2. Implement reversible raw page/block write tests.
3. Implement existing-format NDEF replacement and readback.
4. Implement value-block inspect first, then mutation/restore on a sacrificial Classic card.
5. Preserve before/after/restoration captures.

### Phase 5: Integration selection

Choose which proven explorer functions belong in NFC LAB and expose them through the single serialized `NfcDebugService` worker.

## Testing Strategy

### Build tests

- source ESP-IDF 5.5.4;
- remove stale `sdkconfig` when defaults change;
- `idf.py set-target esp32s3` once;
- `idf.py build` for normal rebuilds;
- commit `dependencies.lock`;
- verify no build or managed-component artifacts are staged.

### Hardware read-only acceptance

For the current single tag:

- scan returns the known UID;
- info returns stable type, ATQA, SAK, user area, and total size;
- dump reads all safe pages/blocks expected for the identified type;
- NDEF read reports either parsed records or an explicit non-NDEF result;
- all commands return the reader to a usable state;
- no write command is invoked.

### Emulation acceptance

- phone/reader sees the configured seven-byte UID;
- ATQA is `0x0044` and SAK is `0x00` for Ultralight/NTAG templates;
- URI and text NDEF records parse correctly;
- state sequence reports Idle, Ready, Active, and Halt transitions;
- reader mode is restored by persisted mode switch and reboot.

### Destructive acceptance

Requires named sacrificial tags. Every test records:

- UID and identified type;
- original memory bytes;
- requested mutation;
- post-write readback;
- restoration readback;
- exact command and confirmation token.

## Implementation and Hardware Results

Project `0117-m5stackchan-nfc-feature-explorer` now implements the proposed single-binary console design. ESP-IDF 5.5.4 resolved the pinned M5Unit-NFC component and compiled the full reader/emulator firmware to `0x67760` bytes. The dependency build confirmed that M5UnitUnified selected the ESP-IDF `i2c_master` backend.

The first read-only probe exposed a lifecycle defect in the wrapper rather than the upstream protocol: enumeration HALTs a discovered PICC, so the next command's REQA cannot see an unmoved tag. `activate_one()` now tries REQA and falls back to WUPA before SELECT. After that fix, one stationary tag passed scan, detailed identification, raw read, NDEF validation, and a full memory dump in one serial session.

The tag is now identified precisely:

```text
UID: 04:91:D4:4C:9E:61:80
Type: NTAG 215
ATQA: 0x0044
SAK: 0x00
Pages: 135
User area: 504 bytes
Total memory: 540 bytes
NFC Forum type: Type 2
```

Page 3 contains capability container `E1 10 3E 00`. Page 4 contains `03 00 FE 00`: a valid zero-length NDEF Message TLV followed by the Terminator TLV. The tag is therefore NDEF-formatted and empty. No card writes were performed.

Both emulation profiles initialize from NVS-selected boot modes and report the official UIDs, ATQA/SAK, and memory sizes. They stayed in `off` without an external reader field. Reader mode was restored and the physical NTAG215 read successfully. Missing and incorrect mutation confirmation strings were also verified to reject execution.

Evidence and exact serial output are in `sources/hardware/21-24-native-feature-explorer.provenance.md` and captures 21–24.

## Open Questions

1. Which physical tags are sacrificial for raw writes, NDEF replacement, and Classic access-bit/value tests?
2. Does the current `04:91:D4:4C:9E:61:80` tag already contain NDEF data?
3. Which additional tags are available: Classic 1K/4K, NTAG213/215/216, DESFire, MIFARE Plus?
4. Should version two expose DESFire format and Ultralight NDEF conversion, or keep irreversible operations only in purpose-built firmware?
5. Does the upstream native component use the ESP-IDF high-level bus adapter consistently enough to compare with `0115` transaction traces?

## References

- M5Stack, “StackChan NFC Near Field Communication”: https://docs.m5stack.com/en/arduino/stackchan/nfc
- M5Stack, `M5Unit-NFC`: https://github.com/m5stack/M5Unit-NFC
- Upstream revision inspected: `93745b547364f310cd64b5155a870103a7800a5d`
- Official page snapshot: `sources/web/04-m5stack-stackchan-nfc-full-official-doc.md`
- Native UID diagnosis: `analysis/02-fresh-base-principles-reconstruction-of-the-esp-idf-st25r3916-failure.md`
- Minimal native reader: `0115-m5stackchan-nfc-reader/`
- Future UI integration target: `0116-m5stackchan-nfc-debug-ui/`
