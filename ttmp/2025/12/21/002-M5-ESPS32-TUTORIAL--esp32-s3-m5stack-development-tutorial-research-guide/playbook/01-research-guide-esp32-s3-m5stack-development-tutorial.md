---
Title: 'Research Guide: ESP32-S3 M5Stack Development Tutorial'
Ticket: 002-M5-ESPS32-TUTORIAL
Status: active
Topics:
    - esp32
    - m5stack
    - freertos
    - esp-idf
    - tutorial
    - grove
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Comprehensive research guide for an intern to write a solid introduction to ESP32-S3 M5Stack development, focusing on FreeRTOS, Grove-type peripherals, and ESP-IDF, with local buildable examples"
LastUpdated: 2025-12-21T08:21:54.994864157-05:00
WhatFor: "Guide an intern through researching and documenting ESP32-S3 M5Stack development patterns, FreeRTOS usage, Grove sensor integration, and ESP-IDF workflows to produce a tutorial introduction"
WhenToUse: "When onboarding an intern to research and write ESP32-S3 M5Stack development documentation; use as a structured research plan with clear deliverables and validation examples"
---

# Research Guide: ESP32-S3 M5Stack Development Tutorial

## Purpose

This guide provides a structured research plan for an intern to write a comprehensive introduction to developing with ESP32-S3 M5Stack hardware. The tutorial should cover:

- **ESP-IDF fundamentals** (build system, components, configuration)
- **FreeRTOS patterns** (tasks, queues, synchronization, ISRs)
- **M5Stack ecosystem** (M5Unified, M5GFX, hardware abstraction)
- **Grove-type peripherals** (I2C sensors, GPIO devices, common patterns)
- **Practical examples** (buildable, testable code samples)

The output should be a tutorial that helps developers new to ESP32-S3 and M5Stack get productive quickly, with emphasis on real-world patterns found in our codebases.

## Context and Inspiration

This research guide is inspired by the book structure debate documented in `001-FIRMWARE-ARCHITECTURE-BOOK`. That debate explored different pedagogical approaches:

- **Didactic "Build It Up"**: Learn-by-building, incremental complexity
- **Educational "Concepts First"**: Platform mental model before code
- **Reference Manual**: Lookup-first, tables and contracts
- **Cookbook**: Task-first, practical recipes

For this tutorial, we recommend a **hybrid approach**: start with a guided learning path (didactic), but include reference sections and practical examples (cookbook).

### Existing Codebases to Study

The intern should study these repositories as source material:

1. **ATOMS3R-CAM-M12-UserDemo** (`ATOMS3R-CAM-M12-UserDemo/`)
   - ESP-IDF v5.1.x project structure
   - FreeRTOS task patterns (`main/service/service_uvc.cpp`, `main/service/service_web_server.cpp`)
   - I2C sensor integration (BMI270 IMU, BMM150 magnetometer)
   - Camera and UVC implementation
   - Shared state management (`main/utils/shared/shared.h`)
   - Build system (`CMakeLists.txt`, `main/Kconfig.projbuild`)

2. **M5Cardputer-UserDemo** (`M5Cardputer-UserDemo/`)
   - M5Unified/M5GFX usage patterns
   - HAL abstraction layer (`main/hal/hal_cardputer.cpp`)
   - FreeRTOS configuration (`sdkconfig` FreeRTOS settings)
   - App framework (`main/apps/`)
   - Peripheral drivers (display, keyboard, speaker, mic)

3. **echo-base--openai-realtime-embedded-sdk** (`echo-base--openai-realtime-embedded-sdk/`)
   - FreeRTOS task creation patterns (`src/webrtc.cpp`)
   - Audio processing with FreeRTOS
   - Component-based architecture
   - ESP-IDF configuration examples (`sdkconfig.defaults`)

### Related Documentation

- `001-FIRMWARE-ARCHITECTURE-BOOK/design-doc/01-book-brainstorm-firmware-m12-platform.md`: Book structure debate
- `001-ANALYZE-ECHO-BASE/reference/01-research-diary.md`: Research methodology example
- `001-FIRMWARE-ARCHITECTURE-BOOK/playbook/01-research-brief-chapter-1-background.md`: Research brief template

## Scope (What to Research)

### Core Topics

1. **ESP-IDF Build System and Project Structure**
   - CMake component model
   - Kconfig configuration system
   - `idf.py` workflow
   - Component dependencies (`idf_component.yml`, `dependencies.lock`)
   - Partition tables (`partitions.csv`)

