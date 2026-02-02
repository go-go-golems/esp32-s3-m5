package listen

import (
	"github.com/go-go-golems/glazed/pkg/cli"
	"github.com/go-go-golems/glazed/pkg/cmds"
	"github.com/go-go-golems/glazed/pkg/cmds/schema"
	"github.com/go-go-golems/zigctl/pkg/zigbee"
	"github.com/spf13/cobra"
)

func Register(root *cobra.Command, defaults zigbee.Config) error {
	listenCmd := &cobra.Command{
		Use:   "listen",
		Short: "Stream Zigbee2MQTT events",
		Long:  "Listen commands subscribe to MQTT topics and stream Zigbee2MQTT events and state updates.",
	}

	stateCmd, err := NewStateCommand(defaults)
	if err != nil {
		return err
	}
	cobraState, err := buildCobra(stateCmd)
	if err != nil {
		return err
	}

	rawCmd, err := NewRawCommand(defaults)
	if err != nil {
		return err
	}
	cobraRaw, err := buildCobra(rawCmd)
	if err != nil {
		return err
	}

	listenCmd.AddCommand(cobraState, cobraRaw)
	root.AddCommand(listenCmd)
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
