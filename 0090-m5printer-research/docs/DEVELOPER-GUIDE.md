---
title: "M5Stack ATOM Printer - Developer & User Guide"
aliases:
  - ATOM Printer Guide
  - Thermal Printer WiFi Guide
tags:
  - article
  - guide
  - m5stack
  - thermal-printer
  - iot
  - esp32
  - mqtt
  - http
created: 2026-04-22
repo: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0090-m5printer-research
status: active
type: article
---

# M5Stack ATOM Printer - Developer & User Guide

A complete guide to using the M5Stack ATOM Thermal Printer Kit (K118) over WiFi. Covers both MQTT and HTTP APIs with working code examples in Python, JavaScript, Arduino, and curl.

---

## Quick Reference

| Item | Details |
|------|---------|
| **Device** | M5Stack ATOM Thermal Printer Kit (SKU: K118) |
| **Controller** | ATOM Lite (ESP32-PICO-D4, 4MB Flash) |
| **Print width** | 58mm (384 dots) |
| **Power** | DC 12V, 2.5A (not included) |
| **Default AP** | SSID: `ATOM_PRINTER-xxxx`, IP: `192.168.4.1` |
| **Default MQTT Broker** | `mqtt.m5stack.com:1883` |
| **Serial** | 9600bps 8N1 (G23=TX, G33=RX, G19=CTS) |

---

## Table of Contents

