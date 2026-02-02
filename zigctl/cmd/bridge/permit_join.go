package bridge

import (
	"context"
	"encoding/json"
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

type PermitJoinCommand struct {
	*cmds.CommandDescription
	defaults zigbee.Config
}

var _ cmds.GlazeCommand = (*PermitJoinCommand)(nil)

type PermitJoinSettings struct {
	Seconds int    `glazed.parameter:"seconds"`
	Device  string `glazed.parameter:"device"`
	Watch   bool   `glazed.parameter:"watch"`
}

func NewPermitJoinCommand(defaults zigbee.Config) (*PermitJoinCommand, error) {
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
		"permit-join",
		cmds.WithShort("Enable Zigbee permit-join"),
		cmds.WithLong(`
Enable permit-join for a limited time so new devices can join the network.

Examples:
  zigctl bridge permit-join --seconds 60
  zigctl bridge permit-join --seconds 60 --watch
  zigctl bridge permit-join --seconds 120 --device office_plug
  zigctl bridge permit-join --output json
`),
		cmds.WithFlags(
			fields.New(
				"seconds",
				fields.TypeInteger,
				fields.WithDefault(60),
				fields.WithHelp("Permit-join window in seconds"),
			),
			fields.New(
				"device",
				fields.TypeString,
				fields.WithDefault(""),
				fields.WithHelp("Limit permit-join to a specific device (friendly name)"),
			),
			fields.New(
				"watch",
				fields.TypeBool,
				fields.WithDefault(false),
				fields.WithHelp("Watch for device join events until the permit-join window closes"),
			),
		),
		cmds.WithLayersList(glazedLayer, zigbeeLayer, commandSettingsLayer),
	)

	return &PermitJoinCommand{CommandDescription: cmdDesc, defaults: defaults}, nil
}

func (c *PermitJoinCommand) RunIntoGlazeProcessor(
	ctx context.Context,
	vals *values.Values,
	gp middlewares.Processor,
) error {
	settings := zigbee.Settings{}
	if err := values.DecodeSectionInto(vals, zigbee.LayerSlug, &settings); err != nil {
		return err
	}

	permit := PermitJoinSettings{}
	if err := values.DecodeSectionInto(vals, schema.DefaultSlug, &permit); err != nil {
		return err
	}

	client, timeout, qos, err := zigbee.Connect(ctx, settings)
	if err != nil {
		return err
	}
	defer client.Disconnect(250)

	payload := map[string]any{
		"value": true,
		"time":  permit.Seconds,
	}
	if permit.Device != "" {
		payload["device"] = permit.Device
	}
	encoded, err := json.Marshal(payload)
	if err != nil {
		return err
	}

	base := settings.BasePrefix()
	responseTopic := zigbee.JoinTopic(base, "bridge", "response", "permit_join")
	resp, err := zigbee.RequestOnce(
		ctx,
		client,
		qos,
		zigbee.JoinTopic(base, "bridge", "request", "permit_join"),
		encoded,
		responseTopic,
		timeout,
	)
	if err != nil {
		return err
	}

	var decoded any
	if err := json.Unmarshal(resp, &decoded); err != nil {
		decoded = string(resp)
	}

	row := types.NewRow(
		types.MRP("topic", responseTopic),
		types.MRP("payload", decoded),
	)
	if err := gp.AddRow(ctx, row); err != nil {
		return err
	}

	if !permit.Watch || permit.Seconds <= 0 {
		return nil
	}

	watchTopic := zigbee.JoinTopic(base, "bridge", "event")
	msgCh := make(chan mqtt.Message, 16)
	handler := func(_ mqtt.Client, msg mqtt.Message) {
		select {
		case msgCh <- msg:
		default:
		}
	}

	subToken := client.Subscribe(watchTopic, qos, handler)
	if !subToken.WaitTimeout(timeout) {
		return context.DeadlineExceeded
	}
	if err := subToken.Error(); err != nil {
		return err
	}
	defer client.Unsubscribe(watchTopic)

	deadline := time.NewTimer(time.Duration(permit.Seconds) * time.Second)
	defer deadline.Stop()

	for {
		select {
		case <-ctx.Done():
			return ctx.Err()
		case <-deadline.C:
			return nil
		case msg := <-msgCh:
			payload := msg.Payload()
			var event map[string]any
			if err := json.Unmarshal(payload, &event); err != nil {
				continue
			}
			eventType := asString(event["type"])
			if !isJoinEvent(eventType) {
				continue
			}
			row := types.NewRow(
				types.MRP("topic", msg.Topic()),
				types.MRP("event_type", eventType),
				types.MRP("device", extractDeviceFromEvent(event)),
				types.MRP("payload", event),
			)
			if err := gp.AddRow(ctx, row); err != nil {
				return err
			}
		}
	}
}

func extractDeviceFromEvent(event map[string]any) string {
	data, ok := event["data"].(map[string]any)
	if !ok {
		return ""
	}
	if name := asString(data["friendly_name"]); name != "" {
		return name
	}
	if name := asString(data["ieee_address"]); name != "" {
		return name
	}
	return ""
}

func isJoinEvent(eventType string) bool {
	switch eventType {
	case "device_joined", "device_interview", "device_announce", "device_network_address_changed":
		return true
	default:
		return false
	}
}
