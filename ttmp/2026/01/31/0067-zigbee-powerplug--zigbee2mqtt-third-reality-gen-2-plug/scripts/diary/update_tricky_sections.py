from pathlib import Path

path = Path('/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/reference/02-diary.md')
lines = path.read_text().splitlines()

replacements = {
    '## Step 1: Create ticket + reference docs (HA section removed)': [
        '- Underlying cause: the diary required a verbatim prompt while the reference doc needed the HA section removed without losing the rest of the content.',
        '- Symptoms: it was easy to either over-delete (losing valid Docker/MQTT guidance) or leave HA-specific references behind.',
        '- Solution: rewrote the quickstart around the Docker Compose path only, then re-scanned for "Home Assistant"/"HA" references to confirm removal.',
    ],
    '## Step 2: Build a detailed MQTT command compendium': [
        '- Underlying cause: Zigbee2MQTT has a wide command surface and version-dependent nuances.',
        '- Symptoms: a flat list risks inaccuracy, missing families, or mixing request vs state topics.',
        '- Solution: organized by command family (bridge, device, groups, OTA, etc.) and added a reference map to the official docs so each section is anchored.',
    ],
    '## Step 4: Add Zigbee sniffing playbook (CLI, nRF sniffer)': [
        '- Underlying cause: Zigbee sniffing content can easily drift into offensive or ambiguous guidance.',
        '- Symptoms: unclear boundaries could imply decrypting or attacking external networks.',
        '- Solution: added an explicit scope section (own network only), focused on using keys you already have, and avoided attack tooling guidance.',
    ],
    '## Step 5: Refine playbook details with official references': [
        '- Underlying cause: extcap behavior, channel guidance, and default keys are easy to mis-state from memory.',
        '- Symptoms: earlier drafts lacked the exact key values and the ZLL channel guidance.',
        '- Solution: cross-checked the Zigbee2MQTT docs and updated the playbook with the explicit key values and channel note.',
    ],
    '## Step 8: Launch services in tmux and resolve port conflict': [
        '- Underlying cause: Mosquitto binds to 1883 by default, but the host already had a listener.',
        '- Symptoms: `docker compose up mosquitto` failed with "bind: address already in use" while the tmux pane kept running.',
        '- Solution: updated the Compose mapping to `1884:1883` and re-ran Mosquitto, leaving Zigbee2MQTT on the internal network port.',
    ],
    '## Step 9: Run bridge request commands and capture responses': [
        '- Underlying cause: some bridge requests publish to state topics instead of `bridge/response/*`.',
        '- Symptoms: request/response tests timed out even though Zigbee2MQTT was healthy.',
        '- Solution: subscribed to the state topics (`bridge/info`, `bridge/devices`, `bridge/definitions`) and documented the split behavior.',
    ],
    '## Step 11: Write postmortem and validation playbook': [
        '- Underlying cause: the playbook needed to mirror the exact runtime (ports, topics, non-responders).',
        '- Symptoms: a generic playbook would direct users to 1883 and `bridge/response/*` only.',
        '- Solution: embedded the observed port mapping and noted which requests responded via state topics.',
    ],
}

def replace_tricky(step_heading, new_lines):
    idx = None
    for i, line in enumerate(lines):
        if line.strip() == step_heading:
            idx = i
            break
    if idx is None:
        print(f'Missing {step_heading}')
        return
    tricky_idx = None
    for i in range(idx, len(lines)):
        if lines[i].strip() == '### What was tricky to build':
            tricky_idx = i
            break
    if tricky_idx is None:
        print(f'Missing tricky section for {step_heading}')
        return
    next_idx = None
    for i in range(tricky_idx + 1, len(lines)):
        if lines[i].strip().startswith('### What warrants a second pair of eyes'):
            next_idx = i
            break
    if next_idx is None:
        print(f'Missing next section for {step_heading}')
        return
    replacement_block = ['### What was tricky to build'] + new_lines + ['']
    lines[tricky_idx:next_idx] = replacement_block

for step, new_lines in replacements.items():
    replace_tricky(step, new_lines)

path.write_text('\n'.join(lines) + '\n')
