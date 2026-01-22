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

type NodesCommand struct {
	*cmds.CommandDescription
}

type NodesSettings struct {
	Server    string `glazed.parameter:"server"`
	TimeoutMS int    `glazed.parameter:"timeout-ms"`
}

var _ cmds.GlazeCommand = &NodesCommand{}

func NewNodesCommand() (*NodesCommand, error) {
	glazedLayer, err := schema.NewGlazedSchema()
	if err != nil {
		return nil, err
	}
	commandSettingsLayer, err := cli.NewCommandSettingsLayer()
	if err != nil {
		return nil, err
	}

	flags := clientFlags()

	cmdDesc := cmds.NewCommandDescription(
		"nodes",
		cmds.WithShort("List nodes via REST"),
		cmds.WithLong(`List discovered nodes from a running mled-server instance.

Examples:
  mled-server nodes
  mled-server nodes --output json
  mled-server nodes --fields node_id,name,ip,rssi,status
`),
		cmds.WithFlags(flags...),
		cmds.WithLayersList(glazedLayer, commandSettingsLayer),
	)

	return &NodesCommand{CommandDescription: cmdDesc}, nil
}

func (c *NodesCommand) RunIntoGlazeProcessor(ctx context.Context, vals *values.Values, gp middlewares.Processor) error {
	settings := &NodesSettings{}
	if err := values.DecodeSectionInto(vals, schema.DefaultSlug, settings); err != nil {
		return fmt.Errorf("failed to parse settings: %w", err)
	}

	client := restclient.New(settings.Server, time.Duration(settings.TimeoutMS)*time.Millisecond)
	nodes, err := client.Nodes(ctx)
	if err != nil {
		return err
	}

	for _, n := range nodes {
		row := types.NewRowFromMap(map[string]any{
			"node_id":         n.NodeID,
			"name":            n.Name,
			"ip":              n.IP,
			"port":            n.Port,
			"rssi":            n.RSSI,
			"uptime_ms":       n.UptimeMS,
			"last_seen":       n.LastSeen,
			"current_pattern": n.CurrentPattern,
			"status":          n.Status,
		})
		if err := gp.AddRow(ctx, row); err != nil {
			return err
		}
	}
	return nil
}
