Got it — exhibition-scale, multiple screens, but not mission control.

---

# MLED Controller — Exhibition Edition

## 1. Main View (Nodes + Quick Control)

```
┌────────────────────────────────────────────────────────────────────┐
│  🎛️  MLED Controller                                              │
├────────────────────────────────────────────────────────────────────┤
│                                                                    │
│  NODES                                          [ Select All ]     │
│                                                                    │
│  ┌────────────────────────────────────────────────────────────┐    │
│  │ ☑  🟢 pedestal-01       -42dBm     🎨 Rainbow              │    │
│  │ ☑  🟢 pedestal-02       -38dBm     🎨 Rainbow              │    │
│  │ ☑  🟢 wall-left         -45dBm     🎨 Rainbow              │    │
│  │ ☐  🟢 wall-right        -51dBm     🎨 Solid Blue           │    │
│  │ ☐  🟡 ceiling           -71dBm     🎨 Off          (weak)  │    │
│  │ ☐  🔴 floor-01          ---        offline (3m)            │    │
│  └────────────────────────────────────────────────────────────┘    │
│                                                                    │
│  5 online · 1 offline · 3 selected                                 │
│                                                                    │
│  ──────────────────────────────────────────────────────────────    │
│                                                                    │
│  QUICK APPLY                                                       │
│                                                                    │
│  [ 🌈 Rainbow ] [ 💙 Calm ] [ 🔥 Warm ] [ ⚡ Pulse ] [ ⬛ Off ]    │
│                                                                    │
│  ──────────────────────────────────────────────────────────────    │
│                                                                    │
│  Brightness: [███████░░░] 70%          [ Apply to Selected ]       │
│                                                                    │
├────────────────────────────────────────────────────────────────────┤
│  [ 🏠 Nodes ]        [ 🎨 Patterns ]        [ ℹ️ Status ]          │
└────────────────────────────────────────────────────────────────────┘
```

---

## 2. Patterns View (Edit & Create Presets)

```
┌────────────────────────────────────────────────────────────────────┐
│  🎛️  MLED Controller                                              │
├────────────────────────────────────────────────────────────────────┤
│                                                                    │
│  PRESETS                                            [ + New ]      │
│                                                                    │
│  ┌────────────────────────────────────────────────────────────┐    │
│  │  🌈 Rainbow          cycle, speed 50                       │    │
│  │  💙 Calm             solid #2244AA, 60%                    │    │
│  │  🔥 Warm             gradient orange→red                   │    │
│  │  ⚡ Pulse            strobe white, 2Hz                     │    │
│  │  ⬛ Off              solid black                           │    │
│  └────────────────────────────────────────────────────────────┘    │
│                                                                    │
│  ──────────────────────────────────────────────────────────────    │
│                                                                    │
│  EDIT: Rainbow                                                     │
│                                                                    │
│  Name:     [ Rainbow____________ ]                                 │
│                                                                    │
│  Type:     [ Rainbow Cycle ▼ ]                                     │
│                                                                    │
│  Speed:    [████░░░░░░] 50                                         │
│                                                                    │
│  Brightness: [███████░░░] 70%                                      │
│                                                                    │
│                                                                    │
│  [ 👁️ Preview on Selected ]    [ 💾 Save ]    [ 🗑️ Delete ]        │
│                                                                    │
├────────────────────────────────────────────────────────────────────┤
│  [ 🏠 Nodes ]        [ 🎨 Patterns ]        [ ℹ️ Status ]          │
└────────────────────────────────────────────────────────────────────┘
```

---

## 3. Status View (What's Wrong?)