1. [Initial Setup](#1-initial-setup)
2. [Method 1: MQTT Printing](#2-method-1-mqtt-printing)
3. [Method 2: HTTP/Web Printing](#3-method-2-httpweb-printing)
4. [Method 3: Serial UART](#4-method-3-serial-uart)
5. [Programming with Arduino](#5-programming-with-arduino)
6. [Python Examples](#6-python-examples)
7. [Image & Dithering](#7-image--dithering)
8. [Troubleshooting](#8-troubleshooting)

---

## 1. Initial Setup

### 1.1 Power On

1. Connect a **12V DC power adapter** (2.5A, 5.5mm barrel jack)
2. The ATOM LED will blink
3. The printer broadcasts a WiFi AP: `ATOM_PRINTER-xxxx`

### 1.2 Configure WiFi (to use MQTT)

1. Connect your phone/computer to `ATOM_PRINTER-xxxx`
2. Open browser → `http://192.168.4.1`
3. Scroll to **WiFi Configuration** section
4. Enter your WiFi SSID and password
5. Click **Connect to WiFi**
6. The printer reboots and connects to your network
7. LED turns solid **green** = WiFi connected

### 1.3 Find the Printer's MAC Address

After connecting to WiFi, the MAC address is shown on the web interface status page (`http://192.168.4.1`).

Format: `XX:XX:XX:XX:XX:XX` (e.g., `A4:CF:12:DE:8E:7F`)

---

## 2. Method 1: MQTT Printing

Once configured with WiFi, the printer connects to `mqtt.m5stack.com:1883` and subscribes to its own MAC address as the MQTT topic.

### 2.1 MQTT Topic

```
Subscribe/Publish Topic: <device MAC address>
```

Example:
```
A4:CF:12:DE:8E:7F
```

### 2.2 MQTT Payload Formats

#### Text
```
TEXT,<position>,<size>:<content>
```

| Field | Description | Example |
|-------|-------------|---------|
| `TEXT` | Command prefix | `TEXT` |
| `<position>` | Horizontal position in dots | `10` |
| `<size>` | Font size (1=normal, 2=double) | `1` |
| `<content>` | Text to print (after `:`) | `Hello World` |

**Examples:**
```
TEXT,10,1:Hello World
TEXT,0,2:Big Text
BAR:123456789
QR:https://example.com
```

#### Barcode
```
BAR:<barcode content>
```

Supported types: CODE128 (default), CODE39, CODE93, EAN13, EAN8, UPC-A, UPC-E, ITF25, CODABAR

#### QR Code
```
QR:<content>
```

### 2.3 Python MQTT Example

```python
# pip install paho-mqtt

import paho.mqtt.client as mqtt

# Replace with your printer's MAC address
MAC = "A4:CF:12:DE:8E:7F"
BROKER = "mqtt.m5stack.com"
PORT = 1883

client = mqtt.Client()

def on_connect(client, userdata, flags, rc):
    print(f"Connected with result code {rc}")
    
def on_disconnect(client, userdata, rc):
    print("Disconnected")

client.on_connect = on_connect
client.on_disconnect = on_disconnect

client.connect(BROKER, PORT, 60)

# Ensure connection is established
client.loop_start()
import time
time.sleep(1)

# Print text
client.publish(MAC, "TEXT,10,1:Hello from Python!")
time.sleep(0.5)

# Print barcode
client.publish(MAC, "BAR:123456789")
time.sleep(0.5)

# Print QR code
client.publish(MAC, "QR:https://example.com")
time.sleep(0.5)

# Print with formatting
client.publish(MAC, "TEXT,0,2:BIG RECEIPT")
time.sleep(0.5)
client.publish(MAC, "TEXT,0,1:Small text below")

client.loop_stop()
client.disconnect()
print("Done!")
```

### 2.4 JavaScript/Node.js MQTT Example

```javascript
// npm install mqtt

const mqtt = require('mqtt');

const MAC = 'A4:CF:12:DE:8E:7F';
const BROKER = 'mqtt.m5stack.com';
const PORT = 1883;

const client = mqtt.connect(`mqtt://${BROKER}:${PORT}`);

client.on('connect', () => {
    console.log('Connected to MQTT broker');
    
    // Print text
    client.publish(MAC, 'TEXT,10,1:Hello from Node.js!');
    
    // Print barcode
    client.publish(MAC, 'BAR:987654321');
    
    // Print QR code
    client.publish(MAC, 'QR:https://m5stack.com');
    
    setTimeout(() => {
        client.end();
        console.log('Done!');
    }, 1000);
});

client.on('error', (err) => {
    console.error('MQTT Error:', err);
});
```

### 2.5 Custom MQTT Broker Setup

You can configure the printer to use your own MQTT broker:

1. Go to `http://192.168.4.1` (while connected to printer AP)
2. Or send HTTP POST to `/mqtt_config` with:
```json
{
    "mqtt_broker": "your-broker.com",
    "mqtt_port": 1883,
    "mqtt_id": "your-client-id",
    "mqtt_user": "username",
    "mqtt_password": "password",
    "mqtt_topic_info": "custom-topic"
}
```

---

## 3. Method 2: HTTP/Web Printing

The printer runs a web server with several API endpoints.

### 3.1 Web Interface

**AP Mode:** Connect to printer's AP → `http://192.168.4.1`

**WiFi Mode:** After configuring WiFi, the printer's IP is shown in the status page. Access via `http://<printer-ip>`.

The web interface provides:
- ASCII text printing
- QR code printing
- Barcode printing
- WiFi configuration
- MQTT configuration
- Device status

### 3.2 HTTP API Endpoints

#### GET `/print` - Print Text/QR/Barcode

```
GET /print?printType=<type>&<field>=<value>
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `printType` | `ASCII`, `QRCode`, `BarCode` | What to print |
| `Pdata` | string | Text content (for ASCII) |
| `QRCode` | string | QR code content |
| `BarCode` | string | Barcode content |
| `newLine` | `on`/`off` | Add newline after |

**Examples:**

```bash
# Print text
curl "http://192.168.4.1/print?printType=ASCII&Pdata=Hello%20World&newLine=on"

# Print QR code
curl "http://192.168.4.1/print?printType=QRCode&QRCode=https://example.com&newLine=on"

# Print barcode
curl "http://192.168.4.1/print?printType=BarCode&BarCode=123456789&newLine=on"
```

#### POST `/wifi_config` - Configure WiFi

```bash
curl -X POST "http://192.168.4.1/wifi_config" \
  -H "Content-Type: application/json" \
  -d '{"ssid":"YourWiFi","password":"YourPassword"}'
```

#### GET `/device_status` - Get Status

```bash
curl "http://192.168.4.1/device_status"
```

Response:
```json
{
    "WIFI_STATE": true,
    "SSID": "YourWiFi",
    "IP": "192.168.1.100",
    "RSSI": -45,
    "MQTT_STATE": "Connected",
    "MQTT_BROKER": "mqtt.m5stack.com",
    "MQTT_TOPIC": "A4:CF:12:DE:8E:7F",
    "WIFI_HTML": "..."
}
```

#### POST `/bmp_size` - Set BMP Dimensions

```bash
curl -X POST "http://192.168.4.1/bmp_size" \
  -H "Content-Type: application/json" \
  -d '{"bmp_width":384,"bmp_height":200}'
```

#### POST `/bmp` - Upload BMP Image

```bash
curl -X POST "http://192.168.4.1/bmp" \
  -H "Content-Type: application/octet-stream" \
  --data-binary "@image.bin"
```

### 3.3 JavaScript Browser Example

```javascript
// Print from any web page
async function printText(text) {
    const response = await fetch(
        `http://192.168.4.1/print?printType=ASCII&Pdata=${encodeURIComponent(text)}&newLine=on`
    );
    return response.text();
}

async function printQR(url) {
    const response = await fetch(
        `http://192.168.4.1/print?printType=QRCode&QRCode=${encodeURIComponent(url)}&newLine=on`
    );
    return response.text();
}

async function getStatus() {
    const response = await fetch('http://192.168.4.1/device_status');
    return await response.json();
}

// Usage
printText('Hello from browser!');
printQR('https://example.com').then(() => console.log('QR printed!'));
```

---

## 4. Method 3: Serial UART

Connect via serial for direct control.

### 4.1 Pinout

| ATOM Pin | Function | Printer Connection |
|----------|----------|-------------------|
| G23 | TX | RX |
| G33 | RX | TX |
| G19 | CTS | RTS (optional) |
| GND | Ground | Ground |

### 4.2 Serial Settings

- **Baud rate:** 9600 bps
- **Data bits:** 8
- **Parity:** None (N)
- **Stop bits:** 1

### 4.3 Serial Protocol Commands

From `ATOM_PRINTER_CMD_v1.06.pdf`:

#### Initialization
```
0x1B, 0x40
```

#### Text Formatting

| Function | Command |
|----------|---------|
| Set bold on | `0x1B, 0x47, 0x01` |
| Set bold off | `0x1B, 0x47, 0x00` |
| Set underline on | `0x1B, 0x2D, 0x01` |
| Set underline off | `0x1B, 0x2D, 0x00` |
| Set character size | `0x1D, 0x21, ((X&0x0f)<<4) \| (Y&0x0f)` |
| New line | `0x0A` |

#### Print BMP
```
0x1D, 0x76, 0x30, 
<width_bytes_lo>, <width_bytes_hi>,
<height_lo>, <height_hi>,
<image_data...>
```

Where `width_bytes = width / 8` (e.g., 384 dots = 48 bytes)

### 4.4 Arduino Serial Example

```cpp
#include <M5Atom.h>

// Using HardwareSerial for ATOM
#define TX_PIN 23
#define RX_PIN 33

void setup() {
    Serial.begin(9600);  // For debugging
    Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
}

void loop() {
    // Initialize printer
    Serial2.write(0x1B); Serial2.write(0x40);
    delay(100);
    
    // Print text
    Serial2.print("Hello from Serial!");
    Serial2.write(0x0A);  // newline
    delay(100);
    
    // Print barcode (CODE128)
    Serial2.write(0x1D); Serial2.write(0x45); Serial2.write(0x43); Serial2.write(0x01); // HRI above
    Serial2.write(0x1D); Serial2.write(0x6B); Serial2.write(0x49); // CODE128
    Serial2.print("123456789");
    Serial2.write(0x00);  // NUL terminator
    delay(100);
    
    // Print QR code
    Serial2.write(0x1D); Serial2.write(0x28); Serial2.write(0x6B); Serial2.write(0x03);
    Serial2.write(0x00); Serial2.write(0x31); Serial2.write(0x45); Serial2.write(0x51); // Level H
    // ... QR data follows
    delay(2000);
}
```

---

## 5. Programming with Arduino

### 5.1 Install Required Libraries

In Arduino Library Manager, install:
- **M5Atom** by M5Stack
- **PubSubClient** by Nick O'Leary
- **ArduinoJson** by Benoit Blanchon

### 5.2 Basic Arduino Example

```cpp
#include <M5Atom.h>
#include <WiFi.h>
#include <PubSubClient.h>

// WiFi credentials
const char* ssid = "YourWiFi";
const char* password = "YourPassword";

// MQTT broker
const char* mqtt_broker = "mqtt.m5stack.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "A4:CF:12:DE:8E:7F";  // Your MAC

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void callback(char* topic, byte* payload, unsigned int len) {
    // Handle incoming MQTT messages
    char message[len + 1];
    strncpy(message, (char*)payload, len);
    message[len] = '\0';
    
    Serial.println(message);
    
    // Parse commands
    String msg = String(message);
    if (msg.startsWith("TEXT")) {
        // Parse TEXT,<pos>,<size>:<content>
        // and print to printer
    }
}

void setup() {
    M5.begin(true, false, true);
    Serial.begin(115200);
    
    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    
    // Setup MQTT
    mqttClient.setServer(mqtt_broker, mqtt_port);
    mqttClient.setCallback(callback);
    
    // Connect to MQTT
    while (!mqttClient.connected()) {
        if (mqttClient.connect("ATOM_PRINTER")) {
            mqttClient.subscribe(mqtt_topic);
            Serial.println("MQTT connected");
        } else {
            delay(1000);
        }
    }
}

void loop() {
    mqttClient.loop();
    M5.update();
}
```

---

## 6. Python Examples

### 6.1 Pure HTTP (No MQTT Dependencies)

```python
#!/usr/bin/env python3
"""Simple HTTP-based printing to ATOM Printer."""

import urllib.request
import urllib.parse

# Printer IP (default in AP mode)
PRINTER_IP = "192.168.4.1"

def print_text(text, newline=True):
    """Print ASCII text."""
    params = urllib.parse.urlencode({
        "printType": "ASCII",
        "Pdata": text,
        "newLine": "on" if newline else "off"
    })
    url = f"http://{PRINTER_IP}/print?{params}"
    with urllib.request.urlopen(url) as response:
        return response.read()

def print_qr(content):
    """Print QR code."""
    params = urllib.parse.urlencode({
        "printType": "QRCode",
        "QRCode": content,
        "newLine": "on"
    })
    url = f"http://{PRINTER_IP}/print?{params}"
    with urllib.request.urlopen(url) as response:
        return response.read()

def print_barcode(data):
    """Print barcode."""
    params = urllib.parse.urlencode({
        "printType": "BarCode",
        "BarCode": data,
        "newLine": "on"
    })
    url = f"http://{PRINTER_IP}/print?{params}"
    with urllib.request.urlopen(url) as response:
        return response.read()

def get_status():
    """Get printer status."""
    url = f"http://{PRINTER_IP}/device_status"
    with urllib.request.urlopen(url) as response:
        import json
        return json.loads(response.read())

# Usage
if __name__ == "__main__":
    print("Getting status...")
    status = get_status()
    print(f"WiFi: {status['WIFI_STATE']}")
    print(f"MQTT: {status['MQTT_STATE']}")
    
    print("\nPrinting...")
    print_text("Hello from Python!")
    print_text("=" * 20)
    print_barcode("123456789")
    print_qr("https://example.com")
    print("Done!")
```

### 6.2 MQTT with Custom Receipt Format

```python
#!/usr/bin/env python3
"""Print a formatted receipt via MQTT."""

import paho.mqtt.client as mqtt
import time
import json

# Configuration
MAC = "A4:CF:12:DE:8E:7F"
BROKER = "mqtt.m5stack.com"
PORT = 1883

def print_receipt(items, total):
    """Print a formatted receipt."""
    client = mqtt.Client()
    client.connect(BROKER, PORT, 60)
    client.loop_start()
    time.sleep(0.5)
    
    # Header
    client.publish(MAC, "TEXT,0,2:RECEIPT")
    time.sleep(0.2)
    client.publish(MAC, "TEXT,0,1:================")
    time.sleep(0.2)
    
    # Items
    for item, price in items:
        line = f"{item:<12} ${price:.2f}"
        client.publish(MAC, f"TEXT,0,1:{line}")
        time.sleep(0.1)
    
    # Total
    client.publish(MAC, "TEXT,0,1:----------------")
    time.sleep(0.2)
    client.publish(MAC, f"TEXT,0,1:TOTAL: ${total:.2f}")
    time.sleep(0.2)
    
    # Barcode
    client.publish(MAC, "BAR:123456789")
    time.sleep(0.2)
    
    # QR code for digital receipt
    client.publish(MAC, "QR:https://example.com/receipt/123")
    
    time.sleep(0.5)
    client.loop_stop()
    client.disconnect()

# Usage
if __name__ == "__main__":
    items = [
        ("Coffee", 4.50),
        ("Sandwich", 8.99),
        ("Cookie", 2.50),
    ]
    total = sum(price for _, price in items)
    print_receipt(items, total)
    print("Receipt printed!")
```

---

## 7. Image & Dithering

### 7.1 BMP Image Requirements

| Property | Value |
|----------|-------|
| Width | 384 pixels (58mm × 8 dots/mm) |
| Height | Variable |
| Format | 1-bit monochrome |
| Bytes per row | 48 bytes (384 / 8) |

### 7.2 Image Processing Pipeline

1. Load image (any format)
2. Resize to 384 pixels wide
3. Convert to 1-bit (dithering or threshold)
4. Pack bits into bytes (MSB first)
5. Send via HTTP BMP upload

### 7.3 Python Image Printing

```python
#!/usr/bin/env python3
"""Convert and print images to ATOM Printer."""

from PIL import Image
import urllib.request
import io

PRINTER_IP = "192.168.4.1"
PRINTER_WIDTH = 384  # pixels

def image_to_bmp_data(image):
    """Convert PIL Image to printer-ready BMP data."""
    # Resize to printer width
    aspect = image.height / image.width
    height = int(PRINTER_WIDTH * aspect)
    image = image.resize((PRINTER_WIDTH, height), Image.LANCZOS)
    
    # Convert to 1-bit with Floyd-Steinberg dithering
    image = image.convert('1', dither=Image.FLOYDSTEINBERG)
    
    # Pack pixels into bytes
    pixels = list(image.getdata())
    width_bytes = PRINTER_WIDTH // 8
    bmp_data = bytearray()
    
    for row in range(image.height):
        for byte_idx in range(width_bytes):
            byte = 0
            for bit in range(8):
                pixel_idx = row * image.width + byte_idx * 8 + bit
                if pixel_idx < len(pixels) and pixels[pixel_idx] > 0:
                    byte |= (1 << (7 - bit))
            bmp_data.append(byte)
    
    return bytes(bmp_data), image.width, image.height

def print_image(image_path):
    """Print an image file."""
    # Load and convert image
    with Image.open(image_path) as img:
        bmp_data, width, height = image_to_bmp_data(img)
    
    # Convert width to bytes
    width_bytes = width // 8
    
    # Set BMP size
    size_url = f"http://{PRINTER_IP}/bmp_size"
    size_data = f'{{"bmp_width":{width},"bmp_height":{height}}}'.encode()
    req = urllib.request.Request(size_url, data=size_data, method='POST')
    req.add_header('Content-Type', 'application/json')
    with urllib.request.urlopen(req) as response:
        print(f"Size set: {response.read()}")
    
    # Upload BMP
    bmp_url = f"http://{PRINTER_IP}/bmp"
    req = urllib.request.Request(bmp_url, data=bmp_data, method='POST')
    req.add_header('Content-Type', 'application/octet-stream')
    with urllib.request.urlopen(req) as response:
        print(f"Image printed: {response.read()}")

# Usage
if __name__ == "__main__":
    print_image("logo.png")
```

### 7.4 Atkinson Dithering (Better Quality)

See the [MaxBittker/dithering](https://github.com/MaxBittker/dithering) repo for Atkinson dithering implementation which produces sharper results on thermal printers.

```python
def atkinson_dithering(image):
    """Atkinson dithering - produces sharper results."""
    img = image.convert('L')  # Grayscale
    pixels = img.load()
    width, height = img.size
    
    for y in range(height - 1):
        for x in range(1, width - 1):
            old_pixel = pixels[x, y]
            new_pixel = 255 if old_pixel > 127 else 0
            pixels[x, y] = new_pixel
            
            error = (old_pixel - new_pixel) // 8
            
            # Distribute error to neighboring pixels
            pixels[x + 1, y] = min(255, max(0, pixels[x + 1, y] + error))
            pixels[x + 2, y] = min(255, max(0, pixels[x + 2, y] + error))
            pixels[x - 1, y + 1] = min(255, max(0, pixels[x - 1, y + 1] + error))
            pixels[x, y + 1] = min(255, max(0, pixels[x, y + 1] + error))
            pixels[x + 1, y + 1] = min(255, max(0, pixels[x + 1, y + 1] + error))
            pixels[x, y + 2] = min(255, max(0, pixels[x, y + 2] + error))
    
    return img
```

---

## 8. Troubleshooting

### LED Status Meanings

| LED Color | Status |
|----------|--------|
| Blinking Green | Initializing |
| Solid Green | WiFi Connected |
| Blinking Red | WiFi Disconnected |
| Solid Blue | MQTT Connected |
| Blinking Blue | MQTT Disconnected |

### Common Issues

#### Printer doesn't respond to MQTT
1. Check WiFi is connected (LED should be green)
2. Verify MAC address is correct
3. Check MQTT state (LED should be blue)
4. Try restarting: hold button for 5 seconds

#### Can't connect to printer AP
1. Power cycle the printer
2. Wait for LED to blink, then connect
3. Default AP auto-closes after a few minutes if no connection

#### Print quality is poor
1. Ensure 12V power supply is providing 2.5A
2. Use high-quality thermal paper
3. Clean the print head with isopropyl alcohol

#### HTTP API not responding
1. Ensure you're connected to the correct network
2. In AP mode, connect to printer's WiFi first
3. Check firewall settings

### Reset to Factory Defaults

Hold the ATOM button for **5 seconds** until the LED blinks rapidly. This clears all WiFi and MQTT settings.

### Finding the MAC Address

1. Check the web interface at `http://192.168.4.1` (status section)
2. Check your router's DHCP client list
3. The MAC is also printed in the serial output at startup

---

## Reference Links

- **Official Docs**: https://docs.m5stack.com/en/atom/atom_printer
- **Firmware Repo**: https://github.com/m5stack/ATOM-PRINTER
- **Arduino Library**: https://github.com/m5stack/M5Atom
- **python-escpos**: https://github.com/python-escpos/python-escpos
- **Dithering**: https://github.com/MaxBittker/dithering
- **Command Spec PDF**: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/atombase/atom_pritner/ATOM_PRINTER_CMD_v1.06.pdf

---

*Last updated: 2026-04-22*
*Source: Reverse-engineered from https://github.com/m5stack/ATOM-PRINTER*
