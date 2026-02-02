package listen

import (
	"context"
	"encoding/json"
	"strings"
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

type StateCommand struct {
	*cmds.CommandDescription
	defaults zigbee.Config
}

var _ cmds.GlazeCommand = (*StateCommand)(nil)

type StateSettings struct {
	Device string `glazed.parameter:"device"`
}

func NewStateCommand(defaults zigbee.Config) (*StateCommand, error) {
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
		"state",
		cmds.WithShort("Stream device state updates"),
		cmds.WithLong(`
Subscribe to Zigbee2MQTT state updates and stream each message as a row.

Examples:
  zigctl listen state
  zigctl listen state --device office_plug
  zigctl listen state --output json
`),
		cmds.WithFlags(
			fields.New(
				"device",
				fields.TypeString,
				fields.WithDefault(""),
				fields.WithHelp("Limit to a specific friendly name (base/<device>)"),
			),
		),
		cmds.WithLayersList(glazedLayer, zigbeeLayer, commandSettingsLayer),
	)

	return &StateCommand{CommandDescription: cmdDesc, defaults: defaults}, nil
}

func (c *StateCommand) RunIntoGlazeProcessor(
	ctx context.Context,
	vals *values.Values,
	gp middlewares.Processor,
) error {
	settings := zigbee.Settings{}
	if err := values.DecodeSectionInto(vals, zigbee.LayerSlug, &settings); err != nil {
		return err
	}

	state := StateSettings{}
	if err := values.DecodeSectionInto(vals, schema.DefaultSlug, &state); err != nil {
		return err
	}

	client, timeout, qos, err := zigbee.Connect(ctx, settings)
	if err != nil {
		return err
	}
	defer client.Disconnect(250)

	base := settings.BasePrefix()
	topic := zigbee.JoinTopic(base, "#")
	if state.Device != "" {
		topic = zigbee.JoinTopic(base, state.Device)
	}

	msgCh := make(chan mqtt.Message, 16)
	handler := func(_ mqtt.Client, msg mqtt.Message) {
		select {
		case msgCh <- msg:
		default:
		}
	}

	subToken := client.Subscribe(topic, qos, handler)
	if !subToken.WaitTimeout(timeout) {
		return context.DeadlineExceeded
	}
	if err := subToken.Error(); err != nil {
		return err
	}
	defer client.Unsubscribe(topic)

	for {
		select {
		case <-ctx.Done():
			return ctx.Err()
		case msg := <-msgCh:
			payload := msg.Payload()
			var decoded any
			if err := json.Unmarshal(payload, &decoded); err != nil {
				decoded = string(payload)
			}
			row := types.NewRow(
				types.MRP("received_at", time.Now().Format(time.RFC3339)),
				types.MRP("topic", msg.Topic()),
				types.MRP("device", extractDevice(base, msg.Topic())),
				types.MRP("payload", decoded),
			)
			if err := gp.AddRow(ctx, row); err != nil {
				return err
			}
		}
	}
}

func extractDevice(base, topic string) string {
	prefix := base + "/"
	if !strings.HasPrefix(topic, prefix) {
		return ""
	}
	rest := strings.TrimPrefix(topic, prefix)
	parts := strings.Split(rest, "/")
	if len(parts) == 0 {
		return ""
	}
	return parts[0]
}