```
┌────────────────────────────────────────────────────────────────────┐
│  🎛️  MLED Controller                                              │
├────────────────────────────────────────────────────────────────────┤
│                                                                    │
│  SYSTEM STATUS                                      ✅ Healthy     │
│                                                                    │
│  Controller:   192.168.1.112 (eth0)                                │
│  Multicast:    239.255.0.1:5000                                    │
│  Nodes:        5/6 online                                          │
│                                                                    │
│  ──────────────────────────────────────────────────────────────    │
│                                                                    │
│  PROBLEMS                                                          │
│                                                                    │
│  ⚠️  ceiling — weak signal (-71dBm), may drop                      │
│  ❌ floor-01 — offline for 3 minutes                               │
│                                                                    │
│  ──────────────────────────────────────────────────────────────    │
│                                                                    │
│  NODE DETAILS                                                      │
│                                                                    │
│  │ Node          │ Signal │ Uptime  │ Pattern     │ Status   │    │
│  ├───────────────┼────────┼─────────┼─────────────┼──────────┤    │
│  │ pedestal-01   │ -42dBm │ 2h 14m  │ Rainbow     │ 🟢 good  │    │
│  │ pedestal-02   │ -38dBm │ 2h 14m  │ Rainbow     │ 🟢 good  │    │
│  │ wall-left     │ -45dBm │ 1h 58m  │ Rainbow     │ 🟢 good  │    │
│  │ wall-right    │ -51dBm │ 2h 14m  │ Solid Blue  │ 🟢 good  │    │
│  │ ceiling       │ -71dBm │ 0h 42m  │ Off         │ 🟡 weak  │    │
│  │ floor-01      │ ---    │ ---     │ ---         │ 🔴 off   │    │
│                                                                    │
│  ──────────────────────────────────────────────────────────────    │
│                                                                    │
│  [ 🔄 Refresh All ]                    [ ⚙️ Settings ]             │
│                                                                    │
├────────────────────────────────────────────────────────────────────┤
│  [ 🏠 Nodes ]        [ 🎨 Patterns ]        [ ℹ️ Status ]          │
└────────────────────────────────────────────────────────────────────┘
```

---

## 4. Settings (Tucked Away)

```
┌────────────────────────────────────────────────────────────────────┐
│  🎛️  MLED Controller › Settings                      [ ← Back ]   │
├────────────────────────────────────────────────────────────────────┤
│                                                                    │
│  NETWORK                                                           │
│                                                                    │
│  Bind IP:         [ eth0: 192.168.1.112 ▼ ]                        │
│  Multicast:       [ 239.255.0.1 ] : [ 5000 ]                       │
│                                                                    │
│  ──────────────────────────────────────────────────────────────    │
│                                                                    │
│  TIMING                                                            │
│                                                                    │
│  Discovery interval:    [ 1000 ] ms                                │
│  Offline threshold:     [ 30   ] seconds                           │
│                                                                    │
│  ──────────────────────────────────────────────────────────────    │
│                                                                    │
│  PRESETS                                                           │
│                                                                    │
│  [ 📤 Export All ]     [ 📥 Import ]                               │
│                                                                    │
│  ──────────────────────────────────────────────────────────────    │
│                                                                    │
│  [ 💾 Save ]                                                       │
│                                                                    │
└────────────────────────────────────────────────────────────────────┘
```

---

## Summary

| Screen | Purpose |
|--------|---------|
| **Nodes** | See everything, quick preset buttons, apply |
| **Patterns** | Edit/create presets with sliders |
| **Status** | "What's broken?" at a glance |
| **Settings** | Network config, hidden away |

3 main tabs + settings. That feel right?

---Away

# MLED Controller — Exhibition Edition Implementation Spec

## Overview

A minimal 3-tab UI for controlling MLED nodes at an exhibition. Direct pattern application (no two-phase cues), preset management, and basic diagnostics.

---

## Protocol Primitives (from mled_protocol.h)

| Primitive | Direction | Purpose | UI Usage |
|-----------|-----------|---------|----------|
| `PING` | Controller → Nodes | Discovery | Continuous background |
| `PONG` | Nodes → Controller | Presence + status | Populates node list |
| `PATTERN_APPLY` | Controller → Nodes | Immediate pattern change | "Apply" button, presets |
| `BEACON` | Controller → Nodes | Time sync broadcast | Background (optional) |
| `TIME_REQ/RESP` | Bidirectional | Clock offset measurement | Status view (optional) |

**Not used in exhibition edition:** `CUE_PREPARE`, `CUE_FIRE`, `CUE_CANCEL`, epoch management.

---

## Architecture

```
┌─────────────────────┐         UDP          ┌─────────────────┐
│  Controller Backend │ ◄──────────────────► │   MLED Nodes    │
│  (Python/asyncio)   │   PING/PONG/APPLY    │   (ESP32-C6)    │
└──────────┬──────────┘                      └─────────────────┘
           │
           │ HTTP + WebSocket (localhost:8080)
           ▼
┌─────────────────────┐
│   Web UI (Preact)   │
│   + Zustand store   │
└─────────────────────┘
```

