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

type SettingsGetCommand struct {
	*cmds.CommandDescription
}

type SettingsGetSettings struct {
	Server    string `glazed.parameter:"server"`
	TimeoutMS int    `glazed.parameter:"timeout-ms"`
}

var _ cmds.GlazeCommand = &SettingsGetCommand{}

func NewSettingsGetCommand() (*SettingsGetCommand, error) {
	glazedLayer, err := schema.NewGlazedSchema()
	if err != nil {
		return nil, err
	}
	commandSettingsLayer, err := cli.NewCommandSettingsLayer()
	if err != nil {
		return nil, err
	}

	cmdDesc := cmds.NewCommandDescription(
		"get",
		cmds.WithParents("settings"),
		cmds.WithShort("Get settings via REST"),
		cmds.WithLong(`Fetch settings from a running mled-server instance (GET /api/settings).

Examples:
  mled-server settings get
  mled-server settings get --output json
`),
		cmds.WithFlags(clientFlags()...),
		cmds.WithLayersList(glazedLayer, commandSettingsLayer),
	)

	return &SettingsGetCommand{CommandDescription: cmdDesc}, nil
}

func (c *SettingsGetCommand) RunIntoGlazeProcessor(ctx context.Context, vals *values.Values, gp middlewares.Processor) error {
	settings := &SettingsGetSettings{}
	if err := values.DecodeSectionInto(vals, schema.DefaultSlug, settings); err != nil {
		return fmt.Errorf("failed to parse settings: %w", err)
	}

	client := restclient.New(settings.Server, time.Duration(settings.TimeoutMS)*time.Millisecond)
	s, err := client.Settings(ctx)
	if err != nil {
		return err
	}

	row := types.NewRowFromMap(map[string]any{
		"server":                settings.Server,
		"bind_ip":               s.BindIP,
		"multicast_group":       s.MulticastGroup,
		"multicast_port":        s.MulticastPort,
		"discovery_interval_ms": s.DiscoveryIntervalMS,
		"offline_threshold_s":   s.OfflineThresholdS,
	})
	return gp.AddRow(ctx, row)
}
