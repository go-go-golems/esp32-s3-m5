package zigbee

import (
	"github.com/go-go-golems/glazed/pkg/cmds/fields"
	"github.com/go-go-golems/glazed/pkg/cmds/schema"
)

const LayerSlug = "zigbee"

func NewZigbeeLayer(defaults Config) (*schema.SectionImpl, error) {
	cfg := defaults.WithDefaults()
	return schema.NewSection(
		LayerSlug,
		"Zigbee",
		schema.WithDescription("Zigbee2MQTT broker settings"),
		schema.WithFields(
			fields.New(
				"broker",
				fields.TypeString,
				fields.WithDefault(cfg.Broker),
				fields.WithHelp("MQTT broker URL (mqtt:// or mqtts://)"),
			),
			fields.New(
				"base-topic",
				fields.TypeString,
				fields.WithDefault(cfg.BaseTopic),
				fields.WithHelp("Base Zigbee2MQTT topic (default: zigbee2mqtt)"),
			),
			fields.New(
				"tls",
				fields.TypeBool,
				fields.WithDefault(cfg.TLS),
				fields.WithHelp("Enable TLS for MQTT connections"),
			),
			fields.New(
				"cafile",
				fields.TypeString,
				fields.WithDefault(cfg.CAFile),
				fields.WithHelp("CA bundle path for TLS"),
			),
			fields.New(
				"cert",
				fields.TypeString,
				fields.WithDefault(cfg.CertFile),
				fields.WithHelp("Client certificate path for TLS"),
			),
			fields.New(
				"key",
				fields.TypeString,
				fields.WithDefault(cfg.KeyFile),
				fields.WithHelp("Client private key path for TLS"),
			),
			fields.New(
				"qos",
				fields.TypeInteger,
				fields.WithDefault(cfg.QOS),
				fields.WithHelp("MQTT QoS (0, 1, or 2)"),
			),
			fields.New(
				"timeout",
				fields.TypeString,
				fields.WithDefault(cfg.Timeout),
				fields.WithHelp("Request timeout (Go duration, e.g. 10s, 1m)"),
			),
		),
	)
}
