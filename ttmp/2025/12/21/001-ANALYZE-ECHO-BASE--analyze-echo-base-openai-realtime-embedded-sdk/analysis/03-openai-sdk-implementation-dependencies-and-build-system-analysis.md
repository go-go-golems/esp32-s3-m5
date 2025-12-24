---
Title: OpenAI SDK Implementation, Dependencies, and Build System Analysis
Ticket: 001-ANALYZE-ECHO-BASE
Status: active
Topics:
    - analysis
    - esp32
    - openai
    - realtime
    - embedded-sdk
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Detailed analysis of the OpenAI Realtime Embedded SDK implementation, its dependencies, build system, and source code organization"
LastUpdated: 2025-12-21T08:09:47.623981758-05:00
WhatFor: "Understanding how the SDK is implemented, what dependencies it uses, how it's built, and where source code comes from"
WhenToUse: "When modifying the SDK, adding dependencies, troubleshooting build issues, or understanding the project structure"
---

# OpenAI SDK Implementation, Dependencies, and Build System Analysis

## Executive Summary

This document provides a comprehensive analysis of the **OpenAI Realtime Embedded SDK** implementation, examining its architecture, dependencies, build system, and source code organization. The SDK is a minimal embedded firmware implementation that enables ESP32-S3 devices to establish real-time bidirectional voice communication with OpenAI's Realtime API using WebRTC technology.

