---
Title: 'Cardputer-ADV MeshCore Companion Terminal: Architecture and Implementation Guide'
Ticket: 0104-CARDCORE-MESHCORE
Status: active
Topics:
    - esp-idf
    - esp32-s3
    - cardputer
    - lora
    - meshcore
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://0038-cardputer-adv-serial-terminal
      Note: USB Serial/JTAG recovery-console donor
    - Path: repo://0083-cardputer-adv-animation-ui
      Note: Queue-driven Cardputer-ADV UI and 8 MB configuration donor
    - Path: repo://components/cardputer_kb
      Note: Reusable Cardputer-ADV TCA8418 scanner and layout donor
    - Path: repo://ttmp/2026/07/13/0104-CARDCORE-MESHCORE--cardputer-adv-meshcore-companion-terminal/sources/meshcore
      Note: Snapshot of pinned upstream MeshCore protocol/examples at 219812b
    - Path: repo://ttmp/2026/07/13/0104-CARDCORE-MESHCORE--cardputer-adv-meshcore-companion-terminal/sources/meshcore-cardputer-adv
      Note: Cardputer-ADV hardware and UX reference snapshot
    - Path: repo://ttmp/2026/07/13/0104-CARDCORE-MESHCORE--cardputer-adv-meshcore-companion-terminal/sources/plai
      Note: GPL native-IDF Cap hardware reference; do not copy without compatible licensing
ExternalSources:
    - https://github.com/meshcore-dev/MeshCore @ 219812b
    - https://github.com/Stachugit/MeshCore-Cardputer-ADV @ e341957
    - https://github.com/d4rkmen/plai @ fda03cf
    - https://github.com/Nicolai-Electronics/meshcore-c @ 1e373d5
Summary: Evidence-based plan for a native ESP-IDF Cardputer-ADV MeshCore companion terminal with an intentionally isolated Arduino compatibility layer.
LastUpdated: 2026-07-13T19:56:19-04:00
WhatFor: Implement the first Cardcore firmware increment and safely evolve it into a standalone MeshCore terminal.
WhenToUse: Read before creating the firmware project, porting MeshCore, or touching Cardputer-ADV LoRa-cap hardware.
---


# Cardputer-ADV MeshCore Companion Terminal: Architecture and Implementation Guide

## Executive summary

Cardcore is a keyboard-first, battery-powered **MeshCore Companion** terminal for the M5Stack Cardputer-ADV plus Cap LoRa-1262. A Companion originates and consumes traffic but deliberately does not repeat it. That is the right first role for a handheld: it establishes real MeshCore interoperability without making an intermittently positioned, battery-operated device part of the network's routing infrastructure. MeshCore documents Companion, Simple Secure Chat, and Simple Room Server as separate examples, and explicitly says Companion nodes do not repeat traffic [S1: `sources/meshcore/README.md:37-49`].

The conservative MVP is an ESP-IDF 5.5.4 project whose board support, tasks, storage, screen, and keyboard are native IDF, while a small `meshcore_compat` component contains Arduino-ESP32, a pinned copy/submodule of MeshCore, and RadioLib. This is not an Arduino sketch: `idf.py`, CMake components, ESP-IDF FreeRTOS, NVS, LittleFS, logging, and USB Serial/JTAG remain the application substrate. Arduino is an explicit compatibility boundary because the inspected MeshCore source includes `Arduino.h`, `Stream`, `FS`, and RadioLib types throughout its core and helper APIs [S1: `sources/meshcore/src/MeshCore.h:26`; `Identity.h:4-48`; `helpers/radiolib/RadioLibWrappers.h:4-23`].

The hardware work is the highest early risk. The Cap LoRa-1262 needs its PI4IOE5V6408 expander at I2C address `0x43` configured narrowly: set only pin 0 to push-pull output and high to enable the RF front end, then let SX1262 DIO2 select RX/TX. Plai's native-IDF implementation makes precisely that distinction [S3: `sources/plai/main/hal/ioex/ioex.h:1-88`; `hal_cardputer.cpp:331-392`]. The design therefore makes raw radio receive/transmit, including an interop test against a stock MeshCore node, the first firmware milestone.

## 1. What an intern is building

The deliverable is a standalone terminal rather than a repeater, phone companion, BBS server, or Meshtastic device.

