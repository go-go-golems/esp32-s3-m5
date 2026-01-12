---
Title: NCP Firmware Architecture and Protocol Analysis
Ticket: 0032-ANALYZE-NCP-FIRMWARE
Status: active
Topics:
    - zigbee
    - ncp
    - esp32
    - firmware
    - protocol
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Comprehensive analysis of the ESP32 Zigbee NCP firmware architecture, protocol implementation, and developer guide
LastUpdated: 2026-01-06T02:08:15.325928245-05:00
WhatFor: Understanding how the NCP firmware works, how to control it, and the underlying protocols
WhenToUse: When developing host applications that communicate with NCP firmware, debugging NCP issues, or learning about Zigbee NCP architecture
---

# NCP H2 Gateway Firmware: Complete Developer Guide

## Table of Contents

1. [Introduction](#introduction)
2. [Zigbee Background](#zigbee-background)
3. [NCP Architecture Overview](#ncp-architecture-overview)
4. [FreeRTOS Integration](#freertos-integration)
5. [Protocol Stack](#protocol-stack)
6. [Code Architecture](#code-architecture)
7. [Command and Control](#command-and-control)
8. [API Reference](#api-reference)
9. [Examples and Use Cases](#examples-and-use-cases)
10. [Debugging and Troubleshooting](#debugging-and-troubleshooting)

---

## Introduction

The ESP32 Zigbee NCP (Network Co-Processor) firmware is a specialized firmware that runs the complete Zigbee protocol stack on an ESP32-H2 device, allowing a host processor (typically an ESP32-S3 or similar) to control Zigbee network operations through a serial protocol. This architecture separates concerns: the NCP handles all Zigbee complexity, while the host focuses on application logic.

### What This Document Covers

This guide provides a comprehensive understanding of:

- **How the NCP firmware works** - Internal architecture and execution flow
- **What it does** - Zigbee stack operations and network management
- **Background information** - Zigbee protocol, FreeRTOS, and Espressif's Zigbee SDK
- **How to control it** - Protocol details and command structure
- **Implementation details** - Code walkthroughs and API references

### Key Files Analyzed

- `esp_zigbee_ncp.c` - Main entry point
- `esp_ncp_main.c` - Event loop and task management
- `esp_ncp_bus.c` - UART transport layer
- `esp_ncp_frame.c` - Protocol framing and SLIP encoding
- `esp_ncp_zb.c` - Zigbee command handlers
- `slip.c` - SLIP encoding/decoding

---

## Zigbee Background

### What is Zigbee?

Zigbee is a wireless communication protocol built on IEEE 802.15.4, designed for low-power, low-data-rate applications in home automation, industrial control, and IoT scenarios. Key characteristics:

- **Low Power**: Devices can operate on batteries for months or years
- **Mesh Networking**: Self-healing mesh topology with routers extending network range
- **Security**: AES-128 encryption with network and link keys
- **Application Profiles**: Standardized device types (Zigbee Home Automation, Zigbee Light Link, etc.)

### Zigbee Network Architecture

A Zigbee network consists of:

1. **Coordinator**: Forms the network, manages security, assigns addresses
2. **Router**: Extends network range, forwards messages, can join devices
3. **End Device**: Leaf nodes that sleep to save power

### Zigbee Protocol Stack

```
┌─────────────────────────────────────┐
│  Application Layer (ZCL)           │  ← Zigbee Cluster Library
├─────────────────────────────────────┤
│  Application Support Layer (APS)    │  ← Endpoint management
├─────────────────────────────────────┤
│  Network Layer (NWK)                │  ← Routing, addressing
├─────────────────────────────────────┤
│  MAC Layer (IEEE 802.15.4)          │  ← Frame format, CSMA-CA
├─────────────────────────────────────┤
│  Physical Layer (2.4 GHz)           │  ← Radio transmission
└─────────────────────────────────────┘
```

### Zigbee Cluster Library (ZCL)

ZCL defines standardized clusters (groups of attributes and commands) for common device types:

- **On/Off Cluster** (0x0006): Control lights, switches
- **Level Control Cluster** (0x0008): Dimming, brightness
- **Temperature Measurement** (0x0402): Sensor readings
- **Basic Cluster** (0x0000): Device information

Each cluster has:
- **Attributes**: State variables (e.g., `OnOff` attribute)
- **Commands**: Actions (e.g., `Toggle` command)
- **Client/Server roles**: Devices can be clients (control) or servers (respond)

---

## NCP Architecture Overview

### What is NCP?

**Network Co-Processor (NCP)** is an architecture where the Zigbee protocol stack runs on a dedicated processor (the NCP), while application logic runs on a host processor. This separation provides:

- **Isolation**: Zigbee stack complexity isolated from host application
- **Flexibility**: Host can be any processor (ESP32-S3, Linux, etc.)
- **Maintainability**: Stack updates don't require host code changes
- **Resource Efficiency**: Host can focus on application without Zigbee overhead

### NCP vs RCP

| Aspect | NCP (Network Co-Processor) | RCP (Radio Co-Processor) |
|--------|---------------------------|--------------------------|
| **Stack Location** | On NCP device | On host device |
| **NCP Responsibilities** | Full Zigbee stack + ZCL | Only 802.15.4 radio operations |
| **Host Responsibilities** | Application logic | Zigbee stack + application |
| **Protocol** | Command/event frames | Spinel (property-based) |
| **Complexity** | Higher on NCP | Higher on host |

### System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Host Processor (ESP32-S3)                 │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │         Application Logic                            │  │
│  │  - HTTP server                                        │  │
│  │  - Device management                                 │  │
│  │  - User interface                                     │  │
│  └──────────────────────────────────────────────────────┘  │
│                          │                                   │
│                          │ ESP ZNSP Protocol                 │
│                          │ (SLIP + Frame + CRC)              │
│                          ▼                                   │
│  ┌──────────────────────────────────────────────────────┐  │
│  │         Host Protocol Layer                          │  │
│  │  - Frame encoding/decoding                           │  │
│  │  - Command dispatch                                  │  │
│  │  - Event handling                                    │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                          │
                          │ UART/SPI
                          │
┌─────────────────────────────────────────────────────────────┐
│              NCP Processor (ESP32-H2)                       │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │         Protocol Layer                                │  │
│  │  - Frame parsing                                      │  │
│  │  - Command routing                                    │  │
│  │  - Response generation                                │  │
│  └──────────────────────────────────────────────────────┘  │
│                          │                                   │
│                          ▼                                   │
│  ┌──────────────────────────────────────────────────────┐  │
│  │         Zigbee Stack (esp-zigbee-lib)                │  │
│  │  - Network formation                                  │  │
│  │  - Device joining                                    │  │
│  │  - Message routing                                    │  │
│  │  - Security management                                │  │
│  └──────────────────────────────────────────────────────┘  │
│                          │                                   │
│                          ▼                                   │
│  ┌──────────────────────────────────────────────────────┐  │
│  │         802.15.4 Radio                               │  │
│  │  - Physical transmission                              │  │
│  │  - CSMA-CA                                            │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### Communication Flow

1. **Host → NCP (Command)**:
   ```
   Host App → Host Protocol Layer → SLIP Encode → UART → 
   NCP UART → SLIP Decode → NCP Protocol Layer → Zigbee Stack
   ```

2. **NCP → Host (Response/Event)**:
   ```
   Zigbee Stack → NCP Protocol Layer → SLIP Encode → UART →
   Host UART → SLIP Decode → Host Protocol Layer → Host App
   ```

---

## FreeRTOS Integration

### Why FreeRTOS?

FreeRTOS is a real-time operating system kernel that provides:

- **Task Scheduling**: Preemptive multitasking
- **Inter-task Communication**: Queues, semaphores, event groups
- **Resource Management**: Mutexes, critical sections
- **Timing**: Delays, timers, tick management

The NCP firmware uses FreeRTOS extensively for concurrent operations.

### FreeRTOS Tasks in NCP Firmware

#### 1. Main NCP Task (`esp_ncp_main_task`)

**Location**: `esp_ncp_main.c:93`

**Purpose**: Processes events from the event queue and dispatches them to appropriate handlers.

**Pseudocode**:
```c
void esp_ncp_main_task(void *pv) {
    esp_ncp_dev_t *dev = (esp_ncp_dev_t *)pv;
    esp_ncp_ctx_t ncp_ctx;
    
    while (dev->run) {
        // Wait for event from queue
        if (xQueueReceive(dev->queue, &ncp_ctx, portMAX_DELAY) == pdTRUE) {
            // Process event (INPUT, OUTPUT, or RESET)
            esp_ncp_process_event(dev, &ncp_ctx);
        }
    }
    
    // Cleanup
    esp_ncp_bus_stop(dev->bus);
    vQueueDelete(dev->queue);
    vTaskDelete(NULL);
}
```

**Key FreeRTOS APIs**:
- `xQueueReceive()`: Blocking receive from queue
- `xQueueSend()`: Send to queue (from ISR or task)
- `xQueueSendFromISR()`: Send from interrupt context

#### 2. Bus Task (`esp_ncp_bus_task`)

**Location**: `esp_ncp_bus.c:70`

**Purpose**: Monitors UART for incoming data and detects SLIP frame boundaries using UART pattern detection.

**Pseudocode**:
```c
void esp_ncp_bus_task(void *pvParameter) {
    uart_event_t event;
    uint8_t *dtmp = malloc(NCP_BUS_BUF_SIZE);
    esp_ncp_bus_t *bus = (esp_ncp_bus_t *)pvParameter;
    
    while (bus->state == BUS_INIT_START) {
        if (xQueueReceive(uart0_queue, &event, portMAX_DELAY)) {
            switch(event.type) {
                case UART_PATTERN_DET:
                    // SLIP_END (0xC0) detected
                    pos = uart_pattern_pop_pos(CONFIG_NCP_BUS_UART_NUM);
                    if (pos > 0) {
                        // Read complete SLIP frame
                        ncp_event.size = uart_read_bytes(..., pos + 1, ...);
                        xStreamBufferSend(bus->output_buf, dtmp, ncp_event.size, 0);
                        esp_ncp_send_event(&ncp_event);
                    }
                    break;
                // Handle other UART events...
            }
        }
    }
    
    free(dtmp);
    vTaskDelete(NULL);
}
```

**Key FreeRTOS APIs**:
- `xQueueReceive()`: Receive UART events
- `xStreamBufferSend()`: Send data to stream buffer
- `xStreamBufferReceive()`: Receive from stream buffer

#### 3. Zigbee Stack Task (`esp_ncp_zb_task`)

**Location**: `esp_ncp_zb.c:561`

**Purpose**: Runs the Zigbee stack main loop, processing Zigbee events and callbacks.

**Pseudocode**:
```c
static void esp_ncp_zb_task(void *pvParameters) {
    // Register action handler for ZCL commands
    esp_zb_core_action_handler_register(esp_ncp_zb_action_handler);
    
    // Enter Zigbee stack main loop (blocks forever)
    esp_zb_stack_main_loop();
    
    vTaskDelete(NULL);
}
```

### FreeRTOS Synchronization Primitives

#### Queues

**Event Queue** (`s_ncp_dev.queue`):
- Type: `QueueHandle_t`
- Size: `NCP_EVENT_QUEUE_LEN` (60)
- Item: `esp_ncp_ctx_t` (event type + size)
- Purpose: Synchronize between UART ISR/tasks and main processing task

**APS Data Queues**:
- `s_aps_data_confirm`: Confirmation messages from Zigbee stack
- `s_aps_data_indication`: Indication messages from Zigbee stack

#### Stream Buffers

**Input Buffer** (`bus->input_buf`):
- Type: `StreamBufferHandle_t`
- Size: `NCP_BUS_RINGBUF_SIZE` (20480 bytes)
- Purpose: Buffer data from host before processing

**Output Buffer** (`bus->output_buf`):
- Type: `StreamBufferHandle_t`
- Size: `NCP_BUS_RINGBUF_SIZE` (20480 bytes)
- Purpose: Buffer data to host after processing

#### Semaphores

**Input Semaphore** (`bus->input_sem`):
- Type: `SemaphoreHandle_t`
- Type: Mutex
- Purpose: Protect `input_buf` from concurrent access

### Task Priorities

| Task | Priority | Stack Size | Purpose |
|------|----------|------------|---------|
| `esp_ncp_main_task` | 23 | 5120 | Main event processing |
| `esp_ncp_bus_task` | 18 | 4096 | UART I/O |
| `esp_ncp_zb_task` | 5 | 4096 | Zigbee stack loop |

Higher priority = more urgent. The main task has highest priority to ensure events are processed promptly.

---

## Protocol Stack

The NCP protocol uses a layered approach:

```
┌─────────────────────────────────────┐
│   Application Layer                  │  ← Zigbee commands/events
│   (Zigbee Operations)               │
├─────────────────────────────────────┤
│   Protocol Layer                    │  ← Frame header + CRC
│   (ESP ZNSP / EGSP-style)           │
├─────────────────────────────────────┤
│   Framing Layer                     │  ← SLIP encoding
│   (Packet Delimiting)               │
├─────────────────────────────────────┤
│   Transport Layer                   │  ← UART/SPI
│   (Physical Communication)           │
└─────────────────────────────────────┘
```

### Transport Layer: UART

**Configuration** (from `Kconfig`):
- **Baud Rate**: Default 115200 (configurable 9600-891200)
- **Data Bits**: 8 bits
- **Stop Bits**: 1 bit
- **Parity**: None
- **Flow Control**: Optional (RTS/CTS)

**UART Pattern Detection**:
The firmware uses ESP32's UART pattern detection feature to identify SLIP frame boundaries:

```c
// Enable pattern detection for SLIP_END (0xC0)
uart_enable_pattern_det_baud_intr(
    CONFIG_NCP_BUS_UART_NUM,
    SLIP_END,      // Pattern byte
    1,             // Pattern length
    1,             // Queue length
    0,             // Timeout
    0              // Post-IDLE timeout
);
```

When `SLIP_END` (0xC0) is detected, the UART driver generates a `UART_PATTERN_DET` event, allowing the bus task to read a complete SLIP frame.

### Framing Layer: SLIP

**SLIP (Serial Line Internet Protocol)** is a simple framing protocol that delimits packets with special characters.

#### SLIP Special Characters

| Character | Value | Meaning |
|-----------|-------|---------|
| `SLIP_END` | 0xC0 | Packet delimiter (start and end) |
| `SLIP_ESC` | 0xDB | Escape character |
| `SLIP_ESC_END` | 0xDC | Escaped END (represents 0xC0 in data) |
| `SLIP_ESC_ESC` | 0xDD | Escaped ESC (represents 0xDB in data) |

#### SLIP Encoding Algorithm

**Location**: `slip.c:19`

```c
esp_err_t slip_encode(const uint8_t *inbuf, uint16_t inlen, 
                      uint8_t **outbuf, uint16_t *outlen) {
    // 1. Send initial END to flush receiver
    send(SLIP_END);
    
    // 2. For each byte in packet:
    for (each byte in inbuf) {
        switch (byte) {
            case SLIP_END:
                send(SLIP_ESC);
                send(SLIP_ESC_END);  // Represents 0xC0
                break;
            case SLIP_ESC:
                send(SLIP_ESC);
                send(SLIP_ESC_ESC);  // Represents 0xDB
                break;
            default:
                send(byte);  // Send as-is
        }
    }
    
    // 3. Send final END to mark packet end
    send(SLIP_END);
}
```

**Example**:
```
Input:  [0x12, 0xC0, 0x34, 0xDB, 0x56]
Output: [0xC0, 0x12, 0xDB, 0xDC, 0x34, 0xDB, 0xDD, 0x56, 0xC0]
        └─┘   └─┘   └───┘   └───┘   └─┘   └───┘   └───┘   └─┘
        END   data  ESC   ESC_END  data  ESC   ESC_ESC  data  END
```

#### SLIP Decoding Algorithm

**Location**: `slip.c:89`

```c
esp_err_t slip_decode(const uint8_t *inbuf, uint16_t inlen,
                      uint8_t **outbuf, uint16_t *outlen) {
    uint8_t *output = calloc(1, inlen * 2);
    uint16_t received = 0;
    
    for (each byte in inbuf) {
        switch (byte) {
            case SLIP_END:
                if (no data buffered) {
                    continue;  // Ignore empty packets
                }
                return output;  // Packet complete
                
            case SLIP_ESC:
                next_byte = read_next();
                switch (next_byte) {
                    case SLIP_ESC_END:
                        output[received++] = SLIP_END;  // 0xC0
                        break;
                    case SLIP_ESC_ESC:
                        output[received++] = SLIP_ESC;  // 0xDB
                        break;
                    default:
                        output[received++] = next_byte;  // Protocol violation
                }
                break;
                
            default:
                output[received++] = byte;
        }
    }
}
```

### Protocol Layer: Frame Format

**Location**: `esp_ncp_frame.h:19`

The protocol frame uses an EGSP-style header (Espressif Zigbee Serial Protocol):

```c
typedef struct {
    struct {
        uint16_t version:4;    // Protocol version (currently 0)
        uint16_t type:4;      // Frame type (see below)
        uint16_t reserved:8;  // Reserved bits
    } flags;
    uint16_t id;              // Command/event ID
    uint8_t  sn;              // Sequence number (for request/response matching)
    uint16_t len;             // Payload length
} __attribute__((packed)) esp_ncp_header_t;
```

**Frame Type** (`flags.type`):
- `0`: Request (host → NCP)
- `1`: Response (NCP → host, answers a request)
- `2`: Notification (NCP → host, asynchronous event)

**Frame Layout**:
```
┌─────────────────────────────────────────┐
│  SLIP_END (0xC0)                        │  ← Start delimiter
├─────────────────────────────────────────┤
│  esp_ncp_header_t (8 bytes)            │
│  ├─ flags (2 bytes)                   │
│  │  ├─ version:4                       │
│  │  ├─ type:4                          │
│  │  └─ reserved:8                      │
│  ├─ id:2 (command/event ID)            │
│  ├─ sn:1 (sequence number)            │
│  └─ len:2 (payload length)             │
├─────────────────────────────────────────┤
│  Payload (variable length)              │
├─────────────────────────────────────────┤
│  CRC16 (2 bytes)                        │  ← Checksum
└─────────────────────────────────────────┘
│  SLIP_END (0xC0)                        │  ← End delimiter
```

**CRC Calculation**:
```c
// CRC16-CCITT (polynomial 0x1021, initial value 0xFFFF)
uint16_t crc_val = esp_crc16_le(
    UINT16_MAX,                    // Initial value
    frame_data,                    // Data to checksum
    header_len + payload_len       // Length (excluding CRC)
);
// CRC is appended after payload
```

### Complete Frame Example

**Request: Get PAN ID** (`ESP_NCP_NETWORK_PAN_ID_GET = 0x000B`)

```
Raw bytes (after SLIP decode):
┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
│ 0x00│ 0x00│ 0x0B│ 0x42│ 0x00│ 0x00│ 0x??│ 0x??│     │     │
└─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
  │     │     │     │     │     │     │     │
  │     │     │     │     │     │     │     └─ CRC16 (2 bytes)
  │     │     │     │     │     │     └─ Payload length (0)
  │     │     │     │     │     └─ Sequence number (0x42)
  │     │     │     │     └─ Command ID low byte (0x0B)
  │     │     │     └─ Command ID high byte (0x00)
  │     │     └─ Type (0 = Request), Version (0)
  │     └─ Reserved (0)
  └─ Flags low byte
```

**Response: PAN ID = 0x1234**

```
Raw bytes (after SLIP decode):
┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
│ 0x10│ 0x00│ 0x0B│ 0x42│ 0x02│ 0x00│ 0x34│ 0x12│ 0x??│ 0x??│
└─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
  │     │     │     │     │     │     │     │     │     │
  │     │     │     │     │     │     │     │     │     └─ CRC16 high
  │     │     │     │     │     │     │     │     └─ CRC16 low
  │     │     │     │     │     │     │     └─ PAN ID high (0x12)
  │     │     │     │     │     │     └─ PAN ID low (0x34)
  │     │     │     │     │     └─ Payload length (2 bytes)
  │     │     │     │     └─ Sequence number (0x42, matches request)
  │     │     │     └─ Command ID (0x000B)
  │     │     └─ Type (1 = Response), Version (0)
  │     └─ Reserved (0)
  └─ Flags
```

---

## Code Architecture

### Entry Point: `app_main()`

**Location**: `esp_zigbee_ncp.c:14`

```c
void app_main(void)
{
    // 1. Initialize NVS (Non-Volatile Storage) for Zigbee stack persistence
    ESP_ERROR_CHECK(nvs_flash_init());
    
    // 2. Initialize NCP with UART connection mode
    ESP_ERROR_CHECK(esp_ncp_init(NCP_HOST_CONNECTION_MODE_UART));
    
    // 3. Start NCP communication (creates tasks, starts UART)
    ESP_ERROR_CHECK(esp_ncp_start());
    
    // Function returns, FreeRTOS scheduler takes over
}
```

**Flow**:
1. **NVS Init**: Required for Zigbee stack to persist network parameters (PAN ID, keys, etc.)
2. **NCP Init**: Sets up bus structure, initializes UART driver
3. **NCP Start**: Creates tasks, starts UART pattern detection, enters event loop

### Initialization: `esp_ncp_init()`

**Location**: `esp_ncp_main.c:116`

```c
esp_err_t esp_ncp_init(esp_ncp_host_connection_mode_t mode)
{
    esp_ncp_bus_t *bus = NULL;
    
    // 1. Allocate and initialize bus structure
    esp_err_t ret = esp_ncp_bus_init(&bus);
    s_ncp_dev.bus = bus;
    
    // 2. Call bus-specific init (UART or SPI)
    return (bus && bus->init) ? bus->init(mode) : ret;
}
```

**What happens**:
- Allocates `esp_ncp_bus_t` structure
- Creates stream buffers (input/output)
- Creates mutex for input buffer protection
- Registers UART init function

### Starting: `esp_ncp_start()`

**Location**: `esp_ncp_main.c:144`

```c
esp_err_t esp_ncp_start(void)
{
    // 1. Create event queue
    s_ncp_dev.queue = xQueueCreate(NCP_EVENT_QUEUE_LEN, sizeof(esp_ncp_ctx_t));
    
    // 2. Mark device as running
    s_ncp_dev.run = true;
    
    // 3. Start bus (creates UART task)
    esp_err_t ret = esp_ncp_bus_start(s_ncp_dev.bus);
    if (ret != ESP_OK) {
        // Cleanup on failure
        return ret;
    }
    
    // 4. Create main processing task
    if (xTaskCreate(esp_ncp_main_task, "esp_ncp_main_task", 
                    NCP_TASK_STACK, &s_ncp_dev, 
                    NCP_TASK_PRIORITY, NULL) != pdTRUE) {
        // Cleanup on failure
        return ESP_FAIL;
    }
    
    return ESP_OK;
}
```

### Bus Initialization: `ncp_bus_init_hdl()`

**Location**: `esp_ncp_bus.c:46`

```c
static esp_err_t ncp_bus_init_hdl(uint8_t transport)
{
    // 1. Configure UART parameters
    uart_config_t uart_config = {
        .baud_rate = CONFIG_NCP_BUS_UART_BAUD_RATE,
        .data_bits = CONFIG_NCP_BUS_UART_BYTE_SIZE,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = CONFIG_NCP_BUS_UART_STOP_BITS,
        .flow_ctrl = CONFIG_NCP_BUS_UART_FLOW_CONTROL,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    // 2. Install UART driver
    uart_driver_install(CONFIG_NCP_BUS_UART_NUM, 
                        NCP_BUS_BUF_SIZE * 2,  // RX buffer
                        NCP_BUS_BUF_SIZE * 2,  // TX buffer
                        20,                    // Queue size
                        &uart0_queue,          // Event queue
                        0);                    // Flags
    
    // 3. Apply configuration
    uart_param_config(CONFIG_NCP_BUS_UART_NUM, &uart_config);
    
    // 4. Set GPIO pins
    uart_set_pin(CONFIG_NCP_BUS_UART_NUM,
                 CONFIG_NCP_BUS_UART_TX_PIN,
                 CONFIG_NCP_BUS_UART_RX_PIN,
                 CONFIG_NCP_BUS_UART_RTS_PIN,
                 CONFIG_NCP_BUS_UART_CTS_PIN);
    
    // 5. Enable pattern detection for SLIP_END (0xC0)
    esp_err_t err = uart_enable_pattern_det_baud_intr(
        CONFIG_NCP_BUS_UART_NUM,
        SLIP_END,    // Pattern byte
        1,           // Pattern length
        1,           // Queue length
        0,           // Timeout
        0            // Post-IDLE timeout
    );
    
    // 6. Reset pattern queue
    uart_pattern_queue_reset(CONFIG_NCP_BUS_UART_NUM, 20);
    
    return ESP_OK;
}
```

### Frame Processing: `esp_ncp_frame_output()`

**Location**: `esp_ncp_frame.c:23`

This function processes incoming frames from the host:

```c
esp_err_t esp_ncp_frame_output(const void *buffer, uint16_t len)
{
    uint8_t *output = NULL;
    uint16_t outlen = 0;
    
    // 1. SLIP decode
    slip_decode(buffer, len, &output, &outlen);
    
    // 2. Validate header size
    if (outlen < sizeof(esp_ncp_header_t)) {
        return ESP_ERR_INVALID_SIZE;
    }
    
    // 3. Parse header
    esp_ncp_header_t *ncp_header = (esp_ncp_header_t *)output;
    uint8_t *payload = output + sizeof(esp_ncp_header_t);
    
    // 4. Validate payload length
    if (ncp_header->len + sizeof(esp_ncp_header_t) + sizeof(uint16_t) > outlen) {
        return ESP_ERR_INVALID_SIZE;
    }
    
    // 5. Verify CRC16
    uint16_t *checksum = (uint16_t *)(output + sizeof(esp_ncp_header_t) + ncp_header->len);
    uint16_t crc_val = esp_crc16_le(UINT16_MAX, output, outlen - sizeof(uint16_t));
    if (crc_val != (*checksum)) {
        return ESP_ERR_INVALID_CRC;
    }
    
    // 6. Dispatch to Zigbee handler
    return esp_ncp_zb_output(ncp_header, payload, ncp_header->len);
}
```

### Zigbee Command Dispatch: `esp_ncp_zb_output()`

**Location**: `esp_ncp_zb.c:1938`

This function routes commands to appropriate handlers:

```c
esp_err_t esp_ncp_zb_output(esp_ncp_header_t *ncp_header, 
                             const void *buffer, uint16_t len)
{
    uint8_t *output = NULL;
    uint16_t outlen = 0;
    esp_err_t ret = ESP_OK;
    
    // 1. Look up command handler in function table
    const size_t func_count = sizeof(ncp_zb_func_table) / sizeof(ncp_zb_func_table[0]);
    for (size_t i = 0; i < func_count; i++) {
        if (ncp_header->id != ncp_zb_func_table[i].id) {
            continue;
        }
        
        // 2. Call handler function
        if (ncp_zb_func_table[i].set_func) {
            ret = ncp_zb_func_table[i].set_func(buffer, len, &output, &outlen);
            
            // 3. Send response back to host
            if (ret == ESP_OK) {
                esp_ncp_resp_input(ncp_header, output, outlen);
            }
        } else {
            ret = ESP_ERR_INVALID_ARG;
        }
        break;
    }
    
    if (output) {
        free(output);
    }
    
    return ret;
}
```

**Function Table**: `ncp_zb_func_table[]` (line 1875) maps command IDs to handler functions:

```c
static const esp_ncp_zb_func_t ncp_zb_func_table[] = {
    {ESP_NCP_NETWORK_INIT, esp_ncp_zb_network_init_fn},
    {ESP_NCP_NETWORK_PAN_ID_GET, esp_ncp_zb_pan_id_get_fn},
    {ESP_NCP_NETWORK_PAN_ID_SET, esp_ncp_zb_pan_id_set_fn},
    // ... many more ...
    {ESP_NCP_ZCL_ENDPOINT_ADD, esp_ncp_zb_add_endpoint_fn},
    {ESP_NCP_APS_DATA_REQUEST, esp_ncp_zb_aps_data_request_fn},
    // ...
};
```

---

## Command and Control

### Command Categories

Commands are organized by functionality:

#### Network Management (0x0000-0x002F)

- **0x0000** `ESP_NCP_NETWORK_INIT`: Initialize Zigbee platform
- **0x0001** `ESP_NCP_NETWORK_START`: Start commissioning process
- **0x0004** `ESP_NCP_NETWORK_FORMNETWORK`: Form a new network (coordinator)
- **0x0005** `ESP_NCP_NETWORK_PERMIT_JOINING`: Allow devices to join
- **0x0006** `ESP_NCP_NETWORK_JOINNETWORK`: Join an existing network
- **0x000B** `ESP_NCP_NETWORK_PAN_ID_GET`: Get PAN ID
- **0x000C** `ESP_NCP_NETWORK_PAN_ID_SET`: Set PAN ID
- **0x0017** `ESP_NCP_NETWORK_PRIMARY_KEY_GET`: Get network key
- **0x0018** `ESP_NCP_NETWORK_PRIMARY_KEY_SET`: Set network key

#### ZCL Endpoint Management (0x0100-0x0107)

- **0x0100** `ESP_NCP_ZCL_ENDPOINT_ADD`: Add an endpoint with clusters
- **0x0101** `ESP_NCP_ZCL_ENDPOINT_DEL`: Remove an endpoint
- **0x0102** `ESP_NCP_ZCL_ATTR_READ`: Read attribute value
- **0x0103** `ESP_NCP_ZCL_ATTR_WRITE`: Write attribute value
- **0x0104** `ESP_NCP_ZCL_ATTR_REPORT`: Configure attribute reporting

#### ZDO Operations (0x0200-0x0202)

- **0x0200** `ESP_NCP_ZDO_BIND_SET`: Create binding
- **0x0201** `ESP_NCP_ZDO_UNBIND_SET`: Remove binding
- **0x0202** `ESP_NCP_ZDO_FIND_MATCH`: Find matching devices

#### APS Data Transfer (0x0300-0x0302)

- **0x0300** `ESP_NCP_APS_DATA_REQUEST`: Send data to device
- **0x0301** `ESP_NCP_APS_DATA_INDICATION`: Receive data from device
- **0x0302** `ESP_NCP_APS_DATA_CONFIRM`: Get transmission confirmation

### Example: Forming a Network

**Host sends**:
```c
// 1. Initialize network
esp_ncp_header_t header = {
    .flags = {.version = 0, .type = 0},  // Request
    .id = ESP_NCP_NETWORK_INIT,
    .sn = 0x01,
    .len = 0
};
// Send frame...

// 2. Configure network parameters
header.id = ESP_NCP_NETWORK_PAN_ID_SET;
header.len = 2;
uint16_t pan_id = 0x1234;
// Send frame with pan_id payload...

// 3. Form network
header.id = ESP_NCP_NETWORK_FORMNETWORK;
header.len = sizeof(esp_zb_cfg_t);
esp_zb_cfg_t cfg = {
    .nwk_cfg = {
        .pan_id = 0x1234,
        .extended_pan_id = {0x01, 0x02, ...},
        // ...
    }
};
// Send frame with cfg payload...
```

**NCP processes**:
```c
// Handler: esp_ncp_zb_form_network_fn()
static esp_err_t esp_ncp_zb_form_network_fn(const uint8_t *input, 
                                             uint16_t inlen,
                                             uint8_t **output, 
                                             uint16_t *outlen)
{
    // 1. Copy config (don't keep pointer to ephemeral buffer)
    static esp_zb_cfg_t s_zb_cfg;
    memcpy(&s_zb_cfg, input, sizeof(s_zb_cfg));
    
    // 2. Initialize Zigbee stack
    esp_zb_init(&s_zb_cfg);
    s_zb_stack_inited = true;
    
    // 3. Set security defaults
    esp_zb_secur_TC_standard_preconfigure_key_set(s_tc_preconfigure_key);
    
    // 4. Apply channel masks
    if (s_channel_mask) {
        esp_zb_set_channel_mask(s_channel_mask);
    }
    
    // 5. Return success status
    esp_ncp_status_t status = ESP_NCP_SUCCESS;
    *output = calloc(1, sizeof(uint8_t));
    *outlen = sizeof(uint8_t);
    memcpy(*output, &status, *outlen);
    
    return ESP_OK;
}
```

**NCP sends notification** (when network forms):
```c
// Signal handler: esp_zb_app_signal_handler()
case ESP_ZB_BDB_SIGNAL_FORMATION:
    if (err_status == ESP_OK) {
        // Network formed successfully
        esp_ncp_zb_formnetwork_parameters_t params = {
            .panId = esp_zb_get_pan_id(),
            .radioChannel = esp_zb_get_current_channel(),
        };
        esp_zb_get_extended_pan_id(params.extendedPanId);
        
        // Send notification to host
        ncp_header.id = ESP_NCP_NETWORK_FORMNETWORK;
        esp_ncp_noti_input(&ncp_header, &params, sizeof(params));
    }
    break;
```

### Example: Adding an Endpoint

**Host sends**:
```c
typedef struct {
    uint8_t endpoint;              // e.g., 1
    uint16_t profileId;            // e.g., 0x0104 (ZHA)
    uint16_t deviceId;             // e.g., 0x0100 (On/Off Light)
    uint8_t appFlags;              // Device version
    uint8_t inputClusterCount;     // e.g., 2
    uint8_t outputClusterCount;     // e.g., 0
} esp_ncp_zb_endpoint_t;

esp_ncp_zb_endpoint_t endpoint = {
    .endpoint = 1,
    .profileId = 0x0104,
    .deviceId = 0x0100,
    .appFlags = 0,
    .inputClusterCount = 2,
    .outputClusterCount = 0
};

uint16_t input_clusters[] = {
    0x0000,  // Basic
    0x0006   // On/Off
};

// Build frame:
// - Header with ESP_NCP_ZCL_ENDPOINT_ADD
// - Payload: endpoint struct + cluster list
// - SLIP encode and send
```

**NCP processes**:
```c
// Handler: esp_ncp_zb_add_endpoint_fn()
static esp_err_t esp_ncp_zb_add_endpoint_fn(const uint8_t *input, ...)
{
    esp_ncp_zb_endpoint_t *ncp_endpoint = (esp_ncp_zb_endpoint_t *)input;
    
    // 1. Extract cluster lists
    uint16_t *input_clusters = calloc(1, inputClusterLength);
    memcpy(input_clusters, input + sizeof(esp_ncp_zb_endpoint_t), ...);
    
    // 2. Create cluster list
    esp_zb_cluster_list_t *cluster_list = esp_zb_zcl_cluster_list_create();
    
    // 3. Add clusters using function table
    for (each cluster_id in input_clusters) {
        for (each entry in cluster_list_fn_table) {
            if (cluster_id == entry.cluster_id) {
                entry.add_cluster_fn(cluster_list, attr_list, 
                                     ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
                break;
            }
        }
    }
    
    // 4. Create endpoint list and register
    esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();
    esp_zb_endpoint_config_t ep_config = {
        .endpoint = ncp_endpoint->endpoint,
        .app_profile_id = ncp_endpoint->profileId,
        .app_device_id = ncp_endpoint->deviceId,
    };
    esp_zb_ep_list_add_ep(ep_list, cluster_list, ep_config);
    esp_zb_device_register(ep_list);
    
    return ESP_OK;
}
```

### Example: Reading an Attribute

**Host sends**:
```c
typedef struct {
    esp_zb_zcl_basic_cmd_t zcl_basic_cmd;  // Destination, endpoints
    uint8_t address_mode;                   // Addressing mode
    uint16_t cluster_id;                    // e.g., 0x0006 (On/Off)
    uint8_t attr_number;                    // Number of attributes
} esp_ncp_zb_read_attr_t;

esp_ncp_zb_read_attr_t read_req = {
    .zcl_basic_cmd = {
        .dst_addr_u.addr_short = 0x0001,
        .dst_endpoint = 1,
        .src_endpoint = 1,
    },
    .address_mode = ESP_ZB_ZCL_ADDR_MODE_SHORT,
    .cluster_id = 0x0006,
    .attr_number = 1
};

uint16_t attr_ids[] = {0x0000};  // OnOff attribute

// Build frame and send...
```

**NCP processes**:
```c
// Handler: esp_ncp_zb_read_attr_fn()
static esp_err_t esp_ncp_zb_read_attr_fn(const uint8_t *input, ...)
{
    esp_ncp_zb_read_attr_t *read_attr = (esp_ncp_zb_read_attr_t *)input;
    uint16_t *attr_field = calloc(1, read_attr->attr_number * sizeof(uint16_t));
    memcpy(attr_field, input + sizeof(esp_ncp_zb_read_attr_t), ...);
    
    // Call Zigbee stack API
    esp_zb_zcl_read_attr_cmd_t read_req = {
        .zcl_basic_cmd = read_attr->zcl_basic_cmd,
        .address_mode = read_attr->address_mode,
        .clusterID = read_attr->cluster_id,
        .attr_number = read_attr->attr_number,
        .attr_field = attr_field,
    };
    
    esp_zb_zcl_read_attr_cmd_req(&read_req);
    
    // Response comes asynchronously via callback
    return ESP_OK;
}
```

**NCP sends response** (via callback):
```c
// Callback: esp_ncp_zb_read_attr_resp_handler()
static esp_err_t esp_ncp_zb_read_attr_resp_handler(...)
{
    // Build response with attribute values
    uint8_t *outbuf = calloc(1, ...);
    // Copy attribute IDs, types, values...
    
    // Send notification
    ncp_header.id = ESP_NCP_ZCL_ATTR_READ;
    esp_ncp_noti_input(&ncp_header, outbuf, outlen);
    
    return ESP_OK;
}
```

---

## API Reference

### Public API (`esp_zb_ncp.h`)

#### Initialization

```c
esp_err_t esp_ncp_init(esp_ncp_host_connection_mode_t mode);
```
Initialize NCP with specified connection mode (UART or SPI).

**Parameters**:
- `mode`: `NCP_HOST_CONNECTION_MODE_UART` or `NCP_HOST_CONNECTION_MODE_SPI`

**Returns**: `ESP_OK` on success

---

```c
esp_err_t esp_ncp_start(void);
```
Start NCP communication. Creates tasks and starts bus.

**Returns**: `ESP_OK` on success

---

```c
esp_err_t esp_ncp_stop(void);
```
Stop NCP communication. Currently a no-op.

**Returns**: `ESP_OK`

---

```c
esp_err_t esp_ncp_deinit(void);
```
De-initialize NCP. Cleans up resources.

**Returns**: `ESP_OK`

---

### Status Codes

```c
typedef enum {
    ESP_NCP_SUCCESS = 0x00,
    ESP_NCP_ERR_FATAL = 0x01,
    ESP_NCP_BAD_ARGUMENT = 0x02,
    ESP_NCP_ERR_NO_MEM = 0x03,
} esp_ncp_status_t;
```

### Network States

```c
typedef enum {
    ESP_NCP_OFFLINES = 0x00,
    ESP_NCP_JOINING = 0x01,
    ESP_NCP_CONNECTED = 0x02,
    ESP_NCP_LEAVING = 0x03,
    ESP_NCP_CONFIRM = 0x04,
    ESP_NCP_INDICATION = 0x05,
} esp_ncp_states_t;
```

### Command IDs Reference

See `esp_ncp_zb.h:65-127` for complete list. Key commands:

**Network**:
- `ESP_NCP_NETWORK_INIT` (0x0000)
- `ESP_NCP_NETWORK_START` (0x0001)
- `ESP_NCP_NETWORK_FORMNETWORK` (0x0004)
- `ESP_NCP_NETWORK_PERMIT_JOINING` (0x0005)

**ZCL**:
- `ESP_NCP_ZCL_ENDPOINT_ADD` (0x0100)
- `ESP_NCP_ZCL_ATTR_READ` (0x0102)
- `ESP_NCP_ZCL_ATTR_WRITE` (0x0103)

**APS**:
- `ESP_NCP_APS_DATA_REQUEST` (0x0300)
- `ESP_NCP_APS_DATA_INDICATION` (0x0301)

---

## Examples and Use Cases

### Use Case 1: Forming a Coordinator Network

**Scenario**: Host wants to create a new Zigbee network.

**Steps**:
1. Initialize NCP
2. Set network parameters (PAN ID, channel)
3. Form network
4. Permit joining
5. Handle device join events

**Host Code** (pseudocode):
```c
// 1. Initialize
esp_ncp_init(NCP_HOST_CONNECTION_MODE_UART);
esp_ncp_start();

// 2. Set PAN ID
send_command(ESP_NCP_NETWORK_PAN_ID_SET, &pan_id, sizeof(pan_id));

// 3. Form network
esp_zb_cfg_t cfg = { /* ... */ };
send_command(ESP_NCP_NETWORK_FORMNETWORK, &cfg, sizeof(cfg));

// 4. Wait for formation notification
wait_for_notification(ESP_NCP_NETWORK_FORMNETWORK);

// 5. Permit joining (60 seconds)
uint8_t seconds = 60;
send_command(ESP_NCP_NETWORK_PERMIT_JOINING, &seconds, sizeof(seconds));

// 6. Handle join events
while (running) {
    notification = wait_for_notification();
    if (notification.id == ESP_NCP_NETWORK_JOINNETWORK) {
        // Device joined!
        handle_device_join(notification.payload);
    }
}
```

### Use Case 2: Controlling a Light

**Scenario**: Host wants to turn on a Zigbee light.

**Steps**:
1. Send On/Off command via APS data request
2. Wait for confirmation
3. Optionally read attribute to verify

**Host Code** (pseudocode):
```c
// 1. Build APS data request
esp_zb_aps_data_t aps_data = {
    .basic_cmd = {
        .dst_addr_u.addr_short = light_addr,
        .dst_endpoint = 1,
        .src_endpoint = 1,
    },
    .dst_addr_mode = ESP_ZB_APS_ADDR_MODE_SHORT,
    .profile_id = 0x0104,  // ZHA
    .cluster_id = 0x0006,  // On/Off
    .tx_options = 0,
    .asdu_length = 3,      // Command: On (0x01)
};

uint8_t on_command[] = {0x01, 0x00, 0x00};  // On command

// 2. Send command
send_command(ESP_NCP_APS_DATA_REQUEST, &aps_data, 
             sizeof(aps_data) + sizeof(on_command));

// 3. Wait for confirmation
notification = wait_for_notification(ESP_NCP_APS_DATA_CONFIRM);
if (notification.confirm_status == 0) {
    // Success!
}
```

### Use Case 3: Reading Sensor Data

**Scenario**: Host wants to read temperature from a sensor.

**Steps**:
1. Read temperature attribute
2. Wait for response
3. Parse attribute value

**Host Code** (pseudocode):
```c
// 1. Build read request
esp_ncp_zb_read_attr_t read_req = {
    .zcl_basic_cmd = {
        .dst_addr_u.addr_short = sensor_addr,
        .dst_endpoint = 1,
        .src_endpoint = 1,
    },
    .address_mode = ESP_ZB_ZCL_ADDR_MODE_SHORT,
    .cluster_id = 0x0402,  // Temperature Measurement
    .attr_number = 1,
};

uint16_t attr_id = 0x0000;  // MeasuredValue

// 2. Send read request
send_command(ESP_NCP_ZCL_ATTR_READ, &read_req, 
             sizeof(read_req) + sizeof(attr_id));

// 3. Wait for response notification
notification = wait_for_notification(ESP_NCP_ZCL_ATTR_READ);

// 4. Parse response
// Response contains: cluster info + attribute count + 
//                     (attribute_id + type + size + value)...
int16_t temperature = parse_temperature(notification.payload);
```

---

## Debugging and Troubleshooting

### Common Issues

#### 1. UART Communication Fails

**Symptoms**: No responses from NCP, timeouts

**Debugging**:
- Check UART pins (TX/RX swapped?)
- Verify baud rate matches on both sides
- Check UART pattern detection is enabled
- Monitor UART events: `ESP_LOGI(TAG, "uart event type: %d", event.type)`

**Solution**:
```c
// Enable debug logging
esp_log_level_set("ESP_NCP_BUS", ESP_LOG_DEBUG);

// Check pattern detection
if (uart_enable_pattern_det_baud_intr(...) != ESP_OK) {
    ESP_LOGE(TAG, "Pattern detection failed!");
}
```

#### 2. SLIP Decoding Errors

**Symptoms**: `ESP_ERR_INVALID_SIZE`, corrupted frames

**Debugging**:
- Log raw UART bytes: `ESP_LOG_BUFFER_HEX_LEVEL(TAG, buffer, len, ESP_LOG_DEBUG)`
- Verify SLIP encoding on host side
- Check for missing `SLIP_END` delimiters

**Solution**:
```c
// In esp_ncp_frame_output():
ESP_LOG_BUFFER_HEX_LEVEL(TAG, buffer, len, ESP_LOG_DEBUG);  // Before decode
ESP_LOG_BUFFER_HEX_LEVEL(TAG, output, outlen, ESP_LOG_DEBUG);  // After decode
```

#### 3. CRC Mismatch

**Symptoms**: `ESP_ERR_INVALID_CRC`

**Debugging**:
- Verify CRC calculation matches on both sides
- Check for buffer overruns
- Verify payload length matches header

**Solution**:
```c
// Log CRC values
ESP_LOGI(TAG, "Calculated CRC: 0x%04x, Received CRC: 0x%04x", 
         crc_val, *checksum);
```

#### 4. Command Not Found

**Symptoms**: No response, or error status

**Debugging**:
- Verify command ID is in function table
- Check frame type (must be 0 for requests)
- Verify payload format matches handler expectations

**Solution**:
```c
// In esp_ncp_zb_output():
ESP_LOGW(TAG, "Command ID 0x%04x not found in function table", 
         ncp_header->id);
```

### Debugging Tools

#### 1. Enable Logging

```c
// Set log levels
esp_log_level_set("ESP_NCP_MAIN", ESP_LOG_DEBUG);
esp_log_level_set("ESP_NCP_BUS", ESP_LOG_DEBUG);
esp_log_level_set("ESP_NCP_FRAME", ESP_LOG_DEBUG);
esp_log_level_set("ESP_NCP_ZB", ESP_LOG_DEBUG);
```

#### 2. Monitor UART Events

```c
// In esp_ncp_bus_task():
ESP_LOGI(TAG, "UART event: type=%d, size=%d", event.type, event.size);
```

#### 3. Trace Frame Flow

```c
// In esp_ncp_frame_output():
ESP_LOGI(TAG, "Frame received: id=0x%04x, len=%d, type=%d", 
         ncp_header->id, ncp_header->len, ncp_header->flags.type);
```

#### 4. Use Wireshark

Capture UART traffic and analyze SLIP frames manually, or use a Zigbee sniffer to monitor network traffic.

---

## Conclusion

The ESP32 Zigbee NCP firmware provides a robust, well-architected solution for separating Zigbee stack complexity from host application logic. By understanding:

- **Architecture**: NCP vs host separation
- **Protocol**: SLIP framing, frame format, CRC
- **FreeRTOS**: Task structure, synchronization
- **Commands**: Function table, handlers, responses
- **Debugging**: Common issues and tools

Developers can effectively build host applications that control Zigbee networks through the NCP firmware.

### Key Takeaways

1. **NCP handles all Zigbee complexity** - Host only needs to send commands and handle events
2. **Protocol is layered** - UART → SLIP → Frame → Command → Zigbee Stack
3. **FreeRTOS enables concurrency** - Multiple tasks handle UART, processing, and Zigbee stack
4. **Commands are synchronous** - Request/response pattern with sequence numbers
5. **Events are asynchronous** - Notifications arrive independently

### Further Reading

- ESP Zigbee SDK Documentation: https://docs.espressif.com/projects/esp-zigbee-sdk/
- Zigbee Specification: https://zigbeealliance.org/
- FreeRTOS Documentation: https://www.freertos.org/
- IEEE 802.15.4 Standard: https://standards.ieee.org/

---

*Document generated: 2026-01-06*
*Last updated: 2026-01-06*
