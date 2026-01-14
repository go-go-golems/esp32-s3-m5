# Quick Start Guide

Get the ESP32 camera streaming system running in under 5 minutes.

## Prerequisites

```bash
# Install FFmpeg
sudo apt-get update && sudo apt-get install -y ffmpeg

# Install Python dependencies
sudo pip3 install websockets aiohttp pillow
```

## Run the System

### Terminal 1: Start Server

```bash
cd esp32-camera-stream
python3 server.py
```

Wait for: `Server is ready!`

### Terminal 2: Start Mock Camera

```bash
cd esp32-camera-stream
python3 mock_camera.py
```

Wait for: `Connected to server!`

### Browser: View Stream

Open in your web browser:

```
http://localhost:8080/mjpeg
```

Click **Connect** button.

## What You Should See

- **Status**: Green "Connected" indicator
- **FPS**: ~30 frames per second
- **Video**: Animated gradient background with moving circle and square
- **Frame counter**: Incrementing continuously

## For Real ESP32 Device

Configure your ESP32 to connect to:

```
ws://YOUR_SERVER_IP:8765
```

Send JPEG frames as binary WebSocket messages.

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Server won't start | Check if ports 8080, 8765, 8766 are available |
| Camera won't connect | Ensure server is running first |
| No video in browser | Try the MJPEG client at `/mjpeg` |
| High CPU usage | Reduce FPS in `mock_camera.py` |

## Next Steps

- Read `README.md` for detailed documentation
- Check `ARCHITECTURE.md` for system design
- Modify `mock_camera.py` to customize animation
- Configure `server.py` for production deployment
