package bridge

import (
	"context"
	"encoding/json"

	"github.com/go-go-golems/glazed/pkg/cli"
	"github.com/go-go-golems/glazed/pkg/cmds"
	"github.com/go-go-golems/glazed/pkg/cmds/schema"
	"github.com/go-go-golems/glazed/pkg/cmds/values"
	"github.com/go-go-golems/glazed/pkg/middlewares"
	"github.com/go-go-golems/glazed/pkg/types"
	"github.com/go-go-golems/zigctl/pkg/zigbee"
)

type InfoCommand struct {
	*cmds.CommandDescription
	defaults zigbee.Config
}

var _ cmds.GlazeCommand = (*InfoCommand)(nil)

func NewInfoCommand(defaults zigbee.Config) (*InfoCommand, error) {
	glazedLayer, err := schema.NewGlazedSchema()
	if err != nil {
		return nil, err
	}

	commandSettingsLayer, err := cli.NewCommandSettingsLayer()
	if err != nil {
		return nil, err
	}

	zigbeeLayer, err := zigbee.NewZigbeeLayer(defaults)
	if err != nil {
		return nil, err
	}

	cmdDesc := cmds.NewCommandDescription(
		"info",
		cmds.WithShort("Fetch Zigbee2MQTT bridge info"),
		cmds.WithLong(`
Fetch bridge information from Zigbee2MQTT.

Examples:
  zigctl bridge info
  zigctl bridge info --output json
  zigctl bridge info --broker mqtt://localhost:1883
`),
		cmds.WithLayersList(glazedLayer, zigbeeLayer, commandSettingsLayer),
	)

	return &InfoCommand{CommandDescription: cmdDesc, defaults: defaults}, nil
}

func (c *InfoCommand) RunIntoGlazeProcessor(
	ctx context.Context,
	vals *values.Values,
	gp middlewares.Processor,
) error {
	settings := zigbee.Settings{}
	if err := values.DecodeSectionInto(vals, zigbee.LayerSlug, &settings); err != nil {
		return err
	}

	client, timeout, qos, err := zigbee.Connect(ctx, settings)
	if err != nil {
		return err
	}
	defer client.Disconnect(250)

	base := settings.BasePrefix()
	responseTopic := zigbee.JoinTopic(base, "bridge", "info")
	payload, err := zigbee.RequestOnce(
		ctx,
		client,
		qos,
		zigbee.JoinTopic(base, "bridge", "request", "info"),
		[]byte("{}"),
		responseTopic,
		timeout,
	)
	if err != nil {
		return err
	}

	var decoded any
	if err := json.Unmarshal(payload, &decoded); err != nil {
		decoded = string(payload)
	}

	row := types.NewRow(
		types.MRP("topic", responseTopic),
		types.MRP("payload", decoded),
	)
	return gp.AddRow(ctx, row)
}
