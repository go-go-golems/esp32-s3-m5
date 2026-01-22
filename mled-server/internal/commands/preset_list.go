package commands

import (
	"context"
	"fmt"
	"time"

	"github.com/go-go-golems/glazed/pkg/cli"
	"github.com/go-go-golems/glazed/pkg/cmds"
	"github.com/go-go-golems/glazed/pkg/cmds/schema"
	"github.com/go-go-golems/glazed/pkg/cmds/values"
	"github.com/go-go-golems/glazed/pkg/middlewares"
	"github.com/go-go-golems/glazed/pkg/types"

	"mled-server/internal/restclient"
)

type PresetListCommand struct {
	*cmds.CommandDescription
}

type PresetListSettings struct {
	Server    string `glazed.parameter:"server"`
	TimeoutMS int    `glazed.parameter:"timeout-ms"`
}

var _ cmds.GlazeCommand = &PresetListCommand{}

func NewPresetListCommand() (*PresetListCommand, error) {
	glazedLayer, err := schema.NewGlazedSchema()
	if err != nil {
		return nil, err
	}
	commandSettingsLayer, err := cli.NewCommandSettingsLayer()
	if err != nil {
		return nil, err
	}

	cmdDesc := cmds.NewCommandDescription(
		"list",
		cmds.WithParents("preset"),
		cmds.WithShort("List presets via REST"),
		cmds.WithLong(`List stored presets from a running mled-server instance (GET /api/presets).

Examples:
  mled-server preset list
  mled-server preset list --output json
`),
		cmds.WithFlags(clientFlags()...),
		cmds.WithLayersList(glazedLayer, commandSettingsLayer),
	)

	return &PresetListCommand{CommandDescription: cmdDesc}, nil
}

func (c *PresetListCommand) RunIntoGlazeProcessor(ctx context.Context, vals *values.Values, gp middlewares.Processor) error {
	settings := &PresetListSettings{}
	if err := values.DecodeSectionInto(vals, schema.DefaultSlug, settings); err != nil {
		return fmt.Errorf("failed to parse settings: %w", err)
	}

	client := restclient.New(settings.Server, time.Duration(settings.TimeoutMS)*time.Millisecond)
	ps, err := client.Presets(ctx)
	if err != nil {
		return err
	}

	for _, p := range ps {
		row := types.NewRowFromStruct(&p, true)
		if err := gp.AddRow(ctx, row); err != nil {
			return err
		}
	}
	return nil
}
