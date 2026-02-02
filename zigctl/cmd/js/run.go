package js

import (
	"context"
	"fmt"

	"github.com/go-go-golems/glazed/pkg/cli"
	"github.com/go-go-golems/glazed/pkg/cmds"
	"github.com/go-go-golems/glazed/pkg/cmds/fields"
	"github.com/go-go-golems/glazed/pkg/cmds/layers"
	"github.com/go-go-golems/glazed/pkg/cmds/schema"
	"github.com/go-go-golems/zigctl/pkg/jsruntime"
)

type RunCommand struct {
	*cmds.CommandDescription
}

type RunSettings struct {
	Script string   `glazed.parameter:"script"`
	Args   []string `glazed.parameter:"arg"`
}

var _ cmds.BareCommand = (*RunCommand)(nil)

func NewRunCommand() (*RunCommand, error) {
	commandSettingsLayer, err := cli.NewCommandSettingsLayer()
	if err != nil {
		return nil, err
	}

	cmdDesc := cmds.NewCommandDescription(
		"run",
		cmds.WithShort("Run a JavaScript file"),
		cmds.WithLong(`
Run a JavaScript file using the zigctl JS runtime.

Examples:
  zigctl js run script.js
  zigctl js run script.js --arg foo --arg bar
`),
		cmds.WithArguments(
			fields.New(
				"script",
				fields.TypeString,
				fields.WithRequired(true),
				fields.WithHelp("Path to a JavaScript file"),
			),
		),
		cmds.WithFlags(
			fields.New(
				"arg",
				fields.TypeStringList,
				fields.WithDefault([]string{}),
				fields.WithHelp("Arguments to expose as global zigctlArgs"),
			),
		),
		cmds.WithLayersList(commandSettingsLayer),
	)

	return &RunCommand{CommandDescription: cmdDesc}, nil
}

func (c *RunCommand) Run(ctx context.Context, parsedLayers *layers.ParsedLayers) error {
	settings := &RunSettings{}
	if err := parsedLayers.InitializeStruct(schema.DefaultSlug, settings); err != nil {
		return err
	}
	if settings.Script == "" {
		return fmt.Errorf("script path is required")
	}

	vm, req := jsruntime.New()
	if len(settings.Args) > 0 {
		if err := vm.Set("zigctlArgs", settings.Args); err != nil {
			return err
		}
	}

	_, err := req.Require(settings.Script)
	return err
}
