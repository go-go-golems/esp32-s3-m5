#!/usr/bin/env bash
set -euo pipefail

# Source ESP-IDF
source /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/.envrc 2>/dev/null || {
    echo "ERROR: Failed to source .envrc. Set up ESP-IDF first."
    exit 1
}

PORT="${1:-/dev/ttyACM0}"

case "${2:-build}" in
    build)
        idf.py build
        ;;
    flash)
        idf.py -p "$PORT" flash
        ;;
    monitor)
        idf.py -p "$PORT" monitor
        ;;
    flash-monitor|tmux-flash-monitor)
        idf.py -p "$PORT" flash monitor
        ;;
    erase-flash)
        idf.py -p "$PORT" erase-flash
        ;;
    *)
        echo "Usage: $0 [PORT] {build|flash|monitor|flash-monitor|erase-flash}"
        exit 1
        ;;
esac
