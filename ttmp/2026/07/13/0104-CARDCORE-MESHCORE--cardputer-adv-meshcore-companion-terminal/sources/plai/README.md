# Plai

**A standalone Meshtastic communicator for M5Stack CardPuter**

> _Plai_ is the Ukrainian word for a mountain trail — a reliable path for your data to travel when you're off the beaten track.

<p align="center">
  <img src="pics/nodes_list.png" width="1280" alt="Plai Node List">
</p>

<p align="center">
  <a href="https://youtu.be/2kFs07OOl1w">
    <img src="https://img.youtube.com/vi/2kFs07OOl1w/maxresdefault.jpg" width="480" alt="Plai Demo Video">
  </a>
  <br>
  <a href="https://youtu.be/2kFs07OOl1w">Watch the demo video</a>
</p>

Most Meshtastic nodes rely on a phone via BLE or WiFi. Plai takes a different approach: it turns the CardPuter into a **self-contained messaging terminal**. No phone required — just you, the LoRa CAP, and the keyboard.

## Why Plai?

- **Full standalone operation** — No WiFi, no BLE. Direct LoRa mesh communication with on-device UI.
- **Unlimited message history** — The entire profile, message history, and node database live on the SD card. Storage is limited only by your card size.
- **Swap and survive** — Reboot or switch firmwares without losing your place in the mesh. Everything persists on SD.
- **Pro navigation** — PgUp / PgDown / Home / End for fast scrolling through long threads and node lists.
- **Debug tools** — Built-in Packet Monitor (last 50 packets) and Trace Route history (last 50 attempts per node).
- **Custom alerts** — Individual channel notifications with distinct sounds.
- **Display sleep** — Screen turns off when idle to save power; wake on keypress or radio activity.
- **Emoji support** — Unicode emoji rendered from PNG files on SD card (`/sdcard/emoji/u<HEX>.png`). Automatically falls back to a placeholder for missing glyphs. Last 10 emoji cached in RAM for fast repeated rendering.
- **Stats app** — Tabbed overview: node info (incl. battery), system (heap, storage, uptime, **firmware version**), radio, node DB (**online nodes**, last hour), GPS, mesh port distribution, **running tasks**.
- **Fully compatible** with Meshtastic network v2.7+
- **Ping auto-reply**: respond automatically when someone #ping's the channel
- **New node greetings**: send a welcome broadcast to the channel and/or a Direct Message when a new node appears

## Apps

### Nodes

Full node management with up to 1000 nodes persisted on SD card.

<p align="center">
  <img src="pics/nodes.png" width="480" alt="Node Detail">
</p>
<p align="center">
  <img src="pics/nodes_list1.png" width="480" alt="Node List">
</p>
<p align="center">
  <img src="pics/nodes_favorites.png" width="480" alt="Node Favorites List">
</p>

- Node list with signal strength, hops, battery, role, encryption indicators
- **Remembers** last **sort order** and **selected node** across reboots
- 9 sorting modes (name, role, signal, hops, last heard, favorites first, distance, etc.) _hotkey_ for sorting [1..9], [TAB] to select sorting mode
- **Sort by distance** — [9] sorts nodes nearest-first using haversine distance from your position (fixed or live GPS). Requires position enabled in settings. Hold [CTRL] in the node list to see distance in km for each node that has a known position.
- Relay node display — see which node relayed each packet _hotkey_ [R] to jump to relay node
- Favorite marking and quick-jump navigation _hotkey_ [F] to toggle favorite
- Ignore nodes — mark nodes as ignored to filter their traffic _hotkey_ [I] to toggle ignored
- Node detail view with hardware model, position, and metrics _hotkey_ [Fn] + [ENTER] to open
- Direct Messages _hotkey_ [ENTER] to open
- Neighbors _hotkey_ [B] — exchange neighbor info with selected node; [Fn] + [B] open neighbors list view
- Exchange node info _hotkey_ [N] — send node info request to selected node
- Exchange position _hotkey_ [P] — send position request to selected node
- Traceroute _hotkey_ [T] to open recent traceroute logs. [Fn] + [T] to start traceroute immediately
- Map view _hotkey_ [M] — offline OSM map centered on the selected node with all node positions
- Quick messages _hotkey_ [Q] — open quick message templates editor