---

## Data Models

### Node

```typescript
interface Node {
  node_id: string;          // hex, e.g. "4A7F2C01"
  name: string;             // from PONG or user-assigned
  ip: string;
  port: number;
  rssi: number;             // dBm, from PONG
  uptime_ms: number;        // from PONG
  last_seen: number;        // unix timestamp ms
  current_pattern: PatternConfig | null;
  status: 'online' | 'weak' | 'offline';
}
```

### PatternConfig

```typescript
interface PatternConfig {
  type: PatternType;
  brightness: number;       // 0-100
  params: Record<string, number | string>;
}

type PatternType = 
  | 'solid' 
  | 'rainbow' 
  | 'gradient' 
  | 'pulse' 
  | 'off';
```

### Preset

```typescript
interface Preset {
  id: string;               // uuid
  name: string;
  icon: string;             // emoji
  config: PatternConfig;
}
```

---

## Backend API

### HTTP Endpoints

| Method | Path | Request Body | Response | Description |
|--------|------|--------------|----------|-------------|
| `GET` | `/api/nodes` | — | `Node[]` | List all known nodes |
| `GET` | `/api/presets` | — | `Preset[]` | List saved presets |
| `POST` | `/api/presets` | `Preset` | `Preset` | Create preset |
| `PUT` | `/api/presets/:id` | `Preset` | `Preset` | Update preset |
| `DELETE` | `/api/presets/:id` | — | `{ok: true}` | Delete preset |
| `POST` | `/api/apply` | `ApplyRequest` | `ApplyResponse` | Apply pattern to nodes |
| `GET` | `/api/settings` | — | `Settings` | Get current settings |
| `PUT` | `/api/settings` | `Settings` | `Settings` | Update settings |

#### ApplyRequest

```typescript
interface ApplyRequest {
  node_ids: string[] | 'all';
  config: PatternConfig;
}
```

#### ApplyResponse

```typescript
interface ApplyResponse {
  sent_to: string[];        // node_ids we sent to
  failed: string[];         // node_ids that were offline
}
```

### WebSocket Events (ws://localhost:8080/ws)

#### Server → Client

| Event | Payload | Trigger |
|-------|---------|---------|
| `node.update` | `Node` | PONG received, status change |
| `node.offline` | `{node_id: string}` | No PONG for threshold |
| `apply.ack` | `{node_id: string, success: boolean}` | After PATTERN_APPLY |
| `error` | `{message: string}` | Network issues, etc. |

#### Client → Server

| Event | Payload | Action |
|-------|---------|--------|
| `refresh` | — | Force PING broadcast |

---

## UI State (Zustand)

```typescript
interface Store {
  // Nodes
  nodes: Map<string, Node>;
  selectedNodeIds: Set<string>;
  
  // Presets
  presets: Preset[];
  editingPreset: Preset | null;
  
  // UI
  currentTab: 'nodes' | 'patterns' | 'status';
  globalBrightness: number;
  
  // Connection
  connected: boolean;
  lastError: string | null;
  
  // Actions
  selectNode(id: string, selected: boolean): void;
  selectAll(): void;
  selectNone(): void;
  applyPreset(preset: Preset): void;
  applyConfig(config: PatternConfig): void;
  savePreset(preset: Preset): void;
  deletePreset(id: string): void;
  setGlobalBrightness(value: number): void;
}
```

---

## Screen Specifications

### Screen 1: Nodes (Main View)

**Purpose:** See all nodes, select targets, quick-apply presets.

**Components:**

```
┌─ NodeList ─────────────────────────────────────────┐
│                                                    │
│  ┌─ NodeRow ────────────────────────────────────┐  │
│  │ [checkbox] [status dot] [name] [rssi] [pattern] │
│  └──────────────────────────────────────────────┘  │
│  ... repeated                                      │
│                                                    │
└────────────────────────────────────────────────────┘

┌─ PresetBar ────────────────────────────────────────┐
│ [PresetButton] [PresetButton] [PresetButton] ...   │
└────────────────────────────────────────────────────┘

┌─ BrightnessControl ────────────────────────────────┐
│ Brightness: [slider] 70%     [Apply to Selected]   │
└────────────────────────────────────────────────────┘
```

