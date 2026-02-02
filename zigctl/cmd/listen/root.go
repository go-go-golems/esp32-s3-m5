package listen

import (
	"github.com/go-go-golems/zigctl/pkg/zigbee"
	"github.com/spf13/cobra"
)

func Register(root *cobra.Command, defaults zigbee.Config) error {
	listenCmd := &cobra.Command{
		Use:   "listen",
		Short: "Stream Zigbee2MQTT events",
		Long:  "Listen commands subscribe to MQTT topics and stream Zigbee2MQTT events and state updates.",
	}
	root.AddCommand(listenCmd)
	_ = defaults
	return nil
}