2. **FreeRTOS Fundamentals for ESP32-S3**
   - Task creation (`xTaskCreate`, `xTaskCreatePinnedToCore`)
   - Task priorities and scheduling
   - Queues (`xQueueCreate`, `xQueueSend`, `xQueueReceive`)
   - Semaphores and mutexes
   - Task notifications
   - ISR-safe APIs (`xQueueSendFromISR`, `xSemaphoreGiveFromISR`)
   - Stack sizing and monitoring
   - Dual-core considerations (core affinity)

3. **M5Stack Ecosystem**
   - M5Unified library (`M5Unified` component)
   - M5GFX graphics library
   - Hardware abstraction patterns (`hal_cardputer.cpp` style)
   - Common M5Stack boards (AtomS3R, Cardputer, Core, etc.)
   - Pin mapping conventions

4. **Grove-Type Peripheral Integration**
   - I2C sensor patterns (scan, init, read)
   - GPIO device patterns (buttons, LEDs, relays)
   - SPI device patterns (displays, SD cards)
   - Common Grove sensors (temperature, humidity, light, motion)
   - Grove connector pinouts and standards

5. **ESP32-S3 Specifics**
   - Memory types (IRAM, DRAM, PSRAM)
   - Dual-core architecture
   - WiFi and Bluetooth basics
   - USB Serial/JTAG interface
   - Pin configuration and GPIO matrix

### Out of Scope (Do NOT Spend Time Here)

- Deep USB/UVC protocol details (covered in M12 firmware book)
- Advanced WiFi/Bluetooth protocols (keep to basics)
- Proprietary IDE workflows (focus on ESP-IDF command-line)
- Hardware design/schematics (pinouts only)
- Speculative content without code examples

## Environment Assumptions

- **Access to codebases**: The intern has read access to the repositories listed above
- **ESP-IDF installed**: ESP-IDF v5.1.x or later is installed and configured
- **Hardware available**: At least one M5Stack ESP32-S3 board (AtomS3R, Cardputer, or similar)
- **Build tools**: `idf.py`, CMake, Ninja, Python 3
- **Documentation tools**: Markdown editor, ability to create code examples
- **Web access**: For researching official ESP-IDF and M5Stack documentation

## Deliverables

The intern should produce **one primary document** plus **example projects**:

### Primary Document: Tutorial Introduction

Create a markdown document: `tutorial-esp32s3-m5stack-introduction.md`

This document should contain:

#### 1. Getting Started (2-3 pages)
- ESP-IDF installation and setup
- Project structure overview (`main/`, `components/`, `CMakeLists.txt`)
- First "Hello World" (blink LED or serial output)
- Build and flash workflow (`idf.py build`, `idf.py flash`, `idf.py monitor`)

#### 2. ESP-IDF Fundamentals (3-4 pages)
- Component model (what is a component, how to create one)
- Kconfig system (`Kconfig.projbuild`, `sdkconfig`)
- CMake basics (`CMakeLists.txt` structure)
- Project configuration (`sdkconfig.defaults`)

#### 3. FreeRTOS Basics (4-5 pages)
- Task creation and management
- Task priorities and scheduling
- Inter-task communication (queues, semaphores)
- ISR handling and ISR-safe APIs
- Stack monitoring and debugging
- Dual-core considerations
- **Code examples**: At least 3 working examples showing different patterns

#### 4. M5Stack Ecosystem (3-4 pages)
- M5Unified library overview
- M5GFX graphics basics
- HAL abstraction patterns
- Common M5Stack board features
- Pin mapping conventions

#### 5. Grove-Type Peripheral Integration (4-5 pages)
- I2C sensor integration pattern
- GPIO device patterns
- Common Grove sensors (with examples)
- Grove connector standards
- **Code examples**: At least 2 working sensor examples

#### 6. Practical Patterns and Best Practices (2-3 pages)
- Memory management (IRAM, DRAM, PSRAM)
- Error handling patterns
- Logging and debugging
- Performance considerations
- Common pitfalls and how to avoid them

#### 7. Example Projects Reference (1-2 pages)
- List of all example projects with descriptions
- How to build and run each example
- What each example demonstrates

### Example Projects (Buildable Code)

Create **at least 5 working example projects** that can be built and flashed:

