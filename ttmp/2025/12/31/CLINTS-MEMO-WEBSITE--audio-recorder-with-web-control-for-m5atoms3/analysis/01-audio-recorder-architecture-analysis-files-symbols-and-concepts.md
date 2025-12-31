---
Title: Audio Recorder Architecture Analysis - Files, Symbols, and Concepts
Ticket: CLINTS-MEMO-WEBSITE
Status: active
Topics:
    - audio-recording
    - web-server
    - esp32-s3
    - m5atoms3
    - i2s
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Comprehensive analysis of files, symbols, and concepts needed to build an audio recorder with web control for M5AtomS3, covering I2S audio capture, web server implementation, and storage/streaming patterns.
LastUpdated: 2025-12-31T13:36:26.425865884-05:00
WhatFor: Reference for implementing audio recording functionality with web-based control interface
WhenToUse: When building audio recording features for M5AtomS3 or similar ESP32-S3 devices
---

# Audio Recorder Architecture Analysis - Files, Symbols, and Concepts

## Overview

This document provides a comprehensive analysis of the files, symbols, and concepts needed to build a small audio recorder with a webpage to control it for the M5AtomS3 device. The analysis covers:

1. **Audio Capture**: I2S-based audio recording using ESP32-S3 hardware
2. **Web Server**: HTTP server for control interface and audio streaming
3. **Storage**: File system patterns for storing audio recordings
4. **Integration**: How to combine these components into a working system

## Target Hardware: M5AtomS3

The M5AtomS3 is an ESP32-S3 based development board with:
- ESP32-S3 microcontroller (dual-core, 240MHz)
- Built-in microphone support (via I2S)
- WiFi connectivity
- Small display (M5GFX compatible)
- Button input
- Grove connector for expansion

## 1. Audio Capture Architecture

### 1.1 I2S Audio Interface

I2S (Inter-IC Sound) is the standard interface for digital audio on ESP32. The ESP32-S3 provides multiple I2S peripherals with DMA support for efficient audio transfer.

#### Key Files and Symbols

**ESP-IDF I2S Driver** (`driver/i2s.h` or `driver/i2s_std.h`):
- `i2s_config_t` - I2S configuration structure
- `i2s_pin_config_t` - Pin configuration structure
- `i2s_driver_install()` - Initialize I2S driver
- `i2s_set_pin()` - Configure I2S pins
- `i2s_read()` - Read audio data from I2S
- `i2s_write()` - Write audio data to I2S
- `i2s_zero_dma_buffer()` - Clear DMA buffers
- `I2S_NUM_0`, `I2S_NUM_1` - I2S port identifiers
- `I2S_MODE_MASTER` - Master mode (ESP32 controls clock)
- `I2S_MODE_RX` - Receive mode (for microphone input)
- `I2S_MODE_TX` - Transmit mode (for speaker output)
- `I2S_BITS_PER_SAMPLE_16BIT` - 16-bit audio samples
- `I2S_CHANNEL_FMT_ONLY_LEFT` - Mono channel format
- `I2S_CHANNEL_FMT_RIGHT_LEFT` - Stereo channel format
- `I2S_COMM_FORMAT_I2S` - I2S communication format
- `I2S_COMM_FORMAT_I2S_MSB` - I2S MSB format

**Reference Implementation**: `echo-base--openai-realtime-embedded-sdk/src/media.cpp`

```cpp
// Example I2S configuration for audio input
i2s_config_t i2s_config_in = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 640,  // 40ms @ 16kHz
    .use_apll = 1,
};

i2s_pin_config_t pin_config_in = {
    .mck_io_num = -1,
    .bck_io_num = 38,      // Bit clock
    .ws_io_num = 39,       // Word select (LRCLK)
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = 40,     // Data input
};
```

### 1.2 M5Stack Mic_Class Implementation

M5Stack provides a higher-level microphone abstraction in `M5Cardputer-UserDemo/main/hal/mic/`.

**Key Files**:
- `Mic_Class.hpp` - Header with `Mic_Class` interface
- `Mic_Class.cpp` - Implementation with I2S setup and recording

