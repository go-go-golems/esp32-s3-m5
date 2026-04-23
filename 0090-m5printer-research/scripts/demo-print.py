#!/usr/bin/env python3
"""
Demo script to print to M5Stack ATOM Printer via MQTT.

Usage:
    python demo-print.py [--mac MAC] [--broker BROKER]
    
Example:
    python demo-print.py --mac 14:08:08:53:87:01
"""

import argparse
import paho.mqtt.client as mqtt
import time


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"✓ Connected to MQTT broker")
    else:
        print(f"✗ Connection failed with code {rc}")

def on_disconnect(client, userdata, rc):
    print(f"! Disconnected (code {rc})")

def demo_print(mac: str, broker: str = "mqtt.m5stack.com", port: int = 1883):
    """Print a demo receipt."""
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect

    print(f"Connecting to {broker}:{port}...")
    client.connect(broker, port, 60)
    client.loop_start()
    time.sleep(1)  # Wait for connection

    print(f"Printing demo to {mac}...")

    # Header
    client.publish(mac, "TEXT,0,2:DEMO RECEIPT")
    time.sleep(0.2)
    client.publish(mac, "TEXT,0,1:================")
    time.sleep(0.2)
    client.publish(mac, "TEXT,0,1:ATOM Printer Test")
    time.sleep(0.2)
    client.publish(mac, "TEXT,0,1:----------------")
    time.sleep(0.2)

    # Demo items
    items = [
        ("Widget A", "5.00"),
        ("Widget B", "12.50"),
        ("Service Fee", "3.99"),
    ]
    for item, price in items:
        line = f"{item:<14} ${price}"
        client.publish(mac, f"TEXT,0,1:{line}")
        time.sleep(0.1)

    # Total
    client.publish(mac, "TEXT,0,1:----------------")
    time.sleep(0.2)
    client.publish(mac, "TEXT,0,1:TOTAL:     $21.49")
    time.sleep(0.2)

    # QR code
    client.publish(mac, "TEXT,0,1:")
    time.sleep(0.2)
    client.publish(mac, "QR:https://m5stack.com")
    time.sleep(0.3)

    # Barcode
    client.publish(mac, "TEXT,0,1:Order #12345")
    time.sleep(0.2)
    client.publish(mac, "BAR:1234512345")
    time.sleep(0.5)

    client.loop_stop()
    client.disconnect()
    print("✓ Demo printed!")


def main():
    parser = argparse.ArgumentParser(description="Print demo to ATOM Printer")
    parser.add_argument("--mac", "-m", default="14:08:08:53:87:01",
                        help="Printer MAC address (default: 14:08:08:53:87:01)")
    parser.add_argument("--broker", "-b", default="mqtt.m5stack.com",
                        help="MQTT broker (default: mqtt.m5stack.com)")
    parser.add_argument("--port", "-p", type=int, default=1883,
                        help="MQTT port (default: 1883)")
    args = parser.parse_args()

    demo_print(args.mac, args.broker, args.port)


if __name__ == "__main__":
    main()
