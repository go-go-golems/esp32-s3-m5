#!/bin/bash

# ESP32 Camera Streaming System - Startup Script
# This script starts both the Go server and mock camera

set -e

cd "$(dirname "$0")"

echo "==================================="
echo "ESP32 Camera Streaming System"
echo "==================================="
echo ""

# Check if server binary exists
if [ ! -f "./server-go" ]; then
    echo "Error: server-go binary not found!"
    echo "Please build it first: go build -o server-go server.go"
    exit 1
fi

# Check if FFmpeg is installed
if ! command -v ffmpeg &> /dev/null; then
    echo "Error: FFmpeg is not installed!"
    echo "Please install it: sudo apt-get install ffmpeg"
    exit 1
fi

# Check Python dependencies
if ! python3 -c "import websockets, PIL" 2>/dev/null; then
    echo "Error: Python dependencies not installed!"
    echo "Please install them: sudo pip3 install websockets pillow"
    exit 1
fi

# Kill any existing processes
echo "Cleaning up existing processes..."
pkill -f "server-go" || true
pkill -f "mock_camera.py" || true
sleep 1

# Start the Go server
echo "Starting Go server..."
./server-go > server-go.log 2>&1 &
SERVER_PID=$!
echo "Server PID: $SERVER_PID"
sleep 2

# Check if server started successfully
if ! ps -p $SERVER_PID > /dev/null; then
    echo "Error: Server failed to start!"
    echo "Check server-go.log for details"
    exit 1
fi

# Start the mock camera
echo "Starting mock camera..."
python3 mock_camera.py > camera.log 2>&1 &
CAMERA_PID=$!
echo "Camera PID: $CAMERA_PID"
sleep 2

# Check if camera started successfully
if ! ps -p $CAMERA_PID > /dev/null; then
    echo "Error: Camera failed to start!"
    echo "Check camera.log for details"
    kill $SERVER_PID
    exit 1
fi

echo ""
echo "==================================="
echo "System is running!"
echo "==================================="
echo ""
echo "Server PID: $SERVER_PID"
echo "Camera PID: $CAMERA_PID"
echo ""
echo "Web interface: http://localhost:8080/mjpeg"
echo "Status API: http://localhost:8080/status"
echo ""
echo "Logs:"
echo "  Server: tail -f server-go.log"
echo "  Camera: tail -f camera.log"
echo ""
echo "To stop:"
echo "  kill $SERVER_PID $CAMERA_PID"
echo ""
echo "Press Ctrl+C to stop all processes..."

# Wait for interrupt
trap "echo 'Stopping...'; kill $SERVER_PID $CAMERA_PID 2>/dev/null; exit 0" INT TERM

# Keep script running
while true; do
    sleep 1
    # Check if processes are still running
    if ! ps -p $SERVER_PID > /dev/null; then
        echo "Server process died!"
        kill $CAMERA_PID 2>/dev/null
        exit 1
    fi
    if ! ps -p $CAMERA_PID > /dev/null; then
        echo "Camera process died!"
        kill $SERVER_PID 2>/dev/null
        exit 1
    fi
done