#### Direct Messages

<p align="center">
  <img src="pics/nodes_dm.png" width="480" alt="Direct Messages">
</p>
<p align="center">
  <img src="pics/input.png" width="480" alt="Message Input">
</p>

- Direct messaging with delivery status (pending → sent → ACK → delivered → failed)
- Channel invitation _hotkey_ [I] — invite the node to a channel (sends `#invite name=key` DM)
- Full keyboard input with Cyrillic layout support
- File-backed message history on SD card
- Clear chat _hotkey_ [BACKSPACE] to clear all messages
- Hold [CTRL] to display message info (**timestamps in local time**)

#### Traceroute

<p align="center">
  <img src="pics/nodes_tr_log.png" width="480" alt="Traceroute Log">
</p>
<p align="center">
  <img src="pics/nodes_tr_details.png" width="480" alt="Traceroute Details">
</p>

- Traceroute with hop-by-hop detail, round-trip duration, and SNR at each hop
- **Success sound** when a traceroute completes
- Last 50 traceroute attempts stored per node
- Visual route map with color-coded signal quality
- Press [T] to start new traceroute

#### Favorites list

Dedicated view of all your favorite nodes, stored persistently on SD card (`favorites.dat`).

- **Open** — [Fn] + [F] from node list
- **Add/remove** — [F] on a node in the main list toggles favorite status (shown in gold)
- **Navigation** — Arrow keys, [PgUp]/[PgDown] for page scroll, [Fn]+[↑] Home / [Fn]+[↓] End
- **Jump to node** — [ENTER] selects the highlighted favorite and returns to node list with that node focused
- **Remove** — [DEL] removes the selected favorite; [Fn]+[DEL] clears all favorites
- Favorites survive node database resets and firmware updates (file-backed)

#### Ignore list

Manage ignored nodes whose traffic is filtered at the mesh layer. Packets from ignored nodes are dropped before processing.

<p align="center">
  <img src="pics/nodes_ignored.png" width="480" alt="Node Ignored List">
</p>

- **Open** — [Fn] + [I] from node list
- **Add/remove** — [I] on a node in the main list toggles ignored status (shown in red)
- Same navigation and shortcuts as favorites — [ENTER] jump to node, [DEL] remove selected, [Fn]+[DEL] clear all
- Ignored nodes stored in `ignorelist.dat` on SD; survives resets and updates

#### Neighbors

View direct neighbors (0-hop nodes) reported by each node via the `NEIGHBORINFO_APP` packet.

<p align="center">
  <img src="pics/nodes_neighbors.png" width="480" alt="Node Neighbors List">
</p>

- **Exchange** — [B] from node list — sends your direct neighbors and requests theirs back
- **Open list** — [Fn] + [B] from node list — shows cached neighbor data for the selected node
- **Navigation** — Arrow keys, [PgUp]/[PgDown] for page scroll, [Fn]+[↑] Home / [Fn]+[↓] End
- **Jump to node** — [ENTER] selects the highlighted neighbor and returns to node list with that node focused
- **Read-only** — The neighbor list is populated automatically from received `NEIGHBORINFO_APP` packets; no manual editing
- Each node's neighbor list stored in `neighbors/<node_id>.dat` on SD; cleaned up when the node is deleted

#### Quick messages

Manage reusable message templates for fast sending in DM and channel chats.

In chat views press [Q] to open the quick message select dialog:

<p align="center">
  <img src="pics/quick_message_select.png" width="480" alt="Quick Message Select">
</p>

Select a message and press [ENTER] to edit it before sending:

<p align="center">
  <img src="pics/quick_message_edit.png" width="480" alt="Quick Message Edit">
</p>

Or press [ENTER] to send the selected message directly.

In _Nodes_ view press [Q] to open the quick message editor, where you can add, edit, and delete messages:

<p align="center">
  <img src="pics/quick_message_view.png" width="480" alt="Quick Message View">
</p>