**Key Symbols**:
- `m5::Mic_Class` - Main microphone class
- `m5::mic_config_t` - Configuration structure
  - `pin_data_in` - I2S data input pin
  - `pin_bck` - Bit clock pin
  - `pin_ws` - Word select pin
  - `pin_mck` - Master clock pin (optional)
  - `sample_rate` - Sampling rate in Hz (default: 16000)
  - `stereo` - Stereo mode flag
  - `over_sampling` - Oversampling factor
  - `magnification` - Gain multiplier
  - `noise_filter_level` - Noise filtering coefficient
  - `use_adc` - Use ADC instead of I2S
  - `dma_buf_len` - DMA buffer length
  - `dma_buf_count` - Number of DMA buffers
  - `i2s_port` - I2S port number
- `Mic_Class::begin()` - Initialize microphone
- `Mic_Class::end()` - Deinitialize microphone
- `Mic_Class::record(int16_t* data, size_t len)` - Record audio samples
- `Mic_Class::isRecording()` - Check recording status
- `Mic_Class::setSampleRate(uint32_t rate)` - Set sample rate

**Usage Pattern**:
```cpp
#include "Mic_Class.hpp"

m5::Mic_Class mic;
m5::mic_config_t mic_cfg;
mic_cfg.pin_data_in = 40;  // GPIO pin for I2S data
mic_cfg.pin_bck = 38;      // Bit clock
mic_cfg.pin_ws = 39;       // Word select
mic_cfg.sample_rate = 16000;
mic_cfg.dma_buf_len = 64;
mic_cfg.dma_buf_count = 4;
mic.config(mic_cfg);
mic.begin();

int16_t audio_buffer[1600];  // 100ms @ 16kHz
mic.record(audio_buffer, 1600);
```

### 1.3 Audio Codec Support (ES8311)

For devices with ES8311 codec (like EchoBase addon), additional initialization is needed.

**Key Files**: `echo-base--openai-realtime-embedded-sdk/src/media.cpp` (lines 104-171)

**Key Symbols**:
- `es8311_handle_t` - ES8311 codec handle
- `es8311_create()` - Create codec handle
- `es8311_init()` - Initialize codec
- `es8311_clock_config_t` - Clock configuration
- `es8311_voice_volume_set()` - Set volume
- `es8311_microphone_config()` - Configure microphone

**Configuration for ATOMS3R with EchoBase**:
```cpp
// I2C configuration for ES8311
i2c_config_t conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = 38,
    .scl_io_num = 39,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master = { .clk_speed = 400000 }
};

// ES8311 initialization
es8311_handle = es8311_create(I2C_PORT, ES8311_ADDRRES_0);
es8311_clock_config_t clk_cfg = {
    .sample_frequency = 16000,
    .mclk_from_mclk_pin = false,
};
es8311_init(es8311_handle, &clk_cfg, ES8311_RESOLUTION_32, ES8311_RESOLUTION_32);
```

### 1.4 Audio Sample Rates and Buffer Sizes

**Common Configurations**:
- **8kHz**: Voice quality, low bandwidth (320 samples = 40ms)
- **16kHz**: Standard quality (640 samples = 40ms)
- **44.1kHz**: CD quality (1764 samples = 40ms)
- **48kHz**: Professional audio (1920 samples = 40ms)

**Buffer Sizing**:
- Buffer size = (sample_rate × duration_ms) / 1000
- Example: 16kHz × 40ms = 640 samples
- Each sample = 2 bytes (16-bit) = 1280 bytes per buffer

## 2. Web Server Architecture

### 2.1 ESP-IDF HTTP Server (esp_http_server)

Native ESP-IDF HTTP server component, used in `esp32-s3-m5/0017-atoms3r-web-ui`.

**Key Files**: `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp`

**Key Symbols**:
- `httpd_handle_t` - HTTP server handle
- `httpd_config_t` - Server configuration
- `httpd_start()` - Start HTTP server
- `httpd_register_uri_handler()` - Register route handler
- `httpd_req_t` - HTTP request structure
- `httpd_resp_send()` - Send HTTP response
- `httpd_resp_set_type()` - Set content type
- `httpd_resp_send_chunk()` - Send chunked response
- `HTTPD_WS_SUPPORT` - WebSocket support flag
- `httpd_ws_frame_t` - WebSocket frame structure
- `httpd_ws_send_frame()` - Send WebSocket frame
- `httpd_ws_recv_frame()` - Receive WebSocket frame

