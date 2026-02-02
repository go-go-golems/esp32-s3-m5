package sniff

import (
	"context"

	"github.com/go-go-golems/glazed/pkg/cli"
	"github.com/go-go-golems/glazed/pkg/cmds"
	"github.com/go-go-golems/glazed/pkg/cmds/schema"
	"github.com/go-go-golems/glazed/pkg/cmds/values"
	"github.com/go-go-golems/glazed/pkg/middlewares"
	"github.com/go-go-golems/glazed/pkg/types"
	"github.com/go-go-golems/zigctl/pkg/sniffer/nrf"
	"github.com/go-go-golems/zigctl/pkg/zigbee"
)

type NrfListCommand struct {
	*cmds.CommandDescription
	defaults zigbee.Config
}

var _ cmds.GlazeCommand = (*NrfListCommand)(nil)

func NewNrfListCommand(defaults zigbee.Config) (*NrfListCommand, error) {
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
		"list",
		cmds.WithShort("List detected nRF sniffers"),
		cmds.WithLong("List connected nRF 802.15.4 sniffer devices detected by USB VID/PID."),
		cmds.WithLayersList(glazedLayer, snifferLayer, commandSettingsLayer),
	)

	return &NrfListCommand{CommandDescription: cmdDesc, defaults: defaults}, nil
}

func (c *NrfListCommand) RunIntoGlazeProcessor(
	ctx context.Context,
	vals *values.Values,
	gp middlewares.Processor,
) error {
	devices, err := nrf.ListDevices()
	if err != nil {
		return err
	}

	for _, device := range devices {
		row := types.NewRow(
			types.MRP("port", device.PortName),
			types.MRP("vid", device.VID),
			types.MRP("pid", device.PID),
			types.MRP("serial", device.SerialNumber),
			types.MRP("manufacturer", device.Manufacturer),
			types.MRP("product", device.Product),
		)
		if err := gp.AddRow(ctx, row); err != nil {
			return err
		}
	}

	return nil
}
