# ESP32 M5Stack Camera Streaming System Architecture

## Overview

This system simulates an ESP32 M5Stack M12 camera streaming JPEG frames over WebSocket to a server that transcodes them to H.264 for efficient client playback.

## System Components

### 1. Mock ESP32 Camera Client (`mock_camera.py`)
- **Purpose**: Simulates ESP32 M5Stack camera device
- **Technology**: Python with PIL/Pillow for image generation
- **Output**: JPEG frames sent via WebSocket
- **Animation**: Simple animated pattern (moving circle/gradient)
- **Frame Rate**: ~15-30 fps
- **Resolution**: 640x480 (typical for M5Stack cameras)

### 2. Streaming Server (`server.py`)
- **Purpose**: Receives JPEG frames and transcodes to H.264
- **Technology**: Python with:
  - `websockets` for WebSocket server
  - `ffmpeg` for H.264 transcoding
  - HTTP server for serving web client and H.264 stream
- **Ports**:
  - 8765: WebSocket server (receives JPEG from camera)
  - 8080: HTTP server (serves web client and H.264 stream)
- **Transcoding**: Real-time JPEG → H.264 using FFmpeg pipe

### 3. Web Client (`client.html`)
- **Purpose**: Display H.264 video stream in browser
- **Technology**: HTML5 + JavaScript
- **Playback**: Uses Media Source Extensions (MSE) or HLS
- **Features**: 
  - Real-time video display
  - Connection status indicator
  - Low-latency playback

## Data Flow

```
[Mock ESP32 Camera]
    ↓ (JPEG frames via WebSocket)
[Streaming Server]
    ↓ (FFmpeg transcoding)
[H.264 Stream]
    ↓ (HTTP/WebSocket)
[Web Client Browser]
```

## Implementation Plan

1. **Phase 1**: Set up project structure and dependencies
2. **Phase 2**: Implement WebSocket server with FFmpeg transcoding pipeline
3. **Phase 3**: Create mock camera client with animated JPEG generation
4. **Phase 4**: Build web client with video playback
5. **Phase 5**: Integration testing and deployment

## Future Enhancement

Once the real ESP32 M5Stack device is available, simply replace the mock camera client with the actual device firmware that connects to the same WebSocket endpoint.
