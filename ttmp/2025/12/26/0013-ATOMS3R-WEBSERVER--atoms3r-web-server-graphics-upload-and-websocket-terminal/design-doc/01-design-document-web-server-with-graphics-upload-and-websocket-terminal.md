---
Title: Design Document - Web Server with Graphics Upload and WebSocket Terminal
Ticket: 0013-ATOMS3R-WEBSERVER
Status: active
Topics:
    - webserver
    - websocket
    - graphics
    - atoms3r
    - preact
    - zustand
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Design for AtomS3R web server enabling graphics upload, WebSocket communication for button presses and terminal RX, and frontend terminal interface using Preact and Zustand
LastUpdated: 2025-12-26T08:54:28.656725757-05:00
WhatFor: ""
WhenToUse: ""
---

# Design Document - Web Server with Graphics Upload and WebSocket Terminal

> **NOTE (Status: draft / superseded):** This document was written as an early design draft and includes Arduino-centric pseudocode (e.g. `ESPAsyncWebServer`, `AsyncWebSocket`, `String`, `File`, and `SPIFFS` examples). After re-checking the repo, the ESP-IDF-first direction for `esp32-s3-m5` tutorials should prefer ESP-IDF-native building blocks (notably `esp_http_server` for HTTP+WebSocket) and a FATFS strategy that is compatible with runtime writes for uploads.
>
> Keep this document for history/brainstorming, but treat it as non-authoritative. The “final” design doc for this ticket will be added alongside this one and should be used for implementation.

## Executive Summary

This document describes the design for a web server running on the M5Stack AtomS3R that enables:
1. **Graphics Upload**: Upload images (PNG, JPEG, GIF, raw RGB565) via HTTP POST and display them on the 128×128 pixel display
2. **WebSocket Communication**: Real-time bidirectional communication for:
   - Receiving button press events from the device
   - Receiving terminal RX data (UART input)
   - Sending terminal TX data (UART output) from the frontend
3. **Frontend Terminal**: Web-based terminal interface built with Preact and Zustand for sending/receiving serial data

This draft explored an ESPAsyncWebServer-based approach alongside M5GFX for display rendering and a Preact/Zustand frontend. During follow-up verification, we should treat ESPAsyncWebServer usage as “Arduino-integrated” (see `ATOMS3R-CAM-M12-UserDemo`) and prefer an ESP-IDF-native stack for a new `esp32-s3-m5` tutorial unless we explicitly accept Arduino as a dependency.

## Problem Statement

Currently, interacting with the AtomS3R requires:
- Physical access to upload graphics (via USB/serial)
- Serial terminal connection for debugging/control
- No remote access to device state (button presses, display content)

We need a solution that:
- Allows remote graphics upload via web interface
- Provides real-time bidirectional communication without polling
- Enables web-based terminal access for debugging
- Works over WiFi without requiring physical USB connection
- Maintains low memory footprint suitable for ESP32-S3 constraints

## Proposed Solution

### Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    Frontend (Browser)                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │
│  │ Graphics     │  │ Terminal     │  │ WebSocket    │    │
│  │ Upload UI    │  │ Interface    │  │ Client       │    │
│  │ (Preact)     │  │ (Preact)     │  │ (Zustand)    │    │
│  └──────────────┘  └──────────────┘  └──────────────┘    │
└─────────────────────────────────────────────────────────────┘
                          │ HTTP/WS
                          │
┌─────────────────────────────────────────────────────────────┐
│              ESP32-S3 (AtomS3R)                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │
│  │ HTTP Server  │  │ WebSocket    │  │ Graphics     │    │
│  │ (Port 80)    │  │ Handler      │  │ Handler      │    │
│  └──────────────┘  └──────────────┘  └──────────────┘    │
│         │                │                    │              │
│  ┌──────┴────────┐ ┌────┴────┐      ┌───────┴──────┐     │
│  │ Button ISR    │ │ UART    │      │ Display      │     │
│  │ (GPIO41)      │ │ Driver  │      │ (M5GFX)       │     │
│  └───────────────┘ └─────────┘      └──────────────┘     │
│         │                │                    │              │
│  ┌──────┴────────────────┴────────────────────┴──────┐     │
│  │           FreeRTOS Event Queue                     │     │
│  └────────────────────────────────────────────────────┘     │
│                                                              │
│  ┌────────────────────────────────────────────────────┐   │
│  │         FATFS Partition (/storage/graphics)         │   │
│  └────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### Component Breakdown

