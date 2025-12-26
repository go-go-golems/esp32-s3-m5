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
Summary: "Guidelines for writing technical guides that teach concepts and patterns to senior engineers, not just document APIs."
LastUpdated: 2025-12-26
WhatFor: "Use when creating new 'how-to' or 'architecture' guides to ensure they are readable, educational, and actionable."
WhenToUse: "When capturing knowledge for the team, documenting a new subsystem, or explaining complex patterns."
---

# Technical Writing Guidelines: Creating Didactic Engineering Guides

## The Goal

Technical documentation often falls into two traps:
1. **Too dry**: API references or raw code snippets with zero context.
2. **Too fluffy**: High-level marketing fluff with no actionable engineering detail.

Our goal is **didactic engineering documentation**: guides that respect the reader's intelligence (assuming senior engineering skills) but do not assume prior knowledge of the specific domain (e.g., ESP-IDF WiFi). We teach the *mental model* first, then the *implementation details*.

## Core Principles

### 1. Narrative First, Code Second

Don't just dump code. Tell the story of *why* the code exists.

**Bad**:
> Here is the WiFi initialization code:
> ```c
> esp_netif_init();
> ```

**Good**:
> Before we can start any WiFi operations, we must initialize the underlying TCP/IP stack. This scaffolding allows the WiFi driver to attach network interfaces later.
> ```c
> esp_netif_init();
> ```

### 2. Establish the Mental Model Early

Senior engineers learn fastest when they have a conceptual framework to hang details on. Start every major section by defining the "nouns" and "verbs" of the subsystem.

*Example*: "In ESP-IDF, WiFi is not just a driver. It is three things: a **Radio** (PHY), a **Network Interface** (Netif/IP), and an **Event Loop** (state changes)."

### 3. Explain the "Why", Not Just the "What"

Documentation should capture the *decision process*, not just the final artifact.

- **Why** did we choose this mode?
- **Why** does the order of operations matter?
- **Why** might this fail in production?

### 4. Progressive Disclosure

Structure the document like a pyramid:
1. **Executive Summary**: The 30-second version.
2. **High-Level Concepts**: The block diagram view.
3. **Detailed Walkthrough**: The step-by-step implementation.
4. **Edge Cases & Debugging**: The deep tracks for when things break.

This allows readers to bail out once they have enough info, or dig deeper if they are stuck.

### 5. Be "Senior-Friendly"

- **Don't** explain what a variable is.
- **Do** explain *lifecycle*, *concurrency*, *memory ownership*, and *blocking behavior*.
- **Don't** use toy examples if possible.
- **Do** discuss trade-offs (e.g., "SoftAP is easier for dev, but STA is required for cloud access").

## Recommended Structure for a Guide

1. **Title & Front-matter**: Standard repo metadata.
2. **Executive Summary**: What is this, and who is it for?
3. **Mental Model / Concepts**: The "theory" section. Define terms.
4. **Use Cases**: When should I use X vs Y? (e.g., SoftAP vs STA).
5. **Implementation Walkthrough**:
   - Numbered steps (1, 2, 3...).
   - For each step: **Why** it's needed, **Code** snippet, **Explanation** of key lines.
6. **Worked Example**: Pointer to real code in the repo that implements this.
7. **Debugging / Gotchas**: "I followed the guide but X happened." Real-world friction points.
8. **Security / Production Notes**: How to take this from "prototype" to "shippable".
9. **Resources**: Links to official docs/RFCs.

## Writing Style Checklist

- [ ] **Active Voice**: "The driver sends an event" (not "An event is sent by the driver").
- [ ] **Contextual Linking**: Link to file paths in the repo, not just abstract names.
- [ ] **Code Comments**: If the code block is complex, comment the *intent*, not the syntax.
- [ ] **Callouts**: Use callouts for "Common Pitfalls" or "Pro Tips".
- [ ] **Honesty**: If a feature is flaky or experimental, say so.

## Example: From "Brief" to "Didactic"

**Brief (Avoid)**:
> `esp_wifi_start()` starts the WiFi. Then you wait for connection.

**Didactic (Preferred)**:
> Calling `esp_wifi_start()` powers on the radio hardware. However, this is an asynchronous operation. The function returns immediately, but the connection process happens in the background. Your application **must not** assume the network is ready until it receives the `IP_EVENT_STA_GOT_IP` event. This separation of "driver started" vs "network ready" is a common source of bugs.