1. **example-01-hello-world**
   - Minimal ESP-IDF project
   - Serial output
   - Blink LED (if available)

2. **example-02-freertos-tasks**
   - Multiple tasks with different priorities
   - Task communication via queue
   - Stack monitoring

3. **example-03-freertos-isr**
   - GPIO interrupt handling
   - ISR-safe queue communication
   - Button debouncing

4. **example-04-m5stack-display**
   - M5GFX display initialization
   - Basic graphics (text, shapes)
   - Display refresh patterns

5. **example-05-grove-i2c-sensor**
   - I2C bus initialization
   - Sensor reading (temperature, humidity, or similar)
   - FreeRTOS task for periodic reading
   - Serial output of sensor data

**Optional additional examples** (if time permits):
- **example-06-dual-core**: Core affinity, inter-core communication
- **example-07-wifi-basic**: WiFi connection, HTTP client
- **example-08-m5stack-hal**: HAL abstraction pattern demonstration

Each example should:
- Be self-contained (can be built independently)
- Include a `README.md` with build instructions
- Have clear, commented code
- Demonstrate specific concepts from the tutorial
- Be testable on real hardware

## Research Methodology

### Step 1: Codebase Analysis

**What to do:**
- Read through the three codebases listed above
- Identify common patterns:
  - How FreeRTOS tasks are created
  - How I2C sensors are initialized
  - How M5Stack libraries are used
  - How components are structured
- Document findings in a research diary (see format below)

**Where to look:**
- `ATOMS3R-CAM-M12-UserDemo/main/service/service_uvc.cpp`: FreeRTOS task patterns
- `ATOMS3R-CAM-M12-UserDemo/main/utils/shared/shared.h`: Synchronization patterns
- `M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp`: HAL abstraction
- `M5Cardputer-UserDemo/main/apps/`: Application patterns
- `echo-base--openai-realtime-embedded-sdk/src/webrtc.cpp`: Task creation with PSRAM

**Research diary format** (create `reference/01-research-diary.md`):
```markdown
## Step N: [Topic]

### What I did
- [Actions taken]

### Why
- [Reasoning]

### What worked
- [Successful findings]

### What I learned
- [Key insights]

### What was tricky
- [Challenges encountered]

### Code examples found
- [File paths and line numbers]
```

### Step 2: Official Documentation Research

**What to do:**
- Read ESP-IDF official documentation:
  - [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/)
  - [FreeRTOS API Reference](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/system/freertos.html)
  - [Component CMakeLists](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/build-system.html)
- Read M5Stack documentation:
  - M5Unified library docs
  - M5GFX library docs
  - Board-specific documentation
- Research Grove connector standards and common sensors

**Where to look:**
- ESP-IDF docs: `https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/`
- M5Stack docs: `https://docs.m5stack.com/`
- Grove docs: `https://wiki.seeedstudio.com/Grove_System/`

**Citation format:**
- Use markdown links: `[ESP-IDF FreeRTOS Guide](https://docs.espressif.com/...)`
- Cite at end of paragraphs or sections

### Step 3: Pattern Extraction

**What to do:**
- Extract common code patterns from codebases
- Create reusable code snippets
- Document pattern variations
- Identify best practices vs. anti-patterns

**Patterns to extract:**
- FreeRTOS task creation (static vs dynamic, core affinity)
- I2C sensor initialization sequence
- Queue-based inter-task communication
- ISR handler patterns
- M5Stack display initialization
- Error handling patterns

### Step 4: Example Project Development

**What to do:**
- Build each example project incrementally
- Test on real hardware
- Document build process
- Capture any gotchas or hardware-specific notes

**Validation criteria:**
- Each example builds without errors
- Each example flashes successfully
- Each example runs and produces expected output
- Code is commented and follows ESP-IDF style guide

### Step 5: Tutorial Writing

**What to do:**
- Write tutorial sections in order
- Include code examples from both codebases and new examples
- Cross-reference between sections
- Add "Try It Yourself" exercises where appropriate

**Writing style:**
- Clear, step-by-step instructions
- Explain "why" not just "how"
- Include common pitfalls
- Use code references to actual files when possible
- Keep examples minimal but complete

## Detailed Research Checklist

Use this checklist to ensure comprehensive coverage. For each item:
- Extract **2-5 key facts** for the tutorial
- Capture **1-3 code examples** from codebases
- Find **1-2 official documentation citations**
- Create **at least one working example** demonstrating the concept

