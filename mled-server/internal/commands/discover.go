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

	"mled-server/internal/mledhost"
)

type DiscoverCommand struct {
	*cmds.CommandDescription
}

type DiscoverSettings struct {
	TimeoutMS      int    `glazed.parameter:"timeout-ms"`
	BindIP         string `glazed.parameter:"bind-ip"`
	Interface      string `glazed.parameter:"iface"`
	MulticastGroup string `glazed.parameter:"mcast-group"`
	MulticastPort  int    `glazed.parameter:"mcast-port"`
	MulticastTTL   int    `glazed.parameter:"mcast-ttl"`
}

var _ cmds.GlazeCommand = &DiscoverCommand{}

func NewDiscoverCommand() (*DiscoverCommand, error) {
	glazedLayer, err := schema.NewGlazedSchema()
	if err != nil {
		return nil, err
	}
	commandSettingsLayer, err := cli.NewCommandSettingsLayer()
	if err != nil {
		return nil, err
	}

	cmdDesc := cmds.NewCommandDescription(
		"discover",
		cmds.WithShort("Discover MLED/1 nodes via PING/PONG"),
		cmds.WithLong(`Discover nodes on the local network by sending a multicast PING and collecting unicast PONG replies.

Examples:
  mled-server discover --timeout-ms 500
  mled-server discover --iface eth0
  mled-server discover --bind-ip 192.168.1.112
  mled-server discover --output json
`),
		cmds.WithFlags(
			fields.New("timeout-ms", fields.TypeInteger, fields.WithDefault(500), fields.WithHelp("How long to wait for PONG replies")),
			fields.New("bind-ip", fields.TypeString, fields.WithDefault("auto"), fields.WithHelp("Bind IP for UDP socket (auto or an IP)")),
			fields.New("iface", fields.TypeString, fields.WithDefault(""), fields.WithHelp("Network interface name for multicast (optional)")),
			fields.New("mcast-group", fields.TypeString, fields.WithDefault("239.255.32.6"), fields.WithHelp("Multicast group IP")),
			fields.New("mcast-port", fields.TypeInteger, fields.WithDefault(4626), fields.WithHelp("Multicast UDP port")),
			fields.New("mcast-ttl", fields.TypeInteger, fields.WithDefault(1), fields.WithHelp("Multicast TTL")),
		),
		cmds.WithLayersList(glazedLayer, commandSettingsLayer),
	)

	return &DiscoverCommand{CommandDescription: cmdDesc}, nil
}

func (c *DiscoverCommand) RunIntoGlazeProcessor(ctx context.Context, vals *values.Values, gp middlewares.Processor) error {
	settings := &DiscoverSettings{}
	if err := values.DecodeSectionInto(vals, schema.DefaultSlug, settings); err != nil {
		return fmt.Errorf("failed to parse settings: %w", err)
	}

	timeout := time.Duration(settings.TimeoutMS) * time.Millisecond
	if timeout <= 0 {
		timeout = 500 * time.Millisecond
	}

	engine := mledhost.NewEngine(mledhost.Config{
		BindIP:            settings.BindIP,
		Interface:         settings.Interface,
		MulticastGroup:    settings.MulticastGroup,
		MulticastPort:     settings.MulticastPort,
		MulticastTTL:      settings.MulticastTTL,
		DiscoveryInterval: 0,
		BeaconInterval:    0,
		OfflineThreshold:  30 * time.Second,
		WeakRSSIDbm:       -70,
	})
	if err := engine.Start(ctx); err != nil {
		return err
	}
	defer engine.Stop()

	_ = engine.SendPing()
	select {
	case <-ctx.Done():
		return ctx.Err()
	case <-time.After(timeout):
	}

	nodes := engine.NodesSnapshot()
	for _, n := range nodes {
		row := types.NewRowFromMap(map[string]any{
			"node_id":   n.NodeID,
			"name":      n.Name,
			"ip":        n.IP,
			"port":      n.Port,
			"rssi":      n.RSSI,
			"uptime_ms": n.UptimeMS,
			"last_seen": n.LastSeen,
			"status":    n.Status,
		})
		if err := gp.AddRow(ctx, row); err != nil {
			return err
		}
	}

	return nil
}
