package mqtt

import (
	"context"

	"github.com/go-go-golems/glazed/pkg/cli"
	"github.com/go-go-golems/glazed/pkg/cmds"
	"github.com/go-go-golems/glazed/pkg/cmds/fields"
	"github.com/go-go-golems/glazed/pkg/cmds/schema"
	"github.com/go-go-golems/glazed/pkg/cmds/values"
	"github.com/go-go-golems/glazed/pkg/middlewares"
	"github.com/go-go-golems/glazed/pkg/types"
	"github.com/go-go-golems/zigctl/pkg/zigbee"
)

type PubCommand struct {
	*cmds.CommandDescription
	defaults zigbee.Config
}

var _ cmds.GlazeCommand = (*PubCommand)(nil)

type PubSettings struct {
	Topic   string `glazed.parameter:"topic"`
	Message string `glazed.parameter:"message"`
}

func NewPubCommand(defaults zigbee.Config) (*PubCommand, error) {
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
		"pub",
		cmds.WithShort("Publish a raw MQTT message"),
		cmds.WithLong(`
Publish a message to an MQTT topic (raw helper).

Examples:
  zigctl mqtt pub --topic 'zigbee2mqtt/bridge/request/info' --message '{}'
  zigctl mqtt pub --topic 'zigbee2mqtt/office_plug/set' --message '{"state":"ON"}'
  zigctl mqtt pub --topic 'zigbee2mqtt/bridge/request/devices' --message '{}'
`),
		cmds.WithFlags(
			fields.New(
				"topic",
				fields.TypeString,
				fields.WithRequired(true),
				fields.WithHelp("MQTT topic to publish"),
			),
			fields.New(
				"message",
				fields.TypeString,
				fields.WithRequired(true),
				fields.WithHelp("Raw message payload"),
			),
		),
		cmds.WithLayersList(glazedLayer, zigbeeLayer, commandSettingsLayer),
	)

	return &PubCommand{CommandDescription: cmdDesc, defaults: defaults}, nil
}

func (c *PubCommand) RunIntoGlazeProcessor(
	ctx context.Context,
	vals *values.Values,
	gp middlewares.Processor,
) error {
	settings := zigbee.Settings{}
	if err := values.DecodeSectionInto(vals, zigbee.LayerSlug, &settings); err != nil {
		return err
	}

	pub := PubSettings{}
	if err := values.DecodeSectionInto(vals, schema.DefaultSlug, &pub); err != nil {
		return err
	}

	client, timeout, qos, err := zigbee.Connect(ctx, settings)
	if err != nil {
		return err
	}
	defer client.Disconnect(250)

	if err := zigbee.Publish(ctx, client, pub.Topic, qos, []byte(pub.Message), timeout); err != nil {
		return err
	}

	row := types.NewRow(
		types.MRP("topic", pub.Topic),
		types.MRP("message", pub.Message),
	)
	return gp.AddRow(ctx, row)
}
