package bridge

import (
	"github.com/go-go-golems/glazed/pkg/cli"
	"github.com/go-go-golems/glazed/pkg/cmds"
	"github.com/go-go-golems/glazed/pkg/cmds/schema"
	"github.com/go-go-golems/zigctl/pkg/zigbee"
	"github.com/spf13/cobra"
)

func Register(root *cobra.Command, defaults zigbee.Config) error {
	bridgeCmd := &cobra.Command{
		Use:   "bridge",
		Short: "Bridge-level Zigbee2MQTT operations",
		Long:  "Bridge commands query Zigbee2MQTT bridge state and perform network-level actions.",
	}

	infoCmd, err := NewInfoCommand(defaults)
	if err != nil {
		return err
	}
	cobraInfo, err := buildCobra(infoCmd)
	if err != nil {
		return err
	}

	devicesCmd, err := NewDevicesCommand(defaults)
	if err != nil {
		return err
	}
	cobraDevices, err := buildCobra(devicesCmd)
	if err != nil {
		return err
	}

	permitJoinCmd, err := NewPermitJoinCommand(defaults)
	if err != nil {
		return err
	}
	cobraPermitJoin, err := buildCobra(permitJoinCmd)
	if err != nil {
		return err
	}

	bridgeCmd.AddCommand(cobraInfo, cobraDevices, cobraPermitJoin)
	root.AddCommand(bridgeCmd)
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
