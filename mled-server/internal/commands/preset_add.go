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

type PresetAddCommand struct {
	*cmds.CommandDescription
}

type PresetAddSettings struct {
	Server    string `glazed.parameter:"server"`
	TimeoutMS int    `glazed.parameter:"timeout-ms"`

	Name        string   `glazed.parameter:"name"`
	Icon        string   `glazed.parameter:"icon"`
	PatternType string   `glazed.parameter:"type"`
	Brightness  int      `glazed.parameter:"brightness"`
	Params      []string `glazed.parameter:"param"`
}

var _ cmds.GlazeCommand = &PresetAddCommand{}

func NewPresetAddCommand() (*PresetAddCommand, error) {
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
		fields.New("name", fields.TypeString, fields.WithRequired(true), fields.WithHelp("Preset name")),
		fields.New("icon", fields.TypeString, fields.WithDefault("✨"), fields.WithHelp("Preset icon (emoji or short label)")),
		fields.New("type", fields.TypeString, fields.WithRequired(true), fields.WithHelp("Pattern type (off, rainbow, solid, pulse, ...)")),
		fields.New("brightness", fields.TypeInteger, fields.WithDefault(75), fields.WithHelp("Brightness percent (0..100)")),
		fields.New("param", fields.TypeStringList, fields.WithDefault([]string{}), fields.WithHelp("Pattern parameter as key=value (repeatable)")),
	)

	cmdDesc := cmds.NewCommandDescription(
		"add",
		cmds.WithParents("preset"),
		cmds.WithShort("Create a preset via REST"),
		cmds.WithLong(`Create a preset by calling the running server's REST API (POST /api/presets).

Examples:
  mled-server preset add --name Calm --icon ✨ --type solid --param color=#2244AA --brightness 75
  mled-server preset add --name Rainbow --icon 🌈 --type rainbow --param speed=50
  mled-server preset add --output json ...
`),
		cmds.WithFlags(flags...),
		cmds.WithLayersList(glazedLayer, commandSettingsLayer),
	)

	return &PresetAddCommand{CommandDescription: cmdDesc}, nil
}

func (c *PresetAddCommand) RunIntoGlazeProcessor(ctx context.Context, vals *values.Values, gp middlewares.Processor) error {
	settings := &PresetAddSettings{}
	if err := values.DecodeSectionInto(vals, schema.DefaultSlug, settings); err != nil {
		return fmt.Errorf("failed to parse settings: %w", err)
	}

	params, err := parseKeyValueParams(settings.Params)
	if err != nil {
		return err
	}
	if strings.TrimSpace(settings.Name) == "" {
		return errors.New("--name is required")
	}
	if strings.TrimSpace(settings.PatternType) == "" {
		return errors.New("--type is required")
	}

	client := restclient.New(settings.Server, time.Duration(settings.TimeoutMS)*time.Millisecond)
	created, err := client.CreatePreset(ctx, storage.Preset{
		Name: settings.Name,
		Icon: settings.Icon,
		Config: storage.PatternConfig{
			Type:       settings.PatternType,
			Brightness: settings.Brightness,
			Params:     params,
		},
	})
	if err != nil {
		return err
	}

	row := types.NewRowFromStruct(&created, true)
	return gp.AddRow(ctx, row)
}