```text
Cardputer-ADV + Cap LoRa-1262
              │
              │ native display / TCA8418 keyboard / battery / NVS / LittleFS
              ▼
┌──────────────────────────────────────────────────────────────────┐
│ Cardcore UI: home, channels, direct messages, nearby, room, status│
└───────────────────────────────┬──────────────────────────────────┘
                                │ typed commands and rendered events
┌───────────────────────────────▼──────────────────────────────────┐
│ Mesh transport facade: stable Cardcore-owned API                  │
├──────────────────────────────────────────────────────────────────┤
│ meshcore_compat: pinned MeshCore Companion implementation         │
│ Arduino-ESP32 + RadioLib (implementation detail)                  │
└───────────────────────────────┬──────────────────────────────────┘
                                │ SX1262 packets
                     ┌──────────▼──────────┐
                     │ MeshCore network    │
                     │ repeater / node /   │
                     │ Room Server         │
                     └─────────────────────┘
```

### MVP contract

Include these capabilities, in this order:

1. Generate or import one MeshCore identity and persist it.
2. Start one regulatory region/preset compiled into the first build; expose changes only after basic radio interoperability is proven.
3. Send and receive adverts, show a bounded recent-node list, and retain contacts.
4. Join a public channel and one configurable channel; send and receive encrypted group text.
5. Send and receive direct messages with acknowledgement state.
6. Read recent Room Server posts and submit one post using the upstream protocol.
7. Continue receiving radio packets while a user edits a message.
8. Persist settings in NVS and a bounded message history in LittleFS.
9. Expose USB Serial/JTAG diagnostics and recover to a useful UI state if radio bring-up fails.

Do **not** add BLE, Wi-Fi, web UI, GPS, maps, audio, microSD, OTA, repeater mode, Room Server mode, BBS threads/replication, or themes to this milestone. Each increases code and test surface while being unnecessary to prove the protocol, user interaction, and radio foundations.

### Acceptance test matrix

| ID | Proof | Required peer / observation |
|---|---|---|
| A1 | Cold boot reaches terminal home reliably | attached antenna and LoRa cap |
| A2 | Chip diagnostics identify SX1262 and start continuous receive | serial log plus status screen |
| A3 | Advert from a stock MeshCore node becomes a nearby/contact entry | stock node/repeater |
| A4 | Encrypted channel text round-trips | stock MeshCore participant |
| A5 | Direct message is delivered and its ACK/timeout is represented | stock MeshCore participant |
| A6 | Room posts can be read and a post submitted | existing Simple Room Server |
| A7 | Incoming packet is rendered/persisted during active composition | scripted peer or second node |
| A8 | identity and channel survive reset | power cycle / reset |
| A9 | absent/corrupt radio produces diagnostics and retry, not a boot loop | cap removed or failure injection |
| A10 | `sdkconfig` disables Wi-Fi, BLE, GPS, and SD for MVP | config review and map size |

## 2. Current evidence and reusable work

### 2.1 Local ESP-IDF assets

This workspace already contains much of the non-radio board groundwork.

* `components/cardputer_kb/` is the preferred keyboard donor. Its public API gives a stable physical `KeyPos`, vendor-style `keyNum`, and `UnifiedScanner`; the README describes the TCA8418 event-FIFO to picture-space mapping [L1: `components/cardputer_kb/README.md:1-22`]. `UnifiedScannerConfig` defaults to I2C0 / GPIO8 / GPIO9 / GPIO11 / address `0x34` [L2: `components/cardputer_kb/include/cardputer_kb/scanner.h:36-78`].
* The scanner implementation is usable today despite an old header comment saying the TCA backend will be added: it probes the controller, drains events, maintains pressed state, and maps 10-wide controller events into 4×14 Cardputer picture-space [L3: `components/cardputer_kb/unified_scanner.cpp:56-163,172-227`]. Fix that stale comment when adopting it, and do not duplicate the mapping.
* `0083-cardputer-adv-animation-ui` is the best local UI/task donor. It starts M5Unified, uses one queue for input events, drains it in an application loop, and renders only when state changed [L4: `0083-cardputer-adv-animation-ui/main/app_main.cpp:67-130`]. Its `ui_kb.cpp` demonstrates semantic navigation and text events, including FN arrows and enter/delete [L5: `0083-cardputer-adv-animation-ui/main/ui_kb.cpp:100-259`].
* `0038-cardputer-adv-serial-terminal` demonstrates the requested recovery console choice: it documents USB Serial/JTAG on `/dev/ttyACM*`, separate from the Grove UART [L6: `0038-cardputer-adv-serial-terminal/README.md:1-20`].
* `0083` has a practical 8 MB partition baseline: 4 MB factory, 1 MB FAT storage, 20 KB NVS, plus custom partition settings and USB Serial/JTAG defaults [L7: `0083-cardputer-adv-animation-ui/partitions.csv:1-6`; `sdkconfig.defaults:6-25`]. For Cardcore, rename the storage partition to LittleFS or use an IDF-compatible LittleFS partition type/label; do not assume an existing FAT partition can mount as LittleFS.

