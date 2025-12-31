---
Title: Audio Recorder with Web Control for M5AtomS3
Ticket: CLINTS-MEMO-WEBSITE
Status: active
Topics:
    - audio-recording
    - web-server
    - esp32-s3
    - m5atoms3
    - i2s
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ATOMS3R-CAM-M12-UserDemo/main/service/apis/api_camera.cpp
      Note: REST API endpoint patterns and async request handling
    - Path: ATOMS3R-CAM-M12-UserDemo/main/service/apis/api_imu.cpp
      Note: WebSocket implementation for real-time data streaming
    - Path: ATOMS3R-CAM-M12-UserDemo/main/service/service_web_server.cpp
      Note: ESPAsyncWebServer setup and WiFi AP configuration
    - Path: M5AtomS3-UserDemo/src/main.cpp
      Note: Basic M5AtomS3 setup and M5Unified integration
    - Path: M5Cardputer-UserDemo/main/hal/mic/Mic_Class.cpp
      Note: I2S microphone implementation with recording API
    - Path: M5Cardputer-UserDemo/main/hal/mic/Mic_Class.hpp
      Note: High-level microphone abstraction API for M5Stack devices
    - Path: echo-base--openai-realtime-embedded-sdk/src/media.cpp
      Note: I2S audio capture implementation with Opus encoding
    - Path: esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp
      Note: Native esp_http_server implementation with WebSocket support
ExternalSources: []
Summary: "Workspace index for CLINTS-MEMO-WEBSITE: audio recorder firmware + device-hosted web UI with REST control and WAV downloads."
LastUpdated: 2025-12-31T13:59:30-05:00
WhatFor: "Provide a single entry point to the design contract, API contracts, tasks, and verification playbook for the website-controlled audio recorder."
WhenToUse: "Start here when implementing or reviewing the ticket, then follow the design doc and playbook."
---


# Audio Recorder with Web Control for M5AtomS3

## Overview

This ticket documents the architecture and implementation plan for building a small audio recorder with a web-based control interface for the M5AtomS3 device. The system will:

1. **Capture audio** via I2S interface from the device's microphone
2. **Store recordings** to file system (SPIFFS or FATFS)
3. **Provide web interface** for controlling recording (start/stop, list files, download)
4. **Stream audio** via HTTP or WebSocket for real-time monitoring

The analysis document (`analysis/01-audio-recorder-architecture-analysis-files-symbols-and-concepts.md`) provides comprehensive coverage of:
- I2S audio capture configuration and APIs
- Web server options (ESPAsyncWebServer vs esp_http_server)
- Storage patterns and file system choices
- Audio streaming approaches
- REST API design for control interface

Current status: **Analysis phase complete** - Ready for implementation planning.

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field
- **Design doc**: `design-doc/01-website-audio-recorder-design-document.md`
- **Analysis**: `analysis/01-audio-recorder-architecture-analysis-files-symbols-and-concepts.md`
- **SPIFFS mkdir analysis**: `analysis/02-spiffs-mkdir-enotsup-and-recordings-layout.md`
- **Diary**: `reference/01-diary.md`
- **Waveform API contract**: `reference/02-waveform-api-contract.md`
- **Verification playbook**: `playbooks/01-verification-playbook-website-audio-recorder.md`

## Status

Current status: **active**

## Topics

- audio-recording
- web-server
- esp32-s3
- m5atoms3
- i2s

## Tasks

See [tasks.md](./tasks.md) for the current task list.

## Changelog

See [changelog.md](./changelog.md) for recent changes and decisions.

## Structure

- design/ - Architecture and design documents
- reference/ - Prompt packs, API contracts, context summaries
- playbooks/ - Command sequences and test procedures
- scripts/ - Temporary code and tooling
- various/ - Working notes and research
- archive/ - Deprecated or reference-only artifacts
