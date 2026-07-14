# PaperS3 E-Reader Native Primitives and Future JavaScript API

This is the document workspace for ticket ESP-50-PAPERS3-EREADER-PRIMITIVES.

## Start here

- [Ticket overview](./index.md)
- [Native primitives analysis, design, and implementation guide](./design-doc/01-papers3-e-reader-primitives-analysis-design-and-implementation-guide.md)
- [Investigation diary](./reference/01-investigation-diary.md)
- [Phased implementation tasks](./tasks.md)
- [Source inventory](./sources/README.md)
- [Reproducible research trace](./scripts/00-research-log.md)

## Structure

- **design-doc/**: architecture, API contracts, decisions, tests, and implementation phases
- **reference/**: chronological investigation diary
- **scripts/**: numbered research scripts and preserved output snapshots
- **sources/local/**: imported s3paper design and studio implementation
- **sources/web/**: Defuddle captures of hardware, driver, runtime, and reader references

## Current state

Research and design are complete. The fourteen implementation phases remain open, beginning with a real-hardware ESP-IDF/M5GFX qualification matrix. MicroQuickJS is intentionally deferred until the native reader vertical slice and generic primitives pass their own acceptance tests.