**Basic Setup**:
```cpp
#include "esp_http_server.h"

httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
httpd_handle_t server = nullptr;
esp_err_t err = httpd_start(&server, &cfg);

httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_handler,
};
httpd_register_uri_handler(server, &root);
```

**WebSocket Support**:
```cpp
#if CONFIG_HTTPD_WS_SUPPORT
httpd_uri_t ws = {
    .uri = "/ws",
    .method = HTTP_GET,
    .handler = ws_handler,
    .user_ctx = NULL,
    .is_websocket = true,
};
httpd_register_uri_handler(server, &ws);
#endif
```

### 2.2 ESPAsyncWebServer (Arduino-based)

Async web server library, used in `ATOMS3R-CAM-M12-UserDemo`.

**Key Files**:
- `ATOMS3R-CAM-M12-UserDemo/components/ESPAsyncWebServer/src/ESPAsyncWebServer.h`
- `ATOMS3R-CAM-M12-UserDemo/main/service/service_web_server.cpp`

**Key Symbols**:
- `AsyncWebServer` - Main server class
- `AsyncWebServerRequest` - Request handler class
- `AsyncWebServerResponse` - Response class
- `AsyncWebSocket` - WebSocket handler class
- `AsyncWebServer::on()` - Register route handler
- `AsyncWebServer::begin()` - Start server
- `AsyncWebServerRequest::send()` - Send response
- `AsyncWebServerRequest::beginResponse()` - Create response
- `AsyncWebServerRequest::beginResponse_P()` - Create PROGMEM response
- `AsyncWebSocket::onEvent()` - WebSocket event handler
- `AsyncWebSocket::textAll()` - Broadcast text to all clients

**Dependencies**:
- `AsyncTCP` - Async TCP library (required)
- `ArduinoJson` - JSON parsing (for API endpoints)

**Basic Setup**:
```cpp
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

AsyncWebServer* server = new AsyncWebServer(80);

// Initialize Arduino (required for WiFi)
initArduino();

// Start WiFi Access Point
WiFi.softAP("AtomS3R-Audio", "", 1, 0, 1, false);

// Register route
server->on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", "<h1>Audio Recorder</h1>");
});

// Start server
server->begin();
```

**WebSocket Setup**:
```cpp
AsyncWebSocket* ws = new AsyncWebSocket("/ws");
ws->onEvent([](AsyncWebSocket* server, AsyncWebSocketClient* client, 
               AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("WebSocket client #%u connected\n", client->id());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
    }
});
server->addHandler(ws);

// Broadcast to all clients
ws->textAll("Hello from server");
```

### 2.3 WiFi Configuration

**Key Files**: 
- `ATOMS3R-CAM-M12-UserDemo/main/service/service_web_server.cpp` (lines 26-38)
- `esp32-s3-m5/0017-atoms3r-web-ui/main/wifi_app.cpp`

**Key Symbols**:
- `WiFi.softAP()` - Start Access Point mode
- `WiFi.begin()` - Connect to WiFi network
- `WiFi.mode()` - Set WiFi mode
- `WiFi.softAPIP()` - Get AP IP address
- `WiFi.localIP()` - Get station IP address
- `WIFI_MODE_AP` - Access Point mode
- `WIFI_MODE_STA` - Station mode
- `WIFI_MODE_APSTA` - Both AP and Station

**Access Point Mode** (simpler, no internet):
```cpp
WiFi.mode(WIFI_MODE_AP);
WiFi.softAP("AtomS3R-Audio", "", 1, 0, 1, false);
IPAddress ap_ip = WiFi.softAPIP();
Serial.printf("AP IP: %s\n", ap_ip.toString().c_str());
```

