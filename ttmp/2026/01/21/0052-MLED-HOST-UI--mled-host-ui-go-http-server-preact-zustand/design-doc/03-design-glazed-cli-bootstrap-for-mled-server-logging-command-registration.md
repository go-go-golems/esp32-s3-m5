---
Title: 'Design: Glazed CLI bootstrap for mled-server (logging + command registration)'
Ticket: 0052-MLED-HOST-UI
Status: active
Topics:
    - ui
    - http
    - webserver
    - backend
    - preact
    - zustand
    - rest
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/mled-server/cmd/mled-server/main.go
      Note: CLI bootstrap entrypoint to add Glazed logging init + centralized command registration
    - Path: esp32-s3-m5/mled-server/internal/commands/apply.go
      Note: REST client verb to apply patterns via /api/apply
    - Path: esp32-s3-m5/mled-server/internal/commands/discover.go
      Note: Existing Glazed GlazeCommand; register via shared helper and inherit root logging
    - Path: esp32-s3-m5/mled-server/internal/commands/nodes.go
      Note: REST client verb to list nodes via /api/nodes
    - Path: esp32-s3-m5/mled-server/internal/commands/presets.go
      Note: REST client verb to list presets via /api/presets
    - Path: esp32-s3-m5/mled-server/internal/commands/serve.go
      Note: Candidate to refactor from plain Cobra to Glazed BareCommand for consistent registration
    - Path: esp32-s3-m5/mled-server/internal/commands/status.go
      Note: REST client verb to query /api/status
    - Path: esp32-s3-m5/mled-server/internal/httpapi/request_logging.go
      Note: HTTP request logging middleware (INFO for mutating
    - Path: esp32-s3-m5/mled-server/internal/restclient/client.go
      Note: Shared HTTP client for CLI verbs
ExternalSources: []
Summary: 'Standardize `mled-server` CLI bootstrap on Glazed: root-level logging flags + early initialization, plus consistent registration of subcommands via `cli.BuildCobraCommand`.'
LastUpdated: 2026-01-21T19:48:39-05:00
WhatFor: 'Describe how `mled-server` should standardize on Glazed for CLI bootstrap: root-level logging initialization plus consistent registration of both structured-output and server commands.'
WhenToUse: Use when refactoring `cmd/mled-server/main.go`, adding new subcommands, or aligning logging/config behavior across CLI tools and long-running server mode.
---



# Design: Glazed CLI bootstrap for mled-server (logging + command registration)

## Executive Summary

`mled-server` already mixes two CLI styles:
- `discover` is a Glazed command bridged into Cobra (`cli.BuildCobraCommand`)
- `serve` is a plain Cobra command (`*cobra.Command`)

This document proposes a single, consistent bootstrap that:
1) Initializes **Glazed logging** at the root (global flags + early init).
2) Registers **all subcommands** via Glazed command constructors (Bare/Glaze/Dual), converting them to Cobra uniformly.

Outcome:
- Every command inherits the same logging flags/behavior (`--log-level`, `--log-format`, …).
- The project gets Glazed’s built-in “command settings” debug flags consistently (`--print-schema`, `--print-yaml`, …) where desired.
- Adding new commands becomes “write Glazed command → register via one helper”, with a shared parser/middleware configuration.

## Problem Statement

Today (as of 2026-01-21), `esp32-s3-m5/mled-server/cmd/mled-server/main.go`:
- Does not initialize Glazed’s logging layer.
- Registers commands inconsistently:
  - `serve` is created/flagged with Cobra directly.
  - `discover` is created as a Glazed command but is special-cased via `cli.BuildCobraCommand`.

This makes it harder to:
- Provide a predictable logging surface across commands and long-running server mode.
- Reuse Glazed patterns (layers, debug flags, consistent help text).
- Add new commands without “deciding a style” each time.

## Proposed Solution

### 1) Root command: attach and initialize Glazed logging

Use Glazed’s logging layer integration pattern (from `glaze help build-first-command`):
- Add logging flags to the Cobra root.
- Initialize logging in `PersistentPreRunE`, so it runs for every subcommand after flags are parsed.

Proposed root command bootstrap:

```go
import (
    glazed_logging "github.com/go-go-golems/glazed/pkg/cmds/logging"
    "github.com/spf13/cobra"
)

rootCmd := &cobra.Command{
    Use:   "mled-server",
    Short: "MLED/1 host controller (Go) with HTTP + WS API",
    PersistentPreRunE: func(cmd *cobra.Command, args []string) error {
        return glazed_logging.InitLoggerFromCobra(cmd)
    },
}

_ = glazed_logging.AddLoggingLayerToRootCommand(rootCmd, "mled-server")
```

Notes:
- Root-level logging is preferred because it covers **both** Glazed-bridged commands and any remaining plain Cobra commands.
- `PersistentPreRunE` runs for subcommands too, so logging is active before command logic executes.

Optional (recommended during migration): route stdlib `log` to the configured Zerolog sink after initialization, so existing `log.Printf(...)` calls honor `--log-format` etc:

```go
import (
    stdlog "log"
    "github.com/rs/zerolog/log"
)

stdlog.SetFlags(0)
stdlog.SetOutput(log.Logger)
```

### 2) Command registration: standardize on Glazed constructors + a single “build” helper