### 2.2 Existing third-party firmwares

| Source | Reuse classification | Evidence / action |
|---|---|---|
| Upstream MeshCore | Protocol reference and dependency | MIT license [S1: `license.txt:1-20`]; Companion, Secure Chat, and Room Server example implementations are source-level references. Pin exact revision `219812b`, then record updates deliberately. |
| Stachugit MeshCore-Cardputer-ADV | Hardware behavior and UX reference; do not transplant application architecture | Its README claims an ADV/Cap LoRa-1262 TFT UI, 150-char composer, keyboard navigation, contacts/channels, settings, and persistence [S2: `README.md:15-174`]. It is PlatformIO/Arduino and enables BLE, GPS, large tables and a 256-message offline queue [S2: `variants/m5stack_cardputer/platformio.ini:80-113`], all outside MVP. |
| Plai | Native-IDF hardware reference only | GPL-3.0 [S3: `LICENSE:1-30`]. It has a clean HAL separation, SX1262 interface, expander driver, and initialization ordering [S3: `hal_cardputer.cpp:291-392`; `radio/sx1262.h:27-178`]. Reimplement from datasheets/observed behavior or seek compatible licensing before copying source. It is Meshtastic, not MeshCore. |
| meshcore-c | Future native transport investigation | Its own README calls it work in progress and says its goal is an ESP-IDF component [S4: `README.md:1-9`]. It is not the conservative interop dependency until it has packet-level and application-level compatibility coverage for this MVP. |

### 2.3 Important correction to the initial proposal

The inspected Cardputer fork uses `GPIO46` to power a cap I/O expander and then writes all expander outputs (`0xFF`) [S2: `M5CardputerBoard.h:13-40`]. Plai's newer native implementation instead treats the Cap LoRa-1262 expander as a device at `0x43` and performs read-modify-write bit operations so only pin 0 becomes a push-pull high output [S3: `ioex.cpp:96-131`; `hal_cardputer.cpp:351-356`]. Cardcore must follow the narrow read-modify-write model. “Write all pins high” is not an acceptable initialization policy because undocumented cap functions may share that bank.

## 3. Hardware model and ownership rules

### 3.1 Pin map

Use these as **candidate board constants** and validate them with a dedicated smoke test before protocol work:

```cpp
namespace cardcore::pins {
constexpr gpio_num_t kLoraCs    = GPIO_NUM_5;
constexpr gpio_num_t kLoraReset = GPIO_NUM_3;
constexpr gpio_num_t kLoraDio1  = GPIO_NUM_4;
constexpr gpio_num_t kLoraBusy  = GPIO_NUM_6;
constexpr gpio_num_t kLoraSck   = GPIO_NUM_40;
constexpr gpio_num_t kLoraMosi  = GPIO_NUM_14;
constexpr gpio_num_t kLoraMiso  = GPIO_NUM_39;

constexpr gpio_num_t kI2cSda    = GPIO_NUM_8;
constexpr gpio_num_t kI2cScl    = GPIO_NUM_9;
constexpr gpio_num_t kKeyboardInt = GPIO_NUM_11;
constexpr uint8_t kKeyboardAddress = 0x34;
constexpr uint8_t kCapIoexpAddress = 0x43;
constexpr gpio_num_t kBatteryAdc = GPIO_NUM_10;
} // namespace cardcore::pins
```

The LoRa SPI/control values are corroborated by the Cardputer fork's variant configuration [S2: `platformio.ini:16-38`]. The local keyboard component independently corroborates the I2C controller values [L2]. Treat the Cap oscillator/TCXO voltage and PA values as **bring-up hypotheses**, not universal constants: the fork has contradictory base and Cap-specific values [S2: `platformio.ini:31-34,61-73`]. Confirm them with the cap documentation, a known-good RF test, and your actual region before enabling high-power transmit.

### 3.2 Shared buses

```text
I2C0 (GPIO8/GPIO9) ── TCA8418 keyboard (0x34)
                    └─ PI4IOE5V6408 Cap expander (0x43)

SPI host (GPIO40/14/39) ── SX1262 (CS GPIO5)
                          └─ microSD socket (separate CS; excluded in MVP)
```

Create exactly one native `i2c_master_bus_handle_t` in `cardputer_bsp`. Add each I2C device once and pass the handle to keyboard and cap-expander subcomponents. Do not allow Arduino `Wire`, M5Unified, and native IDF components to each install their own owner for I2C0. The local scanner currently installs the legacy I2C driver itself [L3: `unified_scanner.cpp:25-54`], so either adapt it to accept the shared new-IDF bus handle or make it the sole I2C owner; the former is the intended Cardcore design.