#### 1. HTTP Server (ESPAsyncWebServer)

**Purpose**: Serve frontend assets and handle graphics upload

**Endpoints**:
- `GET /`: Serve main HTML page (embedded in firmware)
- `GET /api/graphics`: List uploaded graphics files
- `GET /api/graphics/:filename`: Download graphics file
- `POST /api/graphics/upload`: Upload new graphics file
- `DELETE /api/graphics/:filename`: Delete graphics file
- `GET /api/status`: Get device status (IP, free heap, etc.)

**Upload Handler**:
```cpp
server.on("/api/graphics/upload", HTTP_POST,
    [](AsyncWebServerRequest* request) {
        request->send(200, "application/json", 
                     "{\"status\":\"ok\"}");
    },
    [](AsyncWebServerRequest* request, String filename,
       size_t index, uint8_t* data, size_t len, bool final) {
        // Save uploaded file to FATFS partition
        // NOTE: This is Arduino/ESPAsync pseudocode. In an ESP-IDF-first implementation,
        // prefer a raw-body PUT/POST handler + `fopen("/storage/graphics/<name>", "wb")`.
        static File upload_file;
        if (!index) {
            // Pseudocode: open a file on a mounted filesystem (FATFS recommended).
            upload_file = /* open_file_for_write */ ("/storage/graphics/" + filename);
        }
        if (upload_file) {
            upload_file.write(data, len);
        }
        if (final) {
            upload_file.close();
            // Trigger display update if auto-display enabled
        }
    });
```

#### 2. WebSocket Handler

**Purpose**: Real-time bidirectional communication

**Connection**: `ws://<device-ip>/ws`

**Message Protocol** (JSON):

**Client → Server**:
```json
{
  "type": "terminal_tx",
  "data": "Hello from browser\n"
}
```

**Server → Client**:
```json
{
  "type": "button_press",
  "event": "click" | "hold",
  "timestamp": 1234567890
}
```

```json
{
  "type": "terminal_rx",
  "data": "Hello from UART\n",
  "timestamp": 1234567890
}
```

```json
{
  "type": "graphics_updated",
  "filename": "image.png",
  "timestamp": 1234567890
}
```

**Implementation**:
```cpp
AsyncWebSocket ws("/ws");

void onWebSocketEvent(AsyncWebSocket* server,
                      AsyncWebSocketClient* client,
                      AwsEventType type,
                      void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        ESP_LOGI(TAG, "WS client #%u connected", client->id());
        // Send welcome message
        ws.text(client->id(), "{\"type\":\"connected\"}");
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo* info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len) {
            if (info->opcode == WS_TEXT) {
                data[len] = 0;
                handleWebSocketMessage(client->id(), (char*)data);
            }
        }
    }
}

void handleWebSocketMessage(uint32_t client_id, const char* msg) {
    // Parse JSON message
    JsonDocument doc;
    deserializeJson(doc, msg);
    
    String type = doc["type"];
    if (type == "terminal_tx") {
        String data = doc["data"];
        // Send to UART
        uart_write_bytes(UART_NUM_1, data.c_str(), data.length());
    }
}
```

#### 3. Button Handler

**Purpose**: Detect button presses and send events via WebSocket

