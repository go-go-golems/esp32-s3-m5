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

type ApplyCommand struct {
	*cmds.CommandDescription
}

type ApplySettings struct {
	Server    string `glazed.parameter:"server"`
	TimeoutMS int    `glazed.parameter:"timeout-ms"`

	All   bool     `glazed.parameter:"all"`
	Nodes []string `glazed.parameter:"node"`

	PresetID string `glazed.parameter:"preset"`

	PatternType string   `glazed.parameter:"type"`
	Brightness  int      `glazed.parameter:"brightness"`
	Params      []string `glazed.parameter:"param"`
}

var _ cmds.GlazeCommand = &ApplyCommand{}

func NewApplyCommand() (*ApplyCommand, error) {
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
		fields.New("node", fields.TypeStringList, fields.WithDefault([]string{}), fields.WithHelp("Target node_id(s) (hex, as shown by `mled-server nodes`)")),
		fields.New("preset", fields.TypeString, fields.WithDefault(""), fields.WithHelp("Preset ID to apply (loaded from /api/presets)")),
		fields.New("type", fields.TypeString, fields.WithDefault(""), fields.WithHelp("Pattern type (off, rainbow, solid, pulse, ...)")),
		fields.New("brightness", fields.TypeInteger, fields.WithDefault(100), fields.WithHelp("Brightness percent (0..100)")),
		fields.New("param", fields.TypeStringList, fields.WithDefault([]string{}), fields.WithHelp("Pattern parameter as key=value (repeatable), e.g. --param color=#FF0000 --param speed=50")),
	)

	cmdDesc := cmds.NewCommandDescription(
		"apply",
		cmds.WithShort("Apply a pattern via REST"),
		cmds.WithLong(`Apply a pattern to one or more nodes by calling the running server's REST API (/api/apply).

Targets:
  - Default is all nodes.
  - Use --node to target a specific node_id (repeatable).

Sources:
  - Use --preset to apply an existing preset.
  - Or specify --type/--brightness/--param to build a pattern config.

Examples:
  mled-server apply --type off
  mled-server apply --type rainbow --param speed=50
  mled-server apply --type solid --param color=#FF00FF --brightness 30
  mled-server apply --preset 3c6d4f --node 0123ABCD
  mled-server apply --output json
`),
		cmds.WithFlags(flags...),
		cmds.WithLayersList(glazedLayer, commandSettingsLayer),
	)

	return &ApplyCommand{CommandDescription: cmdDesc}, nil
}

func (c *ApplyCommand) RunIntoGlazeProcessor(ctx context.Context, vals *values.Values, gp middlewares.Processor) error {
	settings := &ApplySettings{}
	if err := values.DecodeSectionInto(vals, schema.DefaultSlug, settings); err != nil {
		return fmt.Errorf("failed to parse settings: %w", err)
	}

	if settings.PresetID == "" && settings.PatternType == "" {
		return errors.New("must provide either --preset or --type")
	}
	if settings.PresetID != "" && settings.PatternType != "" {
		return errors.New("provide either --preset or --type, not both")
	}

	all := settings.All
	nodeIDs := settings.Nodes
	if len(nodeIDs) > 0 {
		all = false
	}

	client := restclient.New(settings.Server, time.Duration(settings.TimeoutMS)*time.Millisecond)

	var cfg storage.PatternConfig
	if settings.PresetID != "" {
		ps, err := client.Presets(ctx)
		if err != nil {
			return err
		}
		found := false
		for _, p := range ps {
			if p.ID == settings.PresetID {
				cfg = p.Config
				found = true
				break
			}
		}
		if !found {
			return fmt.Errorf("preset not found: %q", settings.PresetID)
		}
	} else {
		params, err := parseKeyValueParams(settings.Params)
		if err != nil {
			return err
		}
		cfg = storage.PatternConfig{
			Type:       settings.PatternType,
			Brightness: settings.Brightness,
			Params:     params,
		}
	}

	res, err := client.Apply(ctx, restclient.ApplyRequest{
		NodeIDs: func() any {
			if all {
				return "all"
			}
			return nodeIDs
		}(),
		Config: cfg,
	})
	if err != nil {
		return err
	}

	for _, id := range res.SentTo {
		row := types.NewRowFromMap(map[string]any{
			"node_id":      id,
			"result":       "sent",
			"pattern_type": cfg.Type,
			"brightness":   cfg.Brightness,
			"target_all":   all,
			"target_count": len(nodeIDs),
			"server":       settings.Server,
			"preset_id":    settings.PresetID,
		})
		if err := gp.AddRow(ctx, row); err != nil {
			return err
		}
	}
	for _, id := range res.Failed {
		row := types.NewRowFromMap(map[string]any{
			"node_id":      id,
			"result":       "failed",
			"pattern_type": cfg.Type,
			"brightness":   cfg.Brightness,
			"target_all":   all,
			"target_count": len(nodeIDs),
			"server":       settings.Server,
			"preset_id":    settings.PresetID,
		})
		if err := gp.AddRow(ctx, row); err != nil {
			return err
		}
	}

	if len(res.SentTo) == 0 && len(res.Failed) == 0 {
		row := types.NewRowFromMap(map[string]any{
			"result":       "no_targets",
			"pattern_type": cfg.Type,
			"brightness":   cfg.Brightness,
			"target_all":   all,
			"server":       settings.Server,
			"preset_id":    settings.PresetID,
		})
		return gp.AddRow(ctx, row)
	}

	return nil
}

func parseKeyValueParams(kvs []string) (map[string]any, error) {
	if len(kvs) == 0 {
		return map[string]any{}, nil
	}
	out := make(map[string]any, len(kvs))
	for _, kv := range kvs {
		kv = strings.TrimSpace(kv)
		if kv == "" {
			continue
		}
		k, v, ok := strings.Cut(kv, "=")
		if !ok {
			return nil, fmt.Errorf("invalid --param %q (expected key=value)", kv)
		}
		k = strings.TrimSpace(k)
		v = strings.TrimSpace(v)
		if k == "" {
			return nil, fmt.Errorf("invalid --param %q (empty key)", kv)
		}
		out[k] = v
	}
	return out, nil
}
