---
Title: Echo Base SDK Architecture Analysis
Ticket: 001-ANALYZE-ECHO-BASE
Status: active
Topics:
    - analysis
    - esp32
    - openai
    - realtime
    - embedded-sdk
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Comprehensive analysis of the Echo Base OpenAI Realtime Embedded SDK architecture, components, data flow, and implementation details"
LastUpdated: 2025-12-21T08:09:47.623981758-05:00
WhatFor: "Understanding the complete SDK structure, WebRTC implementation, audio pipeline, and integration with OpenAI Realtime API"
WhenToUse: "When working on SDK modifications, debugging connection issues, or integrating with different hardware platforms"
---

# Echo Base SDK Architecture Analysis

## Executive Summary

The Echo Base OpenAI Realtime Embedded SDK represents a sophisticated embedded firmware solution that brings real-time conversational AI capabilities to resource-constrained ESP32-S3 microcontrollers. Unlike traditional embedded voice applications that might use simpler protocols or cloud services with higher latency, this SDK implements a complete WebRTC stack that enables low-latency, bidirectional voice communication directly with OpenAI's Realtime API. The implementation demonstrates how modern web technologies like WebRTC can be successfully adapted for embedded systems, bridging the gap between IoT devices and cloud-based AI services.

The SDK's architecture is built around a carefully orchestrated pipeline that handles everything from hardware-level audio capture to secure network communication. Audio is captured from a microphone using the I2S (Inter-IC Sound) interface, which provides high-quality digital audio input/output capabilities. The captured audio is then compressed using the Opus codec, which is specifically designed for real-time voice communication and provides excellent compression ratios while maintaining voice quality. The encoded audio packets are wrapped in RTP (Real-time Transport Protocol) headers, encrypted using SRTP (Secure Real-time Transport Protocol), and transmitted over a WebRTC peer connection established with OpenAI's servers.

What makes this SDK particularly interesting is its complete implementation of the WebRTC protocol stack, including Interactive Connectivity Establishment (ICE) for network traversal, Datagram Transport Layer Security (DTLS) for secure key exchange, and the full media transport layer. This is no small feat for an embedded system - WebRTC is a complex protocol suite that's typically associated with web browsers and powerful computing devices. The SDK proves that with careful optimization and the right libraries, WebRTC can run efficiently even on microcontrollers with limited resources.

The SDK also demonstrates thoughtful engineering decisions around development workflow. It supports Linux builds that allow developers to test the WebRTC connection logic without physical hardware, significantly accelerating the development and debugging cycle. This dual-target approach (ESP32-S3 and Linux) is becoming increasingly common in embedded development as teams seek to balance hardware-specific testing with rapid iteration cycles.

