# Serial Monitor: structured parsing + multiplexed capabilities

This is the document workspace for ticket ESP-01-SERIAL-MONITOR.

## Structure

- **design/**: Design documents and architecture notes
- **reference/**: Reference documentation and API contracts
- **playbooks/**: Operational playbooks and procedures
- **scripts/**: Utility scripts and automation (ticket-local; see `scripts/README.md`)
- **sources/**: External sources and imported documents
- **various/**: Scratch or meeting notes, working notes
- **archive/**: Optional space for deprecated or reference-only artifacts

## Getting Started

Use docmgr commands to manage this workspace:

- Add documents: `docmgr doc add --ticket ESP-01-SERIAL-MONITOR --doc-type design-doc --title "My Design"`
- Import sources: `docmgr import file --ticket ESP-01-SERIAL-MONITOR --file /path/to/doc.md`
- Update metadata: `docmgr meta update --ticket ESP-01-SERIAL-MONITOR --field Status --value review`