- **Open** — [Q] from node list
- **Add** — [A] opens the text editor to create a new template
- **Edit** — [ENTER] opens the text editor for the selected message
- **Delete** — [DEL] removes the selected message (with confirmation)
- **Navigation** — Arrow keys, [PgUp]/[PgDown] for page scroll, [Fn]+[↑] Home / [Fn]+[↓] End
- Long messages scroll in place when selected
- Templates stored in `templates.txt` on SD; created with defaults on first use

Example (SD card path: `/meshtastic/templates.txt`):

```
Hello! 👋
Welcome! 🎉
Good morning! ☀
Good afternoon! 😊
Good evening! 🌙
Good night! 🌟
Sweet dreams! 😴
Welcome aboard! 🏠
Great to see you! 😄
Howdy, partner! 🤠
Yo, what's up? 😎
Peek-a-boo, who's there? 👀
Hey, champ! 💪
Hey there, buddy! 🐱
Salute, legend! 🏆
Wow, what a meeting! 🤩
Bye! 👋
See you later! 🤝
All the best! 🍀
Take care! 🫶
Have a great day! 🌈
Catch you soon! ✌
Direct! 🎯
1 hop 🐇
2 hops 🐇🐇
3 hops 🐇🐇🐇
4 hops 🐇🐇🐇🐇
5 hops 🐇🐇🐇🐇🐇
```

#### Map View

**Zoom 4, style: topo**

<p align="center">
  <img src="pics/nodes_map_z4.png" width="480" alt="Node Map Zoom 4">
</p>

**Zoom 11, style: osm**

<p align="center">
  <img src="pics/nodes_map_z11.png" width="480" alt="Node Map Zoom 11">
</p>

Offline map powered by OpenStreetMap raster tiles stored on SD card. Node positions are rendered in real-time from the in-memory index — zero SD I/O for markers.

- **Open from node list** — [M] opens the map centered on the selected node
- **Open from detail view** — [M] opens the map centered on the selected node
- **Pan** — Arrow keys [←][→][↑][↓] move the viewport by 25%
- **Zoom** — [Fn]+[↑] zoom in, [Fn]+[↓] zoom out (zoom 2–15)
- **Center on selected node** — [C] re-centers on the selected node
- **Center on our node** — [Fn]+[C] re-centers on our node's GPS position
- **Switch map style** — [TAB] cycles through map styles: osm → dark → voyager → topo
- **Refresh** — [ENTER] re-renders the map (picks up new node positions received via mesh)
- **Back** — [ESC] returns to previous view

Node markers are color-coded: selected node in orange with crosshair. Labels show the node short name (or hex ID fallback) at zoom ≥ 8 or for the selected node.

The map displays zoom level and center coordinates in the bottom corners.

##### Downloading Map Tiles

