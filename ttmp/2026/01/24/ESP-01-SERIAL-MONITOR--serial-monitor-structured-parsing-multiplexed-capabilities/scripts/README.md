# Scripts (Ticket-Local)

## Policy

All scripts **written specifically for ticket `ESP-01-SERIAL-MONITOR`** live in this directory.

Rationale:
- Keeps the investigation reproducible (scripts are versioned alongside the design+diary).
- Makes it easy to refine/replace scripts as the protocol/design evolves.
- Avoids “random one-off scripts” scattered across projects, which become hard to find or update.

## Conventions

- Prefer descriptive filenames with numeric prefixes when there will be several:
  - `01-capture-serial-png.py`
  - `02-parse-monitor-stream.py`
- Scripts should be runnable from repo root (no implicit CWD assumptions), or clearly document required CWD.
- Prefer explicit arguments over environment variables; if env vars are used, document them at the top of the script.
- Keep dependencies minimal; if a dependency is required (e.g. `pyserial`), document install and version constraints.

## How we track scripts

- If a script is created/updated, relate it to the diary (and optionally the design doc) via `docmgr doc relate`, using an absolute path.
- If a script materially changes the recommended workflow, update the ticket `playbooks/` or the main design doc accordingly.
