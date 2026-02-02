package bridge

import (
	"context"
	"encoding/json"

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
	return gp.AddRow(ctx, row)
}
