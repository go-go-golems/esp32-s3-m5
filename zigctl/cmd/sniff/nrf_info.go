package sniff

import (
	"context"

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

type NrfInfoCommand struct {
	*cmds.CommandDescription
	defaults zigbee.Config
}

var _ cmds.GlazeCommand = (*NrfInfoCommand)(nil)

type NrfInfoSettings struct {
	Port string `glazed.parameter:"port"`
	Baud int    `glazed.parameter:"baud"`
}

func NewNrfInfoCommand(defaults zigbee.Config) (*NrfInfoCommand, error) {
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
		"info",
		cmds.WithShort("Show info for an nRF sniffer"),
		cmds.WithLong("Open the nRF sniffer serial port and report basic status and channel info."),
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
		),
		cmds.WithLayersList(glazedLayer, snifferLayer, commandSettingsLayer),
	)

	return &NrfInfoCommand{CommandDescription: cmdDesc, defaults: defaults}, nil
}

func (c *NrfInfoCommand) RunIntoGlazeProcessor(
	ctx context.Context,
	vals *values.Values,
	gp middlewares.Processor,
) error {
	settings := NrfInfoSettings{}
	if err := values.DecodeSectionInto(vals, schema.DefaultSlug, &settings); err != nil {
		return err
	}

	snifferSettings, err := decodeSnifferLayer(vals)
	if err != nil {
		return err
	}

	port, devices, err := resolvePort(settings.Port, snifferSettings)
	if err != nil {
		return err
	}

	baud := settings.Baud
	if baud == 0 {
		baud = snifferSettings.NrfSerialBaud
	}

	device := findDevice(devices, port)
	status := "ok"
	errMsg := ""
	channel := 0
	channelKnown := false
	vid := ""
	pid := ""
	serial := ""
	manufacturer := ""
	product := ""

	session, err := nrf.Open(port, baud, snifferSettings.NrfAutoSleepOnExit)
	if err != nil {
		status = "error"
		errMsg = err.Error()
	} else {
		defer session.Close()
		_ = session.SendCommand("sleep")
		_ = session.SendCommand("shell echo off")
		if ch, err := session.QueryChannel(); err == nil {
			channel = ch
			channelKnown = true
		} else {
			status = "warn"
			errMsg = err.Error()
		}
	}

	row := types.NewRow(
		types.MRP("port", port),
		types.MRP("status", status),
		types.MRP("channel", channel),
		types.MRP("channel_known", channelKnown),
		types.MRP("error", errMsg),
	)

	if device != nil {
		vid = device.VID
		pid = device.PID
		serial = device.SerialNumber
		manufacturer = device.Manufacturer
		product = device.Product
	}

	row.Set("vid", vid)
	row.Set("pid", pid)
	row.Set("serial", serial)
	row.Set("manufacturer", manufacturer)
	row.Set("product", product)

	return gp.AddRow(ctx, row)
}