Do not enable microSD in MVP. The LoRa cap and socket share SPI wires, and adding a second SPI client is viable only after a single IDF SPI host/device-owner design, chip-select behaviour, DMA constraints, and error recovery have been tested.

### 3.3 Cap radio bring-up

The radio task owns every SX1262 call. Its DIO1 GPIO ISR does no protocol parsing; it uses `vTaskNotifyGiveFromISR` and yields if required. The task wakes, checks/clears IRQs, reads packets, runs MeshCore timers, and returns the radio to receive mode.

```text
boot
 ├─ initialize native I2C bus
 ├─ initialize TCA8418 (or leave it live for recovery UI)
 ├─ probe PI4IOE5V6408 at 0x43
 │   ├─ absent: report explicit cap variant / wiring diagnostic
 │   └─ present: RMW direction(P0=output), high-Z(P0=false), output(P0=true)
 ├─ initialize SPI host and SX1262 CS/reset/BUSY/DIO1
 ├─ reset: LOW ≥10 ms, HIGH, wait ≥100 ms (known fork sequence [S2: `target.cpp:193-200`])
 ├─ run radio driver begin/configuration
 ├─ configure DIO2 RF switching only after successful radio initialization
 ├─ apply Cap-specific PA configuration only after successful initialization
 └─ enter receive; publish RADIO_READY or detailed RADIO_FAILED event
```

The Cardputer fork provides the reset, post-init DIO2/PA ordering, and a starting PA tuple [S2: `target.cpp:189-246`]. Plai independently says the Cap's expander P0 must stay high, while DIO2 owns TX/RX selection [S3: `hal_cardputer.cpp:380-384`]. Do not copy Plai's `setDio3AsTcxoCtrl` blindly: its driver uses an explicit TCXO command [S3: `radio/sx1262.h:130-148`], but oscillator configuration must be verified on this specific cap.

**Safety:** never transmit without the supplied antenna attached. Log the configured frequency, bandwidth, spreading factor, coding rate, power, DIO2-RF-switch state, expander probe/bit values, and radio driver error code on every failed initialization.

## 4. Target architecture

### 4.1 Components and dependency direction

```text
main/app_main.cpp
  ├── cardputer_bsp       native board drivers; no MeshCore symbols
  ├── ui                  presentation/controller; no Arduino/MeshCore headers
  ├── app_model           bounded contacts/messages/unread state
  ├── storage             NVS/LittleFS; append/replay only
  └── mesh_transport      Cardcore-owned abstract contract
        └── meshcore_compat  Arduino-ESP32, MeshCore, RadioLib only
```

Suggested project tree:

```text
0104-cardcore-mesh-terminal/
├── CMakeLists.txt
├── sdkconfig.defaults
├── sdkconfig.defaults.esp32s3
├── partitions.csv
├── main/
│   ├── CMakeLists.txt
│   └── app_main.cpp
└── components/
    ├── cardputer_bsp/{include,display,keyboard,battery,cap_io,radio_diag}
    ├── app_model/{include,contacts,messages,channels}
    ├── storage/{include,settings_store,message_log}
    ├── mesh_transport/{include,mesh_transport.cpp}
    ├── meshcore_compat/{include,upstream,meshcore_transport.cpp}
    └── ui/{include,text_editor,ui_controller,screens}
```

`meshcore_compat` must be the only component that includes `Arduino.h`, `RadioLib.h`, MeshCore headers, or Arduino filesystem adapters. It receives commands in Cardcore value types and publishes Cardcore value-type events. This protects the rest of the firmware from MeshCore API/version churn and makes an eventual native adapter a replacement, not a rewrite.

### 4.2 Task/queue model

```text
TCA8418 polling/interrupt task ─ UiInputEvent ─┐
                                                ▼
                                         [UI task, core 1]
                                             │ UiCommand
                                             ▼
                                      [mesh command queue]
                                             │
DIO1 ISR ─ task notification ───► [mesh task, core 0]
                                             │ MeshEvent
                                             ├────────────► [UI event queue]
                                             └────────────► [storage queue]
                                                              │
                                                       [storage task]
```

Only the mesh task calls MeshCore, RadioLib, or SX1262. Only storage calls LittleFS file APIs. The UI owns screen state and composer state. The UI may optimistically render “queued” but must only turn it into sent/acknowledged when the mesh event says so.

