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

type StatusCommand struct {
	*cmds.CommandDescription
}

type StatusSettings struct {
	Server    string `glazed.parameter:"server"`
	TimeoutMS int    `glazed.parameter:"timeout-ms"`
}

var _ cmds.GlazeCommand = &StatusCommand{}

func NewStatusCommand() (*StatusCommand, error) {
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
		"status",
		cmds.WithShort("Fetch server status via REST"),
		cmds.WithLong(`Fetch high-level status from a running mled-server instance.

Examples:
  mled-server status
  mled-server status --server http://localhost:8765
  mled-server status --output json
`),
		cmds.WithFlags(flags...),
		cmds.WithLayersList(glazedLayer, commandSettingsLayer),
	)

	return &StatusCommand{CommandDescription: cmdDesc}, nil
}

func (c *StatusCommand) RunIntoGlazeProcessor(ctx context.Context, vals *values.Values, gp middlewares.Processor) error {
	settings := &StatusSettings{}
	if err := values.DecodeSectionInto(vals, schema.DefaultSlug, settings); err != nil {
		return fmt.Errorf("failed to parse settings: %w", err)
	}

	client := restclient.New(settings.Server, time.Duration(settings.TimeoutMS)*time.Millisecond)
	st, err := client.Status(ctx)
	if err != nil {
		return err
	}

	row := types.NewRowFromMap(map[string]any{
		"server":   settings.Server,
		"running":  st.Running,
		"epoch_id": st.EpochID,
		"show_ms":  st.ShowMS,
	})
	return gp.AddRow(ctx, row)
}
