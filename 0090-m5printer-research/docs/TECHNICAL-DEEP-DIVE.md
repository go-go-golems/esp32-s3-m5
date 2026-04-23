---
title: "ATOM-PRINTER Firmware - Technical Deep Dive"
tags:
  - article
  - technical
  - deep-dive
  - firmware
  - m5stack
  - thermal-printer
  - esp32
  - iot
  - reverse-engineering
created: 2026-04-22
repo: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0090-m5printer-research
status: active
type: article
---

# ATOM-PRINTER Firmware - Technical Deep Dive

A complete reverse-engineering analysis of the M5Stack ATOM Printer firmware architecture, communication protocols, and implementation details. This document covers the full stack: ESP32 firmware, thermal printer protocol, MQTT integration, HTTP web server, and WiFi connectivity.

> [!info]
> This deep dive was created by reading and analyzing the actual source code at `github.com/m5stack/ATOM-PRINTER`. All information comes from reverse-engineering the firmware source.

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [Repository Structure](#2-repository-structure)
3. [The ATOM_PRINTER Library](#3-the-atom_printer-library)
4. [Thermal Printer Protocol](#4-thermal-printer-protocol)
5. [WiFi Module](#5-wifi-module)
6. [MQTT Module](#6-mqtt-module)
7. [HTTP Web Server](#7-http-web-server)
8. [Main Firmware Loop](#8-main-firmware-loop)
9. [State Machine](#9-state-machine)
10. [Data Flow Diagrams](#10-data-flow-diagrams)
11. [Key Implementation Patterns](#11-key-implementation-patterns)
12. [Security Considerations](#12-security-considerations)
13. [How to Write Your Own Firmware](#13-how-to-write-your-own-firmware)

---

## 1. Architecture Overview

The ATOM-PRINTER firmware implements a multi-protocol IoT thermal printer with three communication channels:

```
┌─────────────────────────────────────────────────────────────┐
│                     ESP32 (ATOM Lite)                      │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐   │
│  │  WiFi STA    │  │  WiFi AP     │  │  MQTT Client │   │
│  │  (connects   │  │  (broadcasts │  │  (connects   │   │
│  │  to router)  │  │  for config) │  │  to broker)  │   │
│  └───────┬──────┘  └───────┬──────┘  └───────┬──────┘   │
│          │                 │                 │             │
│          └────────────┬────┴────────────────┘             │
│                       │                                   │
│              ┌────────▼────────┐                         │
│              │   Web Server     │                         │
│              │   (HTTP API)     │                         │
│              └────────┬────────┘                         │
│                       │                                   │
│              ┌────────▼────────┐                         │
│              │  Command Parser  │                         │
│              │  (TEXT/QR/BAR)  │                         │
│              └────────┬────────┘                         │
│                       │                                   │
│              ┌────────▼────────┐                         │
│              │ ATOM_PRINTER.h │                         │
│              │    Library      │                         │
│              └────────┬────────┘                         │
│                       │ Serial (9600 baud)                │
│              ┌────────▼────────┐                         │
│              │ Thermal Printer │                         │
│              │   (58mm 203dpi) │                         │
│              └─────────────────┘                         │
└─────────────────────────────────────────────────────────────┘
```

### Hardware Configuration

| Component | Details |
|-----------|---------|
| **CPU** | ESP32-PICO-D4 (Xtensa dual-core 240MHz) |
| **Flash** | 4MB |
| **RAM** | 520KB SRAM |
| **GPIO** | G23 (TX), G33 (RX), G19 (CTS) |
| **Serial** | HardwareSerial2 at 9600 baud, 8N1 |
| **Power** | 12V DC, 2.5A |

---

## 2. Repository Structure

```
ATOM-PRINTER/
├── src/
│   ├── ATOM_PRINTER.h      # Public API for printer control
│   ├── ATOM_PRINTER.cpp      # Implementation - serial protocol
│   └── ATOM_PRINTER_CMD.h    # Command bytes constants
├── examples/
│   ├── PRINTER_FW/          # Full firmware (WiFi+MQTT+Web)
│   │   ├── PRINTER_FW.ino    # Main sketch entry point
│   │   ├── ATOM_PRINTER_WIFI.cpp/.h
│   │   ├── ATOM_PRINTER_MQTT.cpp/.h
│   │   ├── ATOM_PRINTER_WEB.cpp/.h
│   │   ├── ATOM_PRINTER_CONFIG.h
│   │   └── ATOM_PRINTER_HTML.h  # Embedded web page (HTML/CSS/JS)
│   ├── PRINTER_TEST/         # Simple button-triggered test
│   │   └── PRINTER_TEST.ino
│   └── PRINTER_IMAGE/        # Image printing example
│       └── PRINTER_IMAGE.ino
├── library.json              # Arduino library manifest
└── README.md
```

---

## 3. The ATOM_PRINTER Library

### 3.1 Class Definition

```cpp
// src/ATOM_PRINTER.h

class ATOM_PRINTER {
   private:
    HardwareSerial *_serial;
    bool _debug;
    bool waitMsg(unsigned long timerout = 500);
    void sendCMD(uint8_t *data, size_t size);
    void cleanBuffer();
    uint8_t buffer[256] = {0};

   public:
    void begin(HardwareSerial *serial = &Serial2, int baud = 9600,
               uint8_t RX = 33, uint8_t TX = 23, bool debug = false);
    void init();
    void printPos(uint16_t posx);
    void fontSize(uint8_t font_size);
    void WriteCMD(uint8_t *buff, uint8_t buff_size);
    void newLine(uint8_t count);
    void setBarCodeHRI(BarCodePos_t pos);
    void enableBarCode(bool state);
    void printBarCode(BarCode_t type, String barcode);
    void setQRCodeECL(QRCode_EC_Level_t level);
    void printQRCode(String qrcode);
    void printASCII(String data);
    void printBMP(uint8_t mode, uint16_t xdot, uint16_t ydot, uint8_t *buffer);
};
```

### 3.2 Initialization

```cpp
// src/ATOM_PRINTER.cpp

void ATOM_PRINTER::begin(HardwareSerial *serial, int baud, uint8_t RX, uint8_t TX, bool debug)
{
    _debug  = debug;
    _serial = serial;
    _serial->begin(baud, SERIAL_8N1, RX, TX);  // RX=33, TX=23
}
```

The library uses `HardwareSerial` (Serial2 on ATOM Lite) to communicate with the thermal printer mechanism at 9600 baud, 8 data bits, no parity, 1 stop bit.

### 3.3 Key Methods

#### Initialize Printer
```cpp
void ATOM_PRINTER::init() {
    _serial->write(INIT_PRINTER_CMD, sizeof(INIT_PRINTER_CMD));
    // Sends: 0x1B, 0x40
}
```

#### Set Horizontal Position
```cpp
void ATOM_PRINTER::printPos(uint16_t posx) {
    cleanBuffer();
    memcpy(buffer, PRINT_POS_CMD, sizeof(PRINT_POS_CMD));  // 0x1B, 0x24
    buffer[2] = posx & 0xff;
    buffer[3] = (posx >> 8) & 0xff;
    _serial->write(buffer, 4);
}
```

#### Set Font Size
```cpp
void ATOM_PRINTER::fontSize(uint8_t font_size) {
    if (font_size > 7) font_size = 7;
    memcpy(buffer, FONT_SIZE_CMD, sizeof(FONT_SIZE_CMD));  // 0x1D, 0x21
    buffer[2] = (font_size | (font_size << 4)) & 0xff;
    _serial->write(buffer, 3);
}
```

The font size is encoded as a nibble (4 bits) for width and height: `(width << 4) | height`. Valid values 0-7.

---

## 4. Thermal Printer Protocol

### 4.1 Command Reference

All commands are defined in `ATOM_PRINTER_CMD.h`:

```cpp
// src/ATOM_PRINTER_CMD.h

// Initialization
const uint8_t INIT_PRINTER_CMD[] = {0x1B, 0x40};

// Position
const uint8_t PRINT_POS_CMD[] = {0x1B, 0x24};

// Font Size
const uint8_t FONT_SIZE_CMD[] = {0x1D, 0x21};

// Barcode HRI Position
const uint8_t SET_BAR_CODE_POS_CMD[] = {0x1D, 0x48};

// Enable Barcode
const uint8_t ENABLE_BAR_CODE_MODE_CMD[] = {0x1D, 0x45, 0x43, 0xff};

// Print Barcode
const uint8_t PRINTER_BAR_CODE_CMD[] = {0x1D, 0x6B, 0x41, 0xff};

// QR Code
const uint8_t PRINTER_QRCODE_CMD[] = {0x1D, 0x28, 0x6B, 0x03, 0x00, 0x31, 0x51, 0x30, 0x00};
const uint8_t SET_QRCODE_CMD[] = {0x1D, 0x28, 0x6B, 0xff, 0xff, 0x31, 0x50, 0x30};
const uint8_t SET_QRCODE_ECL_CODE_CMD[] = {0x1D, 0x28, 0x6B, 0x03, 0x00, 0x31, 0x45};

// BMP Print
const uint8_t PRINTER_BMP_CMD[] = {0x1D, 0x76, 0x30, 0xff, 0xff, 0xff, 0xff, 0xff};
```

### 4.2 Barcode Types

```cpp
typedef enum {
    UPC_A = 0x41,     // UPC-A (12 digits)
    UPC_E,             // UPC-E (8 digits)
    JAN13_EAN13,       // EAN-13 (13 digits)
    JAN8_EAN8,         // EAN-8 (8 digits)
    CODE39,            // Code 39
    ITF,               // Interleaved 2 of 5
    CODABAR,           // Codabar
    CODE93,            // Code 93
    CODE128            // Code 128 (default)
} BarCode_t;
```

### 4.3 HRI (Human Readable Interpretation) Position

```cpp
typedef enum { 
    HIDE = 0x00,   // No text below barcode
    ABOVE,         // Text above barcode
    BELOW,         // Text below barcode
    BOTH           // Text above and below
} BarCodePos_t;
```

### 4.4 QR Code Error Correction

```cpp
typedef enum { 
    LEVEL_L = 0x48,  // Low (~7% recovery)
    LEVEL_M,         // Medium (~15%)
    LEVEL_Q,         // Quartile (~25%)
    LEVEL_H          // High (~30%) - DEFAULT
} QRCode_EC_Level_t;
```

### 4.5 BMP Printing

The printer accepts raster bitmaps at 203dpi. For a 58mm printer:

- **Width**: 384 pixels (58mm × 8 dots/mm)
- **Bytes per row**: 48 bytes (384 / 8)
- **Format**: 1-bit monochrome, MSB first

```cpp
void ATOM_PRINTER::printBMP(uint8_t mode, uint16_t xdot, uint16_t ydot, uint8_t *buffer)
{
    if (mode > 3) mode = 3;
    
    // Build command: 0x1D 0x76 0x30 <m> <xL xH> <yL yH>
    uint8_t cmd[] = {0x1D, 0x76, 0x30, mode, 
                     (uint8_t)(xdot/8 & 0x00ff), 
                     (uint8_t)((xdot/8 >> 8) & 0x00ff),
                     (uint8_t)(ydot & 0x00ff), 
                     (uint8_t)((ydot >> 8) & 0x00ff)};
    
    _serial->write(cmd, 8);
    
    // Send pixel data
    uint16_t len = (xdot / 8) * ydot;
    while (len--) {
        _serial->write(*buffer++);
    }
}
```

---

## 5. WiFi Module

### 5.1 Dual-Mode Operation

The firmware operates in `WIFI_AP_STA` mode:

```cpp
// ATOM_PRINTER_WIFI.cpp

void wifiInit() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    
    // Generate AP SSID from MAC address
    String mac_addr = WiFi.softAPmacAddress();
    String last5 = mac_addr.substring(mac_addr.length() - 5, 
                                      mac_addr.length() - 3) +
                   mac_addr.substring(mac_addr.length() - 2);
    String ssid = "ATOM-PRINTER_" + last5;
    
    WiFi.softAP(ssid.c_str());
}
```

**AP Mode**: Broadcasts `ATOM-PRINTER_XXXX` for initial configuration
**STA Mode**: Connects to user's WiFi for MQTT and web access

### 5.2 AP IP Configuration

```cpp
const IPAddress apIP(192, 168, 4, 1);
```

The device uses a captive portal approach:
1. DNS server captures all requests
2. Redirects to the root web page
3. User configures WiFi credentials

### 5.3 WiFi Scan

```cpp
String wifiScan() {
    WiFi.disconnect();
    delay(100);
    int n = WiFi.scanNetworks();
    
    // Deduplicate SSIDs
    String seenSSIDs[n];
    int uniqueCount = 0;
    
    for (int i = 0; i < n; ++i) {
        String currentSSID = WiFi.SSID(i);
        bool isDuplicate = false;
        
        for (int j = 0; j < uniqueCount; ++j) {
            if (currentSSID == seenSSIDs[j]) {
                isDuplicate = true;
                break;
            }
        }
        
        if (!isDuplicate && currentSSID.length() > 0) {
            seenSSIDs[uniqueCount++] = currentSSID;
            str += "<option value=\"" + currentSSID + "\">" 
                   + currentSSID + "</option>";
        }
    }
    return str;  // HTML option list
}
```

### 5.4 Connection with Persistence

```cpp
bool wifiConnect(String _wifi_ssid, String _wifi_password, unsigned long timeout) {
    WiFi.disconnect();
    WiFi.begin(_wifi_ssid.c_str(), _wifi_password.c_str());
    
    unsigned long start = millis();
    while (millis() - start < timeout) {
        if (WiFi.status() == WL_CONNECTED) {
            // Save to NVS for persistence
            preferences.putString("WIFI_SSID", _wifi_ssid);
            preferences.putString("WIFI_PWD", _wifi_password);
            device_state = kWiFiConnected;
            return true;
        }
        device_state = kWiFiDisconnected;
        vTaskDelay(500);
    }
    return false;
}
```

The `preferences` object (backed by ESP32 NVS) stores WiFi credentials across reboots.

---

## 6. MQTT Module

### 6.1 Default Configuration

```cpp
// ATOM_PRINTER_CONFIG.h

#define MQTT_BROKER      "mqtt.m5stack.com"
#define MQTT_PORT        1883
#define MQTT_ID          ""
#define MQTT_USER        ""
#define MQTT_PASSWORD    ""
#define MQTT_TOPIC       ""  // Empty = use MAC address
```

### 6.2 Connection Logic

```cpp
// ATOM_PRINTER_MQTT.cpp

bool mqttConnect(String _mqtt_broker, int _mqtt_port, String _mqtt_id,
                 String _mqtt_user, String _mqtt_password, unsigned long timeout) {
    
    mqttClient.disconnect();
    mqttClient.setServer(_mqtt_broker.c_str(), _mqtt_port);
    
    // Generate random client ID for public brokers
    String mqttid;
    if (_mqtt_broker.indexOf("m5stack") != -1) {
        mqttid = "MQTTID_" + String(random(65536));
    } else if (mqttid == "") {
        mqttid = "MQTTID_" + String(random(65536));
    } else {
        mqttid = _mqtt_id;
    }
    
    // Default to MAC address as topic if not configured
    if (mqtt_topic == "") {
        mqtt_topic = device_mac;
    }
    
    // Generate random credentials for public brokers
    if (_mqtt_user == "") {
        _mqtt_user = "_mqtt_user" + String(random(65536));
    }
    if (_mqtt_password == "") {
        _mqtt_password = "_mqtt_password" + String(random(65536));
    }
    
    // Attempt connection with timeout
    if (mqttClient.connect(mqttid.c_str(), 
                            _mqtt_user.c_str(), 
                            _mqtt_password.c_str())) {
        printer.printASCII("successfully connect to UIFLOW");
        printer.newLine(3);
        printer.printASCII("subscribe: " + mqtt_topic);
        mqttClient.subscribe(mqtt_topic.c_str());
        return true;
    }
    return false;
}
```

### 6.3 Message Callback (Command Parser)

```cpp
// PRINTER_FW.ino

void mqttCallback(char *topic, byte *payload, unsigned int len) {
    char PayloadData[len + 1];
    strncpy(PayloadData, (char *)payload, len);
    PayloadData[len] = '\0';
    
    String Type = String(PayloadData);
    
    if (Type.indexOf("TEXT") >= 0) {
        // Format: TEXT,<position>,<size>:<content>
        Type = Type.substring(5);              // Remove "TEXT,"
        int posx = Type.toInt();               // Get position
        int indexs = Type.indexOf(",");
        Type = Type.substring(indexs + 1);    // Remove position
        int fonts = Type.toInt();              // Get font size
        indexs = Type.indexOf(":");
        
        printer.init();
        printer.printPos(posx);
        printer.fontSize(fonts);
        printer.printASCII(&Type[indexs + 1]);  // Print after ":"
        printer.newLine(3);
        
    } else if (Type.indexOf("QR:") >= 0) {
        printer.init();
        printer.printQRCode(&Type[3]);  // After "QR:"
        printer.newLine(3);
        
    } else if (Type.indexOf("BAR:") >= 0) {
        printer.init();
        printer.setBarCodeHRI(HIDE);
        printer.printBarCode(CODE128, &Type[4]);  // After "BAR:"
        printer.newLine(3);
    }
}
```

### 6.4 MQTT Payload Formats

| Format | Example Payload | Description |
|--------|-----------------|-------------|
| Text | `TEXT,10,1:Hello World` | Position 10, size 1, "Hello World" |
| QR Code | `QR:https://example.com` | QR code with URL |
| Barcode | `BAR:123456789` | CODE128 barcode |

**Text parsing breakdown:**
```
TEXT,10,1:Hello
   └──┬──┘ │ │ └────┬────┘
      │    │ │      └── Content (after ":")
      │    │ └── Font size (1-7)
      │    └── Horizontal position in dots
      └── "TEXT," prefix (5 chars)
```

---

## 7. HTTP Web Server

### 7.1 Routes

```cpp
// ATOM_PRINTER_WEB.cpp

void webServerInit() {
    dnsServer.start(DNS_PORT, "*", apIP);
    
    webServer.onNotFound(handleRoot);           // GET /
    webServer.on("/", HTTP_GET, handleRoot);    // GET /
    webServer.on("/print", HTTP_GET, handlePrint);
    webServer.on("/wifi_config", HTTP_POST, handleWiFiConfig);
    webServer.on("/mqtt_config", HTTP_GET, handleMQTTConfig);
    webServer.on("/device_status", HTTP_GET, handleStatusConfig);
    webServer.on("/bmp_size", HTTP_POST, handleBMPSize);
    webServer.on("/bmp", HTTP_POST, handleBMP);
    
    webServer.begin();
}
```

### 7.2 Print Handler

```cpp
void handlePrint() {
    printType = urlDecode(webServer.arg("printType"));
    
    if (printType == "ASCII") {
        Pdata = urlDecode(webServer.arg("Pdata"));
        newLine = urlDecode(webServer.arg("newLine"));
        
        printer.init();
        printer.printASCII(Pdata);
        
    } else if (printType == "QRCode") {
        QRCode = urlDecode(webServer.arg("QRCode"));
        printer.init();
        printer.printQRCode(QRCode);
        
    } else if (printType == "BarCode") {
        BarCode = urlDecode(webServer.arg("BarCode"));
        printer.init();
        printer.setBarCodeHRI(HIDE);
        printer.printBarCode(CODE128, BarCode);
    }
    
    if (newLine == "on") {
        printer.newLine(1);
    }
    
    webServer.send(200, "text/plain", "OK");
}
```

### 7.3 Status Endpoint

```cpp
void handleStatusConfig() {
    DynamicJsonDocument doc(1024);
    
    if (WiFi.status() == WL_CONNECTED) {
        doc["WIFI_STATE"] = true;
        doc["SSID"] = WiFi.SSID();
        doc["IP"] = WiFi.localIP().toString();
        doc["RSSI"] = WiFi.RSSI();
        
        if (mqttClient.connected()) {
            doc["MQTT_STATE"] = "Connected";
            doc["MQTT_BROKER"] = mqtt_broker;
            doc["MQTT_TOPIC"] = mqtt_topic;
        } else {
            doc["MQTT_STATE"] = "Disconnected";
        }
    } else {
        doc["WIFI_STATE"] = false;
        doc["MQTT_STATE"] = "Disconnected";
    }
    
    doc["WIFI_HTML"] = ssid_html;  // Available networks
    
    String response;
    serializeJson(doc, response);
    webServer.send(200, "application/json", response);
}
```

### 7.4 BMP Upload

The printer handles BMP data in chunks:

```cpp
void handleBMP() {
    HTTPUpload& upload = webServer.upload();
    
    if (upload.status == UPLOAD_FILE_START) {
        bmp_data_offset = 0;
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        // Receive chunk
        memcpy(bmp_buffer + bmp_data_offset, upload.buf, upload.currentSize);
        bmp_data_offset += upload.currentSize;
    } else if (upload.status == UPLOAD_FILE_END) {
        // Print after upload complete
        if (bmp_width > 0 && bmp_height > 0) {
            printer.printBMP(0, bmp_width, bmp_height, bmp_buffer);
        }
    }
}
```

### 7.5 Web Interface (Embedded HTML)

The `ATOM_PRINTER_HTML.h` file contains a complete embedded web UI (~30KB):

- Print forms (ASCII, QR, Barcode)
- WiFi configuration dropdown + manual entry
- MQTT broker configuration
- Device status display
- Real-time updates via polling

The HTML uses vanilla JavaScript with no external dependencies.

---

## 8. Main Firmware Loop

```cpp
// PRINTER_FW.ino

void setup() {
    M5.begin(true, false, true);      // Serial, I2C, Display
    printer.begin();                   // Serial2 at 9600 baud
    
    preferences.begin("PRINTER_CONFIG");
    
    // Start LED status task
    xTaskCreatePinnedToCore(TaskLED, "TaskLED", 2048, NULL, 3, NULL, 0);
    
    // Initialize WiFi AP + scan networks
    wifiInit();
    ssid_html = wifiScan();
    
    // Start HTTP server
    webServerInit();
    
    // Get MAC for MQTT topic
    device_mac = WiFi.softAPmacAddress();
    
    // Setup MQTT
    mqttClient.setBufferSize(4096);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setKeepAlive(10);
    
    // Load saved WiFi credentials
    if (preferences.getString("WIFI_SSID").length() > 1) {
        wifi_ssid = preferences.getString("WIFI_SSID");
        wifi_password = preferences.getString("WIFI_PWD");
    }
}

void loop() {
    webServer.handleClient();
    dnsServer.processNextRequest();
    
    if (WiFi.status() == WL_CONNECTED) {
        if (!mqttClient.connected()) {
            mqttConnect(mqtt_broker, mqtt_port, mqtt_id, 
                        mqtt_user, mqtt_password, 2000);
        } else {
            mqttClient.loop();
        }
    } else if (wifi_ssid != "") {
        wifiConnect(wifi_ssid, wifi_password, 5000);
    }
    
    // Factory reset: hold button 5 seconds
    if (M5.Btn.pressedFor(5000)) {
        preferences.clear();
        esp_restart();
    }
    
    M5.update();
}
```

### 8.1 Reset Mechanism

Holding the ATOM button for 5 seconds:
1. Calls `preferences.clear()` (wipes NVS)
2. Calls `esp_restart()`
3. Device boots fresh, creates new AP

---

## 9. State Machine

```cpp
typedef enum {
    kInit = 0,              // Initializing
    kWiFiConnected,         // WiFi connected
    kWiFiDisconnected,      // WiFi lost
    kMQTTConnected,         // MQTT connected
    kMQTTDisconnected,      // MQTT lost
} Atom_Printer_State_t;
```

### 9.1 LED Status Task

```cpp
void TaskLED(void *pvParameters) {
    while (1) {
        switch (device_state) {
            case kInit:
                flashing(0x00ff00, 20);  // Blinking green
                break;
            case kWiFiConnected:
                M5.dis.drawpix(0, 0x00ff00);  // Solid green
                break;
            case kWiFiDisconnected:
                flashing(0xff0000, 20);  // Blinking red
                break;
            case kMQTTConnected:
                M5.dis.drawpix(0, 0x0000ff);  // Solid blue
                break;
            case kMQTTDisconnected:
                flashing(0x0000ff, 20);  // Blinking blue
                break;
        }
        vTaskDelay(500);
    }
}
```

### 9.2 LED Color Reference

| Color | Meaning |
|-------|---------|
| Blinking Green | Boot/Init |
| Solid Green | WiFi Connected |
| Blinking Red | WiFi Disconnected |
| Solid Blue | MQTT Connected |
| Blinking Blue | MQTT Connected (attempting) |

---

## 10. Data Flow Diagrams

### 10.1 MQTT Message Flow

```
[MQTT Broker]
     │
     │  Publish: "TEXT,10,1:Hello"
     ▼
[mqttClient.loop()]
     │
     ▼
[mqttCallback()]
     │  Parse "TEXT,10,1:Hello"
     │  Extract: pos=10, size=1, text="Hello"
     ▼
[printer.printPos(10)]
     │  Send: 0x1B 0x24 0x0A 0x00
     ▼
[printer.fontSize(1)]
     │  Send: 0x1D 0x21 0x11
     ▼
[printer.printASCII("Hello")]
     │  Send: "Hello" (ASCII bytes)
     ▼
[printer.newLine(3)]
     │  Send: 0x0A 0x0A 0x0A
     ▼
[Thermal Printer]
     │  Renders text on paper
     ▼
[Printed Receipt]
```

### 10.2 HTTP Web Print Flow

```
[Browser]
     │
     │  GET /print?printType=ASCII&Pdata=Hello&newLine=on
     ▼
[webServer.handleClient()]
     │
     ▼
[handlePrint()]
     │  Extract: printType="ASCII", Pdata="Hello"
     ▼
[printer.init()]
     │  Send: 0x1B 0x40
     ▼
[printer.printASCII("Hello")]
     │
     ▼
[printer.newLine(1)]
     │
     ▼
[webServer.send(200, "text/plain", "OK")]
     │
     ▼
[Browser receives "OK"]
```

### 10.3 WiFi Configuration Flow

```
[Browser → 192.168.4.1/wifi_config]
     │
     │  POST { "ssid": "MyWiFi", "password": "secret" }
     ▼
[handleWiFiConfig()]
     │  Parse JSON
     │  Validate SSID not empty
     ▼
[wifiConnect("MyWiFi", "secret", 5000)]
     │
     ├─→ [WiFi.begin()]
     │
     ├─→ Wait for WL_CONNECTED
     │
     ├─→ [preferences.putString("WIFI_SSID", "MyWiFi")]
     │    [preferences.putString("WIFI_PWD", "secret")]
     │
     └─→ [webServer.send(200, "text/plain", "OK")]
```

---

## 11. Key Implementation Patterns

### 11.1 Preferences Persistence

The `Preferences` class (ESP32 NVS) provides key-value storage:

```cpp
// Save
preferences.putString("WIFI_SSID", wifi_ssid);
preferences.putString("WIFI_PWD", wifi_password);
preferences.putString("MQTT_BROKER", mqtt_broker);
preferences.putInt("MQTT_PORT", mqtt_port);

// Load
wifi_ssid = preferences.getString("WIFI_SSID");
wifi_password = preferences.getString("WIFI_PWD");

// Clear all
preferences.clear();
```

### 11.2 URL Decoding

```cpp
String urlDecode(String input) {
    String s = input;
    s.replace("%20", " ");    // Space
    s.replace("+", " ");        // Plus
    s.replace("%21", "!");      // Exclamation
    s.replace("%2C", ",");      // Comma
    s.replace("%3A", ":");      // Colon
    // ... more replacements
    return s;
}
```

### 11.3 Buffer Management

```cpp
void ATOM_PRINTER::cleanBuffer() {
    for (int i = 0; i < 256; i++) {
        buffer[i] = 0;
    }
}

void ATOM_PRINTER::WriteCmd(uint8_t *buff, uint8_t buff_size) {
    cleanBuffer();
    memcpy(buffer, buff, buff_size);
    _serial->write(buffer, buff_size);
}
```

### 11.4 Thread Safety with Mutex

```cpp
xSemaphoreHandle xMQTTMutex = xSemaphoreCreateMutex();

// In MQTT config handler:
xSemaphoreTake(xMQTTMutex, portMAX_DELAY);
// ... modify MQTT settings ...
xSemaphoreGive(xMQTTMutex);
```

### 11.5 Task Priority

```cpp
xTaskCreatePinnedToCore(
    TaskLED,           // Function
    "TaskLED",         // Name
    2048,              // Stack size
    NULL,               // Parameters
    3,                  // Priority (1-25, higher = more priority)
    NULL,               // Task handle
    0                   // Core (0 or 1)
);
```

---

## 12. Security Considerations

### 12.1 Known Issues

1. **Hardcoded MQTT Broker**: Default `mqtt.m5stack.com` - no authentication by default
2. **Random Credentials for Public Brokers**: Auto-generated `_mqtt_user` and `_mqtt_password` are trivially guessable
3. **No TLS/SSL**: MQTT connection is plaintext
4. **AP Mode**: Open WiFi (no password)
5. **No Firmware Signing**: Any compiled firmware can be flashed

### 12.2 Recommendations for Production

1. Use your own MQTT broker with authentication
2. Enable TLS on MQTT broker
3. Implement device authentication tokens
4. Add HTTPS for web interface
5. Implement firmware verification

---

## 13. How to Write Your Own Firmware

### 13.1 Minimal Example

```cpp
#include <M5Atom.h>
#include "ATOM_PRINTER.h"

ATOM_PRINTER printer;

void setup() {
    M5.begin(true, false, true);
    printer.begin();
    printer.init();
}

void loop() {
    if (M5.Btn.wasPressed()) {
        printer.init();
        printer.printASCII("Hello World!");
        printer.newLine(2);
    }
    M5.update();
}
```

### 13.2 With Custom MQTT Integration

```cpp
#include <M5Atom.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "ATOM_PRINTER.h"

const char* ssid = "YourWiFi";
const char* password = "YourPassword";
const char* mqtt_broker = "your-broker.com";
const char* mqtt_topic = "printer";

WiFiClient espClient;
PubSubClient mqttClient(espClient);
ATOM_PRINTER printer;

void mqttCallback(char* topic, byte* payload, unsigned int len) {
    String msg = "";
    for (unsigned int i = 0; i < len; i++) {
        msg += (char)payload[i];
    }
    
    if (msg.startsWith("PRINT:")) {
        String text = msg.substring(6);
        printer.init();
        printer.printASCII(text);
    }
}

void setup() {
    M5.begin(true, false, true);
    printer.begin();
    
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) delay(100);
    
    mqttClient.setServer(mqtt_broker, 1883);
    mqttClient.setCallback(mqttCallback);
    mqttClient.connect("printer-client");
    mqttClient.subscribe(mqtt_topic);
}

void loop() {
    mqttClient.loop();
    M5.update();
}
```

### 13.3 Custom Web Server Endpoint

```cpp
#include <WebServer.h>
WebServer server(80);

void handleCustomPrint() {
    String text = server.arg("text");
    printer.init();
    printer.printASCII(text);
    printer.newLine(1);
    server.send(200, "text/plain", "OK");
}

void setup() {
    // ... other setup ...
    server.on("/custom", handleCustomPrint);
    server.begin();
}

void loop() {
    server.handleClient();
    // ... other loop code ...
}
```

### 13.4 Image Conversion Tool

To prepare images for printing:

```python
#!/usr/bin/env python3
"""Convert image to ATOM Printer BMP format."""

from PIL import Image

def image_to_printer_bmp(input_path, output_path, width=384):
    img = Image.open(input_path).convert('L')
    aspect = img.height / img.width
    height = int(width * aspect)
    img = img.resize((width, height), Image.LANCZOS)
    img = img.convert('1')  # 1-bit
    
    # Save raw bitmap
    img.save(output_path)
    print(f"Saved {width}x{height} image to {output_path}")

if __name__ == "__main__":
    image_to_printer_bmp("input.png", "output.bmp")
```

---

## 14. File Manifest

| File | Purpose |
|------|---------|
| `src/ATOM_PRINTER.h` | Public API |
| `src/ATOM_PRINTER.cpp` | Serial protocol implementation |
| `src/ATOM_PRINTER_CMD.h` | Command byte constants |
| `examples/PRINTER_FW/PRINTER_FW.ino` | Full firmware main sketch |
| `examples/PRINTER_FW/ATOM_PRINTER_WIFI.cpp` | WiFi management |
| `examples/PRINTER_FW/ATOM_PRINTER_MQTT.cpp` | MQTT client |
| `examples/PRINTER_FW/ATOM_PRINTER_WEB.cpp` | HTTP server |
| `examples/PRINTER_FW/ATOM_PRINTER_CONFIG.h` | Configuration constants |
| `examples/PRINTER_FW/ATOM_PRINTER_HTML.h` | Embedded web UI |
| `examples/PRINTER_TEST/PRINTER_TEST.ino` | Simple test firmware |
| `examples/PRINTER_IMAGE/PRINTER_IMAGE.ino` | Image printing example |

---

## Appendix A: Full Command Reference

### Text Commands

| Command | Bytes | Description |
|---------|-------|-------------|
| Initialize | `1B 40` | Reset printer |
| Print position | `1B 24 XH XL` | Set horizontal position |
| Font size | `1D 21 NN` | Set font size (bits 0-3 = height, 4-7 = width) |
| Print text | (ASCII) | Direct text output |
| Newline | `0A` | Line feed |

### Barcode Commands

| Command | Bytes | Description |
|---------|-------|-------------|
| HRI position | `1D 48 NN` | 00=hide, 01=above, 02=below, 03=both |
| Barcode type | `1D 6B TT` | TT = type (0x41-0x49) |
| Enable | `1D 45 43 SS` | 00=off, 01=on |

### QR Code Commands

| Command | Bytes | Description |
|---------|-------|-------------|
| Set data | `1D 28 6B pL pH 31 50 30 [data] 00` | p = data length |
| Print | `1D 28 6B 03 00 31 51 30 00` | Print QR |
| Error correction | `1D 28 6B 03 00 31 45 LL` | LL = level |

### BMP Commands

| Command | Bytes | Description |
|---------|-------|-------------|
| Print BMP | `1D 76 30 m xL xH yL yH [data]` | Raster print |

---

## Appendix B: Memory Considerations

| Component | Size |
|-----------|------|
| MQTT buffer | 4096 bytes |
| BMP buffer | 50KB (50 × 1024) |
| JSON doc | 1024 bytes |
| LED task stack | 2048 bytes |
| Preferences namespace | "PRINTER_CONFIG" |

The BMP buffer (50KB) limits image size to approximately:
- 384 pixels wide × 1000 pixels tall maximum

---

## Appendix C: Dependencies

Required Arduino libraries:
- **M5Atom** by M5Stack
- **PubSubClient** by Nick O'Leary
- **ArduinoJson** by Benoit Blanchon
- **FastLED** by Daniel Garcia

---

*Source: Reverse-engineered from https://github.com/m5stack/ATOM-PRINTER*
*Analysis date: 2026-04-22*
*Source files analyzed: 11 files, ~2000 lines of code*
