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

type NrfDoctorCommand struct {
	*cmds.CommandDescription
	defaults zigbee.Config
}

var _ cmds.GlazeCommand = (*NrfDoctorCommand)(nil)

type NrfDoctorSettings struct {
	Port    string `glazed.parameter:"port"`
	Baud    int    `glazed.parameter:"baud"`
	Channel int    `glazed.parameter:"channel"`
}

func NewNrfDoctorCommand(defaults zigbee.Config) (*NrfDoctorCommand, error) {
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
		"doctor",
		cmds.WithShort("Diagnose nRF sniffer connectivity"),
		cmds.WithLong("Run a series of checks against the nRF sniffer device and report results."),
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
				fields.WithHelp("Optional channel to set as part of the check"),
			),
		),
		cmds.WithLayersList(glazedLayer, snifferLayer, commandSettingsLayer),
	)

	return &NrfDoctorCommand{CommandDescription: cmdDesc, defaults: defaults}, nil
}

func (c *NrfDoctorCommand) RunIntoGlazeProcessor(
	ctx context.Context,
	vals *values.Values,
	gp middlewares.Processor,
) error {
	settings := NrfDoctorSettings{}
	if err := values.DecodeSectionInto(vals, schema.DefaultSlug, &settings); err != nil {
		return err
	}

	snifferSettings, err := decodeSnifferLayer(vals)
	if err != nil {
		return err
	}

	checksFailed := false
	addCheck := func(name, status, detail string) error {
		row := types.NewRow(
			types.MRP("check", name),
			types.MRP("status", status),
			types.MRP("detail", detail),
		)
		return gp.AddRow(ctx, row)
	}

	devices, err := nrf.ListDevices()
	if err != nil {
		checksFailed = true
		if err := addCheck("discover", "error", err.Error()); err != nil {
			return err
		}
	} else if len(devices) == 0 {
		checksFailed = true
		if err := addCheck("discover", "error", "no devices found"); err != nil {
			return err
		}
	} else {
		if err := addCheck("discover", "ok", fmt.Sprintf("%d device(s)", len(devices))); err != nil {
			return err
		}
	}

	port, _, err := resolvePort(settings.Port, snifferSettings)
	if err != nil {
		checksFailed = true
		if err := addCheck("resolve-port", "error", err.Error()); err != nil {
			return err
		}
		if checksFailed {
			return fmt.Errorf("doctor checks failed")
		}
	}

	baud := settings.Baud
	if baud == 0 {
		baud = snifferSettings.NrfSerialBaud
	}

	session, err := nrf.Open(port, baud, snifferSettings.NrfAutoSleepOnExit)
	if err != nil {
		checksFailed = true
		if err := addCheck("open", "error", err.Error()); err != nil {
			return err
		}
		if checksFailed {
			return fmt.Errorf("doctor checks failed")
		}
	}
	defer session.Close()

	if err := session.SendCommand("sleep"); err != nil {
		checksFailed = true
		if err := addCheck("sleep", "error", err.Error()); err != nil {
			return err
		}
	} else {
		if err := addCheck("sleep", "ok", "command sent"); err != nil {
			return err
		}
	}

	_ = session.SendCommand("shell echo off")

	if settings.Channel != 0 {
		if settings.Channel < 11 || settings.Channel > 26 {
			checksFailed = true
			if err := addCheck("set-channel", "error", "channel must be between 11 and 26"); err != nil {
				return err
			}
		} else if err := session.SendCommand(fmt.Sprintf("channel %d", settings.Channel)); err != nil {
			checksFailed = true
			if err := addCheck("set-channel", "error", err.Error()); err != nil {
				return err
			}
		} else if err := addCheck("set-channel", "ok", fmt.Sprintf("channel %d", settings.Channel)); err != nil {
			return err
		}
	}

	if ch, err := session.QueryChannel(); err != nil {
		checksFailed = true
		if err := addCheck("query-channel", "error", err.Error()); err != nil {
			return err
		}
	} else {
		if err := addCheck("query-channel", "ok", fmt.Sprintf("channel %d", ch)); err != nil {
			return err
		}
	}

	if checksFailed {
		return fmt.Errorf("doctor checks failed")
	}
	return nil
}
