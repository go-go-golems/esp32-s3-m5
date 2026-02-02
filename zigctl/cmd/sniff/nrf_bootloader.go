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

type NrfBootloaderCommand struct {
	*cmds.CommandDescription
	defaults zigbee.Config
}

var _ cmds.GlazeCommand = (*NrfBootloaderCommand)(nil)

type NrfBootloaderSettings struct {
	Port string `glazed.parameter:"port"`
	Baud int    `glazed.parameter:"baud"`
}

func NewNrfBootloaderCommand(defaults zigbee.Config) (*NrfBootloaderCommand, error) {
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
		"bootloader",
		cmds.WithShort("Reboot the sniffer into bootloader mode"),
		cmds.WithLong("Send the bootloader command to the nRF sniffer (dongle only)."),
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

	return &NrfBootloaderCommand{CommandDescription: cmdDesc, defaults: defaults}, nil
}

func (c *NrfBootloaderCommand) RunIntoGlazeProcessor(
	ctx context.Context,
	vals *values.Values,
	gp middlewares.Processor,
) error {
	settings := NrfBootloaderSettings{}
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

	if err := session.SendCommand("bootloader"); err != nil {
		return err
	}

	row := types.NewRow(
		types.MRP("port", port),
		types.MRP("status", "sent"),
	)
	return gp.AddRow(ctx, row)
}
