---
Title: 'Technical Writing Guidelines: Creating Didactic Engineering Guides'
Ticket: 0013-ATOMS3R-WEBSERVER
Status: active
Topics:
    - documentation
    - technical-writing
    - guidelines
    - didactic
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/26/0013-ATOMS3R-WEBSERVER--atoms3r-web-server-graphics-upload-and-websocket-terminal/reference/02-esp-idf-wifi-softap-sta-apsta-guide.md
      Note: Example of applying these guidelines
ExternalSources: []
Summary: "Guidelines for writing technical guides that teach concepts and patterns to senior engineers, focused on narrative flow, mental modeling, and explaining the 'why'."
LastUpdated: 2025-12-26
WhatFor: "Use when creating new 'how-to' or 'architecture' guides to ensure they are readable, educational, and actionable."
WhenToUse: "When capturing knowledge for the team, documenting a new subsystem, or explaining complex patterns."
---

# Technical Writing Guidelines: Creating Didactic Engineering Guides

## The Goal

Technical documentation often falls into two traps:
1. **Too dry**: API references or raw code snippets with zero context ("Here is the function signature").
2. **Too fluffy**: High-level marketing fluff with no actionable engineering detail ("Our solution is robust and scalable").

Our goal is **didactic engineering documentation**: guides that respect the reader's intelligence (assuming senior engineering skills) but do not assume prior knowledge of the specific domain (e.g., ESP-IDF WiFi). We teach the *mental model* first, then the *implementation details*.

## Core Principles

### 1. Narrative Flow and Transitions

Don't just list facts; tell a story about the system. Every section should flow logically into the next. Use **signposting** to tell the reader where they are going and why.

**Dry Style (Avoid)**:
> `esp_netif_init()` initializes the TCP/IP stack. `esp_event_loop_create_default()` creates the event loop.

**Narrative Style (Preferred)**:
> Before we can start any WiFi operations, we need to build the infrastructure that supports it. First, we initialize the underlying TCP/IP stack (`esp_netif_init()`) so the WiFi driver has something to attach to. Next, because WiFi operations are asynchronous, we need an event loop (`esp_event_loop_create_default()`) to receive notifications when connections succeed or fail.

**Key Technique**: Use transitional phrases like "The reason this matters is...", "Once that is complete, the next step is...", or "However, there is a catch...".

### 2. Establish the Mental Model Early

Senior engineers learn fastest when they have a conceptual framework to hang details on. Start every major section by defining the "nouns" and "verbs" of the subsystem before showing the code.

*Example*: "In ESP-IDF, WiFi is not just a driver. It is a coordinated system of three components: a **Radio** (PHY layer), a **Network Interface** (IP layer), and an **Event Loop** (state management). You must configure all three for a connection to work."

### 3. Explain the "Why", Not Just the "What"

Documentation should capture the *decision process*, not just the final artifact. Anticipate the reader's skepticism.

- **Why** did we choose this mode over the alternative?
- **Why** is the order of operations critical here?
- **Why** might this approach fail in a production environment?

*Example*: "We use AP+STA mode here not just for connectivity, but because it provides a **recovery path**. If the user enters the wrong STA credentials, the device would otherwise be unreachable. By keeping the SoftAP active, we ensure you can always reconnect to fix the configuration."

### 4. Progressive Disclosure

Structure the document like a pyramid to respect the reader's time:
1. **Executive Summary**: The 30-second version (what is this and why do I care?).
2. **High-Level Concepts**: The block diagram view (mental model).
3. **Detailed Walkthrough**: The step-by-step implementation (the "happy path").
4. **Edge Cases & Debugging**: The deep tracks for when things break (failure modes).

This allows readers to bail out once they have enough info, or dig deeper if they are stuck.

### 5. Be "Senior-Friendly" but Accessible

- **Don't** explain basic programming concepts (what a variable is, how `if` statements work).
- **Do** explain domain-specific concepts (*lifecycle*, *concurrency*, *memory ownership*, *blocking behavior*).
- **Don't** use toy examples if possible. Use real patterns.
- **Do** discuss trade-offs explicitly (e.g., "SoftAP is easier for dev, but STA is required for cloud access").

## Recommended Structure for a Guide

1. **Title & Front-matter**: Standard repo metadata.
2. **Executive Summary**: What is this, who is it for, and what problem does it solve?
3. **Mental Model / Concepts**: The "theory" section. Define the key terms and their relationships.
4. **Use Cases**: When should I use X vs Y? (e.g., SoftAP vs STA). Real-world scenarios.
5. **Implementation Walkthrough**:
   - Numbered steps (1, 2, 3...).
   - For each step: **Why** it's needed, **Code** snippet (with comments on *intent*), **Explanation** of key mechanics.
6. **Worked Example**: Pointer to real code in the repo that implements this pattern.
7. **Debugging / Gotchas**: "I followed the guide but X happened." Common pitfalls, error codes, and how to verify things are working.
8. **Security / Production Notes**: How to take this from "works on my desk" to "shippable product".
9. **Resources**: Links to official docs/RFCs/specs.

## Writing Style Checklist

- [ ] **Conversational Tone**: Write as if you are explaining it to a colleague at a whiteboard. Use "we" and "you".
- [ ] **Active Voice**: "The driver sends an event" (not "An event is sent by the driver").
- [ ] **Contextual Linking**: Link to file paths in the repo (`main/wifi_app.cpp`), not just abstract names.
- [ ] **Code Comments**: If the code block is complex, comment the *intent*, not the syntax.
- [ ] **Callouts**: Use callouts or bold text for "Common Pitfalls" or "Pro Tips".
- [ ] **Honesty**: If a feature is flaky, experimental, or hacky, say so. "This is a workaround for..." builds trust.

## Example: From "Brief" to "Didactic"

**Brief (Avoid)**:
> `esp_wifi_start()` starts the WiFi. Then you wait for connection.

**Didactic (Preferred)**:
> Calling `esp_wifi_start()` powers on the radio hardware. However, this is an **asynchronous operation**. The function returns immediately, but the connection process happens in the background. Your application **must not** assume the network is ready until it receives the `IP_EVENT_STA_GOT_IP` event. This separation of "driver started" vs "network ready" is a common source of bugs for new ESP-IDF developers.