```cpp
void mesh_task(void*) {
  ESP_ERROR_CHECK(cap_lora_init());
  publish_mesh_event(MeshEvent::radio_ready_or_error());

  for (;;) {
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(20));
    transport.process_radio_irq_and_rx();

    MeshCommand command{};
    while (xQueueReceive(mesh_command_q, &command, 0) == pdTRUE) {
      transport.execute(command); // bounded; never accesses UI or LittleFS
    }

    transport.process_timers();
    transport.process_tx_queue();
  }
}
```

The upstream RadioLib wrapper uses a polling `loop()`, starts receive, reads a received packet, then restarts receive [S1: `sources/meshcore/src/helpers/radiolib/RadioLibWrappers.cpp:86-146`]. Cardcore may adapt that behavior, but must retain the single-owner rule and never call it from an ISR.

### 4.3 Stable transport API

The API belongs to Cardcore, so avoid leaking `ContactInfo`, `mesh::Packet`, `String`, or Arduino `Stream` across it.

```cpp
constexpr size_t kTextCapacity = 160;
struct PublicKey { std::array<uint8_t, 32> bytes; };
struct RadioMetrics { int16_t rssi_dbm; int8_t snr_quarter_db; };

enum class MeshEventKind : uint8_t {
  RadioReady, RadioFailed, Advert, ChannelText, DirectText,
  RoomPost, TxQueued, TxAcknowledged, TxFailed
};

struct MeshEvent {
  MeshEventKind kind;
  uint32_t local_id;
  PublicKey peer;
  RadioMetrics metrics;
  uint32_t timestamp;
  uint8_t channel;
  char text[kTextCapacity + 1];
  esp_err_t error;
};

class MeshTransport {
 public:
  virtual esp_err_t start() = 0;
  virtual esp_err_t send_channel(uint8_t channel, std::string_view text,
                                 uint32_t local_id) = 0;
  virtual esp_err_t send_direct(const PublicKey&, std::string_view text,
                                uint32_t local_id) = 0;
  virtual esp_err_t fetch_room_posts(const PublicKey& room) = 0;
  virtual esp_err_t post_room(const PublicKey& room, std::string_view text,
                              uint32_t local_id) = 0;
  virtual esp_err_t send_advert() = 0;
  virtual void process_radio_irq_and_rx() = 0;
  virtual void process_timers() = 0;
  virtual void process_tx_queue() = 0;
  virtual ~MeshTransport() = default;
};
```

The direct-message adapter should call the upstream Companion/Secure Chat path, which uses `sendMessage`, selected contact path state, timestamps and expected ACK handling [S1: `examples/simple_secure_chat/main.cpp:384-418`]. The Room adapter should use the upstream Room Server protocol—not an invented BBS protocol—in this MVP. The server's posts include sender timestamp, signed-text flags, author key prefix, text, and a calculated ACK token [S1: `examples/simple_room_server/MyMesh.cpp:41-114`].

## 5. Data, persistence, and failure semantics

### 5.1 Bounded RAM model

Cardcore should not represent the entire history as a GUI list. Load an indexed working set, retain only a fixed recent display window, and append durable records in the background.

```cpp
constexpr size_t kMaxContacts = 32;
constexpr size_t kMaxChannels = 2;       // Public + one configured channel
constexpr size_t kMaxCachedMessages = 64;
constexpr size_t kTxQueueLength = 8;
constexpr size_t kUiEventQueueLength = 16;
constexpr size_t kMeshCommandQueueLength = 8;
```

A direct or channel message is stored as text plus immutable metadata: local ID, timestamp, peer/contact key or channel, direction, delivery state, RSSI/SNR, and text length. Never identify a contact solely by a display name; names are mutable and non-unique. The public key is the identity.

### 5.2 NVS vs LittleFS

| Store | Contents | Write policy |
|---|---|---|
| NVS namespace `cardcore` | schema version, display preference, selected channel, radio preset ID, node name, region-config checksum | infrequent, transactional key changes |
| NVS protected identity namespace | private identity material, only after validated import/generation | never log private bytes; factory reset explicitly destroys it |
| LittleFS `/messages.log` | append-only message records | storage task batches writes; sync at controlled boundaries |
| LittleFS `/messages.idx` | rebuildable compact index/checkpoint | periodically replace using temp file + rename |
| LittleFS `/contacts.cache` | optional cache only if MeshCore adapter does not own contact persistence | write after debounce; rebuildable from MeshCore authoritative state |

The upstream Secure Chat example stores identity and node preferences using Arduino filesystem APIs, loads a public group, and creates a new identity when absent [S1: `examples/simple_secure_chat/main.cpp:296-356`]. Cardcore may use upstream storage inside the compatibility component initially, but it must document the source of truth and avoid two independent writers for identity or contacts. Recommended transition: adapter owns MeshCore-required identity/contact serialization beneath LittleFS; Cardcore `storage` owns UI message history and settings.