### A) ESP-IDF Build System

**What to extract:**
- Component model (what is a component, how components interact)
- CMake structure (`CMakeLists.txt` at project and component level)
- Kconfig system (`Kconfig.projbuild`, `sdkconfig` generation)
- `idf.py` workflow (build, flash, monitor, menuconfig)
- Component dependencies (`idf_component.yml`)

**Where to look:**
- `ATOMS3R-CAM-M12-UserDemo/CMakeLists.txt`
- `ATOMS3R-CAM-M12-UserDemo/main/CMakeLists.txt`
- `ATOMS3R-CAM-M12-UserDemo/main/Kconfig.projbuild`
- ESP-IDF docs: Build System guide

**Code examples to find:**
- Component `CMakeLists.txt` structure
- Kconfig option definitions
- Component dependency declarations

### B) FreeRTOS Task Management

**What to extract:**
- Task creation (`xTaskCreate`, `xTaskCreatePinnedToCore`, `xTaskCreateStatic`)
- Task priorities (configMAX_PRIORITIES, typical priority levels)
- Task scheduling (preemptive, round-robin)
- Task deletion and cleanup
- Stack sizing (how to determine, monitoring)
- Core affinity (when to use, how to set)

**Where to look:**
- `echo-base--openai-realtime-embedded-sdk/src/webrtc.cpp:18-54` (task creation with PSRAM)
- `ATOMS3R-CAM-M12-UserDemo/main/service/service_uvc.cpp` (UVC service task)
- `ATOMS3R-CAM-M12-UserDemo/main/service/service_web_server.cpp` (web server task)
- ESP-IDF docs: FreeRTOS Task API

**Code examples to find:**
- Dynamic task creation
- Static task creation with PSRAM stack
- Task with core affinity
- Task deletion pattern

### C) FreeRTOS Inter-Task Communication

**What to extract:**
- Queues (`xQueueCreate`, `xQueueSend`, `xQueueReceive`, blocking behavior)
- Semaphores (binary, counting, mutex)
- Task notifications (lightweight alternative to queues)
- Event groups (`xEventGroupCreate`, `xEventGroupSetBits`, `xEventGroupWaitBits`)
- When to use each mechanism

**Where to look:**
- `ATOMS3R-CAM-M12-UserDemo/main/utils/shared/shared.h` (shared state with mutex)
- ESP-IDF docs: FreeRTOS Queue API, Semaphore API
- FreeRTOS official docs: Task Notifications

**Code examples to find:**
- Queue-based producer-consumer pattern
- Mutex for shared resource protection
- Event group for multi-task synchronization

### D) FreeRTOS ISR Handling

