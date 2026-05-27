---
title: Investigation Diary
doc-type: reference
status: active
---

# Diary

## Goal

Record the implementation journey for ticket 0094: Tab5 WiFi/HTTP benchmark firmware.

## Step 1: Ticket creation and design doc

Created ticket 0094 for a dedicated benchmark firmware to measure throughput at every layer of the WiFi-to-HTTP data path on the M5Stack Tab5. Wrote a comprehensive intern-guide-style design document.

### Prompt Context

**User prompt (verbatim):** "Close the ticket, then open a new one, and let's make a new firmware to benchmark the HTTP and wifi speed, for different factors, so that we know where the bottlenecks are in our stack. Create a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet points and pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and then upload to remarkable. Keep a diary as you work."

**Assistant interpretation:** Close ticket 0093, create ticket 0094 for a benchmark firmware, write an intern-grade technical guide with full system architecture, pseudocode, API references, and Mermaid diagrams.

**Inferred user intent:** Get a comprehensive onboarding + design document that an intern can use to understand the full ESP-Hosted WiFi stack and implement a benchmark firmware, then upload it to reMarkable for reading.

### What I did

- Closed ticket 0093 (all tasks complete)
- Created ticket 0094 with title and topics
- Added design doc and diary doc to the ticket
- Wrote 24 KB design doc covering:
  - Section 1: The system (hardware, data path, SDIO bus, ESP-Hosted driver, httpd, WiFi modes)
  - Section 2: What to measure (per-request timing, per-segment timing, system counters, benchmark matrix)
  - Section 3: Firmware architecture (endpoints, pseudocode for upload/download/ping handlers, browser JS)
  - Section 4: Implementation plan (5 phases with tasks)
  - Section 5: Key files and API references (IDF components, timing, heap, RSSI, httpd config, fork base)
  - Section 6: Expected results and what they mean
  - Section 7: Open questions
- Created tasks.md with 5 phases
- Related fork base files to the ticket
- Updated changelog

### Why

The upload optimization article identified a 3s observed vs 0.3s theoretical gap but could not conclusively identify the bottleneck. The benchmark firmware provides the instrumentation to find out. The intern-guide style ensures the document is self-contained and transferable.

### What worked

- Mermaid diagram for the data path clearly shows all the hops
- Pseudocode for the benchmark handlers is concrete enough to implement directly
- The benchmark matrix (payload size x compression x connection mode x concurrency) is systematic

### What didn't work

- N/A — this was a documentation step, no code was built or tested

### What should be done in the future

- Upload to reMarkable (next step)
- Implement the firmware
- Run the benchmarks

### Code review instructions

- Read the design doc: `design-doc/01-benchmark-firmware-design-and-intern-guide.md`
- Verify the SDIO configuration matches `sdkconfig.defaults`
- Verify the httpd defaults match the IDF source (`esp_http_server.h` line 53-80)
