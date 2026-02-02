---
Title: Geppetto layer wiring in Cobra/Glazed
Ticket: 0067-zigbee-powerplug
Status: active
Topics:
    - zigbee
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../code/wesen/corporate-headquarters/geppetto/pkg/layers/layers.go
      Note: Geppetto layer factory + middleware chain
    - Path: ../../../../../../../../../../code/wesen/corporate-headquarters/pinocchio/cmd/pinocchio/main.go
      Note: Root wiring with repository loading + parser config
    - Path: ../../../../../../../../../../code/wesen/corporate-headquarters/pinocchio/pkg/cmds/cobra.go
      Note: Cobra builder wrapper with Geppetto middlewares
    - Path: ../../../../../../../../../../code/wesen/corporate-headquarters/pinocchio/pkg/cmds/loader.go
      Note: Command loader that attaches Geppetto + helpers layers
ExternalSources: []
Summary: How Pinocchio wires Geppetto parameter layers and middlewares into Cobra/Glazed so subcommands share the same layer stack.
LastUpdated: 2026-02-02T00:27:00-05:00
WhatFor: Reference pattern for adding Geppetto layers + middlewares to Cobra commands (including subcommands).
WhenToUse: Use when you need all commands in a tree to share Geppetto layers and config/profile middleware behavior.
---


# Playbook: Geppetto layer wiring in Cobra/Glazed

## Purpose

Explain and document the canonical pattern in Pinocchio/Geppetto for:
- creating Geppetto parameter layers,
- attaching them to commands,
- and ensuring Cobra parsing runs the same middleware chain for subcommands.

## Environment assumptions

- You have access to the `geppetto` and `pinocchio` source trees in:
  - `/home/manuel/code/wesen/corporate-headquarters/geppetto`
  - `/home/manuel/code/wesen/corporate-headquarters/pinocchio`
- You are using Glazed-based commands (`cmds.Command`) and Cobra builders from `github.com/go-go-golems/glazed/pkg/cli`.

## Where to look (source of truth)

- Geppetto layer builder + middleware chain:
  - `/home/manuel/code/wesen/corporate-headquarters/geppetto/pkg/layers/layers.go`
    - `CreateGeppettoLayers(...)`
    - `GetCobraCommandGeppettoMiddlewares(...)`
- Cobra command builder wrapper for Pinocchio commands:
  - `/home/manuel/code/wesen/corporate-headquarters/pinocchio/pkg/cmds/cobra.go`
- Pinocchio root command wiring:
  - `/home/manuel/code/wesen/corporate-headquarters/pinocchio/cmd/pinocchio/main.go`
- Pinocchio command loader (adds helper layer on top of Geppetto layers):
  - `/home/manuel/code/wesen/corporate-headquarters/pinocchio/pkg/cmds/loader.go`

## Pattern overview

There are two distinct responsibilities:

1) **Layer creation** (what flags exist)
   - `CreateGeppettoLayers` returns a list of `cmdlayers.ParameterLayer` objects for AI chat/client/provider configs.
   - Pinocchio adds its own helper layer on top of that list before attaching to commands.

2) **Middleware wiring** (how flags/config/env/profile data are parsed and merged)
   - `GetCobraCommandGeppettoMiddlewares` returns the middleware chain used by Cobra/Glazed parsing.
   - This chain explicitly handles:
     - Cobra flags
     - positional args
     - env (PINOCCHIO_*)
     - config files
     - profiles
     - defaults

## Step-by-step playbook

### Step 1: Create Geppetto layers (with defaults)

Geppetto exposes a single factory that creates all AI-related layers, optionally with defaults:

```go
geppettoLayers, err := geppettolayers.CreateGeppettoLayers(
  geppettolayers.WithDefaultsFromStepSettings(stepSettings),
)
```

Source: `geppetto/pkg/layers/layers.go`

### Step 2: Add your own helper layer (Pinocchio pattern)

Pinocchio adds a “helpers” layer in front of the Geppetto layers:

```go
helpersLayer, err := cmdlayers.NewHelpersParameterLayer()
if err != nil {
  return nil, err
}
ls := append([]layers.ParameterLayer{helpersLayer}, geppettoLayers...)
```

Source: `pinocchio/pkg/cmds/loader.go`

### Step 3: Attach layers to command descriptions

Pinocchio’s loader attaches the resulting layers list to the command description:

