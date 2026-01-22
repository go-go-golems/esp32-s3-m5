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
)

type SettingsSetCommand struct {
	*cmds.CommandDescription
}

type SettingsSetSettings struct {
	Server    string `glazed.parameter:"server"`
	TimeoutMS int    `glazed.parameter:"timeout-ms"`

	BindIP              string `glazed.parameter:"bind-ip"`
	MulticastGroup      string `glazed.parameter:"mcast-group"`
	MulticastPort       int    `glazed.parameter:"mcast-port"`
	DiscoveryIntervalMS int    `glazed.parameter:"discovery-interval-ms"`
	OfflineThresholdS   int    `glazed.parameter:"offline-threshold-s"`
}

var _ cmds.GlazeCommand = &SettingsSetCommand{}

func NewSettingsSetCommand() (*SettingsSetCommand, error) {
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
		fields.New("bind-ip", fields.TypeString, fields.WithDefault(""), fields.WithHelp("Bind IP (empty keeps existing)")),
		fields.New("mcast-group", fields.TypeString, fields.WithDefault(""), fields.WithHelp("Multicast group (empty keeps existing)")),
		fields.New("mcast-port", fields.TypeInteger, fields.WithDefault(0), fields.WithHelp("Multicast port (0 keeps existing)")),
		fields.New("discovery-interval-ms", fields.TypeInteger, fields.WithDefault(0), fields.WithHelp("Discovery interval in ms (0 keeps existing)")),
		fields.New("offline-threshold-s", fields.TypeInteger, fields.WithDefault(0), fields.WithHelp("Offline threshold in seconds (0 keeps existing)")),
	)

	cmdDesc := cmds.NewCommandDescription(
		"set",
		cmds.WithParents("settings"),
		cmds.WithShort("Update settings via REST"),
		cmds.WithLong(`Update settings on a running mled-server instance (PUT /api/settings).

This command GETs the existing settings, applies overrides, then PUTs the full settings object.

Examples:
  mled-server settings set --bind-ip auto
  mled-server settings set --mcast-group 239.255.32.6 --mcast-port 4626
  mled-server settings set --discovery-interval-ms 1000 --offline-threshold-s 30
`),
		cmds.WithFlags(flags...),
		cmds.WithLayersList(glazedLayer, commandSettingsLayer),
	)

	return &SettingsSetCommand{CommandDescription: cmdDesc}, nil
}

func (c *SettingsSetCommand) RunIntoGlazeProcessor(ctx context.Context, vals *values.Values, gp middlewares.Processor) error {
	settings := &SettingsSetSettings{}
	if err := values.DecodeSectionInto(vals, schema.DefaultSlug, settings); err != nil {
		return fmt.Errorf("failed to parse settings: %w", err)
	}

	client := restclient.New(settings.Server, time.Duration(settings.TimeoutMS)*time.Millisecond)
	current, err := client.Settings(ctx)
	if err != nil {
		return err
	}

	next := current
	if settings.BindIP != "" {
		next.BindIP = settings.BindIP
	}
	if settings.MulticastGroup != "" {
		next.MulticastGroup = settings.MulticastGroup
	}
	if settings.MulticastPort != 0 {
		next.MulticastPort = settings.MulticastPort
	}
	if settings.DiscoveryIntervalMS != 0 {
		next.DiscoveryIntervalMS = settings.DiscoveryIntervalMS
	}
	if settings.OfflineThresholdS != 0 {
		next.OfflineThresholdS = settings.OfflineThresholdS
	}

	updated, err := client.UpdateSettings(ctx, next)
	if err != nil {
		return err
	}

	row := types.NewRowFromMap(map[string]any{
		"server":                settings.Server,
		"bind_ip":               updated.BindIP,
		"multicast_group":       updated.MulticastGroup,
		"multicast_port":        updated.MulticastPort,
		"discovery_interval_ms": updated.DiscoveryIntervalMS,
		"offline_threshold_s":   updated.OfflineThresholdS,
	})
	return gp.AddRow(ctx, row)
}