### 5.3 Append-record recovery pseudocode

```text
on boot:
  scan messages.log sequentially
  validate magic, schema, declared length, CRC
  stop at first torn/invalid trailing record
  retain newest 64 records in RAM and rebuild index if stale

on new MeshEvent that should be durable:
  UI model updates immediately
  storage event is queued
  storage task appends {header, payload, crc}
  after N records or controlled idle: fsync/checkpoint
```

A corrupt trailing record is expected after battery loss and must not erase older valid history. The user-visible result should be a one-time “recovered recent history” diagnostic, not a boot failure.

## 6. UI and input design

Use the 240×135 display as a terminal, not a tiny phone UI. Render a fixed status bar (battery, radio state, peer/queue count), a content body, and one-line hints/composer footer. A message composer holds a fixed 160-byte UTF-8-safe buffer initially; accept only complete code points and prevent splitting a multi-byte character when deleting or truncating.

```text
┌ CARDCORE                 88%  ● 4 peers ┐
│ [C] channels  [D] direct [R] room [N] nearby │
│                                             │
│  Last RX 12 s      -97 dBm  SNR 6.5         │
├─────────────────────────────────────────────┤
│ Fn+↑/↓ navigate • Enter select • Fn+Esc back│
└─────────────────────────────────────────────┘
```

The local keyboard module gives physical keys, not the terminal semantics. Create a Cardcore input translation layer that turns `ScanSnapshot` changes into `UiInputEvent` values (`Text`, `Backspace`, `Enter`, `Navigate`, `Back`, `Advert`). The `0083` app is the local evidence for queue-driven semantics, FN arrow navigation, caps/shift state and non-blocking polling [L5]. Do not have screen code inspect raw key numbers.

Incoming radio events must be visible while editing: preserve composer text and cursor; add an unread badge/toast; never steal focus or submit the current text. This exact behavior is acceptance A7.

## 7. Design decisions

### Decision: use an isolated Arduino compatibility component for MVP

- **Context:** upstream MeshCore currently depends broadly on Arduino/FS/Stream/RadioLib types, while pure `meshcore-c` describes itself as WIP [S1, S4].
- **Options considered:** rewrite/port MeshCore now; use meshcore-c now; embed Arduino only around MeshCore; build an Arduino/PlatformIO firmware.
- **Decision:** native ESP-IDF application plus one `meshcore_compat` component containing Arduino-ESP32, pinned MeshCore and RadioLib.
- **Rationale:** minimizes interoperability risk without surrendering IDF ownership of application structure, storage, tasks and diagnostics.
- **Consequences:** component build integration is an early spike; no Arduino types outside the boundary; replacement with `MeshCoreNativeTransport` remains possible later.
- **Status:** accepted.

### Decision: Companion role, never a repeater in MVP

- **Context:** this device is handheld, battery-powered, and has variable antenna placement.
- **Options considered:** Companion, repeater, Room Server, combined role.
- **Decision:** Companion only.
- **Rationale:** matches upstream role semantics and prevents the device from degrading mesh paths [S1: `README.md:10-19`].
- **Consequences:** a stock repeater/Room Server must exist for range and posts; tests require an external peer.
- **Status:** accepted.

### Decision: native board ownership, with a shared I2C bus

- **Context:** keyboard and Cap expander share I2C; fragmented M5/Arduino/native ownership creates hard-to-debug driver conflicts.
- **Options considered:** M5Unified owns all board I/O; Arduino `Wire` owns I2C; Cardcore native BSP owns bus/devices.
- **Decision:** Cardcore BSP creates and owns IDF buses/devices; UI libraries consume its interfaces.
- **Rationale:** prevents duplicate driver installation and makes cap-expander bit-level safety testable.
- **Consequences:** adapt rather than directly copy the legacy-I2C keyboard scanner.
- **Status:** accepted.

### Decision: do not use microSD in MVP

- **Context:** SD and radio share SPI wires; the MVP requires reliable radio first.
- **Options considered:** SD-backed history now; LittleFS now, SD later.
- **Decision:** NVS + LittleFS only.
- **Rationale:** avoids another SPI device, filesystem, power and hot-plug failure surface.
- **Consequences:** bounded history; explicit storage quota and compaction are required.
- **Status:** accepted.

### Decision: treat Cap oscillator/PA settings as experimentally verified board configuration

- **Context:** third-party fork configuration contains conflicting TCXO/current values across base/Cap stanzas [S2: `platformio.ini:31-34,61-73`].
- **Options considered:** hard-code copied values; disable TX until validated; develop board-specific configuration probe.
- **Decision:** create a diagnostic/configuration test and record known-good values in `cap_lora_board_config.h` only after validation.
- **Rationale:** prevents cargo-cult radio configuration and potentially invalid RF operation.
- **Consequences:** raw-RF bring-up is a mandatory gate before MeshCore integration.
- **Status:** accepted.

