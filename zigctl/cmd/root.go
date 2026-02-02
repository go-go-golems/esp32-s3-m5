package cmd

import (
	"fmt"
	"os"

	"github.com/go-go-golems/glazed/pkg/help"
	help_cmd "github.com/go-go-golems/glazed/pkg/help/cmd"
	"github.com/go-go-golems/zigctl/cmd/bridge"
	"github.com/go-go-golems/zigctl/cmd/js"
	"github.com/go-go-golems/zigctl/cmd/listen"
	"github.com/go-go-golems/zigctl/cmd/mqtt"
	"github.com/go-go-golems/zigctl/doc"
	"github.com/go-go-golems/zigctl/pkg/zigbee"
	"github.com/spf13/cobra"
)

func NewRootCommand() (*cobra.Command, error) {
	root := &cobra.Command{
		Use:   "zigctl",
		Short: "CLI for Zigbee2MQTT networks",
		Long:  "zigctl is a CLI for interacting with Zigbee2MQTT over MQTT. It supports bridge commands, device/group control, and streaming listeners.",
	}

	helpSystem := help.NewHelpSystem()
	if err := doc.AddDocToHelpSystem(helpSystem); err != nil {
		return nil, err
	}
	help_cmd.SetupCobraRootCommand(helpSystem, root)

	defaults, _, err := zigbee.LoadDefaultConfig()
	if err != nil {
		return nil, err
	}

	if err := bridge.Register(root, defaults); err != nil {
		return nil, err
	}
	if err := js.Register(root); err != nil {
		return nil, err
	}
	if err := listen.Register(root, defaults); err != nil {
		return nil, err
	}
	if err := mqtt.Register(root, defaults); err != nil {
		return nil, err
	}

	return root, nil
}

func Execute() error {
	root, err := NewRootCommand()
	if err != nil {
		return err
	}
	if err := root.Execute(); err != nil {
		fmt.Fprintln(os.Stderr, err)
		return err
	}
	return nil
}
