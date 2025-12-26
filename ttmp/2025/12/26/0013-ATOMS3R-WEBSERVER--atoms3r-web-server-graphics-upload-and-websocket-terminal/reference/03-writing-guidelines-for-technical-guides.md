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
Summary: "Guidelines for writing technical guides that teach concepts and patterns to senior engineers, focused on narrative flow, mental modeling, mixed-media structure, and concrete referencing."
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

### 1. Narrative Flow and Structural Variety

Don't just write a wall of text or a dry list of bullet points. **Mix your media** to keep the reader engaged and aid understanding.

**The "Sandwich" Technique**:
1. **Narrative Paragraph (The Context)**: Start with a substantive paragraph (3-5 sentences) explaining *what* we are doing and *why*. This sets the stage.
2. **Structured Content (The Detail)**: Use a bulleted list, a mermaid diagram, or a pseudocode block to show the mechanics clearly.
3. **Closing Paragraph (The Implication)**: Follow up with a sentence or two explaining the consequence or "gotcha" of what was just shown.

**Why this works**: Long paragraphs build the mental model; bullet points/code provide referenceable facts. Using both allows the reader to scan *and* deep-dive.

### 2. Be Concrete: Refer to Real Files and Symbols

Abstract descriptions are hard to retain. Always ground your explanation in the actual codebase.

**Abstract (Avoid)**:
> "The system initializes the WiFi driver."

**Concrete (Preferred)**:
> "The system initializes the WiFi driver by calling `esp_wifi_init()` in `main/wifi_app.cpp`. This relies on the configuration structure `WIFI_INIT_CONFIG_DEFAULT()` to set up the internal task stacks."

**Rule**: If you mention a concept (e.g., "The Event Loop"), link it to its symbol (`esp_event_loop_create_default`) or file (`components/esp_event`).

### 3. Establish the Mental Model Early

Senior engineers learn fastest when they have a conceptual framework to hang details on. Start every major section by defining the "nouns" and "verbs" of the subsystem before showing the code.

*Example*: "In ESP-IDF, WiFi is not just a driver. It is a coordinated system of three components: a **Radio** (PHY layer), a **Network Interface** (IP layer), and an **Event Loop** (state management). You must configure all three for a connection to work."

### 4. Explain the "Why", Not Just the "What"

Documentation should capture the *decision process*, not just the final artifact. Anticipate the reader's skepticism.

- **Why** did we choose this mode over the alternative?
- **Why** is the order of operations critical here?
- **Why** might this approach fail in a production environment?

*Example*: "We use AP+STA mode here not just for connectivity, but because it provides a **recovery path**. If the user enters the wrong STA credentials, the device would otherwise be unreachable. By keeping the SoftAP active, we ensure you can always reconnect to fix the configuration."

### 5. Progressive Disclosure

Structure the document like a pyramid to respect the reader's time:
1. **Executive Summary**: The 30-second version (what is this and why do I care?).
2. **High-Level Concepts**: The block diagram view (mental model).
3. **Detailed Walkthrough**: The step-by-step implementation (the "happy path").
4. **Edge Cases & Debugging**: The deep tracks for when things break (failure modes).

## Recommended Structure for a Guide

1. **Title & Front-matter**: Standard repo metadata.
2. **Executive Summary**: What is this, who is it for, and what problem does it solve?
3. **Mental Model / Concepts**: The "theory" section. Define the key terms and their relationships.
4. **Use Cases**: When should I use X vs Y? (e.g., SoftAP vs STA). Real-world scenarios.
5. **Implementation Walkthrough**:
   - Numbered steps (1, 2, 3...).
   - For each step:
     - **Narrative Intro**: Why we are here.
     - **Code/Pseudocode**: The mechanism.
     - **Explanation**: Breaking down specific lines (`esp_wifi_connect()`).
6. **Worked Example**: Pointer to real code in the repo (`main/wifi_app.cpp`) that implements this pattern.
7. **Debugging / Gotchas**: "I followed the guide but X happened." Common pitfalls, error codes, and how to verify things are working.
8. **Resources**: Links to official docs/RFCs/specs.

## Writing Style Checklist

- [ ] **Structural Variety**: Do I have a mix of paragraphs, lists, and code blocks?
- [ ] **Concrete References**: Did I name specific files (`main/main.c`) and symbols (`app_main`)?
- [ ] **Conversational Tone**: Write as if you are explaining it to a colleague at a whiteboard. Use "we" and "you".
- [ ] **Active Voice**: "The driver sends an event" (not "An event is sent by the driver").
- [ ] **Callouts**: Use callouts or bold text for "Common Pitfalls" or "Pro Tips".
- [ ] **Honesty**: If a feature is flaky, experimental, or hacky, say so. "This is a workaround for..." builds trust.

## Example: From "Brief" to "Didactic"

**Brief (Avoid)**:
> `esp_wifi_start()` starts the WiFi. Then you wait for connection.

**Didactic (Preferred)**:
> Calling `esp_wifi_start()` powers on the radio hardware. However, this is an **asynchronous operation**. The function returns immediately, but the connection process happens in the background.
>
> Your application **must not** assume the network is ready until it receives the `IP_EVENT_STA_GOT_IP` event. This separation of "driver started" vs "network ready" is a common source of bugs for new ESP-IDF developers.
>
> **Pseudocode flow**:
> ```text
> start_driver() -> returns OK
> ... wait for event ...
> on WIFI_EVENT_STA_CONNECTED: link is up
> on IP_EVENT_STA_GOT_IP:      network is ready (NOW you can use sockets)
> ```
>
> In our codebase, this logic is handled by the event handler in `main/wifi_app.cpp`.
