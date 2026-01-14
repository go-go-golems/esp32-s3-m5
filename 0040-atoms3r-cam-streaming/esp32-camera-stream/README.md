# ESP32 M5Stack Camera Streaming System

A complete real-time video streaming system that receives JPEG frames from an ESP32 M5Stack camera over WebSocket and transcodes them to H.264 for efficient web client playback.

## System Architecture

The system consists of three main components that work together to provide real-time video streaming:

### 1. Mock ESP32 Camera Client (`mock_camera.py`)

This Python script simulates an ESP32 M5Stack M12 camera by generating animated JPEG frames and streaming them via WebSocket. The mock camera produces a visually engaging animation featuring an animated gradient background, a moving circle with gradient effects, and a moving square outline. Each frame includes metadata overlays showing frame count, FPS, and elapsed time.

The camera operates at **640x480 resolution** and streams at **30 frames per second**, matching typical ESP32 camera specifications. When you have a real ESP32 device, you can replace this mock client with your actual hardware without any changes to the server infrastructure.

### 2. Streaming Server (`server.py`)

The server acts as the central hub of the streaming system, managing multiple concurrent connections and performing real-time video transcoding. It operates three distinct services:

**Camera WebSocket Server (Port 8765)**: Accepts connections from ESP32 cameras (mock or real) and receives JPEG frames as binary WebSocket messages. When a camera connects, the server automatically initializes an FFmpeg transcoding pipeline.

**FFmpeg Transcoding Pipeline**: Converts incoming JPEG frames to H.264 video in real-time using hardware-accelerated encoding where available. The pipeline is configured for ultra-low latency with the `ultrafast` preset and `zerolatency` tuning, making it ideal for live streaming applications. Output is in MPEG-TS format for streamable delivery.

**Viewer WebSocket Server (Port 8766)**: Distributes the transcoded H.264 stream to multiple web clients simultaneously. The server maintains a rolling buffer of recent video data to allow new viewers to start playback quickly.

**HTTP Server (Port 8080)**: Serves the web client interface and provides a status API endpoint for monitoring system health.

### 3. Web Clients

Two web client implementations are provided:

**H.264 Client (`client.html`)**: Uses the MediaSource Extensions API to play H.264 video directly in the browser. This provides the most efficient bandwidth usage and best quality, but requires browser support for the specific codec configuration.

**MJPEG Client (`client_mjpeg.html`)**: Displays JPEG frames directly on an HTML5 canvas, bypassing codec complexity. This approach offers maximum compatibility across browsers and is easier to debug, making it ideal for development and testing.

Both clients feature a modern, responsive interface with real-time statistics including connection status, bitrate, frame count, FPS, and latency measurements.

## Installation

### Prerequisites

The system requires **Python 3.7 or higher** and **FFmpeg 4.0 or higher**. On Ubuntu/Debian systems, install FFmpeg with:

```bash
sudo apt-get update
sudo apt-get install ffmpeg
```

### Python Dependencies

Install the required Python packages:

```bash
pip3 install websockets aiohttp pillow
```

Or use the system package manager:

```bash
sudo pip3 install websockets aiohttp pillow
```

## Usage

### Starting the System

**Step 1: Start the Server**

The server must be running before connecting any cameras or viewers:

```bash
python3 server.py
```

You should see output indicating all three services have started:

```
INFO - HTTP server started on port 8080
INFO - Camera WebSocket server started on port 8765
INFO - Viewer WebSocket server started on port 8766
INFO - Server is ready!
```

**Step 2: Start the Mock Camera**

In a separate terminal, launch the mock camera client:

```bash
python3 mock_camera.py
```

The camera will automatically connect to the server and begin streaming frames. You'll see periodic log messages confirming frame transmission:

```
INFO - Connected to server!
INFO - Sent frame 30 (25113 bytes)
INFO - Sent frame 60 (25821 bytes)
```

**Step 3: Open the Web Client**

Navigate to one of the following URLs in your web browser:

- **H.264 Client**: `http://localhost:8080/`
- **MJPEG Client**: `http://localhost:8080/mjpeg`