Use the `map/download_osm_tiles.py` script to download tiles for offline use. Requires Python 3 and [Pillow](https://pypi.org/project/Pillow/) (`pip install Pillow`).

Tiles are downloaded as JPEG (quality 75) in the standard `{zoom}/{x}/{y}.jpg` slippy map format. Low zoom levels (global overview) are downloaded worldwide; higher zooms are limited to a radius around the center point.

**Available styles:**

| Style     | Description                                            | Best for                         |
| --------- | ------------------------------------------------------ | -------------------------------- |
| `dark`    | CartoDB Dark Matter — dark background, bright features | **Small TFT displays** (default) |
| `osm`     | Standard OpenStreetMap — light, detailed               | General use                      |
| `voyager` | CartoDB Voyager — clean, modern look                   | Light theme                      |
| `topo`    | OpenTopoMap — topographic with elevation contours      | Outdoor / hiking                 |

**Examples:**

```bash
# New York City, dark theme (recommended), 50km radius, zoom 2-12
python map/download_osm_tiles.py --lat 40.7128 --lon -74.006 --radius 50 --style dark

# London, standard OSM with contrast boost for small screen
python map/download_osm_tiles.py --lat 51.5074 --lon -0.1278 --radius 40 --style osm --contrast 1.3

# Tokyo, voyager style, extended zoom for city detail
python map/download_osm_tiles.py --lat 35.6762 --lon 139.6503 --radius 60 --max-zoom 14

# Kyiv, dark theme with extra brightness
python map/download_osm_tiles.py --lat 50.4501 --lon 30.5234 --radius 80 --style dark --brightness 1.2

# Berlin, topographic map for hiking
python map/download_osm_tiles.py --lat 52.52 --lon 13.405 --radius 30 --style topo

# Sydney, wide coverage with lower max zoom to save space
python map/download_osm_tiles.py --lat -33.8688 --lon 151.2093 --radius 100 --max-zoom 10

# Paris, dark theme, global tiles up to zoom 7, regional up to zoom 13
python map/download_osm_tiles.py --lat 48.8566 --lon 2.3522 --radius 50 --global-zoom 7 --max-zoom 13 --style dark
```

**Options:**

| Flag             | Default       | Description                                 |
| ---------------- | ------------- | ------------------------------------------- |
| `--lat`, `--lon` | (required)    | Center coordinates                          |
| `--radius`       | 50            | Coverage radius in km for regional tiles    |
| `--min-zoom`     | 2             | Minimum zoom level                          |
| `--max-zoom`     | 12            | Maximum zoom level                          |
| `--global-zoom`  | 5             | Download ALL tiles globally up to this zoom |
| `--style`        | dark          | Map style: `dark`, `osm`, `voyager`, `topo` |
| `--contrast`     | 1.0           | Contrast multiplier (e.g. 1.3 = +30%)       |
| `--brightness`   | 1.0           | Brightness multiplier                       |
| `--saturation`   | 1.0           | Color saturation multiplier                 |
| `--output`       | `map/<style>` | Output directory                            |

After downloading, copy the style folder (e.g. `map/dark/`) to `/sdcard/map/` on the device SD card, so tiles end up at `/sdcard/map/dark/{z}/{x}/{y}.jpg`. You can have multiple styles on the card and switch between them in **Settings → System → Map style**.

**Prebuilt tiles in zoom 1-8 are available to download: [Google Drive](https://drive.google.com/drive/folders/1w3my-z-5raQ3HxWVPcNoBzR7aFre3rxt?usp=drive_link)**

Download, unzip then can customize with your location with bigger zoom (12-15) using the script.

### Channels

Multi-channel group chat supporting up to 8 channels.

<p align="center">
  <img src="pics/channels.png" width="480" alt="Channel Chat">
</p>
<p align="center">
  <img src="pics/channels_list.png" width="480" alt="Channel List">
</p>
<p align="center">
  <img src="pics/channels_edit.png" width="480" alt="Channel Edit">
</p>
<p align="center">
  <img src="pics/channels_chat.png" width="480" alt="Channel Chat">
</p>
<p align="center">
  <img src="pics/channel_chat_info.png" width="480" alt="Channel Chat Info">
</p>

- Channel list with unread message counts
- **Channel hash** — Each channel shows its hash (e.g. `#A3`) derived from name and PSK. Lets you see how the hash depends on settings and correlate packets in Monitor (packets display the same `#XX` channel byte).
- Channel creation _hotkey_ [Fn] + [SPACE] to open channel creation dialog
- Channel editing _hotkey_ [Fn] + [ENTER] to open channel editing dialog
- Channel chat _hotkey_ [ENTER] to open channel chat
- Individual notification sounds per channel (additional built-in alert tones)

#### New node greetings & #ping auto-reply

Many of us send "test test" and get no reply. Now Plai can reply automatically when you add **#ping** in your channel message — no more wondering if anyone's listening.

- **#ping auto-reply** — Add `#ping` anywhere in a channel message; Plai responds with a configurable template. Macros: `#short`, `#long`, `#id`, `#hops`, `#snr`, `#rssi`
- **New node greetings** — When a node appears for the first time (after receiving their NodeInfo), Plai can send a welcome broadcast to the channel and/or a Direct Message. Same macros apply.
- **Per-channel settings** — Each of the 8 channels has its own greeting and ping reply templates.

There are predefined templates for the greetings and ping reply. You can use them or enter your own custom text, holding [Fn] key.

Example: _"Look who is here! #long, welcome to HAM Community of Smartwill city. I can see you with #hops hops #snr/#rssi"_

#### Channel invitation

Share a channel with another node via Direct Message.

- **Sending** — In DM with a node, press [I]. Select a channel; Plai sends a DM in format `#invite name=base64_psk` (name max 11 chars, key base64-encoded). If the node has no public key, a confirmation is shown before sending unencrypted.
- **Receiving** — Enable **Settings → Security → Invitations**. When a DM starts with `#invite ` and matches `#invite channel_name=base64_psk`, Plai creates a new channel at the first free slot. Duplicate channels (same name and key) are ignored.
- Requires at least one free channel slot to accept an invitation.

### Monitor

Live radio packet feed for debugging and network analysis.

<p align="center">
  <img src="pics/monitor.png" width="480" alt="Packet Detail">
</p>
<p align="center">
  <img src="pics/monitor_list.png" width="480" alt="Packet List">
</p>
<p align="center">
  <img src="pics/monitor_header.png" width="480" alt="Packet Header">
</p>
<p align="center">
  <img src="pics/monitor_payload.png" width="480" alt="Packet Payload">
</p>

- Real-time TX/RX packet display with port labels (TEXT, POS, NODE, TELE, ROUT, TRAC, etc.)
- Channel hash (`#XX`) shown per packet — matches the hash in Channels for easy correlation
- Color-coded direction, node badges, and SNR indicators
- Color-coded packet ID for easy relay identification
- From/To node name resolution from NodeDB
- Scrollable packet list with detail drill-down view (**extra fields**, improved layout)
- Hold **[CTRL]** in packet list for **additional fields**
- Last 50 packets in a static ring buffer
- Select first item for autoscroll

### Stats

Network and system statistics in a tabbed view — at a glance diagnostics without leaving the mesh.

<p align="center">
  <img src="pics/stats.png" width="480" alt="Stats app">
</p>
<p align="center">
  <img src="pics/stats_node.png" width="480" alt="Stats — Node info">
</p>
<p align="center">
  <img src="pics/stats_system.png" width="480" alt="Stats — System info">
</p>
<p align="center">
  <img src="pics/stats_radio.png" width="480" alt="Stats — Radio info">
</p>
<p align="center">
  <img src="pics/stats_nodedb.png" width="480" alt="Stats — Node DB info">
</p>
<p align="center">
  <img src="pics/stats_gps.png" width="480" alt="Stats — GPS info">
</p>
<p align="center">
  <img src="pics/stats_mesh.png" width="480" alt="Stats — Mesh info">
</p>

- **Node** — Node ID, long/short name, role, PKI status, **battery** (when available)
- **System** — Heap (total/free/min), SD storage, uptime, date/time, **firmware version**
- **Radio** — Frequency, modem preset, waveform (SF/BW/CR), TX power, RX/TX packet counts
- **Node DB** — Total nodes, **online** (heard within the last hour), favorites, ignored, messages sent/received
- **GPS** — Fix quality, satellites (used/in view), coordinates, altitude, HDOP
- **Mesh** — Cumulative RX/TX and **port distribution** (Text, NodeInfo, Position, etc.) with percentages — counts reflect **all** packets seen by the radio stack, not a short rolling window; CRC errors shown separately
- **Tasks** — FreeRTOS tasks sorted by priority: name, **CPU core** (when enabled in IDF), **priority**, **stack high-water mark** (color hints when stack is tight)
- Tab navigation — [←][→] switch tabs; [↑][↓] scroll **any** tab with overflow
- Auto-refresh every 2 seconds

### Settings

Complete device and mesh configuration stores in NVS. You can export and import settings to SD card for backup and restore it later.

<p align="center">
  <b><span style="color:red;">&#x26A0;️</span> <span style="color:red;">Mesh keys are in NVS. Don't forget to backup them to SD card if you want to keep them after firmware update!</span></b>
</p>

Node database and chat history are stored on SD card and not affected by firmware updates.

<p align="center">
  <img src="pics/settings.png" width="480" alt="Settings">
</p>

- System: brightness, volume, timezone
- LoRa: region, modem preset, TX power, hop limit
- Security: channel PSK management, invitations (auto-add channels from `#invite` DMs), **derive public key from private key** (X25519)
- Node info: name, short name, role
- Position: GPS enable, fixed position, broadcast interval. GPS time sync: callback-based; system clock updated from GPS only when drift exceeds 60 seconds
- Telemetry: device metrics broadcast
- Export/Import settings to SD card
- Clear all nodes
- UI: **GPS** status icon in the bar only when there is a **position fix**; footer **hints** restored on multi-choice dialogs

## Hardware

### Required

| Component                 | Description                              |
| ------------------------- | ---------------------------------------- |
| **M5Stack CardPuter ADV** | ESP32-S3 portable terminal with keyboard |
| **LoRa CAP**              | M5Stack SX1262 LoRa module (868/915 MHz) |
| **SD Card**               | For profile, messages, and node database |

## Emoji

Plai can render emoji and other characters missing from the built-in font using PNG images stored on the SD card.

### Setup

1. Create the directory `/sdcard/emoji/` on your SD card.
2. Place PNG files named by Unicode codepoint in uppercase hex, e.g.:
   - `u1F600.png` — 😀 (Grinning Face)
   - `u2764.png` — ❤️ (Red Heart)
   - `u1FA9B.png` — 🪛 (Screwdriver)
3. Any PNG size works — images are automatically scaled to match the current font height.

If the PNG file for a codepoint is missing, the character is displayed as the font's default unknown glyph.

The last 10 emoji are cached in memory (full PNG data) so repeated renders are instant with zero SD card I/O.

## Install

Beta version is available in **M5Apps** (Installer → Cloud → Beta tests).

Standalone version will be added to **M5Burner** soon.

> Look for M5Apps in M5Burner.

## Mesh Protocol

Built from scratch on ESP-IDF — not a fork of the Meshtastic firmware.

- **Encryption**: AES-CTR with channel PSK, X25519 public-key cryptography
- **Multi-channel**: Up to 8 channels with individual PSKs
- **Routing**: Hop-limit flooding (1–7 hops) with Meshtastic-compatible duplicate detection
- **Reliability**: ACK/NACK with automatic retries, implicit ACK via rebroadcast
- **Priority TX queue**: ACK > Routing > Admin > Reliable > Default > Background
- **Duty cycle**: Channel and air utilization tracking
- **Channel activity**: Detects traffic on the configured channel; **default frequency slot** follows the primary channel name
- **TX pacing**: Configurable delay for **reply** traffic to reduce collisions
- **Roles**: Telemetry broadcasts are **not** sent for `CLIENT_HIDDEN` nodes
- **Multi-region**: US, EU_433, EU_868, CN, JP, ANZ, KR, TW, RU, IN, and more
- **Packet encoding**: Nanopb (Protocol Buffers) for full Meshtastic wire compatibility

## Building from Source

### Prerequisites

- [ESP-IDF v5.5.x](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/get-started/) (project tested with **5.5.4**)
- ESP32-S3 target
- [Nanopb](https://jpa.kapsi.fi/nanopb/) generator **0.4.9.x** — required to generate the protobuf sources (see below)

### Clone

The Meshtastic `.proto` definitions are included as a git submodule in `protobufs/`, so clone recursively:

```bash
git clone --recursive https://github.com/d4rkmen/plai
```

If you already cloned without `--recursive`, pull the submodule in:

```bash
git submodule update --init --recursive
```

### Generate protobuf sources

The Nanopb C++ sources under `main/meshtastic/` are generated from the `protobufs/` submodule and are **not** checked in, so you must generate them once after cloning (and again whenever the submodule is updated). On Windows:

```bat
regen-protos.bat
```

The script invokes the Nanopb `protoc` generator over `protobufs/meshtastic/*.proto` and writes the `*.pb.cpp` / `*.pb.h` files into `main/meshtastic/`. Edit the `protoc.exe` path inside `regen-protos.bat` to point at your local Nanopb 0.4.9.x generator.

### Configure

Project-wide, target-agnostic configuration lives in `sdkconfig.defaults` and is applied automatically on the first build, so the generated `sdkconfig` always matches the intended setup. As the project moves toward multiple targets, keep shared custom options in `sdkconfig.defaults`; any target-specific overrides belong in `sdkconfig.defaults.<target>` (e.g. `sdkconfig.defaults.esp32s3`).

### Build & Flash

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

### HAL Configuration

Hardware components can be individually toggled via menuconfig:

```bash
idf.py menuconfig
# Navigate to: HAL Configuration
```

| Option             | Default | Description                   |
| ------------------ | ------- | ----------------------------- |
| `HAL_USE_DISPLAY`  | on      | ST7789 display via LovyanGFX  |
| `HAL_USE_KEYBOARD` | on      | Keyboard input (requires I2C) |
| `HAL_USE_RADIO`    | on      | SX1262 LoRa radio             |
| `HAL_USE_SDCARD`   | on      | SD card (FAT/exFAT)           |
| `HAL_USE_GPS`      | on      | ATGM336H GPS                  |
| `HAL_USE_SPEAKER`  | on      | I2S audio output              |
| `HAL_USE_LED`      | on      | WS2812 RGB LED                |
| `HAL_USE_BAT`      | on      | Battery voltage monitor       |
| `HAL_USE_I2C`      | on      | I2C master bus                |
| `HAL_USE_BUTTON`   | on      | Home button                   |

## Project Structure

```
Plai/
├── main/
│   ├── apps/                  # Application layer
│   │   ├── launcher/          # Home screen & system bar
│   │   ├── app_nodes/         # Node list, DM, traceroute
│   │   ├── app_channels/      # Channel group chat
│   │   ├── app_monitor/       # Live packet feed
│   │   ├── app_stats/        # Network & system statistics
│   │   ├── app_settings/     # Configuration UI
│   │   └── utils/             # Shared UI components
│   ├── hal/                   # Hardware Abstraction Layer
│   │   ├── hal.h              # Base HAL class
│   │   ├── hal_cardputer.*    # M5Cardputer implementation
│   │   ├── display/           # LovyanGFX display driver
│   │   ├── keyboard/          # TCA8418 / IOMatrix drivers
│   │   ├── radio/             # SX1262 LoRa driver
│   │   └── ...                # GPS, speaker, LED, battery, etc.
│   ├── mesh/                  # Meshtastic protocol
│   │   ├── mesh_service.*     # Core mesh service
│   │   ├── node_db.*          # Node database (SD-backed)
│   │   ├── mesh_data.*        # Message store & packet log
│   │   └── packet_router.*    # Priority TX/RX queues
│   ├── meshtastic/            # Generated Nanopb sources (from protobufs/, git-ignored)
│   ├── settings/              # NVS settings with cache
│   └── main.cpp               # Entry point
├── map/
│   └── download_osm_tiles.py  # OSM tile downloader for offline map
├── protobufs/                 # Meshtastic .proto definitions (git submodule)
├── components/
│   ├── LovyanGFX/             # Display graphics library
│   ├── mooncake/              # App framework
│   └── Nanopb/                # Protocol Buffers
├── regen-protos.bat           # Regenerates main/meshtastic/ from protobufs/
├── sdkconfig.defaults         # Project-wide build configuration
└── Kconfig.projbuild          # menuconfig HAL options
```

## Support the Project

If you wish to support the project:

**Ethereum (ETH):** `0x249346dFCcE54B0677E6c484c7e9ea27B2424526`

## Credits

- Fonts: [efont](https://openlab.ring.gr.jp/efont/) Unicode bitmap fonts from the Linux distribution
- Emoji: [Google Noto Color Emoji](https://github.com/googlefonts/noto-emoji) — licensed under [SIL Open Font License 1.1](https://github.com/googlefonts/noto-emoji/blob/main/LICENSE)
- Icons: Free icons by [Icons8](https://icons8.com)
- Sounds: [Epidemic Sound](https://www.epidemicsound.com/)
- Platform: [M5Stack](https://m5stack.com/) M5Cardputer

## License

This project is licensed under the [GNU General Public License v3.0](https://www.gnu.org/licenses/gpl-3.0.html) — see [LICENSE](LICENSE) for details.
