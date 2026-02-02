package sniff

import (
	"context"
	"fmt"

	"github.com/go-go-golems/glazed/pkg/cli"
	"github.com/go-go-golems/glazed/pkg/cmds"
	"github.com/go-go-golems/glazed/pkg/cmds/fields"
	"github.com/go-go-golems/glazed/pkg/cmds/schema"
	"github.com/go-go-golems/glazed/pkg/cmds/values"
	"github.com/go-go-golems/glazed/pkg/middlewares"
	"github.com/go-go-golems/glazed/pkg/types"
	"github.com/go-go-golems/zigctl/pkg/sniffer/nrf"
	"github.com/go-go-golems/zigctl/pkg/zigbee"
)

type NrfChannelCommand struct {
	*cmds.CommandDescription
	defaults zigbee.Config
}

var _ cmds.GlazeCommand = (*NrfChannelCommand)(nil)

type NrfChannelSettings struct {
	Port    string `glazed.parameter:"port"`
	Baud    int    `glazed.parameter:"baud"`
	Channel int    `glazed.parameter:"channel"`
}

func NewNrfChannelCommand(defaults zigbee.Config) (*NrfChannelCommand, error) {
	glazedLayer, err := schema.NewGlazedSchema()
	if err != nil {
		return nil, err
	}

	commandSettingsLayer, err := cli.NewCommandSettingsLayer()
	if err != nil {
		return nil, err
	}

	snifferLayer, err := zigbee.NewSnifferLayer(defaults)
	if err != nil {
		return nil, err
	}

	cmdDesc := cmds.NewCommandDescription(
		"channel",
		cmds.WithShort("Get or set the sniffer channel"),
		cmds.WithLong("Set the IEEE 802.15.4 channel on the nRF sniffer or query the current value."),
		cmds.WithFlags(
			fields.New(
				"port",
				fields.TypeString,
				fields.WithHelp("Serial port to use (defaults to sniffer config or auto-detect)"),
			),
			fields.New(
				"baud",
				fields.TypeInteger,
				fields.WithHelp("Serial baud rate (defaults to sniffer config)"),
			),
			fields.New(
				"channel",
				fields.TypeInteger,
				fields.WithHelp("Channel to set (11-26); omit to query"),
			),
		),
		cmds.WithLayersList(glazedLayer, snifferLayer, commandSettingsLayer),
	)

	return &NrfChannelCommand{CommandDescription: cmdDesc, defaults: defaults}, nil
}

func (c *NrfChannelCommand) RunIntoGlazeProcessor(
	ctx context.Context,
	vals *values.Values,
	gp middlewares.Processor,
) error {
	settings := NrfChannelSettings{}
	if err := values.DecodeSectionInto(vals, schema.DefaultSlug, &settings); err != nil {
		return err
	}

	snifferSettings, err := decodeSnifferLayer(vals)
	if err != nil {
		return err
	}

	port, _, err := resolvePort(settings.Port, snifferSettings)
	if err != nil {
		return err
	}

	baud := settings.Baud
	if baud == 0 {
		baud = snifferSettings.NrfSerialBaud
	}

	session, err := nrf.Open(port, baud, snifferSettings.NrfAutoSleepOnExit)
	if err != nil {
		return err
	}
	defer session.Close()

	_ = session.SendCommand("sleep")
	_ = session.SendCommand("shell echo off")

	requested := settings.Channel
	if requested != 0 {
		if requested < 11 || requested > 26 {
			return fmt.Errorf("channel must be between 11 and 26")
		}
		if err := session.SendCommand(fmt.Sprintf("channel %d", requested)); err != nil {
			return err
		}
	}

	current, err := session.QueryChannel()
	if err != nil {
		return err
	}

	row := types.NewRow(
		types.MRP("port", port),
		types.MRP("requested_channel", requested),
		types.MRP("current_channel", current),
	)
	return gp.AddRow(ctx, row)
}