Click the **Connect** button to start receiving the video stream. The status indicator will turn green when connected, and you'll see real-time statistics updating.

### Using with Real ESP32 Hardware

For AtomS3R-CAM, see the `firmware/` folder in this directory for an ESP-IDF implementation with `esp_console` configuration.

When you're ready to use a real ESP32 M5Stack camera, configure your device to connect to the camera WebSocket endpoint:

```
ws://YOUR_SERVER_IP:8765
```

Your ESP32 firmware should send each captured JPEG frame as a binary WebSocket message. The server will handle everything else automatically, including transcoding and distribution to viewers.

## File Structure

```
esp32-camera-stream/
├── server.py              # Main streaming server
├── mock_camera.py         # Mock ESP32 camera simulator
├── client.html            # H.264 web client
├── client_mjpeg.html      # MJPEG web client
├── ARCHITECTURE.md        # Detailed system architecture
├── STATUS.md              # Current system status
└── README.md              # This file
```

## Configuration

### Camera Settings

Edit `mock_camera.py` to adjust camera parameters:

```python
WIDTH = 640        # Frame width in pixels
HEIGHT = 480       # Frame height in pixels
FPS = 30           # Frames per second
SERVER_URL = "ws://localhost:8765"  # Server address
```

### Server Settings

The server uses hardcoded ports that can be modified in `server.py`:

- **8765**: Camera WebSocket input
- **8766**: Viewer WebSocket output  
- **8080**: HTTP server for web clients

### FFmpeg Transcoding

The FFmpeg pipeline in `server.py` can be customized for different quality/performance tradeoffs:

```python
'-preset', 'ultrafast',    # Encoding speed (ultrafast, fast, medium, slow)
'-tune', 'zerolatency',    # Optimization (zerolatency, film, animation)
'-r', '30',                # Output frame rate
'-g', '60',                # GOP size (keyframe interval)
```

## Performance

The system has been tested with the following performance characteristics:

- **Latency**: Typically under 100ms from camera to viewer
- **Bitrate**: Approximately 1000-1500 kbps for 640x480 @ 30fps
- **CPU Usage**: Moderate (depends on FFmpeg preset and frame rate)
- **Concurrent Viewers**: Supports multiple simultaneous viewers with minimal overhead

## Troubleshooting

### Camera Won't Connect

Verify the server is running and listening on port 8765. Check firewall settings if connecting from a different machine.

### No Video in Browser

Try the MJPEG client first (`/mjpeg`) as it has better browser compatibility. Check the browser console for JavaScript errors. Ensure your browser supports the required APIs (WebSocket, Canvas, or MediaSource Extensions).

### High Latency

Reduce the FFmpeg GOP size (`-g` parameter) for more frequent keyframes. Use a faster FFmpeg preset (e.g., `ultrafast`). Check network bandwidth between components.

### FFmpeg Errors

Verify FFmpeg is installed and accessible in PATH. Check FFmpeg stderr output in server logs. Ensure input JPEG frames are valid.

## API Endpoints

### Status Endpoint

```
GET http://localhost:8080/status
```

Returns JSON with current system status:

```json
{
  "camera_clients": 1,
  "viewer_clients": 2,
  "streaming": true
}
```

## Development

The system is designed to be modular and extensible. Key extension points include:

- **Camera Sources**: Replace `mock_camera.py` with real hardware or other video sources
- **Transcoding Options**: Modify FFmpeg parameters for different codecs or quality settings
- **Client Features**: Extend web clients with recording, snapshots, or controls
- **Authentication**: Add WebSocket authentication for production deployments
- **Scaling**: Deploy multiple server instances with load balancing

## License

This project is provided as-is for educational and development purposes.

## Next Steps

Once you have verified the system works with the mock camera, you can proceed to integrate your real ESP32 M5Stack hardware. The mock camera demonstrates the exact protocol your ESP32 firmware should implement: connect to the WebSocket endpoint and send JPEG frames as binary messages at your desired frame rate.