## 8. Phased implementation plan

### Phase 0 — create a clean firmware project and reproducible configuration

1. Create `0104-cardcore-mesh-terminal` beside existing firmware projects.
2. Read its `README.md`, source `/home/manuel/esp/esp-idf-5.5.4/export.sh`, run `idf.py set-target esp32s3`, and use `idf.py build` thereafter.
3. Add `main/idf_component.yml`—not a project-root manifest—for Arduino-ESP32 and any registry component.
4. Start from the 8 MB / USB Serial-JTAG patterns in `0083`, but create a dedicated partition table with NVS, factory app, and LittleFS storage.
5. Disable Wi-Fi/BLE/GPS/SD in defaults; after changing defaults, remove `sdkconfig` before build so they take effect.

Deliverable: empty app boots, logs over the connected `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_AC:A7:04:04:88:F4-if00`, and does not claim radio readiness.

### Phase 1 — board smoke test

Implement `cardputer_bsp` in small independently testable units:

1. display/backlight and a text diagnostics screen;
2. TCA8418 discovery + event counter using the shared I2C bus;
3. ADC battery sample with calibrated/explicitly labelled raw value;
4. Cap expander probe and read-modify-write P0 enable;
5. SX1262 reset/BUSY/SPI chip-status diagnostic;
6. raw receive and transmit against a second known-good device.

Do not enable MeshCore until raw packets are received and transmitted consistently. Add a `status` console command that prints every pin, I2C ACK result, expander registers, SX1262 status/version, selected radio config, last RSSI/SNR, and errors.

### Phase 2 — compatibility spike and wire interoperability

1. Add Arduino-ESP32 as an IDF component and verify an IDF `app_main` can initialize the isolated adapter without taking I2C/SPI ownership from BSP.
2. Vendor/pin upstream MeshCore at commit `219812b` using a documented submodule or dependency archive; record its license notice.
3. Implement the smallest possible `MeshCoreArduinoTransport`: identity load/create, initial advert, continuous radio processing, one public group message, one direct message.
4. Validate wire behavior with a **stock** MeshCore repeater/node, not just a second Cardcore build.
5. Capture raw packet diagnostics only with keys/redacted payloads; never put identity private keys or channel PSKs in logs or ticket docs.

### Phase 3 — model and terminal UI

1. Implement fixed-capacity contacts, messages and channel model.
2. Build Home, Nearby, Contacts, Channel, Direct, Composer, Status and Radio Failure screens.
3. Add toast/unread event behavior while composing.
4. Implement sent/queued/acknowledged/failed display states driven solely by `MeshEvent`.

### Phase 4 — persistence and power-loss behavior

1. Add settings schema and migration version to NVS.
2. Add append-only LittleFS history, checkpoint/recovery tests, storage quota, and compaction policy.
3. Test reset while idle, while composing, after received message, and while a delayed durable write is queued.
4. Expose a deliberate “erase identity and all local data” action with confirmation; no hidden identity regeneration.

### Phase 5 — Room Server client

1. Detect/retain Room Server adverts as contacts with a room role.
2. Implement upstream login/sync/request/post sequence in the adapter.
3. Render recent posts and author key-prefix/name resolution.
4. Validate against unmodified upstream Simple Room Server first.
5. Only after this is stable, write a separate design for MESHBOARD extensions; extensions must be versioned and must not silently alter stock Room Server protocol.

## 9. Test strategy

### Unit tests (host or IDF component tests)

* composer UTF-8 boundary, delete, counter, and 160-byte limit;
* contact replacement/eviction and public-key equality;
* message-log record encoding, CRC rejection, torn trailing recovery and compaction;
* event-to-model reducers (incoming while composing; ACK after UI optimistic row);
* transport command validation and max lengths;
* cap-expander bit-level read-modify-write using a fake I2C register backend.

### Hardware integration tests

| Test | Setup | Pass condition |
|---|---|---|
| keyboard/expander coexistence | ADV + cap | typing while repeatedly reading/writing only P0 works |
| SX1262 detection | antenna attached | reset/BUSY/status diagnostic succeeds repeatedly |
| RX/TX raw | known-good LoRa peer | payload and RSSI/SNR observed in both directions |
| MeshCore advert | stock node/repeater | nearby entry contains expected public-key prefix |
| encrypted group | same channel key on peer | peer receives text and Cardcore renders peer text |
| direct ACK | routable stock peer | UI shows ack or accurately timed failure |
| Room Server | upstream simple server | recent post read and new post visible |
| resilience | inject radio init failure | usable radio-error screen and retry, no crash |

