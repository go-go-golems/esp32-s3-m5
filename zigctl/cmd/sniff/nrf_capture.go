package sniff

import (
	"context"
	"fmt"
	"os"
	"path/filepath"
	"time"

	"github.com/go-go-golems/glazed/pkg/cli"
	"github.com/go-go-golems/glazed/pkg/cmds"
	"github.com/go-go-golems/glazed/pkg/cmds/fields"
	"github.com/go-go-golems/glazed/pkg/cmds/layers"
	"github.com/go-go-golems/glazed/pkg/cmds/schema"
	"github.com/go-go-golems/zigctl/pkg/sniffer/nrf"
	"github.com/go-go-golems/zigctl/pkg/sniffer/pcap"
	"github.com/go-go-golems/zigctl/pkg/zigbee"
)

type NrfCaptureCommand struct {
	*cmds.CommandDescription
	defaults zigbee.Config
}

var _ cmds.BareCommand = (*NrfCaptureCommand)(nil)

type NrfCaptureSettings struct {
	Port    string `glazed.parameter:"port"`
	Baud    int    `glazed.parameter:"baud"`
	Channel int    `glazed.parameter:"channel"`
	Out     string `glazed.parameter:"out"`
	Format  string `glazed.parameter:"format"`
}

func NewNrfCaptureCommand(defaults zigbee.Config) (*NrfCaptureCommand, error) {
	commandSettingsLayer, err := cli.NewCommandSettingsLayer()
	if err != nil {
		return nil, err
	}

	snifferLayer, err := zigbee.NewSnifferLayer(defaults)
	if err != nil {
		return nil, err
	}

	cmdDesc := cmds.NewCommandDescription(
		"capture",
		cmds.WithShort("Capture to a pcapng file"),
		cmds.WithLong("Capture IEEE 802.15.4 frames from the nRF sniffer and write a pcapng file."),
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
				fields.WithHelp("Channel to capture (defaults to sniffer config)"),
			),
			fields.New(
				"out",
				fields.TypeString,
				fields.WithHelp("Output pcapng file path"),
			),
			fields.New(
				"format",
				fields.TypeString,
				fields.WithHelp("Output format: pcapng-tap or pcap-nofcs"),
			),
		),
		cmds.WithLayersList(snifferLayer, commandSettingsLayer),
	)

	return &NrfCaptureCommand{CommandDescription: cmdDesc, defaults: defaults}, nil
}

func (c *NrfCaptureCommand) Run(ctx context.Context, parsedLayers *layers.ParsedLayers) error {
	settings := NrfCaptureSettings{}
	if err := parsedLayers.InitializeStruct(schema.DefaultSlug, &settings); err != nil {
		return err
	}

	snifferSettings, err := initSnifferLayer(parsedLayers)
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

	channel := settings.Channel
	if channel == 0 {
		channel = snifferSettings.NrfDefaultChannel
	}
	if channel < 11 || channel > 26 {
		return fmt.Errorf("channel must be between 11 and 26")
	}

	format, err := resolveFormat(settings.Format, snifferSettings)
	if err != nil {
		return err
	}

	outPath := settings.Out
	if outPath == "" {
		outPath = fmt.Sprintf("sniff-%s.pcapng", time.Now().Format("20060102-150405"))
	}
	if err := os.MkdirAll(filepath.Dir(outPath), 0o755); err != nil {
		return err
	}

	file, err := os.Create(outPath)
	if err != nil {
		return err
	}
	defer file.Close()

	linkType := pcap.LinkTypeIEEE802154TAP
	if format == formatPcapNoFCS {
		linkType = pcap.LinkTypeIEEE802154NoFCS
	}

	writer, err := pcap.NewWriter(file, linkType)
	if err != nil {
		return err
	}
	defer writer.Flush()

	session, err := nrf.Open(port, baud, snifferSettings.NrfAutoSleepOnExit)
	if err != nil {
		return err
	}
	defer session.Close()

	if err := session.Start(channel); err != nil {
		return err
	}
	if snifferSettings.NrfAutoSleepOnExit {
		defer session.Stop()
	}

	handler := func(packet nrf.Packet) error {
		payload := packet.Payload
		if format == formatPcapngTap {
			var err error
			payload, err = pcap.BuildTAPPayload(packet.Channel, packet.RSSI, packet.LQI, packet.Payload)
			if err != nil {
				return err
			}
		}
		ts := time.Unix(0, packet.CorrectedMicros*int64(time.Microsecond))
		return writer.WritePacket(ts, payload)
	}

	return session.Loop(ctx, handler)
}