**Key Technologies:**
- **WebRTC**: Full WebRTC stack via libpeer library (ICE, DTLS-SRTP, RTP) - see [WebRTC specification](https://www.w3.org/TR/webrtc/) and [MDN WebRTC guide](https://developer.mozilla.org/en-US/docs/Web/API/WebRTC_API)
- **Audio Codec**: Opus (30kbps, VoIP mode) - see [Opus codec documentation](https://opus-codec.org/) and [RFC 6716](https://datatracker.ietf.org/doc/html/rfc6716)
- **Audio Interface**: I2S (8kHz or 16kHz depending on board) - see [ESP-IDF I2S driver documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html)
- **Signaling**: HTTP POST with SDP offer/answer exchange - see [OpenAI Realtime API documentation](https://platform.openai.com/docs/guides/realtime)
- **Security**: DTLS-SRTP for encrypted media transport - see [RFC 3711 (SRTP)](https://datatracker.ietf.org/doc/html/rfc3711) and [RFC 5764 (DTLS-SRTP)](https://datatracker.ietf.org/doc/html/rfc5764)
- **Platform**: ESP32-S3 (ESP-IDF) or Linux - see [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/)

## Architecture Overview

The SDK's architecture follows a layered approach that cleanly separates concerns while enabling efficient data flow from hardware peripherals to cloud services. Each layer has a well-defined responsibility, making the system easier to understand, maintain, and modify. The architecture is designed to be modular, with clear interfaces between components, allowing individual pieces to be replaced or upgraded without affecting the entire system.

At the highest level, the SDK can be thought of as a pipeline that transforms audio signals into network packets and vice versa. The pipeline flows in two directions: upstream (from microphone to OpenAI) for voice input, and downstream (from OpenAI to speaker) for AI responses. Both directions share the same underlying infrastructure - WebRTC for transport, Opus for compression, and I2S for hardware interface - but the data flows in opposite directions through the pipeline.

The architecture also demonstrates careful resource management for embedded systems. Audio buffers are sized to balance latency (smaller buffers = lower latency) with stability (larger buffers = fewer dropouts). The WebRTC stack is configured to minimize memory usage while maintaining compatibility with standard WebRTC implementations. The threading model is designed to prevent priority inversion and ensure that time-critical operations (like audio encoding) get the CPU time they need.

### High-Level System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                         │
│  (main.cpp, webrtc.cpp, http.cpp, media.cpp, wifi.cpp)      │
└────────────┬────────────────────────────────────────────────┘
             │
    ┌────────┴────────┐
    │                 │
┌───▼────┐      ┌─────▼──────┐
│  WiFi  │      │    HTTP    │
│  STA   │      │ Signaling  │
└───┬────┘      └─────┬──────┘
    │                 │
    └────────┬────────┘
             │
    ┌────────▼────────┐
    │   WebRTC Peer  │
    │   Connection   │
    │   (libpeer)    │
    └────────┬────────┘
             │
    ┌────────┴────────┐
    │                 │
┌───▼────┐      ┌─────▼──────┐
│  ICE   │      │ DTLS-SRTP  │
│ Agent  │      │  (mbedTLS  │
│        │      │  + libsrtp)│
└───┬────┘      └─────┬──────┘
    │                 │
    └────────┬────────┘
             │
    ┌────────▼────────┐
    │   RTP Media    │
    │  (Opus Audio)  │
    └────────┬────────┘
             │
    ┌────────┴────────┐
    │                 │
┌───▼────┐      ┌─────▼──────┐
│  I2S   │      │   Opus     │
│ Audio  │      │ Encoder/   │
│ I/O    │      │  Decoder   │
└────────┘      └────────────┘
```

### Component Organization

**Source Files (`src/`):**
- `main.cpp` - Application entry point and initialization
- `main.h` - Function declarations
- `webrtc.cpp` - WebRTC peer connection management
- `http.cpp` - HTTP signaling with OpenAI API
- `media.cpp` - Audio capture, encoding, and decoding
- `wifi.cpp` - WiFi station mode connectivity

**Components (`components/`):**
- `peer/` - Wrapper component for libpeer WebRTC library
- `srtp/` - SRTP encryption library (libsrtp)
- `esp-libopus/` - Opus audio codec library

**Dependencies (`deps/`):**
- `libpeer/` - Portable WebRTC implementation in C

**Managed Components (`managed_components/`):**
- `espressif__es8311/` - ES8311 audio codec driver (for M5 ATOMS3R board)

## Initialization Flow

The initialization sequence is critical to the SDK's operation, as it establishes all the necessary components in the correct order and ensures that dependencies are satisfied before components that depend on them are started. The initialization follows a bottom-up approach, starting with low-level system services and building up to application-level functionality. This ordering is important because later initialization steps depend on earlier ones being complete.

The initialization sequence also reveals the SDK's design philosophy around error handling and system reliability. Some initialization steps block until they succeed (like WiFi connection), which ensures that the system doesn't proceed with incomplete setup. Other steps (like audio codec initialization) may fail gracefully, allowing the system to continue operating with reduced functionality. This balance between strict requirements and graceful degradation is common in embedded systems where hardware failures or configuration issues can occur.

Understanding the initialization flow is also important for debugging. If a system fails to start or behaves unexpectedly, tracing through the initialization sequence can help identify where the problem occurs. The sequence also shows which components can be initialized independently (like audio capture and decoder) versus which must be initialized in a specific order (like WiFi before WebRTC).

### ESP32 Initialization Sequence

```cpp
app_main()
  ├─ nvs_flash_init()           // Initialize NVS flash storage
  ├─ esp_event_loop_create_default()  // Create event loop
  ├─ peer_init()                // Initialize WebRTC library
  ├─ oai_init_audio_capture()   // Initialize I2S and audio codec
  ├─ oai_init_audio_decoder()   // Initialize Opus decoder
  ├─ oai_wifi()                 // Connect to WiFi (blocks until IP)
  └─ oai_webrtc()               // Start WebRTC connection (main loop)
```

### Linux Initialization Sequence

```cpp
main()
  ├─ esp_event_loop_create_default()  // Create event loop
  ├─ peer_init()                // Initialize WebRTC library
  └─ oai_webrtc()               // Start WebRTC connection (no audio/WiFi)
```

**Key Differences:**
- Linux build skips NVS flash, audio capture/decoder, and WiFi
- Linux build is for testing WebRTC connection logic without hardware

## Core Components

### 1. Main Entry Point (`src/main.cpp`)

The main entry point serves as the orchestrator for the entire SDK, coordinating the initialization of all subsystems and managing the transition from system startup to active operation. On ESP32, the entry point is `app_main()`, which is called by the ESP-IDF framework after hardware initialization is complete. On Linux, it's the standard `main()` function, allowing the same codebase to run in both environments with appropriate conditional compilation.

The initialization sequence is carefully ordered to ensure that dependencies are satisfied. NVS (Non-Volatile Storage) flash is initialized first because it may be needed by other components for storing configuration or state. The event loop is created early because many ESP-IDF components rely on it for asynchronous communication. The peer library is initialized before audio or WiFi because it sets up data structures that will be used throughout the application's lifetime.

One notable aspect of the initialization is that it blocks on WiFi connection before proceeding to WebRTC. This design decision ensures that the device has network connectivity before attempting to establish a WebRTC connection, which prevents wasted resources and confusing error states. However, this blocking behavior also means that if WiFi connection fails, the device will hang indefinitely, which could be improved with a timeout mechanism.

**Purpose**: Application initialization and component startup

**Key Functions:**
- `app_main()` (ESP32) / `main()` (Linux) - Entry point
- Initializes NVS flash (ESP32 only)
- Creates event loop
- Calls initialization functions in sequence
- Blocks on WiFi connection before starting WebRTC

**Initialization Order:**
1. NVS flash (ESP32) - Provides persistent storage for configuration
2. Event loop - Enables asynchronous event handling throughout the system
3. Peer library (`peer_init()`) - Initializes WebRTC data structures
4. Audio capture (`oai_init_audio_capture()`) - Configures I2S and audio codec hardware
5. Audio decoder (`oai_init_audio_decoder()`) - Prepares Opus decoder for incoming audio
6. WiFi (`oai_wifi()` - blocks) - Connects to WiFi network and obtains IP address
7. WebRTC (`oai_webrtc()` - main loop) - Establishes connection and begins streaming

**Configuration:**
- WiFi SSID/password from compile-time definitions (`WIFI_SSID`, `WIFI_PASSWORD`) - Embedded in firmware binary
- OpenAI API key from compile-time definition (`OPENAI_API_KEY`) - Required for authentication
- OpenAI Realtime API URL: `https://api.openai.com/v1/realtime?model=gpt-4o-mini-realtime-preview-2024-12-17` - Hardcoded model version

For more information on ESP-IDF application structure, see the [ESP-IDF Application Startup Flow documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/startup.html).

### 2. WebRTC Peer Connection (`src/webrtc.cpp`)

The WebRTC peer connection component is the heart of the SDK, managing the complex lifecycle of establishing and maintaining a WebRTC connection with OpenAI's servers. WebRTC connections involve multiple phases: signaling (exchanging connection information), ICE candidate gathering and connectivity checks, DTLS handshaking for security, and finally media streaming. The `webrtc.cpp` file orchestrates all of these phases, handling state transitions and coordinating between the various WebRTC subsystems.

One of the most interesting aspects of this implementation is how it handles the signaling phase. Unlike many WebRTC applications that use WebSockets or other persistent connections for signaling, this SDK uses simple HTTP POST requests. When ICE candidates are gathered, they trigger a callback that sends the SDP offer to OpenAI via HTTP. This approach is simpler and more suitable for embedded systems, though it does mean that the device must initiate the connection rather than being able to receive incoming connection requests.

The connection state machine is critical to understanding how the SDK operates. The state transitions from NEW (initial state) through CHECKING (ICE connectivity checks) to CONNECTED (ICE established) and finally COMPLETED (DTLS handshake complete and media flowing). Each state transition triggers callbacks that allow the application to respond appropriately - for example, the audio encoding task is only created when the connection reaches the CONNECTED state, ensuring that audio isn't sent before the connection is ready.

**Purpose**: Manage WebRTC peer connection lifecycle and audio streaming

**Key Functions:**
- `oai_webrtc()` - Main WebRTC loop that processes packets and manages connection state
- `oai_onconnectionstatechange_task()` - Callback invoked when connection state changes (NEW → CHECKING → CONNECTED → COMPLETED)
- `oai_on_icecandidate_task()` - Callback invoked for each ICE candidate, triggers HTTP signaling
- `oai_send_audio_task()` - FreeRTOS task that captures and encodes audio at regular intervals

**Peer Connection Configuration:**
```cpp
PeerConfiguration {
  .ice_servers = {},              // Empty (no STUN/TURN)
  .audio_codec = CODEC_OPUS,      // Opus audio codec
  .video_codec = CODEC_NONE,      // No video
  .datachannel = DATA_CHANNEL_NONE, // No data channel
  .onaudiotrack = oai_audio_decode, // Audio receive callback
  .onvideotrack = NULL,
  .on_request_keyframe = NULL
}
```

**Connection Lifecycle:**

The WebRTC connection establishment follows the standard WebRTC offer/answer model, but adapted for HTTP-based signaling rather than the more common WebSocket approach. The process begins when the application calls `peer_connection_create_offer()`, which triggers the libpeer library to gather ICE candidates, create a DTLS certificate, and generate a Session Description Protocol (SDP) offer containing all the information OpenAI needs to establish a connection.

1. **Create Offer**: `peer_connection_create_offer()` generates SDP offer containing local capabilities, ICE candidates, and DTLS fingerprint
2. **ICE Candidates**: `oai_on_icecandidate_task()` callback triggered for each candidate as they're discovered
3. **HTTP Signaling**: SDP offer sent to OpenAI via `oai_http_request()` using HTTP POST with `Content-Type: application/sdp`
4. **Set Answer**: SDP answer received from OpenAI and set via `peer_connection_set_remote_description()`, which contains OpenAI's capabilities and network information
5. **ICE Connection**: ICE connectivity checks establish connection by testing candidate pairs and selecting the best path
6. **DTLS Handshake**: DTLS-SRTP handshake completes, establishing encrypted channels and deriving SRTP keys
7. **Connected State**: `PEER_CONNECTION_CONNECTED` state reached when ICE connectivity is confirmed
8. **Audio Task**: `oai_send_audio_task()` created to send audio once connection is ready
9. **Main Loop**: `peer_connection_loop()` processes incoming packets every 15ms, handling RTP/RTCP packets, DTLS messages, and ICE checks

For more information on WebRTC connection establishment, see the [WebRTC API documentation](https://developer.mozilla.org/en-US/docs/Web/API/WebRTC_API) and [RFC 8829 (WebRTC Overview)](https://datatracker.ietf.org/doc/html/rfc8829).

**State Machine:**
```
PEER_CONNECTION_NEW
    ↓ (create offer)
PEER_CONNECTION_CHECKING
    ↓ (ICE connected)
PEER_CONNECTION_CONNECTED
    ↓ (DTLS handshake complete)
PEER_CONNECTION_COMPLETED
    ↓ (or)
PEER_CONNECTION_FAILED / DISCONNECTED / CLOSED
```

**Audio Encoding Task:**
- Created when connection reaches `PEER_CONNECTION_CONNECTED` state
- Stack size: 20KB (ESP32-S3) or 40KB (M5 ATOMS3R)
- Priority: 7 (high priority)
- Pinned to core: 0
- Interval: 15ms (matches WebRTC loop tick)
- Flow: `i2s_read()` → `opus_encode()` → `peer_connection_send_audio()`

**Main Loop:**
- Calls `peer_connection_loop()` every 15ms
- Processes incoming RTP/RTCP packets
- Handles DTLS handshake
- Manages ICE connectivity checks
- On disconnect/close: ESP32 restarts, Linux exits

### 3. HTTP Signaling (`src/http.cpp`)

The HTTP signaling component handles the exchange of Session Description Protocol (SDP) messages with OpenAI's Realtime API. This is a critical component because it's the only way the device communicates with OpenAI's servers before the WebRTC connection is established. Once the WebRTC connection is active, all communication happens peer-to-peer over UDP, but the initial handshake requires this HTTP-based signaling.

The signaling implementation uses ESP-IDF's HTTP client library, which provides a convenient API for making HTTP requests. The implementation is relatively straightforward - it constructs an HTTP POST request with the SDP offer in the body and the API key in the Authorization header. However, there are some important details around error handling and response processing that developers should understand.

One notable aspect is that the response buffer (`answer` parameter) is reused for both constructing the Authorization header and storing the SDP answer. This is a memory optimization common in embedded systems, but it requires careful buffer management to avoid overwriting data. The implementation also has a hardcoded maximum buffer size of 2048 bytes, which should be sufficient for typical SDP messages but could potentially be exceeded with very complex SDP offers containing many ICE candidates.

**Purpose**: Exchange SDP offers/answers with OpenAI Realtime API

**Key Functions:**
- `oai_http_request()` - POST SDP offer to OpenAI, receive SDP answer in response
- `oai_http_event_handler()` - HTTP event handler that processes response chunks and extracts SDP answer

**HTTP Request Flow:**
```cpp
oai_http_request(offer, answer)
  ├─ Initialize HTTP client
  ├─ Set URL: OPENAI_REALTIMEAPI
  ├─ Set method: POST
  ├─ Set headers:
  │   ├─ Content-Type: application/sdp
  │   └─ Authorization: Bearer {OPENAI_API_KEY}
  ├─ Set POST body: SDP offer
  ├─ Perform request
  └─ Extract SDP answer from response body
```

**Request Details:**
- **URL**: `https://api.openai.com/v1/realtime?model=gpt-4o-mini-realtime-preview-2024-12-17` - OpenAI's Realtime API endpoint with specific model version
- **Method**: POST - Standard HTTP POST request
- **Content-Type**: `application/sdp` - Indicates that the request body contains Session Description Protocol data
- **Authorization**: `Bearer {OPENAI_API_KEY}` - Bearer token authentication using API key from compile-time definition
- **Body**: SDP offer (text) - Complete SDP offer message containing media descriptions, ICE candidates, and DTLS fingerprint

**Response Handling:**
- Response body copied to `answer` buffer (max 2048 bytes) - Buffer size should be sufficient for typical SDP answers
- Buffer reused for both auth header construction and SDP answer storage - Memory optimization common in embedded systems
- On error (non-201 status): ESP32 restarts, Linux logs error - Aggressive error handling ensures system doesn't continue in invalid state
- Chunked responses not supported (would cause restart) - Limitation that could be addressed in future versions

For more information on HTTP client usage in ESP-IDF, see the [ESP-IDF HTTP Client documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_client.html). For OpenAI API authentication, see the [OpenAI Authentication guide](https://platform.openai.com/docs/guides/authentication).

**Integration:**
- Called from `oai_on_icecandidate_task()` callback in `webrtc.cpp`
- SDP answer passed to `peer_connection_set_remote_description()`

### 4. Audio Media Pipeline (`src/media.cpp`)

The audio media pipeline is responsible for the complete audio processing workflow, from capturing raw audio samples from the microphone to playing decoded audio through speakers or headphones. This component demonstrates sophisticated audio processing techniques adapted for embedded systems, including DMA-based I2S interfaces, real-time audio encoding/decoding, and careful buffer management to prevent audio dropouts.

The pipeline operates in two directions simultaneously: encoding for transmission (microphone → network) and decoding for playback (network → speaker). Both directions use the Opus codec, but with different configurations optimized for their respective use cases. The encoding path prioritizes low CPU usage (complexity 0) and efficient compression (30 kbps bitrate), while the decoding path focuses on quality and low latency.

One of the more interesting aspects of the audio pipeline is how it handles the different board configurations. The default ESP32-S3 configuration uses separate I2S ports for input and output, running at 8kHz sample rate. The M5 ATOMS3R configuration uses a single duplex I2S port with an ES8311 codec chip, running at 16kHz. This difference reflects the hardware capabilities of each board - the ATOMS3R with EchoBase has more sophisticated audio hardware that supports higher sample rates and better audio quality.

**Purpose**: Audio capture, encoding, and decoding

**Key Functions:**
- `oai_init_audio_capture()` - Initialize I2S interfaces and audio codec hardware (ES8311 for ATOMS3R)
- `oai_init_audio_decoder()` - Initialize Opus decoder for playing received audio
- `oai_init_audio_encoder()` - Initialize Opus encoder for compressing captured audio
- `oai_send_audio()` - Capture audio from I2S, encode with Opus, send via WebRTC
- `oai_audio_decode()` - Decode Opus audio packets and play through I2S

#### Audio Capture Initialization

**ESP32-S3 Default Configuration:**
```cpp
Sample Rate: 8000 Hz
Buffer Samples: 320 (40ms @ 8kHz)
I2S Ports: Separate TX (I2S_NUM_0) and RX (I2S_NUM_1)
Pins:
  - TX: MCLK=0, BCLK=15, LRCLK=16, DATA=17
  - RX: MCLK=0, BCLK=38, LRCLK=39, DATA=40
```

**M5 ATOMS3R Configuration:**
```cpp
Sample Rate: 16000 Hz (EchoBase requirement)
Buffer Samples: 640 (40ms @ 16kHz)
I2S Port: Single duplex port (I2S_NUM_1)
Codec: ES8311 via I2C
I2C: Port I2C_NUM_1, 400kHz, SCL=39, SDA=38
Pins:
  - BCLK=8, LRCLK=6, DATA_OUT=5, DATA_IN=7
  - Channel Format: I2S_CHANNEL_FMT_ALL_LEFT (EchoBase requirement)
```

**I2S Configuration:**

The I2S (Inter-IC Sound) interface configuration is critical for reliable audio capture and playback. I2S is a serial bus standard designed for digital audio data transfer, and ESP32's I2S peripheral provides DMA (Direct Memory Access) capabilities that allow audio data to be transferred between memory and the I2S interface without CPU intervention. This is essential for real-time audio processing, as it prevents audio dropouts that would occur if the CPU had to manually copy every audio sample.

- Mode: Master, TX and/or RX - The ESP32 acts as the I2S master, controlling the clock signals
- Sample Rate: 8kHz (ESP32-S3) or 16kHz (ATOMS3R) - Determines audio quality and bandwidth requirements
- Bits per Sample: 16-bit - Standard resolution for voice applications, balances quality and bandwidth
- Channel Format: Right/Left (ESP32-S3) or All Left (ATOMS3R) - Channel format depends on hardware configuration
- Communication Format: I2S MSB - I2S format with most significant bit first
- DMA Buffers: 8 buffers, 320/640 samples each - Provides buffering to prevent dropouts during CPU-intensive operations
- APLL: Enabled for accurate clocking - Audio PLL provides precise clock generation for sample-accurate timing
- TX Auto Clear: Enabled (prevents underruns) - Automatically clears DMA buffers to prevent audio glitches

For more information on ESP32 I2S configuration, see the [ESP-IDF I2S driver documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html) and [I2S examples](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/i2s).

**ES8311 Codec Initialization (ATOMS3R):**
1. Configure I2C bus (400kHz)
2. Create ES8311 handle (`es8311_create()`)
3. Configure clock (sample rate, resolution)
4. Initialize codec (`es8311_init()`)
5. Set voice volume (80%)
6. Configure microphone (disabled for now)

#### Opus Encoder

The Opus encoder is configured specifically for voice communication with embedded system constraints in mind. Opus is a versatile codec that can handle both speech and music, but for this application, it's tuned specifically for voice using the VoIP application mode. The encoder uses the lowest complexity setting (0) to minimize CPU usage, which is critical on resource-constrained embedded systems. According to the esp-libopus documentation, encoding at 16kHz with complexity 1 uses approximately 70% CPU on an ESP32 running at 240MHz, so complexity 0 is essential for real-time performance.

The bitrate of 30 kbps is a good balance between audio quality and bandwidth usage. For voice communication, this provides clear, intelligible speech while keeping network bandwidth requirements low. The mono channel configuration reduces both encoding complexity and bandwidth compared to stereo, which is appropriate for voice applications where stereo separation isn't necessary.

**Initialization:**
```cpp
opus_encoder_create(SAMPLE_RATE, 1, OPUS_APPLICATION_VOIP)
opus_encoder_ctl(encoder, OPUS_SET_BITRATE(30000))      // 30 kbps
opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(0))      // Low CPU
opus_encoder_ctl(encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE))
```

**Configuration:**
- Sample Rate: 8kHz (ESP32-S3) or 16kHz (ATOMS3R) - Matches I2S sample rate configuration
- Channels: 1 (mono) - Reduces encoding complexity and bandwidth compared to stereo
- Application: VoIP (optimized for voice) - Uses Opus's voice-optimized encoding mode
- Bitrate: 30 kbps - Good balance between quality and bandwidth for voice
- Complexity: 0 (lowest CPU usage) - Essential for real-time performance on ESP32
- Signal Type: Voice - Tells encoder to optimize for speech signals

For more information on Opus encoding, see the [Opus codec documentation](https://opus-codec.org/docs/) and the [esp-libopus repository](https://github.com/XasWorks/esp-libopus) for ESP32-specific performance notes.

**Encoding Flow:**
```cpp
oai_send_audio(peer_connection)
  ├─ i2s_read(I2S_DATA_IN_PORT, encoder_input_buffer, BUFFER_SAMPLES)
  ├─ opus_encode(encoder, input_buffer, samples/2, output_buffer, max_size)
  └─ peer_connection_send_audio(peer_connection, output_buffer, encoded_size)
```

**Buffer Management:**
- Input: `encoder_input_buffer` - I2S samples (320 or 640 samples = 640 or 1280 bytes)
- Output: `encoder_output_buffer` - Opus packets (max 1276 bytes)
- Encoding: `BUFFER_SAMPLES / 2` samples (mono, so divide by 2)

#### Opus Decoder

The Opus decoder handles decompressing audio packets received from OpenAI and preparing them for playback. Interestingly, the decoder is configured for stereo output (2 channels) even though the encoder is mono (1 channel). This asymmetry suggests that OpenAI's Realtime API may send stereo audio, or it could be preparation for future features. The decoder can handle mono input and output stereo by duplicating the mono channel to both left and right outputs.

Decoding is generally less CPU-intensive than encoding, which is why the decoder can handle stereo output without the same performance concerns as encoding. The decoder is initialized early in the startup sequence (before WiFi connection) so it's ready to process audio as soon as the WebRTC connection is established. The output buffer is allocated once at initialization and reused for each decode operation, minimizing memory allocations during runtime.

**Initialization:**
```cpp
opus_decoder_create(SAMPLE_RATE, 2, &error)  // Stereo output
```

**Configuration:**
- Sample Rate: 8kHz (ESP32-S3) or 16kHz (ATOMS3R) - Must match encoder sample rate
- Channels: 2 (stereo) - Note: encoder is mono, decoder is stereo (asymmetry suggests OpenAI sends stereo or future compatibility)
- Output Buffer: `output_buffer` allocated at init (reused) - Pre-allocated buffer avoids runtime allocations

**Decoding Flow:**
```cpp
onaudiotrack callback (from WebRTC)
  └─ oai_audio_decode(data, size)
      ├─ opus_decode(decoder, data, size, output_buffer, BUFFER_SAMPLES, 0)
      └─ i2s_write(I2S_DATA_OUT_PORT, output_buffer, samples * sizeof(int16))
```

**Note**: Decoder is stereo even though encoder is mono. This suggests OpenAI may send stereo audio, or it's for future compatibility. The decoder can handle mono input and output stereo by duplicating channels.

### 5. WiFi Connectivity (`src/wifi.cpp`)

WiFi connectivity is a prerequisite for the SDK's operation, as it provides the Internet connection needed to reach OpenAI's servers. The WiFi implementation uses ESP-IDF's WiFi stack in Station (STA) mode, which allows the device to connect to an existing WiFi network rather than creating its own access point. This is the most common configuration for IoT devices that need Internet access.

The WiFi connection process is event-driven, using ESP-IDF's event loop system to handle connection state changes asynchronously. When the device disconnects from WiFi, the event handler automatically retries the connection up to 5 times. Once an IP address is obtained, a flag is set that unblocks the initialization sequence, allowing the WebRTC connection to proceed.

One important aspect of the WiFi implementation is that it blocks initialization until an IP address is obtained. This ensures that the device doesn't attempt to establish a WebRTC connection without network connectivity, but it also means that if WiFi connection fails, the device will hang indefinitely. This could be improved with a timeout mechanism or fallback behavior.

**Purpose**: Connect to WiFi network for Internet access

**Key Functions:**
- `oai_wifi()` - Initialize WiFi stack, configure credentials, and connect to network (blocks until IP obtained)
- `oai_event_handler()` - Handles WiFi events (disconnect, IP obtained) and manages retry logic

**WiFi Configuration:**
- Mode: Station (STA) - connects to existing network (not access point mode)
- SSID: From `WIFI_SSID` compile-time definition - Embedded in firmware at build time
- Password: From `WIFI_PASSWORD` compile-time definition - Embedded in firmware at build time
- Credentials set via environment variables during build - Set before running `idf.py build`

**Connection Flow:**
```cpp
oai_wifi()
  ├─ Register event handlers (WIFI_EVENT, IP_EVENT)
  ├─ Initialize netif (default WiFi STA)
  ├─ Initialize WiFi (default config)
  ├─ Set WiFi mode (STA)
  ├─ Start WiFi
  ├─ Set WiFi config (SSID, password)
  ├─ Connect to WiFi
  └─ Block until IP address obtained (200ms polling)
```

**Event Handling:**
- `WIFI_EVENT_STA_DISCONNECTED`: Retry connection (up to 5 attempts) - Automatic reconnection on disconnect
- `IP_EVENT_STA_GOT_IP`: Set `g_wifi_connected` flag, unblock initialization - Signals successful connection

**Retry Logic:**
- On disconnect, retry up to 5 times - Provides resilience against temporary network issues
- After 5 failures, initialization still blocks (may need timeout) - Could be improved with timeout or error handling

**Blocking Behavior:**
- Initialization blocks until `g_wifi_connected` flag is set - Ensures network connectivity before proceeding
- Polling interval: 200ms (`vTaskDelay(pdMS_TO_TICKS(200))`) - Checks connection status every 200ms
- No timeout - may block indefinitely if WiFi fails - Potential improvement: add timeout mechanism

For more information on ESP-IDF WiFi configuration, see the [ESP-IDF WiFi API documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html) and [WiFi examples](https://github.com/espressif/esp-idf/tree/master/examples/wifi).

## WebRTC Library (libpeer)

The libpeer library provides the complete WebRTC implementation that powers the SDK's real-time communication capabilities. This is a portable WebRTC library written in C specifically designed for embedded systems and IoT devices. Unlike browser-based WebRTC implementations that rely on JavaScript and web APIs, libpeer is a low-level C library that provides direct control over WebRTC protocols, making it ideal for embedded applications where resource constraints and deterministic behavior are important.

The library implements all the critical WebRTC protocols: ICE for network traversal, DTLS for secure key exchange, SRTP for media encryption, RTP for packetization, and SDP for session description. It's designed to work with BSD sockets, which are widely supported across embedded platforms, and it's optimized for minimal memory footprint and CPU usage. The library is particularly well-suited for this SDK because it provides a complete WebRTC stack without requiring a full operating system or browser runtime.

One of the library's strengths is its modular architecture. Each protocol (ICE, DTLS-SRTP, RTP, SDP) is implemented as a separate component with clear interfaces, making it easier to understand, debug, and modify. The library also includes extensive logging capabilities that can help diagnose connection issues, though these may need to be enabled via compile-time definitions.

### Library Structure

**Location**: `deps/libpeer/src/`

**Key Components:**
- `peer_connection.c/h` - Peer connection state machine and lifecycle management
- `agent.c/h` - ICE agent (candidate gathering, connectivity checks, candidate pair selection)
- `dtls_srtp.c/h` - DTLS handshake and SRTP encryption/decryption
- `rtp.c/h` - RTP packet encoding/decoding with sequence numbers and timestamps
- `sdp.c/h` - SDP generation and parsing for offer/answer exchange
- `stun.c/h` - STUN protocol implementation for NAT traversal
- `sctp.c/h` - SCTP for data channels (not used in this SDK, disabled in configuration)

For more information on libpeer, see the [libpeer GitHub repository](https://github.com/sepfy/libpeer) and [libpeer examples](https://github.com/sepfy/libpeer/tree/main/examples).

### Peer Connection Structure

```c
struct PeerConnection {
  PeerConfiguration config;        // Codec config, callbacks
  PeerConnectionState state;       // Current state
  Agent agent;                     // ICE agent
  DtlsSrtp dtls_srtp;             // DTLS-SRTP session
  Sctp sctp;                       // SCTP (data channels)
  Sdp local_sdp;                  // Local SDP offer
  Sdp remote_sdp;                 // Remote SDP answer
  Buffer* audio_rb;               // Audio ring buffer
  Buffer* video_rb;               // Video ring buffer
  Buffer* data_rb;                // Data channel ring buffer
  RtpEncoder artp_encoder;        // Audio RTP encoder
  RtpEncoder vrtp_encoder;        // Video RTP encoder
  RtpDecoder artp_decoder;        // Audio RTP decoder
  RtpDecoder vrtp_decoder;        // Video RTP decoder
  uint32_t remote_assrc;          // Remote audio SSRC
  uint32_t remote_vssrc;          // Remote video SSRC
};
```

### ICE (Interactive Connectivity Establishment)

Interactive Connectivity Establishment (ICE) is a protocol framework that allows WebRTC applications to establish network connectivity even when devices are behind Network Address Translation (NAT) devices or firewalls. ICE works by gathering multiple candidate network addresses (host address, server-reflexive address via STUN, relay address via TURN) and testing connectivity between candidate pairs to find the best path for media transmission.

The ICE agent is responsible for gathering candidates, performing connectivity checks, and selecting the best candidate pair for the connection. The process involves sending STUN binding requests to test connectivity, waiting for responses, and validating those responses using ICE credentials (username fragment and password) that are exchanged in the SDP. Once a candidate pair shows successful connectivity, it can be nominated for use, and the ICE connection is established.

**Purpose**: Establish network connectivity through NATs and firewalls

**ICE Agent (`agent.c`):**
- Gathers ICE candidates:
  - **Host**: Local IP address - Direct connection candidate, fastest but may not work through NATs
  - **Server Reflexive (srflx)**: Public IP via STUN server (if configured) - Discovered via STUN, works through many NATs
  - **Relay**: TURN server address (if configured) - Relayed through TURN server, works through restrictive NATs/firewalls
- Performs connectivity checks - Tests candidate pairs to find working path
- Selects candidate pair - Chooses best available path based on priority and connectivity
- Manages ICE state machine - Tracks state of each candidate pair (FROZEN → INPROGRESS → SUCCEEDED)

**Candidate Gathering:**
```c
agent_gather_candidate(agent, urls, username, credential)
  ├─ If urls == NULL: Create host candidate
  ├─ If urls starts with "stun:": Create srflx candidate via STUN
  └─ If urls starts with "turn:": Create relay candidate via TURN
```

**Connectivity Checks:**
- Sends STUN binding requests to remote candidates - Tests if packets can reach remote peer
- Waits for STUN binding responses - Validates that remote peer can receive packets
- Validates responses using ICE credentials (ufrag/pwd) - Ensures responses are from legitimate peer
- Nominates candidate pair when connectivity succeeds - Marks pair as ready for media transmission

**ICE State Machine:**
```
FROZEN → INPROGRESS → SUCCEEDED
```

**Note**: This SDK uses empty ICE servers array, so only host candidates are gathered. This works for devices on same network or with direct Internet access, but may fail behind restrictive NATs. For production deployments behind NATs, STUN/TURN servers should be configured. For more information on ICE, see [RFC 8445 (ICE)](https://datatracker.ietf.org/doc/html/rfc8445) and [RFC 5245 (ICE for SIP)](https://datatracker.ietf.org/doc/html/rfc5245).

### DTLS-SRTP (Datagram Transport Layer Security - Secure Real-time Transport Protocol)

DTLS-SRTP is the security mechanism used by WebRTC to encrypt media packets. It combines two protocols: DTLS (Datagram Transport Layer Security) for establishing a secure channel and exchanging keys, and SRTP (Secure Real-time Transport Protocol) for encrypting the actual media packets. This two-phase approach allows WebRTC to use DTLS's certificate-based authentication and key exchange, then derive SRTP keys from the DTLS session for efficient media encryption.

The DTLS handshake establishes a secure channel between the peers, authenticating each side using self-signed certificates and exchanging encryption keys. Once the DTLS handshake is complete, the peers export keying material that's used to derive SRTP encryption keys. This key derivation follows the standard defined in RFC 5764, ensuring compatibility with other WebRTC implementations.

SRTP encryption is applied to every RTP packet, providing confidentiality (packets can't be read), integrity (packets can't be modified), and replay protection (old packets can't be replayed). The encryption uses AES-128 in counter mode with HMAC-SHA1 for authentication, which provides strong security while maintaining good performance on embedded systems.

**Purpose**: Encrypt RTP media packets

**DTLS-SRTP Structure (`dtls_srtp.c`):**
- **DTLS**: Uses mbedTLS for DTLS handshake - Provides certificate-based authentication and key exchange
- **SRTP**: Uses libsrtp for SRTP encryption - Handles per-packet encryption and authentication
- **Key Derivation**: SRTP keys derived from DTLS handshake using `MBEDTLS_TLS_SRTP_AES128_CM_HMAC_SHA1_80` profile - Standard WebRTC key derivation

**DTLS Handshake:**
```c
dtls_srtp_handshake(dtls_srtp, remote_addr)
  ├─ Create self-signed certificate - Generates certificate for this session
  ├─ Initialize mbedTLS SSL context - Sets up DTLS state machine
  ├─ Perform DTLS handshake (client or server role) - Exchanges certificates and keys
  ├─ Export keying material (SRTP keys) - Extracts 60 bytes for SRTP key derivation
  └─ Create SRTP sessions (inbound and outbound) - Initializes SRTP encryption contexts
```

**SRTP Key Derivation:**
- DTLS handshake exports 60 bytes of keying material - Standard length for SRTP key derivation
- First 16 bytes: Remote SRTP master key - Used to encrypt packets sent to remote peer
- Next 14 bytes: Remote SRTP master salt - Used in key derivation for remote peer
- Next 16 bytes: Local SRTP master key - Used to encrypt packets sent from local peer
- Last 14 bytes: Local SRTP master salt - Used in key derivation for local peer

**SRTP Encryption:**
- Outbound RTP: `srtp_protect()` encrypts packet - Adds encryption and authentication tag
- Inbound RTP: `srtp_unprotect()` decrypts packet - Verifies and decrypts received packet
- RTCP: Separate protect/unprotect functions - RTCP packets use different SRTP context

**DTLS-SRTP States:**
```
DTLS_SRTP_STATE_INIT → DTLS_SRTP_STATE_HANDSHAKE → DTLS_SRTP_STATE_CONNECTED
```

For more information on DTLS-SRTP, see [RFC 5764 (DTLS-SRTP)](https://datatracker.ietf.org/doc/html/rfc5764) and [RFC 3711 (SRTP)](https://datatracker.ietf.org/doc/html/rfc3711).

### RTP (Real-time Transport Protocol)

RTP (Real-time Transport Protocol) is the standard protocol for delivering audio and video over IP networks. It provides packetization, sequencing, timing information, and payload type identification, all of which are essential for reconstructing audio streams from network packets. RTP doesn't provide reliability guarantees (packets may be lost or arrive out of order), but it does provide the information needed for applications to handle these issues appropriately.

In this SDK, RTP is used to wrap Opus-encoded audio packets with headers that include sequence numbers (for detecting lost packets), timestamps (for playback timing), and payload type identifiers (to indicate Opus encoding). The RTP headers are relatively small (12 bytes minimum), adding minimal overhead while providing essential functionality. The RTP packets are then encrypted with SRTP before being sent over UDP.

**Purpose**: Transport Opus audio packets over UDP

**RTP Header Structure:**
```c
struct RtpHeader {
  uint16_t version : 2;        // RTP version (always 2)
  uint16_t padding : 1;        // Padding flag
  uint16_t extension : 1;      // Extension header flag
  uint16_t csrccount : 4;     // CSRC count
  uint16_t markerbit : 1;     // Marker bit
  uint16_t type : 7;           // Payload type
  uint16_t seq_number;        // Sequence number
  uint32_t timestamp;          // Timestamp
  uint32_t ssrc;               // Synchronization source
};
```

**RTP Encoder (`rtp.c`):**
- Wraps Opus packets in RTP headers - Adds 12-byte RTP header to each Opus packet
- Payload Type: 111 (PT_OPUS) - Standard payload type for Opus audio (RFC 7587)
- SSRC: 6 (SSRC_OPUS) - Synchronization source identifier for audio stream
- Timestamp increment: `AUDIO_LATENCY * 48000 / 1000` (for Opus, uses 48kHz clock) - Timestamps use Opus's native 48kHz clock regardless of actual sample rate
- Sequence number: Increments for each packet - Used for detecting lost packets and reordering

For more information on RTP, see [RFC 3550 (RTP)](https://datatracker.ietf.org/doc/html/rfc3550) and [RFC 7587 (RTP Payload Format for Opus)](https://datatracker.ietf.org/doc/html/rfc7587).

**RTP Encoder Flow:**
```c
rtp_encoder_encode(encoder, opus_packet, size)
  ├─ Create RTP header
  ├─ Set payload type (PT_OPUS)
  ├─ Set SSRC (SSRC_OPUS)
  ├─ Set sequence number (increment)
  ├─ Set timestamp (increment by timestamp_increment)
  ├─ Copy Opus packet to RTP payload
  └─ Call on_packet callback (encrypts and sends)
```

**RTP Decoder (`rtp.c`):**
- Extracts Opus payload from RTP packets
- Validates RTP header
- Calls `on_packet` callback with Opus payload

**RTP Decoder Flow:**
```c
rtp_decoder_decode(decoder, rtp_packet, size)
  ├─ Validate RTP header
  ├─ Extract SSRC (identify audio vs. video)
  ├─ Extract payload (Opus packet)
  └─ Call on_packet callback (decodes and plays)
```

### SDP (Session Description Protocol)

Session Description Protocol (SDP) is a text-based format for describing multimedia sessions. In WebRTC, SDP is used to exchange information about media capabilities, network addresses, and security parameters between peers. The SDP offer (created by the initiating peer) and answer (created by the responding peer) contain all the information needed to establish a WebRTC connection.

The SDP format is human-readable text, making it easy to debug connection issues by examining the SDP messages. However, parsing SDP correctly can be complex due to the format's flexibility and the many optional attributes. The libpeer library handles SDP generation and parsing, ensuring that the SDP messages conform to WebRTC standards and are compatible with OpenAI's servers.

**Purpose**: Describe media session and exchange connection information

**SDP Generation (`sdp.c`):**
- Creates SDP offer/answer with:
  - Session description (v=0, o=, s=, t=) - Protocol version, origin, session name, timing
  - Media description (m=audio) - Media type (audio), port, protocol (RTP/SAVPF), payload types
  - Codec information (rtpmap for Opus) - Maps payload type to codec name and parameters
  - ICE candidates (a=candidate) - Network addresses for ICE connectivity checks
  - ICE credentials (a=ice-ufrag, a=ice-pwd) - Username fragment and password for ICE authentication
  - DTLS fingerprint (a=fingerprint:sha-256) - SHA-256 hash of DTLS certificate for verification
  - DTLS setup (a=setup:active/passive) - Indicates which peer initiates DTLS handshake

**SDP Format Example:**
```
v=0
o=- 0 0 IN IP4 127.0.0.1
s=-
t=0 0
m=audio 5004 RTP/SAVPF 111
a=rtpmap:111 opus/48000/2
a=ice-ufrag:xxxx
a=ice-pwd:xxxx
a=fingerprint:sha-256 xxxx
a=setup:active
a=candidate:1 1 UDP 1 192.168.1.100 5004 typ host
```

**SDP Parsing:**
- Parses remote SDP answer to extract:
  - ICE credentials (ufrag, pwd) - Used to authenticate ICE connectivity check messages
  - ICE candidates - Remote peer's network addresses for connectivity testing
  - Media descriptions - Confirms codec support and media parameters
  - DTLS fingerprint - Used to verify remote peer's DTLS certificate matches SDP

For more information on SDP, see [RFC 4566 (SDP)](https://datatracker.ietf.org/doc/html/rfc4566) and [WebRTC SDP examples](https://webrtc.github.io/samples/src/content/peerconnection/pc1/).

## Data Flow

Understanding the data flow through the SDK is essential for debugging issues, optimizing performance, and modifying the system. The data flows in two directions: upstream (from microphone to OpenAI) for voice input, and downstream (from OpenAI to speaker) for AI responses. Both directions share similar processing steps but in reverse order, and both must maintain real-time performance to avoid audio dropouts or excessive latency.

The data flow diagrams below show the complete path that audio data takes from hardware capture to network transmission, and from network reception to hardware playback. Each step in the pipeline adds some latency, and understanding these latencies helps in optimizing the system for the best possible user experience. The total latency from microphone to speaker (excluding network latency) is typically 100-150ms, which is acceptable for conversational applications but could be improved with further optimization.

### Audio Capture and Transmission Flow

```
Microphone (I2S)
    ↓
I2S DMA Buffer (8 buffers × 320/640 samples)
    ↓
i2s_read() [oai_send_audio_task, 15ms interval]
    ↓
encoder_input_buffer (320/640 samples = 640/1280 bytes)
    ↓
opus_encode() [30 kbps, complexity 0, VoIP mode]
    ↓
encoder_output_buffer (Opus packet, max 1276 bytes)
    ↓
peer_connection_send_audio()
    ↓
Audio Ring Buffer (peer_connection->audio_rb)
    ↓
RTP Encoder [rtp_encoder_encode()]
    ├─ Add RTP header (PT=111, SSRC=6, seq, timestamp)
    └─ Call on_packet callback
        ↓
DTLS-SRTP Encrypt [dtls_srtp_encrypt_rtp_packet()]
    ↓
ICE Agent Send [agent_send()]
    ↓
UDP Socket [sendto()]
    ↓
Internet → OpenAI Realtime API
```

### Audio Reception and Playback Flow

```
OpenAI Realtime API → Internet
    ↓
UDP Socket [recv()]
    ↓
ICE Agent Receive [agent_recv()]
    ↓
DTLS-SRTP Decrypt [dtls_srtp_decrypt_rtp_packet()]
    ↓
RTP Decoder [rtp_decoder_decode()]
    ├─ Validate RTP header
    ├─ Extract SSRC (identify audio stream)
    └─ Extract Opus payload
        ↓
onaudiotrack Callback [oai_audio_decode()]
    ↓
Opus Decoder [opus_decode()]
    ├─ Decode Opus packet
    └─ Output stereo PCM (BUFFER_SAMPLES samples)
        ↓
output_buffer (PCM samples)
    ↓
i2s_write() [I2S_DATA_OUT_PORT]
    ↓
I2S DMA Buffer
    ↓
Speaker/Headphones
```

### WebRTC Connection Establishment Flow

```
Application Start
    ↓
peer_init() [Initialize WebRTC library]
    ↓
peer_connection_create() [Create peer connection]
    ↓
peer_connection_create_offer() [Generate SDP offer]
    ├─ Gather ICE candidates (host)
    ├─ Create DTLS certificate
    ├─ Generate SDP with candidates, fingerprint
    └─ Trigger onicecandidate callback
        ↓
oai_on_icecandidate_task() [Callback]
    ↓
oai_http_request() [POST SDP offer to OpenAI]
    ├─ HTTP POST to https://api.openai.com/v1/realtime?model=...
    ├─ Headers: Content-Type: application/sdp, Authorization: Bearer {key}
    ├─ Body: SDP offer
    └─ Response: SDP answer
        ↓
peer_connection_set_remote_description() [Set SDP answer]
    ├─ Parse remote SDP (candidates, credentials)
    ├─ Create candidate pairs
    └─ Start ICE connectivity checks
        ↓
ICE Connectivity Checks [agent_connectivity_check()]
    ├─ Send STUN binding requests
    ├─ Receive STUN binding responses
    └─ Validate responses
        ↓
ICE Connected [PEER_CONNECTION_CONNECTED]
    ↓
DTLS Handshake [dtls_srtp_handshake()]
    ├─ Exchange certificates
    ├─ Derive SRTP keys
    └─ Create SRTP sessions
        ↓
DTLS Connected [PEER_CONNECTION_COMPLETED]
    ↓
Create Audio Encoding Task [oai_send_audio_task]
    ↓
Start Audio Streaming [peer_connection_loop()]
```

## Build System

The SDK uses ESP-IDF's component-based build system built on CMake, which provides powerful dependency management and cross-platform support. The build system is organized hierarchically, with a root CMakeLists.txt that sets up the overall project, and component-level CMakeLists.txt files that define individual components and their dependencies. This organization allows components to be developed and tested independently while still integrating seamlessly into the final firmware.

One notable aspect of the build system is how it handles platform differences. The same codebase can be built for both ESP32-S3 and Linux targets, with conditional compilation used to include or exclude platform-specific code. This dual-target approach significantly accelerates development by allowing developers to test WebRTC connection logic without physical hardware, though the Linux build has limitations (no audio or WiFi support).

### CMake Structure

**Root CMakeLists.txt:**
- Sets compile definitions (WiFi SSID/password, API key, API URL) - Embeds configuration in firmware binary
- Configures component directories - Tells ESP-IDF where to find components
- Includes ESP-IDF project - Initializes ESP-IDF build system

**Component CMakeLists.txt (`src/CMakeLists.txt`):**
- Registers source files conditionally (Linux vs. ESP32) - Different source files for different platforms
- Sets component requirements (peer, esp-libopus, esp_http_client, etc.) - Declares dependencies
- Configures compiler flags for dependencies - Suppresses warnings from third-party code

**Peer Component (`components/peer/CMakeLists.txt`):**
- Wraps libpeer source files - Makes libpeer available as ESP-IDF component
- Disables usrsctp (SCTP not used) - Removes unused SCTP functionality to reduce code size
- Disables keepalives - Reduces network overhead
- Sets compile definitions for ESP32 - Platform-specific configuration

For more information on ESP-IDF build system, see the [ESP-IDF Build System documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html).

### Build Configuration

**Kconfig (`src/Kconfig.projbuild`):**
- Board selection: `OPENAI_BOARD_ESP32_S3` or `OPENAI_BOARD_M5_ATOMS3R`

**SDK Config (`sdkconfig.defaults`):**
- ESP Event Loop: Disable ISR posting
- TLS: Skip certificate verification (insecure, for development)
- DTLS-SRTP: Enable DTLS protocol
- Main Task Stack: 16KB (libpeer requires large stack)
- CPU Frequency: 240 MHz
- SPIRAM: Enabled, OCT mode
- Watchdog: Disabled
- Compiler: Performance optimization, disable assertions

**Partition Table (`partitions.csv`):**
- NVS: 24KB
- PHY Init: 4KB
- Factory App: 1.5MB

### Build Process

```bash
# Set environment variables
export WIFI_SSID="your_ssid"
export WIFI_PASSWORD="your_password"
export OPENAI_API_KEY="your_api_key"

# Set target
idf.py set-target esp32s3  # or linux

# Build
idf.py build

# Flash (ESP32)
sudo -E idf.py flash

# Run (Linux)
./build/src.elf
```

## Dependencies

### External Libraries

**libpeer (`deps/libpeer/`):**
- **Purpose**: Portable WebRTC implementation in C
- **Features**: ICE, DTLS-SRTP, RTP, SDP, STUN, TURN
- **Dependencies**: mbedtls, libsrtp, usrsctp (disabled), cJSON
- **License**: See `deps/libpeer/LICENSE`

**esp-libopus (`components/esp-libopus/`):**
- **Purpose**: Opus audio codec
- **Features**: Encoder and decoder
- **License**: See component directory

**libsrtp (`components/srtp/`):**
- **Purpose**: SRTP encryption library
- **Features**: RTP/RTCP encryption and decryption
- **License**: See component directory

**ES8311 (`managed_components/espressif__es8311/`):**
- **Purpose**: ES8311 audio codec driver
- **Features**: I2C control, I2S interface
- **Used**: M5 ATOMS3R board only

### ESP-IDF Components

- `esp_wifi` - WiFi stack
- `esp_http_client` - HTTP client
- `nvs_flash` - Non-volatile storage
- `driver` - Hardware drivers (I2S, I2C)
- `esp_netif` - Network interface
- `esp_event` - Event loop
- `mbedtls` - TLS/DTLS library (via libpeer)
- `json` - JSON parsing (cJSON, via libpeer)

## Hardware Configuration

### ESP32-S3 Default Board

**Audio Configuration:**
- Sample Rate: 8 kHz
- I2S Ports: Separate TX (I2S_NUM_0) and RX (I2S_NUM_1)
- Pins:
  - TX: MCLK=GPIO0, BCLK=GPIO15, LRCLK=GPIO16, DATA=GPIO17
  - RX: MCLK=GPIO0, BCLK=GPIO38, LRCLK=GPIO39, DATA=GPIO40

**Stack Sizes:**
- Audio encoding task: 20KB (PSRAM)

### M5 ATOMS3R with EchoBase

**Audio Configuration:**
- Sample Rate: 16 kHz (EchoBase requirement)
- I2S Port: Single duplex port (I2S_NUM_1)
- Codec: ES8311 via I2C
- I2C: Port I2C_NUM_1, 400kHz, SCL=GPIO39, SDA=GPIO38
- Pins:
  - BCLK=GPIO8, LRCLK=GPIO6, DATA_OUT=GPIO5, DATA_IN=GPIO7
  - Channel Format: `I2S_CHANNEL_FMT_ALL_LEFT` (EchoBase requirement)

**Stack Sizes:**
- Audio encoding task: 40KB (PSRAM, increased due to 16kHz)

## Threading Model

### FreeRTOS Tasks

1. **Main Task** (`app_main`)
   - Priority: Default (1)
   - Stack: Default (16KB configured)
   - Purpose: Initialization

2. **WebRTC Loop Task** (`oai_webrtc` main loop)
   - Priority: Default (1)
   - Stack: Default
   - Purpose: WebRTC packet processing (runs in main task after init)

3. **Audio Encoding Task** (`oai_send_audio_task`)
   - Priority: 7 (high)
   - Stack: 20KB (ESP32-S3) or 40KB (ATOMS3R), PSRAM
   - Core: Pinned to core 0
   - Purpose: Audio capture and encoding (15ms interval)

4. **WiFi Task** (ESP-IDF internal)
   - Created by ESP-IDF WiFi stack
   - Purpose: WiFi event handling

5. **HTTP Client Task** (ESP-IDF internal)
   - Created by ESP-IDF HTTP client
   - Purpose: HTTP request handling

### Synchronization

- **Event Loop**: Thread-safe event posting (ESP-IDF)
- **I2S**: DMA-based, non-blocking with timeouts
- **WebRTC**: Single-threaded main loop, callbacks from network stack
- **Audio Buffers**: Ring buffers in peer connection (no explicit locking shown)

## Error Handling

### Initialization Errors

- **NVS Flash**: Erase and reinit on version mismatch
- **WiFi**: Retry up to 5 times on disconnect
- **Audio Codec**: Log error, continue (may cause audio issues)
- **Peer Connection**: Restart on creation failure (ESP32)

### Runtime Errors

- **HTTP Request Failure**: Restart on non-201 status (ESP32)
- **WebRTC Disconnect**: Restart on disconnect/close (ESP32)
- **Audio Encoding**: No explicit error handling (may drop frames)
- **Audio Decoding**: No explicit error handling (may cause silence)

### Error Recovery

- **ESP32**: Restart on critical errors (HTTP failure, WebRTC disconnect)
- **Linux**: Log errors, exit on critical failures
- **No Retry Logic**: Most errors cause immediate restart/exit

## Performance Characteristics

Understanding the SDK's performance characteristics is important for optimizing system behavior, debugging issues, and planning hardware requirements. The performance metrics below are based on the current implementation with default settings, and actual performance may vary depending on network conditions, CPU load, and hardware configuration.

### Audio Latency

Audio latency is a critical metric for real-time voice communication, as high latency makes conversations feel unnatural and can cause users to talk over each other. The SDK's latency is composed of several components: capture buffering, encoding time, network transmission, decoding time, and playback buffering. The total latency from microphone to speaker (excluding network latency) is typically 100-150ms, which is acceptable for conversational applications but could be improved with smaller buffers or faster processing.

- **Capture Interval**: 15ms (audio encoding task) - Time between audio capture operations
- **Buffer Size**: 40ms (320 samples @ 8kHz or 640 samples @ 16kHz) - I2S DMA buffer size, adds latency but prevents dropouts
- **Encoding**: Opus encoding adds ~1-5ms - CPU time for Opus compression
- **Network**: Variable (depends on Internet connection) - Can range from 20ms (local) to 200ms+ (distant servers)
- **Decoding**: Opus decoding adds ~1-5ms - CPU time for Opus decompression
- **Playback**: I2S DMA buffering adds ~40ms - Buffer size for smooth playback
- **Total Estimated Latency**: ~100-150ms (excluding network) - Acceptable for voice, but could be optimized further

### CPU Usage

CPU usage is an important consideration for embedded systems, as excessive CPU usage can cause audio dropouts, network timeouts, or system instability. The SDK is designed to minimize CPU usage while maintaining real-time performance, using low-complexity Opus encoding and efficient WebRTC processing.

- **Opus Encoding**: Complexity 0 (lowest CPU usage) - Approximately 30-40% CPU at 8kHz, 50-60% at 16kHz on ESP32-S3 @ 240MHz
- **Opus Decoding**: Moderate CPU usage - Less CPU-intensive than encoding, approximately 20-30% CPU
- **WebRTC Processing**: Low (mostly I/O bound) - Most time spent waiting for network I/O, minimal CPU usage
- **DTLS-SRTP**: Moderate (encryption/decryption) - Cryptographic operations add CPU overhead, but only during active transmission

### Memory Usage

Memory usage is carefully managed in the SDK, as ESP32-S3 has limited RAM (512KB SRAM + 8MB PSRAM). The SDK uses PSRAM for large buffers (like audio encoding task stack) and SRAM for frequently accessed data structures. Understanding memory usage helps in optimizing the system and avoiding out-of-memory conditions.

- **Audio Buffers**: ~2-4KB (I2S DMA + Opus buffers) - Relatively small, fits in SRAM
- **WebRTC**: ~50-100KB (peer connection structure, buffers) - Largest memory consumer, uses PSRAM
- **Stack**: 20-40KB (audio encoding task) - Allocated from PSRAM to avoid SRAM exhaustion
- **Heap**: Variable (depends on connection state) - Grows during connection establishment, stabilizes during streaming

### Network Bandwidth

Network bandwidth requirements are relatively modest thanks to Opus's efficient compression. The total bandwidth per direction is approximately 35-40 kbps, which is well within the capabilities of modern WiFi networks and most Internet connections. This low bandwidth requirement makes the SDK suitable for deployment in bandwidth-constrained environments.

- **Audio Bitrate**: 30 kbps (Opus encoding) - Base audio data rate
- **RTP Overhead**: ~12 bytes per packet - RTP header adds minimal overhead
- **DTLS-SRTP Overhead**: ~4-16 bytes per packet - Encryption and authentication add overhead
- **Total Estimated**: ~35-40 kbps per direction - Very reasonable for voice communication

For more information on Opus performance, see the [esp-libopus performance notes](https://github.com/XasWorks/esp-libopus) and [Opus codec performance documentation](https://opus-codec.org/performance/).

## Limitations and Constraints

1. **No STUN/TURN**: Empty ICE servers array - may fail behind restrictive NATs
2. **Single Audio Stream**: Only one audio stream supported
3. **No Video**: Video codec set to CODEC_NONE
4. **No Data Channel**: Data channel disabled
5. **Fixed Sample Rates**: 8kHz or 16kHz (no dynamic adjustment)
6. **Fixed Opus Bitrate**: 30 kbps (no adaptive bitrate)
7. **Restart on Error**: ESP32 restarts on HTTP/WebRTC errors (no graceful recovery)
8. **Blocking WiFi**: WiFi initialization blocks indefinitely (no timeout)
9. **Limited Error Handling**: Many errors cause restart/exit without retry
10. **Linux Build Limitations**: No audio/WiFi support (WebRTC only)

## Future Enhancement Opportunities

1. **STUN/TURN Support**: Add ICE servers for NAT traversal
2. **Adaptive Bitrate**: Adjust Opus bitrate based on network conditions
3. **Error Recovery**: Implement retry logic instead of restart
4. **WiFi Timeout**: Add timeout for WiFi connection
5. **Audio Quality Metrics**: Monitor latency, dropouts, encoding errors
6. **Multiple Audio Streams**: Support multiple concurrent audio streams
7. **Data Channel**: Enable data channel for text/control messages
8. **Video Support**: Add video encoding/decoding
9. **Configuration Storage**: Store WiFi credentials in NVS instead of compile-time
10. **OTA Updates**: Implement over-the-air firmware updates

## Key Files Reference

### Application Source Files

- `src/main.cpp` - Entry point and initialization
- `src/main.h` - Function declarations
- `src/webrtc.cpp` - WebRTC peer connection management
- `src/http.cpp` - HTTP signaling with OpenAI API
- `src/media.cpp` - Audio capture, encoding, and decoding
- `src/wifi.cpp` - WiFi connectivity

### WebRTC Library Files

- `deps/libpeer/src/peer_connection.c/h` - Peer connection implementation
- `deps/libpeer/src/agent.c/h` - ICE agent
- `deps/libpeer/src/dtls_srtp.c/h` - DTLS-SRTP implementation
- `deps/libpeer/src/rtp.c/h` - RTP encoding/decoding
- `deps/libpeer/src/sdp.c/h` - SDP generation/parsing
- `deps/libpeer/src/stun.c/h` - STUN protocol

### Build Configuration

- `CMakeLists.txt` - Root build configuration
- `src/CMakeLists.txt` - Source component configuration
- `src/Kconfig.projbuild` - Board selection Kconfig
- `sdkconfig.defaults` - Default SDK configuration
- `partitions.csv` - Partition table

### Component Wrappers

- `components/peer/CMakeLists.txt` - Peer component wrapper
- `components/srtp/` - SRTP library
- `components/esp-libopus/` - Opus codec library

## API Reference Summary

### Main Application APIs

**Initialization:**
- `peer_init()` - Initialize WebRTC library
- `oai_init_audio_capture()` - Initialize I2S and audio codec
- `oai_init_audio_decoder()` - Initialize Opus decoder
- `oai_init_audio_encoder()` - Initialize Opus encoder
- `oai_wifi()` - Connect to WiFi (blocks until IP)

**WebRTC:**
- `peer_connection_create()` - Create peer connection
- `peer_connection_create_offer()` - Generate SDP offer
- `peer_connection_set_remote_description()` - Set SDP answer
- `peer_connection_loop()` - Process WebRTC packets
- `peer_connection_send_audio()` - Send audio packet
- `peer_connection_onicecandidate()` - Register ICE candidate callback
- `peer_connection_oniceconnectionstatechange()` - Register state change callback

**HTTP:**
- `oai_http_request()` - POST SDP offer, receive answer

**Audio:**
- `oai_send_audio()` - Capture and encode audio
- `oai_audio_decode()` - Decode and play audio

### WebRTC Library APIs (libpeer)

**Peer Connection:**
- `peer_connection_create()` - Create peer connection
- `peer_connection_destroy()` - Destroy peer connection
- `peer_connection_close()` - Close connection
- `peer_connection_loop()` - Main processing loop
- `peer_connection_send_audio()` - Send audio packet
- `peer_connection_send_video()` - Send video packet
- `peer_connection_create_offer()` - Create SDP offer
- `peer_connection_set_remote_description()` - Set remote SDP

**ICE:**
- `agent_gather_candidate()` - Gather ICE candidates
- `agent_connectivity_check()` - Perform connectivity check
- `agent_send()` - Send UDP packet
- `agent_recv()` - Receive UDP packet

**DTLS-SRTP:**
- `dtls_srtp_init()` - Initialize DTLS-SRTP
- `dtls_srtp_handshake()` - Perform DTLS handshake
- `dtls_srtp_encrypt_rtp_packet()` - Encrypt RTP packet
- `dtls_srtp_decrypt_rtp_packet()` - Decrypt RTP packet

**RTP:**
- `rtp_encoder_init()` - Initialize RTP encoder
- `rtp_encoder_encode()` - Encode RTP packet
- `rtp_decoder_init()` - Initialize RTP decoder
- `rtp_decoder_decode()` - Decode RTP packet

**SDP:**
- `sdp_create()` - Create SDP
- `sdp_append_opus()` - Append Opus media description
- `sdp_append()` - Append SDP line

## Related Documentation and Resources

### Official Documentation

- **ESP-IDF Programming Guide**: Comprehensive [ESP-IDF documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/) covering all aspects of ESP32 development
- **ESP-IDF API Reference**: Detailed [API reference](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/) for all ESP-IDF components
- **ESP-IDF Examples**: [Example projects](https://github.com/espressif/esp-idf/tree/master/examples) demonstrating various ESP-IDF features
- **OpenAI Realtime API**: [Official Realtime API documentation](https://platform.openai.com/docs/guides/realtime) explaining capabilities and usage

### WebRTC Resources

- **WebRTC Specification**: [W3C WebRTC specification](https://www.w3.org/TR/webrtc/) - Authoritative technical reference
- **MDN WebRTC Guide**: [Mozilla's WebRTC documentation](https://developer.mozilla.org/en-US/docs/Web/API/WebRTC_API) - Excellent tutorials and examples
- **WebRTC Samples**: [WebRTC samples repository](https://webrtc.github.io/samples/) - Practical examples and demos
- **libpeer Repository**: [libpeer GitHub repository](https://github.com/sepfy/libpeer) - WebRTC library used by this SDK
- **RFC 8829**: [WebRTC Overview](https://datatracker.ietf.org/doc/html/rfc8829) - IETF WebRTC specification overview

### Audio Codec Resources

- **Opus Codec**: [Opus codec website](https://opus-codec.org/) - Documentation, benchmarks, and implementation guides
- **RFC 6716**: [Opus specification](https://datatracker.ietf.org/doc/html/rfc6716) - Opus audio codec standard
- **RFC 7587**: [RTP Payload Format for Opus](https://datatracker.ietf.org/doc/html/rfc7587) - How Opus is transported over RTP
- **esp-libopus**: [esp-libopus repository](https://github.com/XasWorks/esp-libopus) - ESP32 port with performance notes

### Networking and Security Resources

- **RFC 8445**: [ICE specification](https://datatracker.ietf.org/doc/html/rfc8445) - Interactive Connectivity Establishment
- **RFC 5764**: [DTLS-SRTP](https://datatracker.ietf.org/doc/html/rfc5764) - Datagram Transport Layer Security for SRTP
- **RFC 3711**: [SRTP specification](https://datatracker.ietf.org/doc/html/rfc3711) - Secure Real-time Transport Protocol
- **RFC 3550**: [RTP specification](https://datatracker.ietf.org/doc/html/rfc3550) - Real-time Transport Protocol
- **RFC 4566**: [SDP specification](https://datatracker.ietf.org/doc/html/rfc4566) - Session Description Protocol

### ESP32 Hardware Resources

- **ESP32-S3 Datasheet**: [ESP32-S3 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
- **ESP-IDF I2S Driver**: [I2S API documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html)
- **ESP-IDF WiFi API**: [WiFi API documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html)
- **ESP-IDF HTTP Client**: [HTTP Client API documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_client.html)

### OpenAI API Resources

- **OpenAI API Quickstart**: [Quickstart guide](https://platform.openai.com/docs/quickstart) for getting started with OpenAI APIs
- **OpenAI API Tutorials**: [Tutorial collection](https://platform.openai.com/docs/tutorials) with step-by-step guides
- **OpenAI Authentication**: [Authentication guide](https://platform.openai.com/docs/guides/authentication) for API key management
- **OpenAI Community**: [Developer forum](https://community.openai.com/) for questions and discussions