```go
cmds.WithLayersList(ls...)
```

Source: `pinocchio/pkg/cmds/loader.go`

This is the critical step: **layers live on the CommandDescription** (not on Cobra directly). Cobra parsing will then include these layers when built.

### Step 4: Build Cobra commands with Geppetto middlewares

Pinocchio wraps the Cobra builder so every command gets the same middleware chain:

```go
func BuildCobraCommandWithGeppettoMiddlewares(cmd cmds.Command, options ...cli.CobraOption) (*cobra.Command, error) {
  config := cli.CobraParserConfig{
    MiddlewaresFunc: layers2.GetCobraCommandGeppettoMiddlewares,
    ShortHelpLayers: []string{layers.DefaultSlug, cmdlayers.GeppettoHelpersSlug},
  }
  options_ := append([]cli.CobraOption{cli.WithParserConfig(config)}, options...)
  return cli.BuildCobraCommand(cmd, options_...)
}
```

Source: `pinocchio/pkg/cmds/cobra.go`

### Step 5: Ensure repository command loading uses the same middleware chain

Pinocchio uses `repositories.LoadRepositories` with explicit parser config:

```go
allCommands, err := repositories.LoadRepositories(
  helpSystem,
  rootCmd,
  repositories_,
  cli.WithCobraMiddlewaresFunc(layers2.GetCobraCommandGeppettoMiddlewares),
  cli.WithCobraShortHelpLayers(layers.DefaultSlug, cmdlayers.GeppettoHelpersSlug),
  cli.WithProfileSettingsLayer(),
  cli.WithCreateCommandSettingsLayer(),
)
```

Source: `pinocchio/cmd/pinocchio/main.go`

This ensures dynamically loaded commands get the same middleware stack as hard-coded commands.

## What the Geppetto middlewares do (important details)

`GetCobraCommandGeppettoMiddlewares` does more than just “ParseFromCobraCommand”. It:

- **Bootstraps** command settings from Cobra + env + defaults (without config) so it can resolve the profile selection early.
- **Resolves config files** once and reuses that list for both bootstrap and main parsing.
- **Bootstraps profile settings** (profile name + file) from config + env + flags + defaults.
- **Applies middlewares in a deliberate precedence order**:
  1) Cobra flags
  2) Positional args
  3) Env (PINOCCHIO_*)
  4) Profile values (from profile file)
  5) Config files
  6) Defaults

Notes:
- The config mapper filters out non-layer keys (e.g., `repositories`) so config parsing does not break.
- The profile middleware is applied *after* config (so profiles override config) but before defaults.

Source: `geppetto/pkg/layers/layers.go`.

## Minimal pseudocode template (reuse this pattern)

```go
// 1) Build layers
baseLayers, _ := geppettolayers.CreateGeppettoLayers(...)
helperLayer, _ := cmdlayers.NewHelpersParameterLayer()
allLayers := append([]layers.ParameterLayer{helperLayer}, baseLayers...)

// 2) Attach to command description
cmd := cmds.NewCommandDescription(
  "my-command",
  cmds.WithLayersList(allLayers...),
  // ... other options
)

// 3) Build Cobra with Geppetto middlewares
cobraCmd, _ := cli.BuildCobraCommand(
  cmd,
  cli.WithParserConfig(cli.CobraParserConfig{
    MiddlewaresFunc: geppettolayers.GetCobraCommandGeppettoMiddlewares,
    ShortHelpLayers: []string{layers.DefaultSlug, cmdlayers.GeppettoHelpersSlug},
  }),
)
```

## Exit criteria

- All commands in a tree parse the same layer stack (Geppetto + helpers).
- Subcommands behave identically whether invoked directly or loaded dynamically.
- Profiles and config files override defaults in the expected order.

## Failure modes to watch

- **Duplicate flags**: attaching the same layer to parent + child commands causes Cobra flag registration errors.
- **Profiles ignored**: if `GetCobraCommandGeppettoMiddlewares` isn’t used, profile settings will not be parsed correctly.
- **Config keys not mapped to layers**: missing configMapper can cause unexpected parsing errors.

## Expected outputs (sanity checks)

- `--profile` should change AI settings without having to repeat flags.
- `PINOCCHIO_*` env vars should override config.
- The same command should parse identically whether loaded from repositories or built explicitly.