**Behavior:**

| Interaction | Action |
|-------------|--------|
| Check node | Toggle `selectedNodeIds` |
| "Select All" | Select all online nodes |
| Click preset | `POST /api/apply` with preset config + selected nodes |
| Adjust brightness | Update `globalBrightness` (local state) |
| "Apply to Selected" | `POST /api/apply` with current pattern + brightness |

**Status dot logic:**

| Condition | Color | Label |
|-----------|-------|-------|
| `last_seen < 5s` | 🟢 | — |
| `last_seen < 30s` | 🟡 | (weak) |
| `last_seen ≥ 30s` | 🔴 | offline |
| `rssi < -70dBm` | 🟡 | (weak signal) |

---

### Screen 2: Patterns (Preset Editor)

**Purpose:** Create, edit, preview presets.

**Components:**

```
┌─ PresetList ───────────────────────────────────────┐
│ [icon] [name] [summary]                  [+ New]   │
│ ... repeated, selectable                           │
└────────────────────────────────────────────────────┘

┌─ PresetEditor ─────────────────────────────────────┐
│ Name: [input]                                      │
│ Icon: [emoji picker or input]                      │
│ Type: [dropdown]                                   │
│                                                    │
│ ┌─ PatternParams (dynamic) ─────────────────────┐  │
│ │ (varies by pattern type)                      │  │
│ └───────────────────────────────────────────────┘  │
│                                                    │
│ Brightness: [slider]                               │
│                                                    │
│ [Preview] [Save] [Delete]                          │
└────────────────────────────────────────────────────┘
```

**Pattern parameters by type:**

| Type | Parameters |
|------|------------|
| `solid` | `color: hex` |
| `rainbow` | `speed: 0-100` |
| `gradient` | `color_start: hex`, `color_end: hex` |
| `pulse` | `color: hex`, `speed: 0-100` |
| `off` | (none) |

**Behavior:**

| Interaction | Action |
|-------------|--------|
| Select preset | Load into editor |
| "+ New" | Create blank preset, open editor |
| "Preview" | `POST /api/apply` with editor config to selected nodes |
| "Save" | `POST/PUT /api/presets` |
| "Delete" | `DELETE /api/presets/:id` (with confirm) |

---

### Screen 3: Status

**Purpose:** See what's healthy, what's broken.

**Components:**

```
┌─ SystemBanner ─────────────────────────────────────┐
│ [status icon] [summary text]                       │
│ Controller: [ip]   Nodes: [n/m online]             │
└────────────────────────────────────────────────────┘

┌─ ProblemList ──────────────────────────────────────┐
│ [⚠️/❌] [node name] — [problem description]         │
│ ... repeated (or "No problems" if empty)           │
└────────────────────────────────────────────────────┘

┌─ NodeTable ────────────────────────────────────────┐
│ Name | Signal | Uptime | Pattern | Status          │
│ -----+--------+--------+---------+--------         │
│ ...                                                │
└────────────────────────────────────────────────────┘

[Refresh All]                        [Settings]
```

**Problem detection rules:**

| Condition | Icon | Message |
|-----------|------|---------|
| `status === 'offline'` | ❌ | `{name} — offline for {duration}` |
| `rssi < -70dBm` | ⚠️ | `{name} — weak signal ({rssi}dBm)` |
| `last_seen > 10s && < 30s` | ⚠️ | `{name} — intermittent connection` |

---

### Screen 4: Settings (Modal or Sub-page)

**Purpose:** Network config, import/export.

**Fields:**

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `bind_ip` | dropdown | auto | Network interface |
| `multicast_group` | text | `239.255.0.1` | Multicast address |
| `multicast_port` | number | `5000` | UDP port |
| `discovery_interval_ms` | number | `1000` | PING frequency |
| `offline_threshold_s` | number | `30` | When to mark offline |

**Actions:**

| Button | Behavior |
|--------|----------|
| "Export Presets" | Download `presets.json` |
| "Import Presets" | File picker, merge/replace presets |
| "Save" | `PUT /api/settings`, restart discovery |

---

## Implementation Plan

### Phase 1: Backend Core (Days 1-2)

