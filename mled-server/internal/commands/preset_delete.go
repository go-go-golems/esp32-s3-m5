package commands

import (
	"context"
	"errors"
	"fmt"
	"strings"
	"time"

	"github.com/go-go-golems/glazed/pkg/cli"
	"github.com/go-go-golems/glazed/pkg/cmds"
	"github.com/go-go-golems/glazed/pkg/cmds/fields"
	"github.com/go-go-golems/glazed/pkg/cmds/schema"
	"github.com/go-go-golems/glazed/pkg/cmds/values"
	"github.com/go-go-golems/glazed/pkg/middlewares"
	"github.com/go-go-golems/glazed/pkg/types"

	"mled-server/internal/restclient"
)

type PresetDeleteCommand struct {
	*cmds.CommandDescription
}

type PresetDeleteSettings struct {
	Server    string `glazed.parameter:"server"`
	TimeoutMS int    `glazed.parameter:"timeout-ms"`

	ID string `glazed.parameter:"id"`
}

var _ cmds.GlazeCommand = &PresetDeleteCommand{}

func NewPresetDeleteCommand() (*PresetDeleteCommand, error) {
	glazedLayer, err := schema.NewGlazedSchema()
	if err != nil {
		return nil, err
	}
	commandSettingsLayer, err := cli.NewCommandSettingsLayer()
	if err != nil {
		return nil, err
	}

	flags := append([]*fields.Definition{}, clientFlags()...)
	flags = append(flags, fields.New("id", fields.TypeString, fields.WithRequired(true), fields.WithHelp("Preset ID")))

	cmdDesc := cmds.NewCommandDescription(
		"delete",
		cmds.WithParents("preset"),
		cmds.WithShort("Delete a preset via REST"),
		cmds.WithLong(`Delete a preset by calling the running server's REST API (DELETE /api/presets/:id).

Examples:
  mled-server preset delete --id preset-123
`),
		cmds.WithFlags(flags...),
		cmds.WithLayersList(glazedLayer, commandSettingsLayer),
	)

	return &PresetDeleteCommand{CommandDescription: cmdDesc}, nil
}

func (c *PresetDeleteCommand) RunIntoGlazeProcessor(ctx context.Context, vals *values.Values, gp middlewares.Processor) error {
	settings := &PresetDeleteSettings{}
	if err := values.DecodeSectionInto(vals, schema.DefaultSlug, settings); err != nil {
		return fmt.Errorf("failed to parse settings: %w", err)
	}

	id := strings.TrimSpace(settings.ID)
	if id == "" {
		return errors.New("--id is required")
	}

	client := restclient.New(settings.Server, time.Duration(settings.TimeoutMS)*time.Millisecond)
	if err := client.DeletePreset(ctx, id); err != nil {
		return err
	}

	row := types.NewRowFromMap(map[string]any{
		"ok":     true,
		"id":     id,
		"server": settings.Server,
	})
	return gp.AddRow(ctx, row)
}
