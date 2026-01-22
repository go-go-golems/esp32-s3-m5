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

type ApplySolidCommand struct {
	*cmds.CommandDescription
}

type ApplySolidSettings struct {
	Server    string `glazed.parameter:"server"`
	TimeoutMS int    `glazed.parameter:"timeout-ms"`

	All   bool     `glazed.parameter:"all"`
	Nodes []string `glazed.parameter:"node"`

	Color      string `glazed.parameter:"color"`
	Brightness int    `glazed.parameter:"brightness"`
}

var _ cmds.GlazeCommand = &ApplySolidCommand{}

func NewApplySolidCommand() (*ApplySolidCommand, error) {
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
		fields.New("color", fields.TypeString, fields.WithDefault("#FFFFFF"), fields.WithHelp("Hex color like #RRGGBB")),
		fields.New("brightness", fields.TypeInteger, fields.WithDefault(75), fields.WithHelp("Brightness percent (0..100)")),
	)

	cmdDesc := cmds.NewCommandDescription(
		"apply-solid",
		cmds.WithShort("Apply solid color via REST"),
		cmds.WithLong(`Apply a solid color by calling the running server's REST API (/api/apply).

Examples:
  mled-server apply-solid --color #FF00FF --brightness 30 --all
  mled-server apply-solid --color #00FF00 --node 0123ABCD
`),
		cmds.WithFlags(flags...),
		cmds.WithLayersList(glazedLayer, commandSettingsLayer),
	)

	return &ApplySolidCommand{CommandDescription: cmdDesc}, nil
}

func (c *ApplySolidCommand) RunIntoGlazeProcessor(ctx context.Context, vals *values.Values, gp middlewares.Processor) error {
	settings := &ApplySolidSettings{}
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
			Type:       "solid",
			Brightness: settings.Brightness,
			Params: map[string]any{
				"color": settings.Color,
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
			"pattern":    "solid",
			"color":      settings.Color,
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
			"pattern":    "solid",
			"color":      settings.Color,
			"brightness": settings.Brightness,
			"server":     settings.Server,
		})); err != nil {
			return err
		}
	}

	if len(res.SentTo) == 0 && len(res.Failed) == 0 {
		return gp.AddRow(ctx, types.NewRowFromMap(map[string]any{
			"result":     "no_targets",
			"pattern":    "solid",
			"color":      settings.Color,
			"brightness": settings.Brightness,
			"server":     settings.Server,
		}))
	}

	return nil
}
