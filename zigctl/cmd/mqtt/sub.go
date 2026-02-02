package mqtt

import (
	"context"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
	"github.com/go-go-golems/glazed/pkg/cli"
	"github.com/go-go-golems/glazed/pkg/cmds"
	"github.com/go-go-golems/glazed/pkg/cmds/fields"
	"github.com/go-go-golems/glazed/pkg/cmds/schema"
	"github.com/go-go-golems/glazed/pkg/cmds/values"
	"github.com/go-go-golems/glazed/pkg/middlewares"
	"github.com/go-go-golems/glazed/pkg/types"
	"github.com/go-go-golems/zigctl/pkg/zigbee"
)

type SubCommand struct {
	*cmds.CommandDescription
	defaults zigbee.Config
}

var _ cmds.GlazeCommand = (*SubCommand)(nil)

type SubSettings struct {
	Topic string `glazed.parameter:"topic"`
}

func NewSubCommand(defaults zigbee.Config) (*SubCommand, error) {
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
		"sub",
		cmds.WithShort("Subscribe to a raw MQTT topic"),
		cmds.WithLong(`
Subscribe to a raw MQTT topic and print messages as rows.

Examples:
  zigctl mqtt sub --topic 'zigbee2mqtt/#'
  zigctl mqtt sub --topic 'zigbee2mqtt/bridge/logging'
  zigctl mqtt sub --topic 'zigbee2mqtt/#' --output json
`),
		cmds.WithFlags(
			fields.New(
				"topic",
				fields.TypeString,
				fields.WithRequired(true),
				fields.WithHelp("MQTT topic to subscribe to"),
			),
		),
		cmds.WithLayersList(glazedLayer, zigbeeLayer, commandSettingsLayer),
	)

	return &SubCommand{CommandDescription: cmdDesc, defaults: defaults}, nil
}

func (c *SubCommand) RunIntoGlazeProcessor(
	ctx context.Context,
	vals *values.Values,
	gp middlewares.Processor,
) error {
	settings := zigbee.Settings{}
	if err := values.DecodeSectionInto(vals, zigbee.LayerSlug, &settings); err != nil {
		return err
	}

	sub := SubSettings{}
	if err := values.DecodeSectionInto(vals, schema.DefaultSlug, &sub); err != nil {
		return err
	}

	client, timeout, qos, err := zigbee.Connect(ctx, settings)
	if err != nil {
		return err
	}
	defer client.Disconnect(250)

	msgCh := make(chan mqtt.Message, 16)
	handler := func(_ mqtt.Client, msg mqtt.Message) {
		select {
		case msgCh <- msg:
		default:
		}
	}

	subToken := client.Subscribe(sub.Topic, qos, handler)
	if !subToken.WaitTimeout(timeout) {
		return context.DeadlineExceeded
	}
	if err := subToken.Error(); err != nil {
		return err
	}
	defer client.Unsubscribe(sub.Topic)

	for {
		select {
		case <-ctx.Done():
			return ctx.Err()
		case msg := <-msgCh:
			row := types.NewRow(
				types.MRP("received_at", time.Now().Format(time.RFC3339)),
				types.MRP("topic", msg.Topic()),
				types.MRP("payload", string(msg.Payload())),
			)
			if err := gp.AddRow(ctx, row); err != nil {
				return err
			}
		}
	}
}
