package main

import (
	"fmt"
	stdlog "log"
	"os"

	"github.com/go-go-golems/glazed/pkg/cli"
	"github.com/go-go-golems/glazed/pkg/cmds"
	glazed_logging "github.com/go-go-golems/glazed/pkg/cmds/logging"
	"github.com/go-go-golems/glazed/pkg/cmds/schema"
	"github.com/go-go-golems/glazed/pkg/help"
	help_cmd "github.com/go-go-golems/glazed/pkg/help/cmd"
	"github.com/rs/zerolog/log"
	"github.com/spf13/cobra"

	"mled-server/internal/commands"
)

func main() {
	rootCmd := &cobra.Command{
		Use:   "mled-server",
		Short: "MLED/1 host controller (Go) with HTTP + WS API",
		PersistentPreRunE: func(cmd *cobra.Command, args []string) error {
			if err := glazed_logging.InitLoggerFromCobra(cmd); err != nil {
				return err
			}
			stdlog.SetFlags(0)
			stdlog.SetOutput(log.Logger)
			return nil
		},
	}

	_ = glazed_logging.AddLoggingLayerToRootCommand(rootCmd, "mled-server")

	helpSystem := help.NewHelpSystem()
	help_cmd.SetupCobraRootCommand(helpSystem, rootCmd)

	serveCmd, err := commands.NewServeCommand()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error creating serve command: %v\n", err)
		os.Exit(1)
	}
	discoverCmd, err := commands.NewDiscoverCommand()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error creating discover command: %v\n", err)
		os.Exit(1)
	}
	statusCmd, err := commands.NewStatusCommand()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error creating status command: %v\n", err)
		os.Exit(1)
	}
	nodesCmd, err := commands.NewNodesCommand()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error creating nodes command: %v\n", err)
		os.Exit(1)
	}
	presetsCmd, err := commands.NewPresetsCommand()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error creating presets command: %v\n", err)
		os.Exit(1)
	}
	applyCmd, err := commands.NewApplyCommand()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error creating apply command: %v\n", err)
		os.Exit(1)
	}
	applyPresetCmd, err := commands.NewApplyPresetCommand()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error creating apply-preset command: %v\n", err)
		os.Exit(1)
	}
	applySolidCmd, err := commands.NewApplySolidCommand()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error creating apply-solid command: %v\n", err)
		os.Exit(1)
	}
	applyRainbowCmd, err := commands.NewApplyRainbowCommand()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error creating apply-rainbow command: %v\n", err)
		os.Exit(1)
	}
	presetAddCmd, err := commands.NewPresetAddCommand()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error creating preset add command: %v\n", err)
		os.Exit(1)
	}
	presetListCmd, err := commands.NewPresetListCommand()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error creating preset list command: %v\n", err)
		os.Exit(1)
	}
	presetUpdateCmd, err := commands.NewPresetUpdateCommand()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error creating preset update command: %v\n", err)
		os.Exit(1)
	}
	presetDeleteCmd, err := commands.NewPresetDeleteCommand()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error creating preset delete command: %v\n", err)
		os.Exit(1)
	}
	settingsGetCmd, err := commands.NewSettingsGetCommand()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error creating settings get command: %v\n", err)
		os.Exit(1)
	}
	settingsSetCmd, err := commands.NewSettingsSetCommand()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error creating settings set command: %v\n", err)
		os.Exit(1)
	}

	allCommands := []cmds.Command{
		serveCmd,
		discoverCmd,
		statusCmd,
		nodesCmd,
		presetsCmd,
		applyCmd,
		applyPresetCmd,
		applySolidCmd,
		applyRainbowCmd,
		presetAddCmd,
		presetListCmd,
		presetUpdateCmd,
		presetDeleteCmd,
		settingsGetCmd,
		settingsSetCmd,
	}

	if err := cli.AddCommandsToRootCommand(rootCmd, allCommands, nil,
		cli.WithParserConfig(cli.CobraParserConfig{
			ShortHelpLayers: []string{schema.DefaultSlug},
			MiddlewaresFunc: cli.CobraCommandDefaultMiddlewares,
		}),
	); err != nil {
		fmt.Fprintf(os.Stderr, "Error registering commands: %v\n", err)
		os.Exit(1)
	}

	if err := rootCmd.Execute(); err != nil {
		os.Exit(1)
	}
}
