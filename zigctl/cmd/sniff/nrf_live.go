package sniff

import (
	"context"
	"fmt"
	"os"
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

type NrfLiveCommand struct {
	*cmds.CommandDescription
	defaults zigbee.Config
}

var _ cmds.BareCommand = (*NrfLiveCommand)(nil)

type NrfLiveSettings struct {
	Port    string `glazed.parameter:"port"`
	Baud    int    `glazed.parameter:"baud"`
	Channel int    `glazed.parameter:"channel"`
	Format  string `glazed.parameter:"format"`
}

func NewNrfLiveCommand(defaults zigbee.Config) (*NrfLiveCommand, error) {
	commandSettingsLayer, err := cli.NewCommandSettingsLayer()
	if err != nil {
		return nil, err
	}

	snifferLayer, err := zigbee.NewSnifferLayer(defaults)
	if err != nil {
		return nil, err
	}

	cmdDesc := cmds.NewCommandDescription(
		"live",
		cmds.WithShort("Stream pcapng to stdout"),
		cmds.WithLong("Stream IEEE 802.15.4 frames as pcapng to stdout for Wireshark/tshark piping."),
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
				"format",
				fields.TypeString,
				fields.WithHelp("Output format: pcapng-tap or pcap-nofcs"),
			),
		),
		cmds.WithLayersList(snifferLayer, commandSettingsLayer),
	)

	return &NrfLiveCommand{CommandDescription: cmdDesc, defaults: defaults}, nil
}

func (c *NrfLiveCommand) Run(ctx context.Context, parsedLayers *layers.ParsedLayers) error {
	settings := NrfLiveSettings{}
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

	linkType := pcap.LinkTypeIEEE802154TAP
	if format == formatPcapNoFCS {
		linkType = pcap.LinkTypeIEEE802154NoFCS
	}

	writer, err := pcap.NewWriter(os.Stdout, linkType)
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
