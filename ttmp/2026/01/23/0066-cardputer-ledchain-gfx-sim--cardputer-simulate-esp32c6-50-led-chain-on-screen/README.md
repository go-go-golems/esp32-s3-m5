# Cardputer: simulate ESP32C6 50-LED chain on screen

This is the document workspace for ticket 0066-cardputer-ledchain-gfx-sim.

## Structure

- **design/**: Design documents and architecture notes
- **reference/**: Reference documentation and API contracts
- **playbooks/**: Operational playbooks and procedures
- **scripts/**: Utility scripts and automation
- **sources/**: External sources and imported documents
- **various/**: Scratch or meeting notes, working notes
- **archive/**: Optional space for deprecated or reference-only artifacts

## Getting Started

Use docmgr commands to manage this workspace:

- Add documents: `docmgr doc add --ticket 0066-cardputer-ledchain-gfx-sim --doc-type design-doc --title "My Design"`
- Import sources: `docmgr import file --ticket 0066-cardputer-ledchain-gfx-sim --file /path/to/doc.md`
- Update metadata: `docmgr meta update --ticket 0066-cardputer-ledchain-gfx-sim --field Status --value review`

## GPIO notes (G3/G4)

Ticket 0066 uses **board labels** `G3` and `G4` as the JS-exposed GPIOs (for sequencers / toggles).

Because “G3/G4” are *board labels* (not ESP32 pin numbers), the firmware uses explicit Kconfig mappings:

- `CONFIG_TUTORIAL_0066_G3_GPIO` (default: 3)
- `CONFIG_TUTORIAL_0066_G4_GPIO` (default: 4)

These defaults are inferred from an evidence-based precedent:

- In tutorial `0047-cardputer-adv-lvgl-chain-encoder-list`, the board labels `G1` and `G2` default to GPIO1 and GPIO2.

If your hardware wiring differs, override these in `menuconfig` (project menu `0066: Cardputer LED chain GFX sim`), and validate by running:

- `js eval gpio.toggle('G3'); gpio.toggle('G4');`

and confirming pin behavior with a logic probe / scope / multimeter.
