package bridge

import (
	"context"
	"encoding/json"
	"fmt"

	"github.com/go-go-golems/glazed/pkg/cli"
	"github.com/go-go-golems/glazed/pkg/cmds"
	"github.com/go-go-golems/glazed/pkg/cmds/schema"
	"github.com/go-go-golems/glazed/pkg/cmds/values"
	"github.com/go-go-golems/glazed/pkg/middlewares"
	"github.com/go-go-golems/glazed/pkg/types"
	"github.com/go-go-golems/zigctl/pkg/zigbee"
)

type DevicesCommand struct {
	*cmds.CommandDescription
	defaults zigbee.Config
}

var _ cmds.GlazeCommand = (*DevicesCommand)(nil)

func NewDevicesCommand(defaults zigbee.Config) (*DevicesCommand, error) {
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
		"devices",
		cmds.WithShort("List devices known to Zigbee2MQTT"),
		cmds.WithLong(`
Request the device list from Zigbee2MQTT and render it as structured rows.

Examples:
  zigctl bridge devices
  zigctl bridge devices --output json
  zigctl bridge devices --fields friendly_name,ieee_address,model_id
`),
		cmds.WithLayersList(glazedLayer, zigbeeLayer, commandSettingsLayer),
	)

	return &DevicesCommand{CommandDescription: cmdDesc, defaults: defaults}, nil
}

func (c *DevicesCommand) RunIntoGlazeProcessor(
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
	responseTopic := zigbee.JoinTopic(base, "bridge", "devices")
	payload, err := zigbee.RequestOnce(
		ctx,
		client,
		qos,
		zigbee.JoinTopic(base, "bridge", "request", "devices"),
		[]byte("{}"),
		responseTopic,
		timeout,
	)
	if err != nil {
		return err
	}

	var devices []map[string]any
	if err := json.Unmarshal(payload, &devices); err != nil {
		return fmt.Errorf("decode devices: %w", err)
	}

	for _, device := range devices {
		row := types.NewRow(
			types.MRP("friendly_name", asString(device["friendly_name"])),
			types.MRP("ieee_address", asString(device["ieee_address"])),
			types.MRP("type", asString(device["type"])),
			types.MRP("manufacturer", asString(device["manufacturer"])),
			types.MRP("model_id", asString(device["model_id"])),
			types.MRP("description", asString(device["description"])),
			types.MRP("power_source", asString(device["power_source"])),
			types.MRP("interview_completed", asBool(device["interview_completed"])),
		)
		if err := gp.AddRow(ctx, row); err != nil {
			return err
		}
	}

	return nil
}

func asString(v any) string {
	if v == nil {
		return ""
	}
	s, ok := v.(string)
	if ok {
		return s
	}
	return fmt.Sprint(v)
}

func asBool(v any) bool {
	b, ok := v.(bool)
	if ok {
		return b
	}
	return false
}
