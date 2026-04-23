#!/bin/bash
# Print to ATOMS3R via MQTT
# Created: 2026-04-22

BROKER="mqtt.m5stack.com"
PORT="1883"
TOPIC="14:08:08:53:87:00"

case "$1" in
    text)
        TEXT="${2:-Hello from computer!}"
        mosquitto_pub -h "$BROKER" -p "$PORT" -t "$TOPIC" -m "TEXT,10,1:$TEXT"
        echo "Sent text: $TEXT"
        ;;
    qr)
        TEXT="${2:-https://m5stack.com}"
        mosquitto_pub -h "$BROKER" -p "$PORT" -t "$TOPIC" -m "QR:$TEXT"
        echo "Sent QR: $TEXT"
        ;;
    barcode)
        TEXT="${2:-1234567890}"
        mosquitto_pub -h "$BROKER" -p "$PORT" -t "$TOPIC" -m "BAR:$TEXT"
        echo "Sent barcode: $TEXT"
        ;;
    *)
        echo "Usage: $0 {text|qr|barcode} [message]"
        echo ""
        echo "Examples:"
        echo "  $0 text 'Hello World!'"
        echo "  $0 qr 'https://example.com'"
        echo "  $0 barcode 'ABC123'"
        ;;
esac
