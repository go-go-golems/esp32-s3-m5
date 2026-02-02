package zigbee

import (
	"github.com/go-go-golems/glazed/pkg/cmds/fields"
	"github.com/go-go-golems/glazed/pkg/cmds/schema"
)

const SnifferLayerSlug = "sniffer"

func NewSnifferLayer(defaults Config) (*schema.SectionImpl, error) {
	cfg := defaults.WithDefaults()
	return schema.NewSection(
		SnifferLayerSlug,
		"Sniffer",
		schema.WithDescription("Sniffer hardware defaults"),
		schema.WithFields(
			fields.New(
				"nrf-default-port",
				fields.TypeString,
				fields.WithDefault(cfg.Sniffer.Nrf.DefaultPort),
				fields.WithHelp("Default serial port for nRF sniffer"),
			),
			fields.New(
				"nrf-default-channel",
				fields.TypeInteger,
				fields.WithDefault(cfg.Sniffer.Nrf.DefaultChannel),
				fields.WithHelp("Default IEEE 802.15.4 channel"),
			),
			fields.New(
				"nrf-default-format",
				fields.TypeString,
				fields.WithDefault(cfg.Sniffer.Nrf.DefaultFormat),
				fields.WithHelp("Default output format (pcapng-tap or pcap-nofcs)"),
			),
			fields.New(
				"nrf-auto-sleep-on-exit",
				fields.TypeBool,
				fields.WithDefault(cfg.Sniffer.Nrf.AutoSleepOnExit),
				fields.WithHelp("Send sleep on exit to stop capture"),
			),
			fields.New(
				"nrf-serial-baud",
				fields.TypeInteger,
				fields.WithDefault(cfg.Sniffer.Nrf.SerialBaud),
				fields.WithHelp("Serial baud rate for nRF sniffer"),
			),
		),
	)
}
