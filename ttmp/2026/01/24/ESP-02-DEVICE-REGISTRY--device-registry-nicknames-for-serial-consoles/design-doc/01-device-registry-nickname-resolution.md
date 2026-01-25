---
Title: Device Registry + Nickname Resolution
Ticket: ESP-02-DEVICE-REGISTRY
Status: active
Topics:
    - esp32
    - serial
    - esper
    - tooling
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/cmd/esper/main.go
      Note: Monitor resolves nickname passed as --port
    - Path: esper/pkg/commands/devicescmd/root.go
      Note: Cobra wiring for devices commands
    - Path: esper/pkg/commands/scancmd/scan.go
      Note: Scan output annotated with nickname metadata
    - Path: esper/pkg/devices/registry.go
      Note: Registry schema + XDG config path (ESP-02)
    - Path: esper/pkg/devices/resolve.go
      Note: Nickname to port resolution logic
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-24T21:05:14.449094451-05:00
WhatFor: ""
WhenToUse: ""
---


# Device Registry + Nickname Resolution

## Executive Summary

We add a small per-user “device registry” to `esper` so humans can refer to boards by stable nicknames rather than unstable `/dev/tty*` paths or guesswork. The registry lives in the XDG config directory as JSON (e.g. `~/.config/esper/devices.json`) and maps a stable hardware identifier (initially: the USB serial string exposed in sysfs) to metadata (`nickname`, `name`, `description`, etc.).

The registry is used in two places:

1) `esper scan` annotates discovered ports with nickname/name/description.  
2) `esper` monitor can accept a nickname and resolve it to the correct serial console path.

The implementation is intentionally minimal and Linux-first, matching the current scan implementation and development environment. It is also designed to evolve: the schema includes a version field and leaves room for additional match keys and richer device identity later.

## Problem Statement

Today, interacting with ESP devices over serial is needlessly “ephemeral”:

- The same physical device can appear as `/dev/ttyACM0` one day and `/dev/ttyACM1` the next.
- Some users have multiple Espressif devices connected simultaneously (S3 + C6 + others), and “guess by port” is error-prone.
- The scan tool can score and even probe the chip family (via esptool), but that does not answer the human question “which of these is my AtomS3R vs my XIAO C6?”.

We need a stable way to attach human-friendly identifiers (nicknames) to physical devices, so that downstream operations (monitoring, flashing, coredump decode, etc.) can target devices deterministically and ergonomically.

## Proposed Solution

### Data model

We store a JSON file under XDG config:

- If `$XDG_CONFIG_HOME` is set: `$XDG_CONFIG_HOME/esper/devices.json`
- Else: `$HOME/.config/esper/devices.json`

The file contains a versioned registry:

```json
{
  "version": 1,
  "devices": [
    {
      "usb_serial": "D8:85:AC:A4:FB:7C",
      "nickname": "atoms3r",
      "name": "M5 AtomS3R",
      "description": "bench device; used for display experiments",
      "preferred_path": "/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D8:85:AC:A4:FB:7C-if00"
    }
  ]
}
```

Design principles:

- **Stable key first**: the initial key is `usb_serial` (from sysfs `.../serial`), because it is stable per physical USB device and already collected by `esper scan`.
- **Human metadata**: `nickname` is the primary ergonomic identifier; `name` and `description` are optional and purely descriptive.
- **Optional preferred path**: we can store a best path (usually `/dev/serial/by-id/...`) as a hint, but we still validate it exists at runtime and can fall back to scanning.
- **Versioning**: a `version` field enables schema evolution without guessing.

### CLI surface

Minimal CRUD:

- `esper devices list`  
  Lists all registry entries (structured output suitable for Glazed pipelines).

- `esper devices set --usb-serial <serial> --nickname <nick> [--name ...] [--description ...] [--preferred-path ...]`  
  Creates or updates an entry.

- `esper devices remove --usb-serial <serial>` (and optionally `--nickname <nick>` later)  
  Removes an entry.

Resolution:

- `esper --port <nickname>` works: if the provided port does not look like a path/glob, treat it as a nickname and resolve it.
- (Optional future) `esper --device <nickname>` can be an explicit alias that avoids ambiguity.

### Runtime matching and resolution

We resolve nicknames to port paths as follows:

1) Load registry.
2) Find entry by `nickname` (or by explicit `usb_serial` if provided).
3) If `preferred_path` is present and exists, use it.
4) Otherwise run `ScanLinux` and pick the first port whose discovered `serial` matches the registry’s `usb_serial`. Prefer by-id paths if available.

### Integration points

- `esper scan`:
  - loads registry once,
  - for each discovered port, if `port.serial` matches a registry `usb_serial`, include `nickname`, `name`, `description` in the emitted row.

- `esper` monitor:
  - before opening the serial port, attempt nickname resolution as described above.

### Operational concerns

- **Atomic writes**: update `devices.json` by writing to a temporary file in the same directory and `rename(2)` into place, to avoid partial writes.
- **Permissions**: create the file with user-only permissions where possible (e.g. `0600`). While not strictly secret, it may contain notes.
- **Error handling**:
  - if config file does not exist: treat as empty registry.
  - if JSON is invalid: return a clear error and do not overwrite.
  - if resolution finds no connected device: return an error that suggests running `esper scan`.

## Design Decisions

1) **Keying by USB serial** (initially)  
Rationale: stable, already available in scan results, and works for USB-Serial/JTAG devices. This does not preclude adding other match keys later (e.g. FTDI serials, udev attributes, chip MAC readouts).

2) **Keep the registry per-user (XDG config)**  
Rationale: nicknames are inherently user/workflow specific; they should not require root, and they should not be part of the repository by default.

3) **Prefer by-id paths at runtime, but don’t trust them blindly**  
Rationale: `/dev/serial/by-id` is typically stable and user-friendly, but can be missing on some systems. The scanner remains the source of truth.

4) **Minimal CRUD commands**  
Rationale: keep Phase 1 small and uncontroversial; we can add import/export, tags, templated naming, and “learn from scan” ergonomics later.

## Alternatives Considered

1) **Key by `/dev/serial/by-id/...` symlink name**  
Rejected as the primary key: it is usually stable but not guaranteed, and it encodes multiple attributes in a string that is less semantically stable than the underlying serial.

2) **Key by chip MAC** (read via ROM/app)  
Deferred: would require opening the port, potentially resetting the device, and may vary by transport. It is useful later but heavier than needed for the first version.

3) **Rely on udev rules and static symlinks**  
Works for some users but pushes complexity into system configuration, is not portable, and is harder to share/document as part of `esper`.

## Implementation Plan

1) Implement `pkg/devices` to load/save a `devices.json` registry from XDG config.
2) Add `esper devices` subcommands for list/set/remove.
3) Extend monitor startup to resolve nickname → port path.
4) Extend `esper scan` output rows with registry metadata.
5) Add usage examples and keep a detailed diary with commit hashes.

## Open Questions

1) Should nickname resolution be implicit via `--port <nick>` (current plan), or explicit via `--device <nick>` (less magic)? We can support both.

2) Do we want to support multiple match keys from day one (usb serial + by-id + vendor/product)? For now, the design supports extension but implements only usb serial.

3) How do we want to handle collisions?
   - Duplicate nicknames (two devices with the same nickname).
   - One usb serial with multiple entries (should not happen).

## References

- ESP-01 ticket (`esper scan` + esptool probing) for current scan fields and constraints.