**Implementation**:
```cpp
static QueueHandle_t button_queue;

static void IRAM_ATTR button_isr(void* arg) {
    BaseType_t hp_task_woken = pdFALSE;
    ButtonEvent ev = {.type = BUTTON_CLICK, .timestamp = xTaskGetTickCountFromISR()};
    xQueueSendFromISR(button_queue, &ev, &hp_task_woken);
    if (hp_task_woken) {
        portYIELD_FROM_ISR();
    }
}

void button_task(void* arg) {
    ButtonEvent ev;
    while (true) {
        if (xQueueReceive(button_queue, &ev, portMAX_DELAY)) {
            // Debounce logic
            static uint32_t last_press = 0;
            uint32_t now = xTaskGetTickCount();
            if (now - last_press > pdMS_TO_TICKS(200)) {
                last_press = now;
                // Send to WebSocket clients
                JsonDocument doc;
                doc["type"] = "button_press";
                doc["event"] = ev.type == BUTTON_CLICK ? "click" : "hold";
                doc["timestamp"] = now;
                String msg;
                serializeJson(doc, msg);
                ws.textAll(msg);
            }
        }
    }
}
```

#### 4. UART Terminal Handler

**Purpose**: Bridge UART RX to WebSocket, WebSocket TX to UART

**Implementation**:
```cpp
void uart_rx_task(void* arg) {
    uint8_t buf[256];
    while (true) {
        int len = uart_read_bytes(UART_NUM_1, buf, sizeof(buf) - 1,
                                   pdMS_TO_TICKS(100));
        if (len > 0) {
            buf[len] = 0;
            // Send to WebSocket clients
            JsonDocument doc;
            doc["type"] = "terminal_rx";
            doc["data"] = (char*)buf;
            doc["timestamp"] = xTaskGetTickCount();
            String msg;
            serializeJson(doc, msg);
            ws.textAll(msg);
        }
    }
}

// Called from WebSocket handler
void send_to_uart(const char* data, size_t len) {
    uart_write_bytes(UART_NUM_1, data, len);
}
```

#### 5. Graphics Display Handler

**Purpose**: Display uploaded graphics on screen

**Supported Formats**:
- PNG (via M5GFX decoder)
- JPEG (via JPEG decoder library)
- GIF (via AnimatedGIF library)
- Raw RGB565 (direct pixel data)

**Implementation**:
```cpp
void display_graphics(const char* filename) {
    // NOTE: This is Arduino/ESPAsync pseudocode. In an ESP-IDF-first implementation,
    // use POSIX I/O on the mounted FATFS path, e.g. `FILE* f = fopen(path, "rb")`.
    File file = /* open_file_for_read */ (String("/storage/graphics/") + filename);
    if (!file) {
        ESP_LOGE(TAG, "Failed to open %s", filename);
        return;
    }
    
    size_t file_size = file.size();
    uint8_t* buffer = (uint8_t*)malloc(file_size);
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate buffer");
        file.close();
        return;
    }
    
    file.read(buffer, file_size);
    file.close();
    
    // Detect format and decode
    if (strstr(filename, ".png")) {
        display.drawPng(buffer, file_size, 0, 0);
    } else if (strstr(filename, ".jpg") || strstr(filename, ".jpeg")) {
        // JPEG decode and display
    } else if (strstr(filename, ".gif")) {
        // GIF decode and display frame-by-frame
    } else if (strstr(filename, ".rgb565")) {
        // Direct RGB565 display
        uint16_t* pixels = (uint16_t*)buffer;
        for (int y = 0; y < 128; y++) {
            for (int x = 0; x < 128; x++) {
                canvas.drawPixel(x, y, pixels[y * 128 + x]);
            }
        }
        canvas.pushSprite(0, 0);
    }
    
    free(buffer);
}
```

#### 6. Frontend (Preact + Zustand)

**Purpose**: Web-based UI for graphics upload and terminal

**Project Structure**:
```
frontend/
├── src/
│   ├── components/
│   │   ├── GraphicsUpload.tsx
│   │   ├── Terminal.tsx
│   │   └── StatusBar.tsx
│   ├── stores/
│   │   └── websocket.ts      # Zustand store for WebSocket
│   ├── App.tsx
│   └── main.tsx
├── package.json
└── vite.config.ts
```

