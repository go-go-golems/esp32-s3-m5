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
	"mled-server/internal/storage"
)

type PresetUpdateCommand struct {
	*cmds.CommandDescription
}

type PresetUpdateSettings struct {
	Server    string `glazed.parameter:"server"`
	TimeoutMS int    `glazed.parameter:"timeout-ms"`

	ID          string   `glazed.parameter:"id"`
	Name        string   `glazed.parameter:"name"`
	Icon        string   `glazed.parameter:"icon"`
	PatternType string   `glazed.parameter:"type"`
	Brightness  int      `glazed.parameter:"brightness"`
	Params      []string `glazed.parameter:"param"`
}

var _ cmds.GlazeCommand = &PresetUpdateCommand{}

func NewPresetUpdateCommand() (*PresetUpdateCommand, error) {
	glazedLayer, err := schema.NewGlazedSchema()
	if err != nil {
		return nil, err
	}
	commandSettingsLayer, err := cli.NewCommandSettingsLayer()
	if err != nil {
		return nil, err
	}

	flags := append([]*fields.Definition{}, clientFlags()...)
	flags = append(flags,
		fields.New("id", fields.TypeString, fields.WithRequired(true), fields.WithHelp("Preset ID")),
		fields.New("name", fields.TypeString, fields.WithDefault(""), fields.WithHelp("Preset name (optional; keep existing if empty)")),
		fields.New("icon", fields.TypeString, fields.WithDefault(""), fields.WithHelp("Preset icon (optional; keep existing if empty)")),
		fields.New("type", fields.TypeString, fields.WithDefault(""), fields.WithHelp("Pattern type (optional; keep existing if empty)")),
		fields.New("brightness", fields.TypeInteger, fields.WithDefault(-1), fields.WithHelp("Brightness percent (0..100); -1 keeps existing")),
		fields.New("param", fields.TypeStringList, fields.WithDefault([]string{}), fields.WithHelp("Replace params with these key=value pairs (repeatable). Empty keeps existing")),
	)

	cmdDesc := cmds.NewCommandDescription(
		"update",
		cmds.WithParents("preset"),
		cmds.WithShort("Update a preset via REST"),
		cmds.WithLong(`Update a preset by calling the running server's REST API (PUT /api/presets/:id).

This command loads the current preset list from the server, applies your changes, then PUTs the updated preset.

Examples:
  mled-server preset update --id preset-123 --name "New Name"
  mled-server preset update --id preset-123 --param color=#FF0000 --param speed=50
  mled-server preset update --id preset-123 --brightness 20
`),
		cmds.WithFlags(flags...),
		cmds.WithLayersList(glazedLayer, commandSettingsLayer),
	)

	return &PresetUpdateCommand{CommandDescription: cmdDesc}, nil
}

func (c *PresetUpdateCommand) RunIntoGlazeProcessor(ctx context.Context, vals *values.Values, gp middlewares.Processor) error {
	settings := &PresetUpdateSettings{}
	if err := values.DecodeSectionInto(vals, schema.DefaultSlug, settings); err != nil {
		return fmt.Errorf("failed to parse settings: %w", err)
	}

	id := strings.TrimSpace(settings.ID)
	if id == "" {
		return errors.New("--id is required")
	}

	client := restclient.New(settings.Server, time.Duration(settings.TimeoutMS)*time.Millisecond)
	ps, err := client.Presets(ctx)
	if err != nil {
		return err
	}

	var existing storage.Preset
	found := false
	for _, p := range ps {
		if p.ID == id {
			existing = p
			found = true
			break
		}
	}
	if !found {
		return fmt.Errorf("preset not found: %q", id)
	}

	next := existing
	if strings.TrimSpace(settings.Name) != "" {
		next.Name = settings.Name
	}
	if strings.TrimSpace(settings.Icon) != "" {
		next.Icon = settings.Icon
	}
	if strings.TrimSpace(settings.PatternType) != "" {
		next.Config.Type = settings.PatternType
	}
	if settings.Brightness >= 0 {
		next.Config.Brightness = settings.Brightness
	}
	if len(settings.Params) > 0 {
		params, err := parseKeyValueParams(settings.Params)
		if err != nil {
			return err
		}
		next.Config.Params = params
	}

	updated, err := client.UpdatePreset(ctx, id, next)
	if err != nil {
		return err
	}

	row := types.NewRowFromStruct(&updated, true)
	return gp.AddRow(ctx, row)
}
