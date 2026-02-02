package mqtt

import (
	"github.com/go-go-golems/zigctl/pkg/zigbee"
	"github.com/spf13/cobra"
)

func Register(root *cobra.Command, defaults zigbee.Config) error {
	mqttCmd := &cobra.Command{
		Use:   "mqtt",
		Short: "Raw MQTT publish/subscribe helpers",
		Long:  "MQTT commands provide low-level publish/subscribe access to Zigbee2MQTT topics.",
	}
	root.AddCommand(mqttCmd)
	_ = defaults
	return nil
}
