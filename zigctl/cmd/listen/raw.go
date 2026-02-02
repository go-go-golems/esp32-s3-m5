package listen

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

type RawCommand struct {
	*cmds.CommandDescription
	defaults zigbee.Config
}

var _ cmds.GlazeCommand = (*RawCommand)(nil)

type RawSettings struct {
	Topic string `glazed.parameter:"topic"`
}

func NewRawCommand(defaults zigbee.Config) (*RawCommand, error) {
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
		"raw",
		cmds.WithShort("Subscribe to an arbitrary MQTT topic"),
		cmds.WithLong(`
Subscribe to a raw MQTT topic and print each message as a row.

Examples:
  zigctl listen raw --topic 'zigbee2mqtt/#'
  zigctl listen raw --topic 'zigbee2mqtt/bridge/logging'
  zigctl listen raw --topic 'zigbee2mqtt/#' --output json
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

	return &RawCommand{CommandDescription: cmdDesc, defaults: defaults}, nil
}

func (c *RawCommand) RunIntoGlazeProcessor(
	ctx context.Context,
	vals *values.Values,
	gp middlewares.Processor,
) error {
	settings := zigbee.Settings{}
	if err := values.DecodeSectionInto(vals, zigbee.LayerSlug, &settings); err != nil {
		return err
	}

	raw := RawSettings{}
	if err := values.DecodeSectionInto(vals, schema.DefaultSlug, &raw); err != nil {
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

	subToken := client.Subscribe(raw.Topic, qos, handler)
	if !subToken.WaitTimeout(timeout) {
		return context.DeadlineExceeded
	}
	if err := subToken.Error(); err != nil {
		return err
	}
	defer client.Unsubscribe(raw.Topic)

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