**Station Mode** (connects to existing WiFi):
```cpp
WiFi.mode(WIFI_MODE_STA);
WiFi.begin("SSID", "password");
while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
}
Serial.printf("Connected! IP: %s\n", WiFi.localIP().toString().c_str());
```

## 3. Audio Storage and Streaming

### 3.1 File System Storage

**SPIFFS** (SPI Flash File System):
- Built into ESP-IDF
- Read-only or read-write
- Limited to 4MB typically
- Good for small files

**FATFS** (FAT File System):
- Supports larger storage
- Can use external SD card
- More complex setup

**Key Symbols**:
- `spiffs_mount()` - Mount SPIFFS partition
- `FILE* fopen()` - Open file
- `fwrite()` - Write data
- `fread()` - Read data
- `fclose()` - Close file

### 3.2 Audio Streaming Patterns

**HTTP Streaming** (MJPEG-like pattern):
```cpp
// Stream audio chunks via HTTP
server->on("/api/audio/stream", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(
        "audio/wav", 
        nullptr,
        [](uint8_t* buffer, size_t maxLen, size_t index) -> size_t {
            // Read audio data from I2S
            size_t bytes_read = 0;
            i2s_read(I2S_PORT, buffer, maxLen, &bytes_read, portMAX_DELAY);
            return bytes_read;
        }
    );
    response->addHeader("Content-Type", "audio/wav");
    request->send(response);
});
```

**WebSocket Streaming**:
```cpp
// Stream audio via WebSocket
void audio_stream_task(void* param) {
    int16_t audio_buffer[640];
    while (1) {
        size_t bytes_read = 0;
        i2s_read(I2S_PORT, audio_buffer, sizeof(audio_buffer), 
                 &bytes_read, portMAX_DELAY);
        
        if (ws->count() > 0) {
            ws->binaryAll((uint8_t*)audio_buffer, bytes_read);
        }
        vTaskDelay(pdMS_TO_TICKS(40));  // ~40ms chunks
    }
}
```

**Chunked Transfer Encoding**:
```cpp
// Send audio in chunks
server->on("/api/audio/chunk", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginChunkedResponse(
        "application/octet-stream",
        [](uint8_t* buffer, size_t maxLen, size_t index) -> size_t {
            if (index >= MAX_CHUNKS) return 0;
            // Read next chunk from storage
            return read_audio_chunk(buffer, maxLen, index);
        }
    );
    request->send(response);
});
```

### 3.3 Audio Format Considerations

**WAV Format**:
- Uncompressed, easy to implement
- Large file sizes
- Standard header format

**PCM Raw**:
- No header, just raw samples
- Smallest overhead
- Requires metadata separately

**Opus Encoding**:
- Compressed format
- Lower bandwidth
- Requires encoder library
- Reference: `echo-base--openai-realtime-embedded-sdk/src/media.cpp`

## 4. Control API Design

### 4.1 REST API Endpoints

**Recording Control**:
- `POST /api/recording/start` - Start recording
- `POST /api/recording/stop` - Stop recording
- `GET /api/recording/status` - Get recording status
- `GET /api/recording/list` - List recorded files
- `GET /api/recording/download/:filename` - Download recording
- `DELETE /api/recording/:filename` - Delete recording

**Example Implementation**:
```cpp
void load_recording_apis(AsyncWebServer& server) {
    // Start recording
    server.on("/api/recording/start", HTTP_POST, 
        [](AsyncWebServerRequest* request) {
            if (start_recording()) {
                request->send(200, "application/json", "{\"ok\":true}");
            } else {
                request->send(500, "application/json", "{\"ok\":false}");
            }
        });
    
    // Stop recording
    server.on("/api/recording/stop", HTTP_POST,
        [](AsyncWebServerRequest* request) {
            stop_recording();
            request->send(200, "application/json", "{\"ok\":true}");
        });
    
    // Get status
    server.on("/api/recording/status", HTTP_GET,
        [](AsyncWebServerRequest* request) {
            JsonDocument doc;
            doc["recording"] = is_recording();
            doc["duration"] = get_recording_duration();
            String json;
            serializeJson(doc, json);
            request->send(200, "application/json", json);
        });
}
```

### 4.2 JSON Handling

