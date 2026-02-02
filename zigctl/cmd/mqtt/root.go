package mqtt

import (
	"github.com/go-go-golems/glazed/pkg/cli"
	"github.com/go-go-golems/glazed/pkg/cmds"
	"github.com/go-go-golems/glazed/pkg/cmds/schema"
	"github.com/go-go-golems/zigctl/pkg/zigbee"
	"github.com/spf13/cobra"
)

func Register(root *cobra.Command, defaults zigbee.Config) error {
	mqttCmd := &cobra.Command{
		Use:   "mqtt",
		Short: "Raw MQTT publish/subscribe helpers",
		Long:  "MQTT commands provide low-level publish/subscribe access to Zigbee2MQTT topics.",
	}

	pubCmd, err := NewPubCommand(defaults)
	if err != nil {
		return err
	}
	cobraPub, err := buildCobra(pubCmd)
	if err != nil {
		return err
	}

	subCmd, err := NewSubCommand(defaults)
	if err != nil {
		return err
	}
	cobraSub, err := buildCobra(subCmd)
	if err != nil {
		return err
	}

	mqttCmd.AddCommand(cobraPub, cobraSub)
	root.AddCommand(mqttCmd)
	return nil
}

func buildCobra(cmd cmds.Command) (*cobra.Command, error) {
	return cli.BuildCobraCommand(cmd,
		cli.WithParserConfig(cli.CobraParserConfig{
			ShortHelpLayers: []string{schema.DefaultSlug, zigbee.LayerSlug},
			MiddlewaresFunc: cli.CobraCommandDefaultMiddlewares,
		}),
	)
}
