---
Title: Diary
Ticket: ESP-02-DEVICE-REGISTRY
Status: active
Topics:
    - esp32
    - serial
    - esper
    - tooling
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-24T21:05:14.788420828-05:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Implement a per-user device registry for `esper` so we can assign nicknames (plus name/description) to physical ESP devices and resolve those nicknames to the correct serial console at runtime.

## Step 1: Ticket Setup + Initial Design

This step sets up the new docmgr ticket workspace and documents the intended feature: a versioned `devices.json` registry stored under XDG config (e.g. `~/.config/esper/devices.json`). The goal is to stop “guessing by port name” and instead refer to devices by stable nicknames.

### Prompt Context

**User prompt (verbatim):** "Let's start a new docmgr ticket ESP-02-... (you chose the name) and we want to assign descriptions, names and nicknames to certaindevices (by serial, but the store in json in ~/.config (XDG_CONFIG stuff) contains all the json to look up). That way we can refer to nicknames and get the serial console and all, instead of having to guess

Add a design document on how to implement that feature, then create tasks in the ticket. then implement the tasks, checkk them off, write your detailed diary and commit to git."

**Assistant interpretation:** Create a new ticket, write a design doc and task list for a per-user device registry, then implement it in `esper` and keep the diary updated with commit hashes while checking tasks off.

**Inferred user intent:** Make serial workflows deterministic and ergonomic when multiple devices are connected by letting humans refer to boards by nicknames instead of `/dev/tty*`.

### What I did
- Created ticket `ESP-02-DEVICE-REGISTRY` and added:
  - a design document (`design-doc/01-device-registry-nickname-resolution.md`)
  - this diary (`reference/01-diary.md`)
- Created an initial task list for implementation.

### Why
- Documentation first keeps the CLI surface and storage format stable as we start wiring this into `esper scan` and monitor.

### What worked
- `docmgr` created the ticket workspace and documents successfully.

### What didn't work
- I accidentally used shell backticks in a `docmgr task add` command, which caused zsh to try to execute `esper` instead of storing the literal text. I fixed the resulting tasks file to remove/replace the broken entries.

### What I learned
- For docmgr, never include unquoted backticks in shell arguments; zsh will interpret them as command substitution.

### What was tricky to build
- N/A (this step is setup + documentation).

### What warrants a second pair of eyes
- N/A.

### What should be done in the future
- Implement the registry package and CLI wiring, then integrate scan+monitor resolution.

## Step 2: Implement Registry Load/Save (XDG Config) + Nickname Resolution

This step implements the core persistence layer: a versioned registry stored as JSON under the user’s XDG config directory. The registry can be loaded when present, defaults to an empty registry when missing, and is saved using an atomic write strategy (write temp file then rename).

It also includes a first resolution helper that maps a nickname to a port path by consulting the registry and, when needed, running `ScanLinux` to find the currently connected device that matches the recorded USB serial string.

### Prompt Context

**User prompt (verbatim):** "Let's start a new docmgr ticket ESP-02-... (you chose the name) and we want to assign descriptions, names and nicknames to certaindevices (by serial, but the store in json in ~/.config (XDG_CONFIG stuff) contains all the json to look up). That way we can refer to nicknames and get the serial console and all, instead of having to guess

Add a design document on how to implement that feature, then create tasks in the ticket. then implement the tasks, checkk them off, write your detailed diary and commit to git."

**Assistant interpretation:** Implement the storage + lookup mechanics for a per-user device registry and provide a nickname→port resolution function.

**Inferred user intent:** A stable, user-controlled mapping from “this physical board” to “this nickname” that future tooling can reuse.

**Commit (code):** 91d291d — "devices: add XDG registry load/save"

### What I did
- Added `pkg/devices`:
  - `ConfigPath()` returns the XDG config path for `devices.json`.
  - `Load()` and `Save()` read/write a versioned registry.
  - `ResolveNicknameToPort()` resolves nickname → current serial console path, preferring `preferred_path` if present.

### Why
- Nicknames are only useful if they persist across sessions and can be resolved deterministically.

### What worked
- Atomic JSON writes keep the registry robust against partial writes.

### What didn't work
- N/A.

### What I learned
- The simplest practical key for ESP USB Serial/JTAG devices is the USB serial string exposed via sysfs; it is already harvested by `esper scan`.

### What was tricky to build
- Keeping the registry API minimal while leaving clear expansion points (schema versioning, additional match keys).

### What warrants a second pair of eyes
- Whether we should add `fsync` on the temp file and directory for stronger durability guarantees, or keep the simple rename-based atomicity.

### What should be done in the future
- Add higher-level commands that let users populate the registry directly from scan output (ergonomics).

## Step 3: Add `esper devices` Commands + Annotate Scan + Resolve in Monitor

This step wires the registry into the CLI so the feature is usable end-to-end. We add a `devices` command group that can list registry entries (Glazed output) and mutate entries via `set`/`remove`. We then integrate the registry into `esper scan` (adds nickname/name/description columns) and into the monitor entrypoint (allow `--port <nickname>`).

### Prompt Context

**User prompt (verbatim):** "Let's start a new docmgr ticket ESP-02-... (you chose the name) and we want to assign descriptions, names and nicknames to certaindevices (by serial, but the store in json in ~/.config (XDG_CONFIG stuff) contains all the json to look up). That way we can refer to nicknames and get the serial console and all, instead of having to guess

Add a design document on how to implement that feature, then create tasks in the ticket. then implement the tasks, checkk them off, write your detailed diary and commit to git."

**Assistant interpretation:** Add CLI commands to manage the registry, then integrate registry metadata into scan output and nickname resolution into monitor startup.

**Inferred user intent:** Make `esper` usable in the “two devices connected” scenario without guessing which port is which.

**Commit (code):** 72f2c72 — "cmd: add devices registry commands"

### What I did
- Added `esper devices`:
  - `esper devices list` (Glazed structured output)
  - `esper devices set` (create/update)
  - `esper devices remove`
  - `esper devices resolve` (debug helper)
  - `esper devices path` (prints config path)
- Extended `esper scan` rows with registry metadata:
  - `nickname`, `device_name`, `device_description`
- Updated the monitor entrypoint to resolve `--port <nickname>` into a concrete port path (and fail with a clear error if the nickname is unknown).

### Why
- The storage layer alone doesn’t improve day-to-day ergonomics; the CLI wiring does.
- Scan annotation makes it easier to spot “which device is which” before launching monitor/flash workflows.

### What worked
- After setting an entry, `esper scan --fields ...nickname...` shows the annotation immediately.
- `esper devices resolve` prints the expected `/dev/serial/by-id/...` path.

### What didn't work
- N/A.

### What I learned
- Glazed is a good fit for “inventory list” commands like `devices list` and `scan`, while mutating commands can stay simple Cobra commands.

### What was tricky to build
- Getting the boundaries right: keep the scan logic in `pkg/scan`, and do “registry joins” at the command layer (`pkg/commands/scancmd`).

### What warrants a second pair of eyes
- The heuristic “non-path --port means nickname” is convenient but slightly magical; we may want an explicit `--device` alias later.

### What should be done in the future
- Add `esper monitor --device <nick>` (explicit flag) and/or accept nicknames anywhere we accept a port glob.