**ArduinoJson Library**:
- `JsonDocument` - JSON document container
- `serializeJson()` - Serialize to string
- `deserializeJson()` - Parse JSON string

**Example**:
```cpp
#include <ArduinoJson.h>

JsonDocument doc;
doc["command"] = "start";
doc["sample_rate"] = 16000;
doc["duration"] = 60000;  // 60 seconds

String json;
serializeJson(doc, json);

// Parse JSON
JsonDocument req;
deserializeJson(req, request->arg("plain"));
String cmd = req["command"];
```

## 5. Integration Architecture

### 5.1 Component Organization

**Recommended Structure**:
```
main/
├── audio_recorder.cpp      # Audio recording logic
├── audio_recorder.h
├── web_server.cpp          # Web server setup
├── web_server.h
├── storage.cpp             # File system operations
├── storage.h
└── main.cpp                # Main application
```

### 5.2 Task Architecture

**Audio Capture Task**:
- High priority (5-6)
- Reads from I2S continuously
- Writes to circular buffer or file
- Runs independently of web server

**Web Server Task**:
- Lower priority (1-2)
- Handles HTTP requests
- Can trigger recording start/stop
- Serves audio files

**Example Task Setup**:
```cpp
void audio_capture_task(void* param) {
    int16_t buffer[640];
    FILE* file = nullptr;
    
    while (1) {
        if (recording_active) {
            if (!file) {
                file = fopen("/spiffs/recording.wav", "wb");
                write_wav_header(file);
            }
            
            size_t bytes_read = 0;
            i2s_read(I2S_PORT, buffer, sizeof(buffer), 
                    &bytes_read, portMAX_DELAY);
            fwrite(buffer, 1, bytes_read, file);
        } else {
            if (file) {
                fclose(file);
                file = nullptr;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

xTaskCreate(audio_capture_task, "audio_capture", 4096, NULL, 5, NULL);
```

### 5.3 State Management

**Recording States**:
- `IDLE` - Not recording
- `RECORDING` - Actively recording
- `STOPPING` - Finishing current buffer
- `ERROR` - Error state

**State Machine**:
```cpp
enum RecordingState {
    REC_STATE_IDLE,
    REC_STATE_RECORDING,
    REC_STATE_STOPPING,
    REC_STATE_ERROR
};

RecordingState rec_state = REC_STATE_IDLE;
SemaphoreHandle_t state_mutex = xSemaphoreCreateMutex();
```

## 6. Reference Implementations

### 6.1 Audio Capture Examples

1. **EchoBase SDK** (`echo-base--openai-realtime-embedded-sdk/src/media.cpp`):
   - Full I2S setup with ES8311 codec
   - Opus encoding integration
   - WebRTC streaming

2. **M5Cardputer Mic_Class** (`M5Cardputer-UserDemo/main/hal/mic/`):
   - High-level microphone abstraction
   - I2S configuration handling
   - Recording API

3. **M5AtomS3 UserDemo** (`M5AtomS3-UserDemo/src/main.cpp`):
   - Basic ESP32-S3 setup
   - M5Unified integration
   - Display and input handling

### 6.2 Web Server Examples

1. **ATOMS3R-CAM-M12** (`ATOMS3R-CAM-M12-UserDemo/main/service/`):
   - ESPAsyncWebServer usage
   - REST API implementation
   - WebSocket for real-time data
   - File serving patterns

2. **ESP32-S3-M5 Tutorial 0017** (`esp32-s3-m5/0017-atoms3r-web-ui/`):
   - Native esp_http_server
   - WebSocket support
   - File upload/download
   - JSON API patterns

## 7. Key Concepts and Patterns

### 7.1 DMA Buffering

I2S uses DMA (Direct Memory Access) to transfer audio data without CPU intervention:
- **DMA Buffer Count**: Number of buffers (typically 4-8)
- **DMA Buffer Length**: Samples per buffer (typically 64-640)
- **Double Buffering**: Prevents audio dropouts
- **Buffer Underrun**: Occurs when CPU can't keep up

### 7.2 Sample Rate and Latency