**Zustand Store**:
```typescript
import create from 'zustand';

interface WebSocketState {
  connected: boolean;
  messages: Message[];
  send: (type: string, data: any) => void;
  connect: () => void;
  disconnect: () => void;
}

interface Message {
  type: 'button_press' | 'terminal_rx' | 'graphics_updated';
  data?: any;
  timestamp: number;
}

const useWebSocket = create<WebSocketState>((set, get) => {
  let ws: WebSocket | null = null;
  
  return {
    connected: false,
    messages: [],
    
    connect: () => {
      const deviceIp = localStorage.getItem('deviceIp') || '192.168.4.1';
      ws = new WebSocket(`ws://${deviceIp}/ws`);
      
      ws.onopen = () => set({ connected: true });
      ws.onclose = () => set({ connected: false });
      ws.onmessage = (event) => {
        const msg = JSON.parse(event.data);
        set((state) => ({
          messages: [...state.messages, msg]
        }));
      };
    },
    
    send: (type: string, data: any) => {
      if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ type, data }));
      }
    },
    
    disconnect: () => {
      ws?.close();
      set({ connected: false });
    }
  };
});
```

**Terminal Component**:
```tsx
import { useRef, useEffect } from 'preact/hooks';
import { useWebSocket } from '../stores/websocket';

export function Terminal() {
  const terminalRef = useRef<HTMLDivElement>(null);
  const inputRef = useRef<HTMLInputElement>(null);
  const { messages, send, connected } = useWebSocket();
  
  useEffect(() => {
    // Scroll to bottom on new message
    if (terminalRef.current) {
      terminalRef.current.scrollTop = terminalRef.current.scrollHeight;
    }
  }, [messages]);
  
  const handleSubmit = (e: Event) => {
    e.preventDefault();
    const input = inputRef.current;
    if (input && input.value) {
      send('terminal_tx', input.value + '\n');
      input.value = '';
    }
  };
  
  const terminalMessages = messages.filter(m => 
    m.type === 'terminal_rx'
  );
  
  return (
    <div class="terminal">
      <div ref={terminalRef} class="terminal-output">
        {terminalMessages.map(msg => (
          <div>{msg.data}</div>
        ))}
      </div>
      <form onSubmit={handleSubmit}>
        <input
          ref={inputRef}
          type="text"
          disabled={!connected}
          placeholder={connected ? "Type command..." : "Disconnected"}
        />
      </form>
    </div>
  );
}
```

**Graphics Upload Component**:
```tsx
import { useState } from 'preact/hooks';