**Critical Understanding**: This SDK does NOT use an official OpenAI SDK library. While OpenAI provides official SDKs for languages like Python, JavaScript, Java, and Swift (documented at [OpenAI's API documentation](https://platform.openai.com/docs/api-reference)), those SDKs are designed for REST API interactions (Chat, Completions, Embeddings, etc.) and do not support the Realtime API's WebRTC-based architecture. Instead, this embedded SDK implements a complete WebRTC stack from scratch, using direct HTTP POST requests for Session Description Protocol (SDP) offer/answer exchange with OpenAI's servers, and then establishing peer-to-peer WebRTC connections for encrypted audio streaming.

The SDK represents a self-contained implementation that bridges the gap between embedded systems and OpenAI's cloud-based Realtime API. It handles all the complexity of WebRTC connection establishment, including Interactive Connectivity Establishment (ICE) for NAT traversal, Datagram Transport Layer Security (DTLS) handshaking for secure key exchange, Secure Real-time Transport Protocol (SRTP) for encrypted media transport, and Real-time Transport Protocol (RTP) for packetized audio delivery. All of this is implemented using the libpeer library, which provides a portable WebRTC implementation written in C specifically for IoT and embedded devices. For more information on WebRTC fundamentals, developers can refer to the [WebRTC specification](https://www.w3.org/TR/webrtc/) and the [MDN WebRTC documentation](https://developer.mozilla.org/en-US/docs/Web/API/WebRTC_API).

The project is licensed under the MIT License with copyright held by OpenAI (Copyright 2024), indicating this is an official OpenAI project designed to enable embedded device integration with their Realtime API service. This makes it particularly valuable for developers building voice-enabled IoT devices, smart speakers, or other embedded systems that need to interact with OpenAI's conversational AI capabilities in real-time. For developers new to OpenAI's Realtime API, the [official Realtime API documentation](https://platform.openai.com/docs/guides/realtime) provides comprehensive information about the API's capabilities and usage patterns.

## SDK Implementation Overview

### What This SDK Is

The **OpenAI Realtime Embedded SDK** represents a complete embedded firmware solution designed specifically for ESP32-S3 microcontrollers. It provides real-time bidirectional voice communication capabilities, enabling embedded devices to have natural conversations with OpenAI's Realtime API. The SDK handles the entire communication stack, from capturing audio from a microphone via I2S interfaces, encoding it using the Opus codec optimized for voice, establishing secure WebRTC connections with OpenAI's servers, and streaming audio bidirectionally with minimal latency.

The implementation is particularly notable for its comprehensive WebRTC stack integration. Unlike typical embedded applications that might use simpler protocols, this SDK implements the full WebRTC specification, including Interactive Connectivity Establishment (ICE) for handling network address translation and firewall traversal, Datagram Transport Layer Security (DTLS) for establishing secure communication channels, Secure Real-time Transport Protocol (SRTP) for encrypting audio packets, and Real-time Transport Protocol (RTP) for packetizing and delivering audio streams. This complete implementation ensures compatibility with OpenAI's infrastructure while maintaining the security and reliability standards required for production voice applications.

The SDK also includes sophisticated audio processing capabilities. It supports two different board configurations: a default ESP32-S3 setup running at 8kHz sample rate, and an M5Stack ATOMS3R board with EchoBase hardware running at 16kHz. The audio pipeline includes both encoding (for sending voice to OpenAI) and decoding (for playing OpenAI's responses), with the Opus codec configured specifically for Voice over IP (VoIP) applications with optimized bitrate and complexity settings for embedded systems.

### What This SDK Is NOT

It's important to clarify what this SDK is not, as there can be confusion about its relationship to OpenAI's official SDK offerings. This SDK is **not a wrapper** around an official OpenAI SDK library. OpenAI's official SDKs for Python, JavaScript, Java, Swift, and C++ are designed for REST API interactions and do not support the WebRTC-based Realtime API architecture. Those SDKs handle HTTP requests for text-based interactions, but the Realtime API requires a fundamentally different approach using peer-to-peer WebRTC connections.

The SDK is also **not using** OpenAI's official SDKs in any way. There are no imports or dependencies on OpenAI's Python `openai` package, JavaScript `openai` npm package, or any other official SDK. Instead, it implements direct HTTP POST requests to OpenAI's Realtime API endpoint for signaling, and then uses the libpeer WebRTC library for establishing the actual media connection.

Furthermore, this is **not a client library** in the traditional sense. It's a complete embedded firmware implementation that runs directly on the ESP32-S3 microcontroller. Unlike client libraries that you import into your application, this SDK IS the application - it provides the main entry point (`app_main()`), manages the entire system lifecycle, and handles all hardware interactions directly.

Interestingly, the SDK does **not use** WebSocket-based signaling, which is common in many WebRTC implementations. Instead, it uses simple HTTP POST requests to exchange Session Description Protocol (SDP) offers and answers. This approach is simpler and more suitable for embedded systems with limited resources, though it does require the device to initiate the connection rather than maintaining a persistent WebSocket connection.

### Implementation Approach

The SDK's implementation approach reflects the unique requirements of embedded systems and the constraints of the ESP32-S3 platform. The architecture is built around four key pillars that work together to enable real-time voice communication with OpenAI's cloud infrastructure.

**HTTP Signaling** forms the foundation of the connection establishment process. When the device starts up and completes WiFi connection, it creates a WebRTC offer containing its network capabilities, supported codecs, and ICE candidates. This offer is formatted as a Session Description Protocol (SDP) message and sent directly to OpenAI's Realtime API endpoint via an HTTP POST request. OpenAI's servers respond with an SDP answer containing their own capabilities and network information. This simple request-response pattern avoids the complexity of maintaining persistent WebSocket connections while still enabling the WebRTC handshake.

**WebRTC Stack** implementation is handled entirely by the libpeer library, which provides a portable WebRTC implementation written in C specifically for embedded devices. This library abstracts away the complexity of ICE candidate gathering, DTLS handshaking, SRTP key derivation, and RTP packet handling. The SDK integrates libpeer through a component wrapper that customizes its configuration for this specific use case, disabling features like SCTP (used for data channels, which aren't needed) and keepalives (to reduce network overhead).

**Audio Pipeline** implementation handles the complete audio processing workflow. Audio is captured from the microphone via I2S interfaces, which provide high-quality digital audio input/output. The captured audio is encoded using the Opus codec, which is specifically designed for real-time voice communication and provides excellent compression while maintaining voice quality. The encoded audio is then packetized into RTP packets, encrypted with SRTP, and sent over the WebRTC connection. In the reverse direction, incoming audio packets are decrypted, decoded from Opus format, and played through speakers or headphones via I2S output.

**Platform Abstraction** allows the SDK to support both ESP32-S3 hardware targets and Linux builds for development and testing. The ESP-IDF framework provides the core platform abstraction, handling hardware drivers, networking, and system services. For Linux builds, compatibility layers provide stubs for ESP-IDF APIs and FreeRTOS functionality, allowing developers to test the WebRTC connection logic without physical hardware. This dual-target approach significantly accelerates development and debugging cycles.

## Dependencies Analysis

Understanding the SDK's dependency structure is crucial for building, maintaining, and modifying the project. The dependencies are organized into three distinct categories, each with different management strategies and update mechanisms. This organization reflects the different types of code integration: external open-source libraries that need version control, ESP-IDF ecosystem components that can be automatically managed, and core platform functionality that comes with the ESP-IDF framework itself.

### Dependency Categories

The SDK's dependency management follows a hierarchical approach that balances reproducibility, ease of updates, and integration with the ESP-IDF ecosystem. **Git Submodules** represent external repositories that are directly integrated into the project's source tree. These are typically third-party libraries that have been ported or adapted for ESP32, and they're tracked at specific commits to ensure reproducible builds. **ESP-IDF Managed Components** are components available through Espressif's Component Manager registry, which provides automatic dependency resolution and version management similar to package managers like npm or pip. Finally, **ESP-IDF Core Components** are built into the ESP-IDF framework itself and don't require separate dependency management - they're always available when using ESP-IDF.

This three-tier approach allows the project to leverage the ESP-IDF ecosystem while maintaining control over critical dependencies like the WebRTC library. Git submodules ensure that external libraries are pinned to specific versions, preventing unexpected breaking changes. Managed components provide convenience and automatic updates within version constraints. Core components offer stable, well-tested platform functionality that's guaranteed to be available.

### Git Submodules (`.gitmodules`)

The project tracks 4 external repositories as Git submodules:

#### 1. libpeer (`deps/libpeer`)

The libpeer library is the cornerstone of the SDK's WebRTC implementation. Located at `https://github.com/sean-der/libpeer`, this is actually a fork of the original `sepfy/libpeer` repository, which itself is a portable WebRTC implementation written in C specifically designed for IoT and embedded devices. The library provides a complete WebRTC stack implementation that would otherwise be extremely complex to implement from scratch.

libpeer's architecture is particularly well-suited for embedded systems because it's written in C (avoiding C++ overhead), uses BSD sockets for networking (widely supported), and is designed to work with limited resources. The library implements all the critical WebRTC protocols: Interactive Connectivity Establishment (ICE) for handling network address translation and firewall traversal, Datagram Transport Layer Security (DTLS) for secure key exchange, Secure Real-time Transport Protocol (SRTP) for encrypting media packets, and Real-time Transport Protocol (RTP) for packetizing audio and video streams.

The library also includes Session Description Protocol (SDP) generation and parsing, which is essential for the offer/answer exchange with OpenAI's servers. Additionally, it supports STUN (Session Traversal Utilities for NAT) and TURN (Traversal Using Relays around NAT) servers for handling complex network topologies, though the SDK currently uses an empty ICE servers array, relying only on host candidates.

The library is tracked as a Git submodule at a specific commit rather than a branch or tag, which ensures reproducible builds but requires manual updates. It's licensed under the MIT License, making it compatible with the SDK's own MIT license. The library has its own dependencies managed through ESP-IDF Component Manager: `sepfy/srtp` version 2.0.4 or higher for SRTP encryption, and `sepfy/usrsctp` version 0.9.5 or higher for SCTP support (though SCTP is actually disabled in the SDK's configuration).

#### 2. SRTP (`components/srtp`)

The SRTP (Secure Real-time Transport Protocol) library is a critical security component that provides encryption and authentication for RTP media packets. This particular version is a port of the standard libsrtp library specifically adapted for ESP32 platforms. SRTP is essential for WebRTC security, as it ensures that audio packets transmitted over the network are encrypted and cannot be intercepted or tampered with.

The library is sourced from `https://git@github.com/sepfy/esp_ports`, which appears to be a repository containing ESP32 ports of various networking libraries. The component is tracked at a specific commit to ensure reproducible builds, and according to its `idf_component.yml` file, it's version 2.3.0 of libsrtp. The library is used by libpeer for encrypting outbound RTP packets and decrypting inbound RTP packets, forming a critical part of the WebRTC security stack. For more information on SRTP, developers can refer to the [RFC 3711 specification](https://datatracker.ietf.org/doc/html/rfc3711) and the [libsrtp documentation](https://github.com/cisco/libsrtp).

#### 3. esp-libopus (`components/esp-libopus`)

The esp-libopus library, located at `https://github.com/XasWorks/esp-libopus.git`, is a port of the Opus audio codec specifically adapted for ESP32 microcontrollers. Opus is a highly efficient audio codec developed by the Xiph.Org Foundation and standardized by the Internet Engineering Task Force (IETF). It's particularly well-suited for real-time voice communication because it provides excellent compression ratios while maintaining high voice quality, and it's designed to handle packet loss gracefully.

The ESP32 port is particularly important because it uses a **fixed-point implementation** rather than floating-point arithmetic. ESP32 microcontrollers don't have hardware floating-point units, so floating-point operations are extremely slow (emulated in software). By using fixed-point arithmetic, the codec can run efficiently on the ESP32's integer-only CPU cores. This is configured through the `FIXED_POINT` compile definition, which enables fixed-point mode throughout the codec.

The library includes both encoder and decoder functionality, allowing the SDK to compress audio for transmission (encoding) and decompress received audio for playback (decoding). The build configuration disables the float API (`DISABLE_FLOAT_API`) to ensure only fixed-point functions are used, and sets optimization level O2 for performance. The library includes the complete Opus codec implementation, including the CELT (Constrained Energy Lapped Transform) component for high-quality audio and the SILK component optimized for speech, along with the main Opus source files that combine these technologies.

#### 4. esp-protocols (`components/esp-protocols`)

The esp-protocols component is Espressif's collection of protocol implementations and compatibility utilities. While this component contains many protocol implementations, the SDK specifically uses it for its Linux compatibility layer, which enables developers to build and test the WebRTC connection logic on Linux systems without requiring physical ESP32 hardware. This is an invaluable development tool that significantly accelerates the development and debugging cycle.

The Linux compatibility layer provides stubs and implementations for ESP-IDF APIs that don't exist natively on Linux. Specifically, the SDK uses `linux_compat/esp_timer` for timer functionality and `linux_compat/freertos` for FreeRTOS task and synchronization primitives. These compatibility layers allow the same codebase to compile and run on both ESP32 and Linux, though with limitations - the Linux build doesn't support audio capture/playback or WiFi functionality, focusing only on the WebRTC connection logic. This component is sourced from `https://github.com/espressif/esp-protocols.git`, Espressif's official repository for protocol implementations. For more information on ESP-IDF development practices, developers can refer to the [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/).

### ESP-IDF Managed Components

ESP-IDF Component Manager provides a convenient way to manage dependencies similar to package managers like npm or pip. Components are automatically downloaded from Espressif's component registry, with version constraints specified in `idf_component.yml` files. This approach simplifies dependency management and ensures compatibility with the ESP-IDF framework version being used.

#### Source Component (`src/idf_component.yml`)

The main application component declares its dependencies in `src/idf_component.yml`, which tells ESP-IDF Component Manager what external components are needed. The configuration specifies a minimum ESP-IDF version of 4.1.0 (though the actual resolved version is 5.4.1 according to `dependencies.lock`), and declares a dependency on the ES8311 audio codec driver.

**Managed Components:**

1. **es8311** (`espressif/es8311`)

The ES8311 is a high-performance audio codec chip commonly used in audio development boards. This managed component provides a complete driver for interfacing with ES8311 chips via I2C for control and I2S for audio data. The driver handles all the low-level details of configuring the codec, setting sample rates, adjusting volume levels, and configuring microphone inputs.

The component is only used when building for the M5 ATOMS3R board configuration (`CONFIG_OPENAI_BOARD_M5_ATOMS3R`), which includes EchoBase hardware that uses the ES8311 codec. For the default ESP32-S3 board configuration, the SDK uses direct I2S interfaces without a codec chip. The version constraint `^1.0.0~1` means any version compatible with 1.0.0 or higher, allowing automatic updates while maintaining API compatibility. The component is automatically downloaded to `managed_components/espressif__es8311/` during the build process. For more information on ESP-IDF Component Manager, see the [Component Manager documentation](https://docs.espressif.com/projects/idf-component-manager/en/latest/).

#### libpeer Component Dependencies

From `deps/libpeer/idf_component.yml`:
```yaml
dependencies:
  sepfy/srtp: ^2.0.4
  sepfy/usrsctp: ^0.9.5
```

**Note**: These are resolved by ESP-IDF Component Manager, but `usrsctp` is disabled in the peer component wrapper (`components/peer/CMakeLists.txt`).

### ESP-IDF Core Components

ESP-IDF includes a comprehensive set of core components that are always available when using the framework. These components don't require explicit dependency management - they're built into ESP-IDF itself and automatically linked when needed. The SDK leverages several of these core components for essential functionality.

The **esp_wifi** component provides the complete WiFi stack, handling everything from scanning for networks to establishing connections and managing power modes. It's used by the SDK to connect to WiFi networks so the device can reach OpenAI's servers over the Internet. The **esp_http_client** component provides a high-level HTTP client API built on top of ESP-IDF's networking stack, which the SDK uses to send SDP offers to OpenAI's Realtime API endpoint.

The **nvs_flash** (Non-Volatile Storage) component manages flash-based key-value storage, which is used by ESP-IDF for storing WiFi credentials, system configuration, and other persistent data. While the SDK currently embeds WiFi credentials at compile time, NVS could be used for runtime credential management in future versions. The **driver** component provides hardware abstraction for peripherals like I2S (for audio) and I2C (for codec control), abstracting away the low-level register manipulation.

The **esp_netif** component provides a network interface abstraction layer that sits between the application and the underlying network stack (WiFi, Ethernet, etc.), making it easier to write network code that works across different connection types. The **esp_event** component implements an event loop system that allows different parts of the system to communicate asynchronously through events, which is used extensively by ESP-IDF components for things like WiFi connection state changes.

The **mbedtls** component is a port of the mbed TLS (formerly PolarSSL) cryptographic library, providing TLS/DTLS functionality that libpeer uses for secure WebRTC connections. Finally, the **json** component provides cJSON, a lightweight JSON parser that libpeer uses for parsing SDP and other JSON-formatted data. All of these components are well-documented in the [ESP-IDF API Reference](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/).

### Dependency Tree

```
OpenAI Realtime Embedded SDK
├── Git Submodules
│   ├── libpeer (deps/libpeer)
│   │   ├── mbedtls (ESP-IDF core)
│   │   ├── srtp (components/srtp)
│   │   ├── cJSON (ESP-IDF core)
│   │   └── usrsctp (disabled)
│   ├── srtp (components/srtp)
│   ├── esp-libopus (components/esp-libopus)
│   └── esp-protocols (components/esp-protocols) [Linux only]
├── ESP-IDF Managed Components
│   └── es8311 (managed_components/espressif__es8311) [ATOMS3R only]
└── ESP-IDF Core Components
    ├── esp_wifi
    ├── esp_http_client
    ├── nvs_flash
    ├── driver
    ├── esp_netif
    ├── esp_event
    ├── mbedtls
    └── json
```

## Build System Analysis

The SDK's build system is built on top of the ESP-IDF (Espressif IoT Development Framework), which itself uses CMake as its build system. This choice reflects the modern direction of embedded development, moving away from traditional Makefiles toward more powerful and maintainable build systems. CMake provides cross-platform build configuration, better dependency management, and integration with modern development tools.

ESP-IDF's use of CMake brings several advantages to the SDK. It provides a component-based architecture where each piece of functionality (WiFi, HTTP client, drivers, etc.) is a separate component that can be included or excluded as needed. This modularity allows the SDK to include only the components it actually uses, reducing firmware size and complexity. CMake also handles the complex task of setting up cross-compilation toolchains, managing include paths, and linking libraries correctly for the ESP32 target architecture.

The build system is designed to be both powerful for advanced users and simple for basic usage. Developers can build the project with a single command (`idf.py build`) after setting environment variables, but the underlying CMake configuration files allow for extensive customization if needed. The system also supports multiple build targets (ESP32-S3 and Linux), allowing developers to test WebRTC connection logic without physical hardware.

### Build Configuration Files

The build system configuration is spread across multiple CMakeLists.txt files, following ESP-IDF's component-based architecture. Each component has its own CMakeLists.txt file that declares its source files, include directories, and dependencies. The root CMakeLists.txt file sets up the overall project configuration, including compile definitions, component directories, and platform-specific settings.

#### Root `CMakeLists.txt`

The root CMakeLists.txt file serves as the entry point for the build system. It sets up the overall project configuration, including compile-time definitions for API keys and WiFi credentials, component directories, and platform-specific settings. One notable aspect is that environment variables are read at build time and converted to compile definitions, which means credentials are baked into the firmware binary. This is a common pattern in embedded systems where runtime configuration storage may be limited.

**Key Configuration:**
```cmake
cmake_minimum_required(VERSION 3.19)

# Audio sending disabled (not performant enough)
add_compile_definitions(SEND_AUDIO=0)

# WiFi credentials (ESP32 only)
if(NOT IDF_TARGET STREQUAL linux)
  add_compile_definitions(WIFI_SSID="$ENV{WIFI_SSID}")
  add_compile_definitions(WIFI_PASSWORD="$ENV{WIFI_PASSWORD}")
endif()

# OpenAI API configuration
add_compile_definitions(OPENAI_API_KEY="$ENV{OPENAI_API_KEY}")
add_compile_definitions(OPENAI_REALTIMEAPI="https://api.openai.com/v1/realtime?model=gpt-4o-mini-realtime-preview-2024-12-17")

# Component directories
set(COMPONENTS src)
set(EXTRA_COMPONENT_DIRS "src" "components/srtp" "components/peer" "components/esp-libopus")

# Linux compatibility
if(IDF_TARGET STREQUAL linux)
  add_compile_definitions(LINUX_BUILD=1)
  list(APPEND EXTRA_COMPONENT_DIRS
    $ENV{IDF_PATH}/examples/protocols/linux_stubs/esp_stubs
    "components/esp-protocols/common_components/linux_compat/esp_timer"
    "components/esp-protocols/common_components/linux_compat/freertos"
  )
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(src)
```

The root CMakeLists.txt file serves as the entry point for the entire build system. It sets up the project structure, defines compile-time constants, and configures the component directories. One notable aspect is that it requires CMake version 3.19 or higher, which ensures access to modern CMake features needed for ESP-IDF integration.

The build system relies heavily on environment variables for configuration, which is a common pattern in embedded development. The WiFi credentials (`WIFI_SSID` and `WIFI_PASSWORD`) and the OpenAI API key (`OPENAI_API_KEY`) are read from the environment and embedded as compile-time definitions. This approach means that sensitive credentials are baked into the firmware binary, which has security implications but is necessary for headless embedded devices that can't prompt for credentials at runtime.

The OpenAI API URL is hardcoded with the specific model parameter (`gpt-4o-mini-realtime-preview-2024-12-17`), which means the SDK is tied to a specific model version. This is likely intentional, as different model versions might have different API requirements or capabilities. The Linux build configuration adds compatibility layers that provide stubs for ESP-IDF APIs and FreeRTOS functionality, allowing the WebRTC connection logic to be tested without ESP32 hardware.

#### Source Component `src/CMakeLists.txt`

**Configuration:**
```cmake
set(COMMON_SRC "webrtc.cpp" "main.cpp" "http.cpp")

if(IDF_TARGET STREQUAL linux)
  idf_component_register(
    SRCS ${COMMON_SRC}
    REQUIRES peer esp-libopus esp_http_client)
else()
  idf_component_register(
    SRCS ${COMMON_SRC} "wifi.cpp" "media.cpp"
    REQUIRES driver esp_wifi nvs_flash peer esp_psram esp-libopus esp_http_client)
endif()

# Compiler flag overrides for dependencies
idf_component_get_property(lib peer COMPONENT_LIB)
target_compile_options(${lib} PRIVATE -Wno-error=restrict)
target_compile_options(${lib} PRIVATE -Wno-error=stringop-truncation)

idf_component_get_property(lib srtp COMPONENT_LIB)
target_compile_options(${lib} PRIVATE -Wno-error=incompatible-pointer-types)

idf_component_get_property(lib esp-libopus COMPONENT_LIB)
target_compile_options(${lib} PRIVATE -Wno-error=maybe-uninitialized)
target_compile_options(${lib} PRIVATE -Wno-error=stringop-overread)
```

**Key Points:**
- Conditional source files (Linux vs. ESP32)
- Different component requirements per platform
- Compiler warning suppressions for dependencies

#### Peer Component Wrapper `components/peer/CMakeLists.txt`

**Configuration:**
```cmake
set(PEER_PROJECT_PATH "../../deps/libpeer")
file(GLOB CODES "${PEER_PROJECT_PATH}/src/*.c")

idf_component_register(
  SRCS ${CODES}
  INCLUDE_DIRS "${PEER_PROJECT_PATH}/src"
  REQUIRES mbedtls srtp json esp_netif
)

# Disable usrsctp (SCTP not used)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libpeer/src/config.h INPUT_CONTENT)
string(REPLACE "#define HAVE_USRSCTP" "" MODIFIED_CONTENT ${INPUT_CONTENT})
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libpeer/src/config.h ${MODIFIED_CONTENT})

# Disable KeepAlives
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libpeer/src/config.h INPUT_CONTENT)
string(REPLACE "#define KEEPALIVE_CONNCHECK 10000" "#define KEEPALIVE_CONNCHECK 0" MODIFIED_CONTENT ${INPUT_CONTENT})
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libpeer/src/config.h ${MODIFIED_CONTENT})

if(NOT IDF_TARGET STREQUAL linux)
  add_definitions("-DESP32")
endif()
add_definitions("-DHTTP_DO_NOT_USE_CUSTOM_CONFIG -DMQTT_DO_NOT_USE_CUSTOM_CONFIG -DDISABLE_PEER_SIGNALING=true")
```

**Key Points:**
- Wraps libpeer source files
- Modifies libpeer config.h at build time (disables SCTP, keepalives)
- Sets ESP32-specific definitions
- Disables libpeer's built-in signaling (uses custom HTTP signaling)

### SDK Configuration (`sdkconfig.defaults`)

The `sdkconfig.defaults` file contains default configuration values for ESP-IDF's Kconfig-based configuration system. These defaults are applied when the project is first configured or when `idf.py reconfigure` is run. Developers can override these defaults using `idf.py menuconfig` or by directly editing the `sdkconfig` file that's generated during configuration.

The SDK's default configuration is optimized for performance and development convenience, though some settings (particularly security-related ones) should be changed for production deployments. The configuration disables some safety features like watchdogs and certificate verification to simplify development, but these should be re-enabled for production use.

**Key Settings:**
```ini
# ESP Event Loop
CONFIG_ESP_EVENT_POST_FROM_ISR=n
CONFIG_ESP_EVENT_POST_FROM_IRAM_ISR=n

# TLS (Development - Insecure!)
CONFIG_ESP_TLS_INSECURE=y
CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY=y

# DTLS-SRTP
CONFIG_MBEDTLS_SSL_PROTO_DTLS=y

# Stack Size
CONFIG_ESP_MAIN_TASK_STACK_SIZE=16384

# Partition Table
CONFIG_PARTITION_TABLE_CUSTOM=y

# CPU Frequency
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y

# SPIRAM
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y

# Watchdog (Disabled)
# CONFIG_ESP_INT_WDT is not set
# CONFIG_ESP_TASK_WDT_EN is not set

# Compiler Optimization
CONFIG_COMPILER_OPTIMIZATION_PERF=y
CONFIG_COMPILER_OPTIMIZATION_ASSERTIONS_DISABLE=y
```

**Security Configuration**: One critical security setting in the SDK configuration is that TLS certificate verification is disabled (`CONFIG_ESP_TLS_INSECURE=y`). This setting allows the device to connect to HTTPS endpoints without verifying that the server's SSL certificate is valid and trusted. While this simplifies development and testing (especially when using self-signed certificates or dealing with certificate chain issues), it creates a significant security vulnerability in production deployments. An attacker could perform a man-in-the-middle attack by presenting a fake certificate, and the device would accept it without verification. **This setting should absolutely be changed to enable certificate verification (`CONFIG_ESP_TLS_INSECURE=n`) before deploying to production**, and the appropriate certificate authority (CA) certificates should be included in the firmware to verify OpenAI's servers.

### Kconfig Configuration (`src/Kconfig.projbuild`)

Kconfig is the configuration system used by ESP-IDF (and many other embedded projects like the Linux kernel) to manage build-time configuration options. The `src/Kconfig.projbuild` file defines a custom configuration menu that appears when developers run `idf.py menuconfig`. This allows developers to select which board configuration to use without modifying source code directly.

The SDK currently supports two board configurations: the default ESP32-S3 board and the M5Stack ATOMS3R board with EchoBase hardware. These configurations differ primarily in their audio setup - the default board uses direct I2S interfaces at 8kHz, while the ATOMS3R board uses the ES8311 codec at 16kHz. The selection affects which source files are compiled and which hardware drivers are initialized.

**Board Selection:**
```kconfig
menu "OpenAI Board Configuration"

    choice OPENAI_BOARD_TYPE
    prompt "openai board type"
    default OPENAI_BOARD_ESP32_S3

        config OPENAI_BOARD_ESP32_S3
            bool "default demo board ESP32-S3"

        config OPENAI_BOARD_M5_ATOMS3R
            bool "M5Stack ATOMS3R with EchoBase"

    endchoice

endmenu
```

**Usage**: Selected via `idf.py menuconfig` or `sdkconfig` file.

### Build Process

The build process for the SDK follows a standard ESP-IDF workflow, but with some specific requirements and steps that are unique to this project. Understanding the complete build process is essential for both development and troubleshooting.

#### Prerequisites

Before building the SDK, several prerequisites must be installed and configured. The most fundamental requirement is **ESP-IDF version 4.1.0 or later**, which provides the entire development framework including the cross-compilation toolchain, build system, and runtime libraries. ESP-IDF installation involves downloading the framework, setting up the Python environment it requires, and installing platform-specific tools. Once installed, the ESP-IDF environment must be "sourced" using `. $IDF_PATH/export.sh` on Linux/Mac or `export.bat` on Windows, which sets up environment variables and adds the toolchain to the PATH.

**Python 3** is required because ESP-IDF's build system and many of its tools are written in Python. The build process uses Python scripts for code generation, component management, and various build-time tasks. The specific Python version requirements depend on the ESP-IDF version being used, but Python 3.6 or later is typically required.

**Git** is essential for managing the project's Git submodules. When you clone the repository, the submodules aren't automatically included - you must explicitly initialize them using `git submodule update --init --recursive`. This command downloads all the external dependencies (libpeer, srtp, esp-libopus, esp-protocols) into their respective directories.

Interestingly, the README mentions that **protoc** (Protocol Buffers compiler) must be in your PATH with `protobufc` installed. However, examination of the codebase reveals that Protocol Buffers are not actually used in the current implementation. This requirement may be a legacy from earlier development or may be planned for future features. Regardless, it's listed as a requirement, so developers should install it to avoid potential build issues.

For continuous integration and automated builds, **Docker** is used with the `espressif/idf:latest` image, which provides a pre-configured environment with ESP-IDF and all necessary tools. This ensures consistent builds across different development machines and CI/CD systems.

#### Build Steps

**1. Clone Repository with Submodules:**
```bash
git clone --recursive <repository-url>
cd echo-base--openai-realtime-embedded-sdk
```

**Or initialize submodules after clone:**
```bash
git submodule update --init --recursive
```

**2. Set Environment Variables:**
```bash
export WIFI_SSID="your_wifi_ssid"
export WIFI_PASSWORD="your_wifi_password"
export OPENAI_API_KEY="your_openai_api_key"
```

**3. Set Target Platform:**
```bash
idf.py set-target esp32s3  # or linux
```

**4. Configure (Optional):**
```bash
idf.py menuconfig
```
- Select board type (ESP32-S3 or M5 ATOMS3R)
- Modify other settings if needed

**5. Build:**
```bash
idf.py build
```

**6. Flash (ESP32 only):**
```bash
sudo -E idf.py flash
```

**7. Run (Linux):**
```bash
./build/src.elf
```

#### CI/CD Build (GitHub Actions)

**Workflow** (`.github/workflows/build.yaml`):
```yaml
name: Build
on:
  push:
    branches: [master]
  pull_request:

jobs:
  build:
    strategy:
      matrix:
        target: [esp32s3, linux]
    runs-on: ubuntu-latest
    steps:
    - name: Checkout repo
      uses: actions/checkout@v2
      with:
        submodules: 'recursive'
    - name: Build
      run: |
        docker run -v $PWD:/project -w /project -u 0 \
        -e HOME=/tmp -e WIFI_SSID=A -e WIFI_PASSWORD=B -e OPENAI_API_KEY=X \
        espressif/idf:latest \
        /bin/bash -c 'idf.py --preview set-target ${{ matrix.target }} && idf.py build'
```

**Key Points:**
- Builds for both `esp32s3` and `linux` targets
- Uses Docker with `espressif/idf:latest` image
- Dummy credentials for build (actual credentials set at runtime)

### Build Output

**ESP32 Build:**
- Binary: `build/src.bin`
- ELF: `build/src.elf`
- Partition table: `build/partition_table.bin`
- Bootloader: `build/bootloader/bootloader.bin`

**Linux Build:**
- ELF: `build/src.elf` (executable)

### Partition Table (`partitions.csv`)

```csv
# ESP-IDF Partition Table
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x180000,
```

**Partitions:**
- **nvs**: 24KB - Non-volatile storage
- **phy_init**: 4KB - PHY initialization data
- **factory**: 1.5MB - Application firmware

## Source Code Organization

### Repository Structure

```
echo-base--openai-realtime-embedded-sdk/
├── .github/
│   └── workflows/
│       ├── build.yaml              # CI/CD build workflow
│       └── clang-format-check.yml  # Code formatting check
├── components/
│   ├── peer/                       # Peer component wrapper
│   │   └── CMakeLists.txt
│   ├── srtp/                       # SRTP library (submodule)
│   ├── esp-libopus/                # Opus codec (submodule)
│   └── esp-protocols/              # Espressif protocols (submodule)
├── deps/
│   └── libpeer/                    # WebRTC library (submodule)
│       ├── src/                    # libpeer source
│       ├── third_party/            # libpeer dependencies
│       └── idf_component.yml
├── managed_components/
│   └── espressif__es8311/          # ES8311 codec (managed)
├── src/                            # Application source
│   ├── main.cpp                    # Entry point
│   ├── main.h                      # Function declarations
│   ├── webrtc.cpp                  # WebRTC implementation
│   ├── http.cpp                    # HTTP signaling
│   ├── media.cpp                   # Audio capture/encoding
│   ├── wifi.cpp                    # WiFi connectivity
│   ├── CMakeLists.txt              # Component build config
│   ├── idf_component.yml           # Component dependencies
│   └── Kconfig.projbuild          # Kconfig menu
├── .gitmodules                     # Git submodule definitions
├── CMakeLists.txt                  # Root build config
├── sdkconfig.defaults              # Default SDK configuration
├── partitions.csv                  # Partition table
├── LICENSE                         # MIT License
└── README.md                       # Project documentation
```

### Application Source Files

**Location**: `src/`

1. **main.cpp** - Application entry point
   - `app_main()` (ESP32) / `main()` (Linux)
   - Initialization sequence

2. **main.h** - Function declarations
   - Public API for application functions

3. **webrtc.cpp** - WebRTC peer connection
   - Peer connection lifecycle
   - ICE candidate handling
   - Audio streaming task

4. **http.cpp** - HTTP signaling
   - SDP offer/answer exchange with OpenAI
   - HTTP client implementation

5. **media.cpp** - Audio pipeline
   - I2S configuration
   - Opus encoder/decoder
   - Audio capture and playback

6. **wifi.cpp** - WiFi connectivity
   - Station mode setup
   - Connection management

### External Library Sources

**libpeer** (`deps/libpeer/src/`):
- `peer_connection.c/h` - Peer connection implementation
- `agent.c/h` - ICE agent
- `dtls_srtp.c/h` - DTLS-SRTP encryption
- `rtp.c/h` - RTP encoding/decoding
- `sdp.c/h` - SDP generation/parsing
- `stun.c/h` - STUN protocol
- `sctp.c/h` - SCTP (disabled)
- `buffer.c/h` - Ring buffer implementation
- `ports.c/h` - Platform abstraction

**esp-libopus** (`components/esp-libopus/`):
- `src/` - Opus encoder/decoder source
- `celt/` - CELT codec source
- `silk/` - SILK codec source
- `include/opus.h` - Public API

**srtp** (`components/srtp/`):
- libsrtp library source (port for ESP32)

## License and Copyright

### SDK License

**License**: MIT License

**Copyright**: Copyright (c) 2024 OpenAI

**Full License Text** (`LICENSE`):
```
MIT License

Copyright (c) 2024 OpenAI

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

### Dependency Licenses

- **libpeer**: MIT License
- **esp-libopus**: Opus codec license (see repository)
- **srtp**: See repository license
- **esp-protocols**: Espressif license (see repository)
- **es8311**: Espressif license (see repository)

## Version Management

### Git Submodule Versioning

Submodules are tracked at **specific commits**, not branches or tags:
- Ensures reproducible builds
- Prevents accidental updates
- Requires manual updates via `git submodule update --remote`

### ESP-IDF Component Versioning

Managed components use semantic versioning:
- `espressif/es8311: ^1.0.0~1` - Compatible with 1.0.0 or higher

### ESP-IDF Version Requirement

- Minimum: ESP-IDF v4.1.0
- Tested: Latest stable (as of 2024)

## Build System Quirks and Notes

The SDK's build system has several quirks and design decisions that developers should be aware of, as they can affect maintenance, updates, and troubleshooting.

### 1. Config File Modification

One of the more unusual aspects of the build system is that the peer component wrapper **modifies** the libpeer library's configuration file (`deps/libpeer/src/config.h`) at build time. Specifically, it uses CMake's `file(READ)` and `file(WRITE)` functions to search for and replace configuration defines. It disables `HAVE_USRSCTP` (because SCTP for data channels isn't used in this implementation) and sets `KEEPALIVE_CONNCHECK` to 0 (to disable connection keepalive checks and reduce network overhead).

This approach has significant implications for maintenance. Because the modifications are made directly to the submodule's source files, they become part of the submodule's working tree. If a developer updates the libpeer submodule to a newer version (using `git submodule update --remote`), these modifications will be lost and the build will fail or behave differently. The modifications are re-applied on every build, but only if the build system runs successfully. This creates a dependency between the build system and the submodule's source code structure - if libpeer changes how its config.h is organized, the modification logic in `components/peer/CMakeLists.txt` may break.

A more maintainable approach would be to use a patch file (via `git apply`) or to fork libpeer with these changes committed. However, the current approach works as long as developers are aware of it and don't manually modify the submodule without understanding the build-time modifications.

### 2. Compiler Warning Suppressions

Multiple compiler warnings are suppressed for dependencies:
- `-Wno-error=restrict` (libpeer)
- `-Wno-error=stringop-truncation` (libpeer)
- `-Wno-error=incompatible-pointer-types` (srtp)
- `-Wno-error=maybe-uninitialized` (esp-libopus)
- `-Wno-error=stringop-overread` (esp-libopus)

**Note**: These suppressions indicate potential code quality issues in dependencies.

### 3. TLS Certificate Verification Disabled

`CONFIG_ESP_TLS_INSECURE=y` disables certificate verification.

**Security Risk**: This makes the device vulnerable to man-in-the-middle attacks.

**Recommendation**: Enable certificate verification in production builds.

### 4. Audio Sending Disabled

`SEND_AUDIO=0` compile definition disables audio sending.

**Note**: Comment in `CMakeLists.txt` says "Audio Sending is implemented, but not performant enough yet". This suggests audio sending code exists but is disabled.

### 5. Linux Build Compatibility

Linux build uses compatibility layers:
- `esp_stubs` - ESP-IDF API stubs
- `linux_compat/esp_timer` - Timer compatibility
- `linux_compat/freertos` - FreeRTOS compatibility

**Limitation**: Linux build doesn't support audio or WiFi (hardware-specific).

## Dependency Update Process

### Updating Git Submodules

**Check for updates:**
```bash
git submodule update --remote
```

**Update specific submodule:**
```bash
cd deps/libpeer  # or other submodule
git pull origin main
cd ../..
git add deps/libpeer
git commit -m "Update libpeer submodule"
```

**Update all submodules:**
```bash
git submodule update --remote --merge
```

### Updating ESP-IDF Managed Components

Managed components are updated automatically by ESP-IDF Component Manager based on version constraints in `idf_component.yml`.

**Force update:**
```bash
idf.py reconfigure
```

**Check component versions:**
```bash
idf.py show-property-value COMPONENT_OVERRIDEN_DIR
```

## Troubleshooting Build Issues

### Common Issues

1. **Missing Environment Variables**
   - Error: `FATAL_ERROR "Env variable OPENAI_API_KEY must be set"`
   - Solution: Set `OPENAI_API_KEY`, `WIFI_SSID`, `WIFI_PASSWORD`

2. **Submodule Not Initialized**
   - Error: Missing source files in `deps/` or `components/`
   - Solution: `git submodule update --init --recursive`

3. **ESP-IDF Not Sourced**
   - Error: `IDF_PATH` not set
   - Solution: Source ESP-IDF environment (`. $IDF_PATH/export.sh`)

4. **Compiler Warnings as Errors**
   - Error: Build fails on dependency warnings
   - Solution: Check `src/CMakeLists.txt` for warning suppressions

5. **Config File Modification Lost**
   - Issue: `deps/libpeer/src/config.h` modifications reverted
   - Solution: Re-run build (modifications are applied at build time)

## Related Documentation and Resources

### Official Documentation

- **ESP-IDF Documentation**: The comprehensive [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/) covers all aspects of ESP32 development, from getting started to advanced topics like power management and security.
- **ESP-IDF API Reference**: The [API Reference](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/) provides detailed documentation for all ESP-IDF APIs, including WiFi, HTTP client, I2S, and more.
- **ESP-IDF Component Manager**: Learn about managing dependencies with the [Component Manager documentation](https://docs.espressif.com/projects/idf-component-manager/en/latest/).
- **OpenAI Realtime API**: The [official Realtime API documentation](https://platform.openai.com/docs/guides/realtime) explains the API's capabilities, WebRTC integration, and usage patterns.

### WebRTC and Networking Resources

- **WebRTC Specification**: The [W3C WebRTC specification](https://www.w3.org/TR/webrtc/) provides the authoritative technical reference for WebRTC protocols and APIs.
- **MDN WebRTC Guide**: Mozilla's [WebRTC documentation](https://developer.mozilla.org/en-US/docs/Web/API/WebRTC_API) offers excellent tutorials and examples for understanding WebRTC concepts.
- **libpeer Repository**: The [libpeer GitHub repository](https://github.com/sepfy/libpeer) contains the source code, examples, and documentation for the WebRTC library used by this SDK.
- **SRTP Specification**: [RFC 3711](https://datatracker.ietf.org/doc/html/rfc3711) defines the Secure Real-time Transport Protocol used for encrypting RTP packets.

### Audio Codec Resources

- **Opus Codec**: The [Opus codec website](https://opus-codec.org/) provides documentation, performance benchmarks, and implementation guides.
- **Opus Specification**: [RFC 6716](https://datatracker.ietf.org/doc/html/rfc6716) defines the Opus audio codec specification.
- **esp-libopus Repository**: The [esp-libopus GitHub repository](https://github.com/XasWorks/esp-libopus) contains the ESP32 port of Opus with important performance notes for embedded systems.

### ESP32 Development Resources

- **ESP32-S3 Datasheet**: The [ESP32-S3 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf) provides detailed hardware information.
- **ESP-IDF Examples**: The [ESP-IDF Examples repository](https://github.com/espressif/esp-idf/tree/master/examples) contains numerous example projects demonstrating various ESP-IDF features.
- **ESP32 Forum**: The [ESP32 Forum](https://www.esp32.com/) is an active community where developers share knowledge and get help.

### OpenAI API Resources

- **OpenAI API Quickstart**: The [OpenAI API Quickstart Guide](https://platform.openai.com/docs/quickstart) helps developers get started with OpenAI's APIs.
- **OpenAI API Tutorials**: [OpenAI's tutorial collection](https://platform.openai.com/docs/tutorials) provides step-by-step guides for building AI applications.
- **OpenAI Community**: The [OpenAI Developer Forum](https://community.openai.com/) is a great place to ask questions and learn from other developers.

### Build System and Development Tools

- **CMake Documentation**: The [CMake documentation](https://cmake.org/documentation/) provides comprehensive information about CMake build system features.
- **ESP-IDF Build System**: The [ESP-IDF Build System documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html) explains how ESP-IDF's component-based build system works.
- **Git Submodules**: The [Git Submodules documentation](https://git-scm.com/book/en/v2/Git-Tools-Submodules) explains how to work with Git submodules.
