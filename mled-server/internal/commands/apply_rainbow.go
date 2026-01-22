package commands

import (
	"context"
	"fmt"
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

type ApplyRainbowCommand struct {
	*cmds.CommandDescription
}

type ApplyRainbowSettings struct {
	Server    string `glazed.parameter:"server"`
	TimeoutMS int    `glazed.parameter:"timeout-ms"`

	All   bool     `glazed.parameter:"all"`
	Nodes []string `glazed.parameter:"node"`

	Speed      int `glazed.parameter:"speed"`
	Brightness int `glazed.parameter:"brightness"`
}

var _ cmds.GlazeCommand = &ApplyRainbowCommand{}

func NewApplyRainbowCommand() (*ApplyRainbowCommand, error) {
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
		fields.New("speed", fields.TypeInteger, fields.WithDefault(50), fields.WithHelp("Speed (0..100, mapped by server)")),
		fields.New("brightness", fields.TypeInteger, fields.WithDefault(75), fields.WithHelp("Brightness percent (0..100)")),
	)

	cmdDesc := cmds.NewCommandDescription(
		"apply-rainbow",
		cmds.WithShort("Apply rainbow via REST"),
		cmds.WithLong(`Apply rainbow by calling the running server's REST API (/api/apply).

Examples:
  mled-server apply-rainbow --speed 50 --brightness 70 --all
  mled-server apply-rainbow --speed 10 --node 0123ABCD
`),
		cmds.WithFlags(flags...),
		cmds.WithLayersList(glazedLayer, commandSettingsLayer),
	)

	return &ApplyRainbowCommand{CommandDescription: cmdDesc}, nil
}

func (c *ApplyRainbowCommand) RunIntoGlazeProcessor(ctx context.Context, vals *values.Values, gp middlewares.Processor) error {
	settings := &ApplyRainbowSettings{}
	if err := values.DecodeSectionInto(vals, schema.DefaultSlug, settings); err != nil {
		return fmt.Errorf("failed to parse settings: %w", err)
	}

	all := settings.All
	nodeIDs := settings.Nodes
	if len(nodeIDs) > 0 {
		all = false
	}

	client := restclient.New(settings.Server, time.Duration(settings.TimeoutMS)*time.Millisecond)
	res, err := client.Apply(ctx, restclient.ApplyRequest{
		NodeIDs: func() any {
			if all {
				return "all"
			}
			return nodeIDs
		}(),
		Config: storage.PatternConfig{
			Type:       "rainbow",
			Brightness: settings.Brightness,
			Params: map[string]any{
				"speed": settings.Speed,
			},
		},
	})
	if err != nil {
		return err
	}

	for _, nid := range res.SentTo {
		if err := gp.AddRow(ctx, types.NewRowFromMap(map[string]any{
			"node_id":    nid,
			"result":     "sent",
			"pattern":    "rainbow",
			"speed":      settings.Speed,
			"brightness": settings.Brightness,
			"server":     settings.Server,
		})); err != nil {
			return err
		}
	}
	for _, nid := range res.Failed {
		if err := gp.AddRow(ctx, types.NewRowFromMap(map[string]any{
			"node_id":    nid,
			"result":     "failed",
			"pattern":    "rainbow",
			"speed":      settings.Speed,
			"brightness": settings.Brightness,
			"server":     settings.Server,
		})); err != nil {
			return err
		}
	}

	if len(res.SentTo) == 0 && len(res.Failed) == 0 {
		return gp.AddRow(ctx, types.NewRowFromMap(map[string]any{
			"result":     "no_targets",
			"pattern":    "rainbow",
			"speed":      settings.Speed,
			"brightness": settings.Brightness,
			"server":     settings.Server,
		}))
	}

	return nil
}
