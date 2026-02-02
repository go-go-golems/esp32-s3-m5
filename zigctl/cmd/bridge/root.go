package bridge

import (
	"github.com/go-go-golems/zigctl/pkg/zigbee"
	"github.com/spf13/cobra"
)

func Register(root *cobra.Command, defaults zigbee.Config) error {
	bridgeCmd := &cobra.Command{
		Use:   "bridge",
		Short: "Bridge-level Zigbee2MQTT operations",
		Long:  "Bridge commands query Zigbee2MQTT bridge state and perform network-level actions.",
	}
	root.AddCommand(bridgeCmd)
	_ = defaults
	return nil
}
