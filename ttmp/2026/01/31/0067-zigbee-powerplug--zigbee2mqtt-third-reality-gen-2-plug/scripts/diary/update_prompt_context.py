from pathlib import Path

path = Path('/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/reference/02-diary.md')
lines = path.read_text().splitlines()

def replace_prompt(step_heading, replacement):
    idx = None
    for i, line in enumerate(lines):
        if line.strip() == step_heading:
            idx = i
            break
    if idx is None:
        print(f'Missing {step_heading}')
        return
    pc_idx = None
    for i in range(idx, len(lines)):
        if lines[i].strip() == '### Prompt Context':
            pc_idx = i
            break
    if pc_idx is None:
        print(f'Missing Prompt Context for {step_heading}')
        return
    up_idx = None
    for i in range(pc_idx + 1, len(lines)):
        if lines[i].strip().startswith('**User prompt (verbatim):**'):
            up_idx = i
            break
    if up_idx is None:
        print(f'Missing user prompt for {step_heading}')
        return
    ai_idx = None
    for i in range(up_idx + 1, len(lines)):
        if lines[i].strip().startswith('**Assistant interpretation:**'):
            ai_idx = i
            break
    if ai_idx is None:
        print(f'Missing assistant interpretation for {step_heading}')
        return
    new_block = [f'**User prompt (verbatim):** {replacement}', '']
    lines[up_idx:ai_idx] = new_block

replace_prompt('## Step 5: Refine playbook details with official references', '(same as Step 4)')
replace_prompt('## Step 8: Launch services in tmux and resolve port conflict', '(same as Step 7)')
replace_prompt('## Step 9: Run bridge request commands and capture responses', '(same as Step 7)')
replace_prompt('## Step 11: Write postmortem and validation playbook', '(same as Step 7)')
replace_prompt('## Step 13: Normalize postmortem to ASCII and re-upload diary', '(same as Step 12)')

path.write_text('\n'.join(lines) + '\n')