export function GraphicsUpload() {
  const [uploading, setUploading] = useState(false);
  const [deviceIp, setDeviceIp] = useState(
    localStorage.getItem('deviceIp') || '192.168.4.1'
  );
  
  const handleUpload = async (e: Event) => {
    const input = e.target as HTMLInputElement;
    const file = input.files?.[0];
    if (!file) return;
    
    setUploading(true);
    const formData = new FormData();
    formData.append('file', file);
    
    try {
      const res = await fetch(`http://${deviceIp}/api/graphics/upload`, {
        method: 'POST',
        body: formData
      });
      if (res.ok) {
        alert('Upload successful!');
      } else {
        alert('Upload failed');
      }
    } catch (err) {
      alert('Upload error: ' + err);
    } finally {
      setUploading(false);
    }
  };
  
  return (
    <div class="graphics-upload">
      <input
        type="file"
        accept=".png,.jpg,.jpeg,.gif,.rgb565"
        onChange={handleUpload}
        disabled={uploading}
      />
      {uploading && <div>Uploading...</div>}
    </div>
  );
}
```

### Asset Bundling Strategy

**Frontend Assets**: Embedded in firmware using memory-mapped partition

**Process**:
1. Build frontend with Vite: `npm run build`
2. Generate C array from `dist/index.html`:
   ```bash
   xxd -i dist/index.html > frontend_html.h
   ```
3. Include in firmware and serve:
   ```cpp
   #include "frontend_html.h"
   
   server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
       request->send_P(200, "text/html", 
                      index_html_start, 
                      index_html_end - index_html_start);
   });
   ```

**Graphics Storage**: FATFS partition (`/storage/graphics/`)

**Partition Table**:
```
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 1M,
storage,  data, fat,     ,        2M,
```

## Design Decisions

### 1. Framework Choice: ESP-IDF over Arduino

**Decision**: Use ESP-IDF framework

**Rationale**:
- Better control over memory management (critical for web server)
- Native FreeRTOS integration for multi-tasking
- More efficient (no Arduino core overhead)
- Better support for advanced features (OTA, secure boot)

**Trade-offs**:
- Steeper learning curve
- More verbose API
- Requires ESP-IDF toolchain setup

### 2. Web Server Library: ESPAsyncWebServer

**Decision**: Use ESPAsyncWebServer library

**Rationale**:
- Asynchronous, non-blocking (critical for real-time WebSocket)
- Built-in WebSocket support
- File upload handling
- Active maintenance and community support

**Alternatives Considered**:
- ESP-IDF HTTP server: More control but more complex
- Arduino WebServer: Blocking, not suitable for WebSocket

### 3. Graphics Storage: FATFS over Memory-Mapped

**Decision**: Use FATFS partition for uploaded graphics

**Rationale**:
- Dynamic asset management (add/remove without rebuild)
- Can update graphics via web interface
- Standard filesystem API
- Supports multiple files

**Trade-offs**:
- RAM overhead for file buffers
- Slower access than memory-mapped
- Requires wear leveling for flash

**Alternative Considered**:
- Memory-mapped partition: Faster but fixed asset set, requires firmware rebuild

### 4. Frontend Framework: Preact over React

**Decision**: Use Preact for frontend

**Rationale**:
- Smaller bundle size (~3KB vs ~40KB for React)
- Same API as React (easy migration path)
- Faster performance
- Better for embedded device constraints

**Trade-offs**:
- Smaller ecosystem than React
- Some React libraries may not work

### 5. State Management: Zustand over Redux

**Decision**: Use Zustand for state management

**Rationale**:
- Minimal boilerplate
- Small bundle size (~1KB)
- Simple API
- Sufficient for this use case

**Trade-offs**:
- Less tooling than Redux
- Smaller ecosystem

### 6. WebSocket Protocol: JSON over Binary

**Decision**: Use JSON for WebSocket messages

**Rationale**:
- Human-readable (easier debugging)
- Easy to parse in JavaScript
- Sufficient for this use case (low message frequency)

**Trade-offs**:
- Larger message size than binary
- JSON parsing overhead

**Alternative Considered**:
- Binary protocol: Smaller messages but more complex parsing

### 7. Button Debouncing: Software in Task

**Decision**: Debounce button presses in FreeRTOS task, not ISR

**Rationale**:
- ISR should be minimal (only queue send)
- Debouncing logic can be more sophisticated in task
- Easier to test and debug

**Trade-offs**:
- Slight delay in event delivery (acceptable for button presses)

## Alternatives Considered

### 1. SPIFFS instead of FATFS

**Rejected Because**:
- SPIFFS is deprecated in ESP-IDF
- FATFS has better wear leveling
- FATFS is more standard (easier to mount on host)

### 2. HTTP Long Polling instead of WebSocket

**Rejected Because**:
- Higher latency
- More HTTP overhead
- WebSocket provides true bidirectional communication

### 3. React instead of Preact

**Rejected Because**:
- Larger bundle size (not suitable for embedded constraints)
- Slower performance
- Overkill for this use case

### 4. MQTT instead of WebSocket

**Rejected Because**:
- Requires MQTT broker (additional infrastructure)
- More complex setup
- WebSocket is sufficient for direct device communication

## Implementation Plan

### Phase 1: Foundation (Week 1)

1. **Set up ESP-IDF project structure**
   - Base on `0013-atoms3r-gif-console` project
   - Configure partition table (factory app + FATFS storage)
   - Set up build system

2. **Integrate ESPAsyncWebServer**
   - Add as component or submodule
   - Test basic HTTP server (serve static HTML)
   - Test WiFi AP mode

3. **Implement basic display handler**
   - Port M5GFX initialization code
   - Test canvas rendering
   - Test PNG display

**Deliverables**:
- Working ESP-IDF project
- Basic HTTP server serving HTML
- Display initialization working

### Phase 2: Core Features (Week 2)

1. **Implement graphics upload**
   - FATFS mount and initialization
   - HTTP POST handler for file upload
   - File storage in `/storage/graphics/`
   - Graphics display function

2. **Implement WebSocket handler**
   - WebSocket server setup
   - Message parsing (JSON)
   - Client connection management

3. **Implement button handler**
   - GPIO ISR setup
   - FreeRTOS queue for events
   - Button task for debouncing
   - WebSocket event sending

**Deliverables**:
- Graphics upload working
- WebSocket connection established
- Button events sent to WebSocket

### Phase 3: Terminal Integration (Week 3)

1. **Implement UART handler**
   - UART driver initialization
   - UART RX task (read → WebSocket)
   - WebSocket TX → UART forwarding

2. **Test terminal functionality**
   - End-to-end test: browser → WebSocket → UART → device
   - End-to-end test: UART → WebSocket → browser

**Deliverables**:
- Terminal bidirectional communication working

### Phase 4: Frontend Development (Week 4)

1. **Set up Preact project**
   - Initialize Vite + Preact project
   - Install Zustand
   - Set up build configuration

2. **Implement WebSocket store**
   - Zustand store for WebSocket state
   - Connection management
   - Message handling

3. **Implement UI components**
   - Graphics upload component
   - Terminal component
   - Status bar component

4. **Bundle and embed frontend**
   - Build frontend assets
   - Generate C array
   - Embed in firmware
   - Test serving from memory

**Deliverables**:
- Complete frontend application
- Frontend embedded in firmware

### Phase 5: Testing and Polish (Week 5)

1. **End-to-end testing**
   - Graphics upload → display
   - Button press → WebSocket → frontend
   - Terminal TX → UART → device
   - Terminal RX → WebSocket → frontend

2. **Error handling**
   - Network disconnection handling
   - File system error handling
   - Memory error handling

3. **Documentation**
   - API documentation
   - User guide
   - Build instructions

**Deliverables**:
- Fully tested system
- Complete documentation

## Open Questions

1. **Graphics Format Priority**: Which formats should be prioritized? (PNG, JPEG, GIF, RGB565)
   - **Answer**: Start with PNG (M5GFX has built-in decoder), add others as needed

2. **Auto-Display on Upload**: Should uploaded graphics automatically display?
   - **Answer**: Make it configurable (default: yes)

3. **Graphics Size Limit**: What's the maximum file size?
   - **Answer**: Limit to 64KB per file (128×128×2 bytes = 32KB for RGB565, 64KB allows for compression overhead)

4. **Multiple WebSocket Clients**: Should multiple clients receive all events?
   - **Answer**: Yes, broadcast to all connected clients

5. **Terminal History**: Should terminal maintain scrollback history?
   - **Answer**: Yes, limit to last 1000 lines in frontend

6. **Security**: Should we add authentication?
   - **Answer**: Not in MVP, add later if needed

## References

- [Analysis Document](./01-codebase-analysis-atoms3r-web-server-architecture.md): Detailed codebase analysis
- [Diary](./reference/01-diary.md): Implementation diary
- ESPAsyncWebServer: https://github.com/me-no-dev/ESPAsyncWebServer
- M5GFX: https://github.com/m5stack/M5GFX
- Preact: https://preactjs.com/
- Zustand: https://github.com/pmndrs/zustand
- ESP-IDF Programming Guide: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/