- [ ] UDP socket handling (bind, multicast join)
- [ ] PING loop (configurable interval)
- [ ] PONG parser → node state
- [ ] PATTERN_APPLY encoder + sender
- [ ] In-memory node store with TTL expiry
- [ ] HTTP server (FastAPI or aiohttp)
  - [ ] `GET /api/nodes`
  - [ ] `POST /api/apply`
- [ ] WebSocket server
  - [ ] `node.update` events
  - [ ] `node.offline` events

### Phase 2: Backend Presets (Day 3)

- [ ] Preset CRUD endpoints
- [ ] JSON file persistence (`~/.mled/presets.json`)
- [ ] Settings endpoints
- [ ] Settings file persistence (`~/.mled/config.json`)

### Phase 3: UI Shell (Day 4)

- [ ] Preact + Zustand project setup
- [ ] Tab navigation component
- [ ] WebSocket connection + reconnect logic
- [ ] Store hydration from HTTP endpoints

### Phase 4: Nodes Screen (Day 5)

- [ ] NodeList component
- [ ] NodeRow with checkbox, status dot, info
- [ ] Selection state management
- [ ] PresetBar with quick-apply buttons
- [ ] BrightnessControl slider
- [ ] "Apply to Selected" action

### Phase 5: Patterns Screen (Day 6)

- [ ] PresetList component
- [ ] PresetEditor form
- [ ] Dynamic PatternParams by type
- [ ] Color picker for solid/gradient
- [ ] Preview / Save / Delete actions
- [ ] New preset flow

### Phase 6: Status Screen (Day 7)

- [ ] SystemBanner with health summary
- [ ] ProblemList with auto-detection
- [ ] NodeTable sortable view
- [ ] Refresh button
- [ ] Settings modal/page

### Phase 7: Polish (Day 8)

- [ ] Loading states
- [ ] Error toasts
- [ ] Offline/reconnecting banner
- [ ] Keyboard shortcuts (select all: ⌘A, apply: Enter)
- [ ] Mobile-friendly layout
- [ ] Test with real nodes

---

## File Structure

```
mled-controller/
├── backend/
│   ├── main.py              # entry point
│   ├── udp.py               # MLED protocol handling
│   ├── nodes.py             # node state management
│   ├── presets.py           # preset CRUD
│   ├── settings.py          # config management
│   ├── api.py               # HTTP routes
│   └── ws.py                # WebSocket handler
│
├── frontend/
│   ├── index.html
│   ├── src/
│   │   ├── main.tsx         # entry
│   │   ├── store.ts         # Zustand store
│   │   ├── api.ts           # HTTP + WS client
│   │   ├── App.tsx          # shell + tabs
│   │   ├── screens/
│   │   │   ├── Nodes.tsx
│   │   │   ├── Patterns.tsx
│   │   │   └── Status.tsx
│   │   └── components/
│   │       ├── NodeRow.tsx
│   │       ├── PresetButton.tsx
│   │       ├── PresetEditor.tsx
│   │       ├── Slider.tsx
│   │       └── ColorPicker.tsx
│   └── package.json
│
├── config/
│   └── default_presets.json
│
└── README.md
```

---

## Default Presets

```json
[
  {
    "id": "preset-rainbow",
    "name": "Rainbow",
    "icon": "🌈",
    "config": {
      "type": "rainbow",
      "brightness": 75,
      "params": { "speed": 50 }
    }
  },
  {
    "id": "preset-calm",
    "name": "Calm",
    "icon": "💙",
    "config": {
      "type": "solid",
      "brightness": 60,
      "params": { "color": "#2244AA" }
    }
  },
  {
    "id": "preset-warm",
    "name": "Warm",
    "icon": "🔥",
    "config": {
      "type": "gradient",
      "brightness": 70,
      "params": { "color_start": "#FF6B35", "color_end": "#AA2200" }
    }
  },
  {
    "id": "preset-pulse",
    "name": "Pulse",
    "icon": "⚡",
    "config": {
      "type": "pulse",
      "brightness": 80,
      "params": { "color": "#FFFFFF", "speed": 60 }
    }
  },
  {
    "id": "preset-off",
    "name": "Off",
    "icon": "⬛",
    "config": {
      "type": "off",
      "brightness": 0,
      "params": {}
    }
  }
]
```

---