- **Sample Rate**: Determines audio quality and bandwidth
- **Buffer Size**: Affects latency (larger = more latency, less CPU load)
- **Trade-off**: Lower latency requires more CPU, higher latency is more stable

### 7.3 Async vs Sync Patterns

- **ESPAsyncWebServer**: Event-driven, non-blocking
- **esp_http_server**: Request/response handlers, can block
- **WebSocket**: Bidirectional, real-time communication
- **HTTP Streaming**: One-way, chunked transfer

### 7.4 Memory Management

- **Heap Fragmentation**: Use fixed-size buffers
- **SPIRAM**: Can use external PSRAM for larger buffers
- **FreeRTOS Heap**: Monitor free heap with `ESP.getFreeHeap()`

## 8. Implementation Checklist

### Phase 1: Basic Audio Capture
- [ ] Initialize I2S driver
- [ ] Configure I2S pins
- [ ] Read audio samples from I2S
- [ ] Verify audio data integrity
- [ ] Test with different sample rates

### Phase 2: Web Server Setup
- [ ] Initialize WiFi (AP or STA mode)
- [ ] Create HTTP server
- [ ] Implement basic routes (/, /api/status)
- [ ] Test server accessibility
- [ ] Add CORS headers if needed

### Phase 3: Recording Control
- [ ] Implement start/stop recording API
- [ ] Add state management
- [ ] Create audio capture task
- [ ] Test recording start/stop
- [ ] Add error handling

### Phase 4: Storage
- [ ] Mount file system (SPIFFS/FATFS)
- [ ] Implement file write operations
- [ ] Add WAV header writing
- [ ] Test file creation and reading
- [ ] Implement file listing API

### Phase 5: Streaming
- [ ] Implement HTTP audio streaming
- [ ] Add WebSocket support (optional)
- [ ] Test streaming performance
- [ ] Optimize buffer sizes
- [ ] Add chunked transfer

### Phase 6: Web Interface
- [ ] Create HTML control page
- [ ] Add JavaScript for API calls
- [ ] Implement recording visualization
- [ ] Add file download functionality
- [ ] Test on multiple browsers

## 9. Common Pitfalls and Solutions

### 9.1 Audio Dropouts
**Problem**: Audio data missing or corrupted
**Solutions**:
- Increase DMA buffer count
- Increase task priority
- Reduce CPU load in other tasks
- Use APLL for accurate clocking

### 9.2 Web Server Timeouts
**Problem**: HTTP requests timeout
**Solutions**:
- Use async handlers (ESPAsyncWebServer)
- Implement chunked responses for large data
- Increase timeout values
- Use WebSocket for real-time data

### 9.3 Memory Issues
**Problem**: Out of memory errors
**Solutions**:
- Use streaming instead of buffering entire file
- Limit buffer sizes
- Monitor heap usage
- Use external PSRAM if available

### 9.4 WiFi Connectivity
**Problem**: Can't connect to device
**Solutions**:
- Use AP mode for initial setup
- Check WiFi channel (avoid crowded channels)
- Verify SSID and password
- Check IP address assignment

## 10. Next Steps

1. **Choose Web Server Library**: ESPAsyncWebServer (Arduino) vs esp_http_server (native)
2. **Select Storage Method**: SPIFFS (simple) vs FATFS (larger files)
3. **Design API**: REST endpoints for control
4. **Implement Audio Pipeline**: I2S → Buffer → Storage/Stream
5. **Create Web Interface**: HTML/JS control page
6. **Test and Optimize**: Performance tuning and error handling

## Related Files

This analysis references files from multiple projects:
- `echo-base--openai-realtime-embedded-sdk/src/media.cpp` - Audio capture with Opus
- `M5Cardputer-UserDemo/main/hal/mic/` - Mic_Class implementation
- `ATOMS3R-CAM-M12-UserDemo/main/service/service_web_server.cpp` - ESPAsyncWebServer example
- `ATOMS3R-CAM-M12-UserDemo/main/service/apis/` - REST API examples
- `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp` - Native HTTP server example
- `M5AtomS3-UserDemo/src/main.cpp` - Basic M5AtomS3 setup
