---
Title: Comic script: Zigbee adventure (zine style)
Ticket: 0067-zigbee-powerplug
Status: active
Topics:
    - zigbee
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Comic book script and dialogue retelling the Zigbee2MQTT project as a Why the Lucky Stiff-style zine adventure."
LastUpdated: 2026-01-31T15:16:10-05:00
WhatFor: "Creative lab artifact: a comic script capturing the project's workflow and discoveries."
WhenToUse: "Use for internal storytelling, presentations, or onboarding with a playful tone."
---

# Comic Script: Zigbee Adventure (Why the Lucky Stiff / Zine Style)

## Goal

Create a full comic script and dialogue, in playful zine culture tone, dramatizing the real project steps: docs, validation run, bridge requests, sniffing, and the final upload sprint.

## Context

This is a stylized retelling of the Zigbee2MQTT + power plug project. It blends factual events (tmux runs, port conflict, MQTT bridge quirks, doc writes) with a whimsical narrator and hand-lettered stage directions.

## Quick Reference

### Cast

- **Narrator (Zine Editor):** Pocket scissors, tape, an overfull notebook.
- **The Operator:** Lab hoodie, a swarm of sticky notes.
- **The Coordinator (Sonoff Dongle):** A quiet, steady lighthouse.
- **Mosquitto:** A broker with a megaphone and a tiny crown.
- **Zigbee2MQTT:** A courier on a bicycle with a satchel full of topics.
- **The Plug (3RSP02028BZ):** A stubborn, adorable lamp with stage fright.
- **The Sniffer (nRF 802.15.4):** A prism that hears the air.

### Tone notes

- Hand-lettered labels, cut-and-paste captions, and lovingly messy stage directions.
- The narrator occasionally breaks the fourth wall to explain a technical detail.
- Short, punchy dialog bubbles; longer notes are margin scribbles.

## Script (Panels + Dialogue)

### Page 1 — "Ticket #0067: The Zigbee Circus"

**Panel 1**: A corkboard titled "ZIGBEE POWER PLUG". A ticket stub: "0067". Sticky notes like confetti.
- Narrator: "Every good adventure starts with a ticket and a nervous laugh."
- Operator: "No HA this time. Plain Linux. We keep it honest."

**Panel 2**: A tiny map labeled "Three Things" with doodles.
- Narrator: "You need a coordinator, a broker, and a courier."
- Operator (pointing): "Dongle. Mosquitto. Zigbee2MQTT."

**Panel 3**: The Plug peeks from behind a bookshelf.
- Plug: "Do I have to join?"
- Operator: "Just for sixty seconds. You'll be fine."

### Page 2 — "Docker Compose Blues"

**Panel 1**: The Operator at a terminal. tmux panes split like a comic panel inside a panel.
- Operator: "Two panes. Two services. One shaky heart."

**Panel 2**: Mosquitto tries to take the stage but bounces off a wall labeled "1883".
- Mosquitto: "MY PORT."
- Wall: "Occupied."
- Operator (small text): "1883 is already taken. We'll take the alley."

**Panel 3**: A tiny sign flips to "1884".
- Narrator: "The show moves one street over."
- Mosquitto (bowing): "Now presenting on 1884."

### Page 3 — "The Bridge Requests"

**Panel 1**: Zigbee2MQTT rides a bike, tossing envelopes labeled "bridge/request".
- Zigbee2MQTT: "Permit join!"
- Response Envelope: "OK."

**Panel 2**: A stack of boxes labeled "bridge/info", "bridge/devices", "bridge/definitions".
- Narrator: "Sometimes the answers arrive by state topic."
- Operator: "Stop waiting on the response address."

**Panel 3**: The Operator opens an envelope labeled "logging". It's empty.
- Operator: "Hello?"
- Narrator (margin note): "Logging changes go through options. No direct logging request."

### Page 4 — "Sniffer in the Air"

**Panel 1**: The Sniffer floats like a kite with a USB tail.
- Sniffer: "I hear the air."
- Operator: "Channel 11. You are my ear."

**Panel 2**: A padlock icon over a zigbee packet.
- Narrator: "The packets are locked with keys you already own."
- Operator (scribble): "Network key. Trust Center key."

**Panel 3**: A pcapng file icon stamped "Collected".
- Sniffer: "All your joins belong to me."
- Operator: "I will decode you later."

### Page 5 — "The Great Documentation Parade"

**Panel 1**: A marching band of documents: Quickstart, Compendium, Playbook, Postmortem.
- Narrator: "We marched the docs through town."
- Operator: "And updated the tricky bits."

**Panel 2**: A zine stapler labeled "Diary".
- Narrator: "Every adventure needs a diary with sharp corners."
- Operator: "And now we only repeat the prompt once."

**Panel 3**: A small report labeled "Step 9 Verification".
- Operator: "We checked the docs. We didn't hallucinate."
- Narrator: "A rare victory."

### Page 6 — "Upload Night"

**Panel 1**: A sleepy courier (remarquee) carrying PDFs to a folder sign: "Projects/2026/01/Zigbee".
- Courier: "I bring your pages to the slate."
- Narrator: "We filed them under a new moon."

**Panel 2**: The Plug stands in a spotlight, now joined.
- Plug: "Okay, I'm in."
- Operator: "Now say ON."
- Plug: "ON."

**Panel 3**: A final banner: "STATUS: OPERATIONAL" with confetti.
- Narrator: "A tiny system, a loud story."
- Operator: "And still more experiments to run."

## Usage Examples

### Print-ready notes

- Use monospace blocks for shell commands as "stage directions".
- Put key MQTT topics on paper tape labels.
- Add doodled arrows between "request" and "state" topics.

### Sample sidebar (zine margin)

> "Remember: state topics are a river. Responses are postcards."

## Related

- `reference/06-lab-status-slides-obsidian.md`
- `reference/07-research-memo-bell-labs-xerox-parc-style.md`
- `reference/04-postmortem-zigbee2mqtt-bridge-request-validation.md`