Adopt a consistent pattern:
1) Every subcommand is created by a constructor in `mled-server/internal/commands`.
2) Registration always goes through `cli.BuildCobraCommand(...)` with a shared parser configuration.

Recommended shared build helper:

```go
func buildGlazedCobraCommand(cmd cmds.Command) (*cobra.Command, error) {
    return cli.BuildCobraCommand(cmd,
        cli.WithParserConfig(cli.CobraParserConfig{
            ShortHelpLayers: []string{schema.DefaultSlug},
            MiddlewaresFunc: cli.CobraCommandDefaultMiddlewares,
        }),
    )
}
```

`main.go` then becomes:
- “create root”
- “attach logging”
- “setup help system”
- “register commands”
- “execute”

### 3) Convert `serve` to a Glazed BareCommand (recommended)

To satisfy “register commands using Glazed”, convert `serve` from a `*cobra.Command` to a Glazed command:
- Implement `cmds.BareCommand` (human-mode output, no rows).
- Keep all existing flags as `fields.New(...)` definitions.
- Keep the existing behavior of starting the HTTP server and waiting for SIGINT/SIGTERM.
- Add `cli.NewCommandSettingsLayer()` so `serve` also supports Glazed’s debug flags (optional but useful).

Rationale:
- `serve` is not a “structured output” command; it’s long-running server mode, so BareCommand is the natural fit.
- We still gain:
  - consistent help formatting,
  - layered config parsing capabilities (if later enabled),
  - consistent registration and logging bootstrap.

### 4) Keep `discover` as a GlazeCommand

`discover` already follows the Glazed GlazeCommand pattern and yields rows for `--output json/csv/...`.
The only change needed is to register it via the shared helper (and, if desired, add the logging layer to the root so `discover` also gets consistent logging flags).

### 5) Help system remains Cobra-based

Keep the current approach:

```go
helpSystem := help.NewHelpSystem()
help_cmd.SetupCobraRootCommand(helpSystem, rootCmd)
```

Glazed commands still render fine via Cobra; the enhanced help system continues to work as-is.

## Design Decisions

### Root-level logging vs per-command logging layer

Two viable ways to initialize Glazed logging:
1) **Root-level Cobra integration**:
   - `logging.AddLoggingLayerToRootCommand(rootCmd, "...")`
   - `logging.InitLoggerFromCobra(cmd)` in `PersistentPreRunE`
2) **Per-command Glazed layer**:
   - include `logging.NewLoggingLayer()` in each command’s `WithLayersList(...)`
   - call `logging.SetupLoggingFromParsedLayers(parsedLayers)` inside each command’s `Run...`

This design chooses (1) because:
- It centralizes logging setup in one place.
- It covers mixed command types (Glazed + plain Cobra) during migration.
- It makes logging available before any subcommand does significant work.

### Make `serve` a BareCommand (not a GlazeCommand)

`serve` does not naturally produce row data. Making it a BareCommand keeps:
- a clean CLI surface (no irrelevant output-format flags),
- the long-running server semantics,
- the ability to adopt Glazed layers for config/debugging without forcing structured output.

## Alternatives Considered

### Keep `serve` as plain Cobra, only Glaze-ify `discover`

Pros:
- smallest code change

Cons:
- does not meet the goal of “register commands using Glazed”
- keeps two different patterns for adding flags/help/registration

### Add per-command logging layers and initialize inside each command

Pros:
- logging configuration becomes part of each command’s schema/layers

Cons:
- duplicated setup across commands
- more complex migration for `serve` (especially if it remains plain Cobra)

### Convert `serve` to a GlazeCommand

Pros:
- could support machine-readable status output on start, etc.

Cons:
- output layers/flags are mostly irrelevant for a long-running server
- risks confusing users with output-format flags where they don’t apply

## Implementation Plan

1) Update `esp32-s3-m5/mled-server/cmd/mled-server/main.go`:
   - add Glazed logging layer flags to root
   - initialize logging in `PersistentPreRunE`
   - consolidate Glazed command registration into a helper
2) Refactor `mled-server/internal/commands/serve.go`:
   - replace `NewServeCommand() *cobra.Command` with `NewServeCommand() (*ServeCommand, error)`
   - implement `cmds.BareCommand` (and keep behavior identical)
3) Keep `mled-server/internal/commands/discover.go` mostly unchanged:
   - register through the shared helper
4) Optional: migrate internal logging calls:
   - either route stdlib `log` through Zerolog (quick)
   - or switch to `github.com/rs/zerolog/log` calls where structured fields matter
5) Validate:
   - `mled-server --help` shows logging flags
   - `mled-server serve --help` and `mled-server discover --help` behave as expected
   - `mled-server discover --output json` still works

## Open Questions

1) Should `serve` support Glazed config overlays (config files / env var layers) as a first-class feature, or stay purely flag-driven?
2) Should `serve` expose a structured “status” subcommand (dual command pattern) for scripting/automation?
3) Do we want to standardize on Zerolog in all packages, or keep stdlib log routed through Zerolog for now?

## References

- `glaze help build-first-command` (logging bootstrap pattern and `cli.BuildCobraCommand` integration)
- `glaze help logging-layer-reference` (full API surface of the logging layer)