**What to extract:**
- ISR handler registration (`gpio_set_intr_type`, `gpio_isr_handler_add`)
- ISR-safe APIs (`xQueueSendFromISR`, `xSemaphoreGiveFromISR`)
- ISR restrictions (what you can't do in ISR)
- Deferred interrupt processing (using tasks)
- IRAM considerations (`IRAM_ATTR`)

**Where to look:**
- `M5Cardputer-UserDemo/main/hal/button/Button.cpp` (button interrupt handling)
- `ATOMS3R-CAM-M12-UserDemo/main/utils/ir_nec_transceiver/ir_nec_transceiver.c` (RMT ISR)
- ESP-IDF docs: GPIO Interrupts, ISR Restrictions

**Code examples to find:**
- GPIO interrupt handler
- ISR-safe queue communication
- Deferred interrupt processing task

### E) M5Stack Library Usage

**What to extract:**
- M5Unified initialization (`M5.begin()`)
- M5GFX display initialization and usage
- HAL abstraction patterns
- Common M5Stack APIs (buttons, power, etc.)

**Where to look:**
- `M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp` (HAL initialization)
- `M5Cardputer-UserDemo/main/hal/display/hal_display.hpp` (display abstraction)
- M5Stack official docs

**Code examples to find:**
- M5Unified initialization sequence
- M5GFX display drawing
- HAL abstraction pattern

### F) I2C Sensor Integration

**What to extract:**
- I2C bus initialization (`i2c_driver_install`, `i2c_param_config`)
- I2C device scanning
- Sensor initialization sequence
- Reading sensor data (blocking vs. non-blocking)
- Error handling (device not found, read errors)

**Where to look:**
- `ATOMS3R-CAM-M12-UserDemo/main/usb_webcam_main.cpp:49-57` (I2C scan)
- `ATOMS3R-CAM-M12-UserDemo/main/utils/bmi270/src/bmi270.cpp` (BMI270 sensor)
- ESP-IDF docs: I2C Driver API

**Code examples to find:**
- I2C bus initialization
- I2C device scan
- Sensor read pattern

### G) Grove Connector Standards

**What to extract:**
- Grove connector pinout (VCC, GND, SDA, SCL for I2C)
- Common Grove I2C sensors
- Grove digital/analog sensors
- Grove connector variants (4-pin, 6-pin)

**Where to look:**
- Seeed Studio Grove documentation
- Grove sensor libraries
- M5Stack Grove unit documentation

**Code examples to find:**
- Grove I2C sensor connection
- Grove digital sensor reading

### H) ESP32-S3 Memory Management

**What to extract:**
- Memory types (IRAM, DRAM, PSRAM)
- Heap allocation (`heap_caps_malloc`, `MALLOC_CAP_SPIRAM`)
- Stack allocation (PSRAM stacks for tasks)
- Cache considerations
- Memory constraints and best practices

**Where to look:**
- `echo-base--openai-realtime-embedded-sdk/src/webrtc.cpp:41-51` (PSRAM stack allocation)
- ESP-IDF docs: Memory Types, External RAM

**Code examples to find:**
- PSRAM allocation for task stack
- Heap capability flags usage

## Source Quality Rules

- **Prefer official vendor docs**: ESP-IDF docs, M5Stack docs, FreeRTOS docs
- **Code examples from real projects**: Use examples from the three codebases
- **Cite sources**: Every non-trivial claim needs a citation
- **Validate examples**: All code examples should be tested on real hardware

## Citation Format

Use markdown links for citations:

```markdown
ESP32-S3 supports dual-core Xtensa LX7 processors. ([ESP-IDF ESP32-S3 Overview](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/))
```

For code references, use the code reference format:

```12:25:ATOMS3R-CAM-M12-UserDemo/main/service/service_uvc.cpp
// Task creation example
xTaskCreate(uvc_task, "uvc_service", 4096, NULL, 5, NULL);
```

## Example Projects Specification

### Example 1: Hello World

**Purpose**: Minimal ESP-IDF project, serial output, LED blink

**Files:**
- `main/main.c` (or `main.cpp`)
- `CMakeLists.txt`
- `README.md`

**Demonstrates:**
- ESP-IDF project structure
- `app_main()` entry point
- Serial logging (`ESP_LOGI`)
- GPIO control (LED)
- Build and flash workflow

**Expected output:**
- Serial log: "Hello from ESP32-S3!"
- LED blinks every second

### Example 2: FreeRTOS Tasks

**Purpose**: Multiple tasks, queue communication, priority demonstration

**Files:**
- `main/main.c`
- `main/tasks.h`, `main/tasks.c`
- `CMakeLists.txt`
- `README.md`

**Demonstrates:**
- Task creation (`xTaskCreate`)
- Queue creation and usage (`xQueueCreate`, `xQueueSend`, `xQueueReceive`)
- Task priorities
- Task delays (`vTaskDelay`)

**Expected output:**
- Serial log showing task execution order
- Queue messages passed between tasks

### Example 3: FreeRTOS ISR

**Purpose**: GPIO interrupt, ISR-safe communication

**Files:**
- `main/main.c`
- `main/isr_handler.c`, `main/isr_handler.h`
- `CMakeLists.txt`
- `README.md`

**Demonstrates:**
- GPIO interrupt setup
- ISR handler (`IRAM_ATTR`)
- ISR-safe queue (`xQueueSendFromISR`)
- Deferred processing task

**Expected output:**
- Button press triggers interrupt
- ISR sends message via queue
- Task processes message

### Example 4: M5Stack Display

**Purpose**: M5GFX display initialization and basic graphics

**Files:**
- `main/main.cpp`
- `CMakeLists.txt`
- `idf_component.yml` (M5Unified dependency)
- `README.md`

**Demonstrates:**
- M5Unified initialization
- M5GFX display setup
- Drawing text and shapes
- Display refresh

**Expected output:**
- Display shows "Hello M5Stack!"
- Animated graphics or shapes

### Example 5: Grove I2C Sensor

**Purpose**: I2C sensor reading (temperature, humidity, or similar)

**Files:**
- `main/main.c`
- `main/sensor.c`, `main/sensor.h`
- `CMakeLists.txt`
- `README.md`

**Demonstrates:**
- I2C bus initialization
- I2C device scanning
- Sensor initialization
- Periodic reading in FreeRTOS task
- Serial output of sensor data

**Expected output:**
- Serial log: Sensor readings every second
- Format: "Temperature: 25.3°C, Humidity: 60.2%"

**Hardware requirements:**
- Grove I2C sensor (e.g., DHT12, SHT30, or similar)
- Grove connector or I2C breakout

## Exit Criteria

The intern is done when:

1. **Research diary complete**: `reference/01-research-diary.md` documents all research steps
2. **Tutorial document complete**: `tutorial-esp32s3-m5stack-introduction.md` contains all required sections
3. **All examples build**: Each example project builds without errors (`idf.py build`)
4. **All examples tested**: Each example has been flashed and tested on hardware
5. **Code examples validated**: All code snippets in tutorial are tested and working
6. **Citations complete**: All non-trivial claims have citations
7. **Review ready**: Document is ready for technical review

## Timeline Estimate

- **Week 1**: Codebase analysis, research diary setup, ESP-IDF fundamentals research
- **Week 2**: FreeRTOS research, pattern extraction, example projects 1-2
- **Week 3**: M5Stack research, Grove research, example projects 3-4
- **Week 4**: Example project 5, tutorial writing, review and refinement

**Total**: ~4 weeks for a thorough tutorial with 5+ examples

## Notes

### Reader Persona

This tutorial is for:
- **Developers new to ESP32-S3** but familiar with embedded systems
- **Developers new to FreeRTOS** but familiar with RTOS concepts
- **Developers new to M5Stack** but familiar with Arduino/embedded hardware
- **Developers who learn by doing** (hands-on examples are critical)

### What "Good" Looks Like

The tutorial should:
- **Be actionable**: Readers can follow along and build examples
- **Explain "why"**: Not just "how" but reasoning behind patterns
- **Reference real code**: Point to actual files in our codebases
- **Include gotchas**: Common pitfalls and how to avoid them
- **Be testable**: All examples work on real hardware

### Quality Checklist

Before submitting:
- [ ] All code examples compile
- [ ] All examples tested on hardware
- [ ] All citations are valid links
- [ ] Code references use correct format (`startLine:endLine:filepath`)
- [ ] Examples include README with build instructions
- [ ] Tutorial flows logically (can be read start-to-finish)
- [ ] Each section builds on previous sections
- [ ] Common pitfalls are documented

## Commands

### Setup Research Environment

```bash
# Clone or access the codebases
cd /path/to/workspace

# Set up ESP-IDF (if not already done)
source ~/esp/esp-idf/export.sh

# Verify ESP-IDF version
idf.py --version

# Create example project directories
mkdir -p examples/example-01-hello-world/main
mkdir -p examples/example-02-freertos-tasks/main
# ... etc
```

### Build and Test Examples

```bash
# For each example project
cd examples/example-XX-name
idf.py build
idf.py flash
idf.py monitor
```

### Research Workflow

```bash
# Create research diary
touch reference/01-research-diary.md

# Create tutorial document
touch tutorial-esp32s3-m5stack-introduction.md

# As you research, update diary
# As you write, update tutorial
# As you build examples, test them
```

## Related Files to Reference

When writing the tutorial, reference these files as examples:

- **FreeRTOS task creation**: `echo-base--openai-realtime-embedded-sdk/src/webrtc.cpp:18-54`
- **Shared state with mutex**: `ATOMS3R-CAM-M12-UserDemo/main/utils/shared/shared.h`
- **I2C sensor init**: `ATOMS3R-CAM-M12-UserDemo/main/usb_webcam_main.cpp:49-80`
- **HAL abstraction**: `M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp`
- **Component structure**: `ATOMS3R-CAM-M12-UserDemo/main/CMakeLists.txt`
- **Kconfig example**: `ATOMS3R-CAM-M12-UserDemo/main/Kconfig.projbuild`

## Success Metrics

The tutorial is successful if:
1. A developer new to ESP32-S3 can follow it and build working examples
2. The examples demonstrate real-world patterns (not toy examples)
3. The tutorial references actual code from our codebases
4. Readers can extend the examples to build their own projects
5. The tutorial serves as a reference for common patterns
