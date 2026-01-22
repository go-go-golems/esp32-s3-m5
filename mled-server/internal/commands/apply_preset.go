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

type ApplyPresetCommand struct {
	*cmds.CommandDescription
}

type ApplyPresetSettings struct {
	Server    string `glazed.parameter:"server"`
	TimeoutMS int    `glazed.parameter:"timeout-ms"`

	All   bool     `glazed.parameter:"all"`
	Nodes []string `glazed.parameter:"node"`

	PresetID string `glazed.parameter:"preset"`
}

var _ cmds.GlazeCommand = &ApplyPresetCommand{}

func NewApplyPresetCommand() (*ApplyPresetCommand, error) {
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
		fields.New("all", fields.TypeBool, fields.WithDefault(true), fields.WithHelp("Apply to all nodes (default). If --node is provided, it overrides --all")),
		fields.New("node", fields.TypeStringList, fields.WithDefault([]string{}), fields.WithHelp("Target node_id(s) (hex, repeatable)")),
		fields.New("preset", fields.TypeString, fields.WithRequired(true), fields.WithHelp("Preset ID to apply")),
	)

	cmdDesc := cmds.NewCommandDescription(
		"apply-preset",
		cmds.WithShort("Apply a preset via REST"),
		cmds.WithLong(`Apply an existing preset by calling the running server's REST API (/api/apply).

Examples:
  mled-server apply-preset --preset preset-123 --all
  mled-server apply-preset --preset preset-123 --node 0123ABCD
`),
		cmds.WithFlags(flags...),
		cmds.WithLayersList(glazedLayer, commandSettingsLayer),
	)

	return &ApplyPresetCommand{CommandDescription: cmdDesc}, nil
}

func (c *ApplyPresetCommand) RunIntoGlazeProcessor(ctx context.Context, vals *values.Values, gp middlewares.Processor) error {
	settings := &ApplyPresetSettings{}
	if err := values.DecodeSectionInto(vals, schema.DefaultSlug, settings); err != nil {
		return fmt.Errorf("failed to parse settings: %w", err)
	}

	id := strings.TrimSpace(settings.PresetID)
	if id == "" {
		return errors.New("--preset is required")
	}

	all := settings.All
	nodeIDs := settings.Nodes
	if len(nodeIDs) > 0 {
		all = false
	}

	client := restclient.New(settings.Server, time.Duration(settings.TimeoutMS)*time.Millisecond)
	ps, err := client.Presets(ctx)
	if err != nil {
		return err
	}
	var cfg *restclient.ApplyRequest
	for _, p := range ps {
		if p.ID == id {
			cfg = &restclient.ApplyRequest{
				NodeIDs: func() any {
					if all {
						return "all"
					}
					return nodeIDs
				}(),
				Config: p.Config,
			}
			break
		}
	}
	if cfg == nil {
		return fmt.Errorf("preset not found: %q", id)
	}

	res, err := client.Apply(ctx, *cfg)
	if err != nil {
		return err
	}

	for _, nid := range res.SentTo {
		if err := gp.AddRow(ctx, types.NewRowFromMap(map[string]any{
			"node_id":   nid,
			"result":    "sent",
			"preset_id": id,
			"server":    settings.Server,
		})); err != nil {
			return err
		}
	}
	for _, nid := range res.Failed {
		if err := gp.AddRow(ctx, types.NewRowFromMap(map[string]any{
			"node_id":   nid,
			"result":    "failed",
			"preset_id": id,
			"server":    settings.Server,
		})); err != nil {
			return err
		}
	}

	if len(res.SentTo) == 0 && len(res.Failed) == 0 {
		return gp.AddRow(ctx, types.NewRowFromMap(map[string]any{
			"result":    "no_targets",
			"preset_id": id,
			"server":    settings.Server,
		}))
	}

	return nil
}