Run serial work with one owner. Use the stable `/dev/serial/by-id/...` name, not a second concurrent monitor. If a manual reset is necessary during a future probe, hold one monitor session open and ask the operator before the reset rather than retrying serial opens.

## 10. Risks, constraints, and open questions

1. **Arduino as IDF component integration:** prove build/link/runtime ownership with a dedicated spike before UI work. The architecture is intentionally chosen to contain this risk, not pretend it is absent.
2. **Radio configuration:** PA/current/TCXO settings require hardware validation. The fork is evidence of a working direction, not a substitute for measurement or board documentation.
3. **Regulatory operation:** region, frequency, power, duty-cycle and antenna use must be explicitly selected for the deployment jurisdiction. Never ship an unconstrained high-power default.
4. **Memory:** upstream fork's `MAX_CONTACTS=200`, 30 groups, and 256 offline queue [S2: `platformio.ini:84-88`] are not a target. Profile stack/heap at every phase and preserve fixed limits.
5. **Time:** without GPS/Wi-Fi, timestamp quality may be weak after battery loss. Decide whether RTC clock/last-known time is sufficient for MVP and display “time unknown” rather than inventing wall clock accuracy.
6. **Identity import:** define a safe, user-confirmed import encoding and erasure behavior before implementing it. Do not type raw private keys into serial logs.
7. **License:** upstream MeshCore is MIT, while Plai is GPL-3.0. The implementation must not copy GPL-derived code into an intended MIT/non-GPL product without a deliberate licensing decision.
8. **Current connected board:** the serial/JTAG device is present at the stable by-id path, but this research ticket did not take ownership of the port or flash/probe the device. Phase 1 must establish what firmware is currently on it before erasing anything.

## 11. Review checklist for the first implementation PR

- [ ] No UI/storage/BSP file includes Arduino or MeshCore headers.
- [ ] Exactly one task calls radio/MeshCore APIs; ISR only signals it.
- [ ] Exactly one native I2C bus owner creates TCA8418 and cap-expander devices.
- [ ] Expander writes use read-modify-write and touch only P0 for RF enable.
- [ ] Antenna warning and explicit radio diagnostics exist before TX test.
- [ ] No microSD/Wi-Fi/BLE/GPS code or config enabled.
- [ ] Queue depths, text limits, contact/message limits are constants and all failures are surfaced.
- [ ] Identity/channel secrets are never logged or stored in UI history.
- [ ] Raw radio and stock-MeshCore interop evidence is attached before declaring protocol support.
- [ ] Partition/default changes were tested after removing stale `sdkconfig`.

## References

### Local evidence

* [L1] `components/cardputer_kb/README.md:1-22` — reusable keyboard component scope.
* [L2] `components/cardputer_kb/include/cardputer_kb/scanner.h:36-78` — ADV defaults and scanner API.
* [L3] `components/cardputer_kb/unified_scanner.cpp:25-227` — TCA8418 implementation and legacy I2C ownership caveat.
* [L4] `0083-cardputer-adv-animation-ui/main/app_main.cpp:67-130` — UI queue/render loop pattern.
* [L5] `0083-cardputer-adv-animation-ui/main/ui_kb.cpp:100-259` — semantic input mapping.
* [L6] `0038-cardputer-adv-serial-terminal/README.md:1-20` — USB Serial/JTAG recovery-console pattern.
* [L7] `0083-cardputer-adv-animation-ui/sdkconfig.defaults:6-25` and `partitions.csv:1-6` — 8 MB baseline.

### Retrieved source snapshots (ticket `sources/`)

* [S1] `sources/meshcore` — `meshcore-dev/MeshCore`, commit `219812b`, MIT.
* [S2] `sources/meshcore-cardputer-adv` — Stachugit fork, commit `e341957`, MIT as stated in `license.txt`.
* [S3] `sources/plai` — d4rkmen/plai, commit `fda03cf`, GPL-3.0; reference only unless licensing is made compatible.
* [S4] `sources/meshcore-c` — Nicolai-Electronics/meshcore-c, commit `1e373d5`, WIP native-C investigation.

### User-supplied external references to verify during implementation

* M5Stack Cardputer-ADV and Cap LoRa-1262 documentation.
* Espressif Arduino-as-IDF-component documentation (pin the version compatible with ESP-IDF 5.5.4).
* RadioLib ESP Component Registry entry and its current ESP-IDF integration instructions.
* MeshCore packet/payload/companion protocol documents in `sources/meshcore/docs/`.
