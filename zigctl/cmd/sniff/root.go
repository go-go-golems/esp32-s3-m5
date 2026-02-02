package sniff

import (
	"github.com/go-go-golems/glazed/pkg/cli"
	"github.com/go-go-golems/glazed/pkg/cmds"
	"github.com/go-go-golems/glazed/pkg/cmds/schema"
	"github.com/go-go-golems/zigctl/pkg/zigbee"
	"github.com/spf13/cobra"
)

func Register(root *cobra.Command, defaults zigbee.Config) error {
	sniffCmd := &cobra.Command{
		Use:   "sniff",
		Short: "Capture over-the-air Zigbee (802.15.4) traffic",
		Long:  "Sniffer commands capture IEEE 802.15.4 frames and stream them to pcap/pcapng outputs.",
	}

	nrfCmd := &cobra.Command{
		Use:   "nrf",
		Short: "nRF 802.15.4 sniffer commands",
		Long:  "Interact with the Nordic nRF 802.15.4 sniffer over USB serial.",
	}

	listCmd, err := NewNrfListCommand(defaults)
	if err != nil {
		return err
	}
	cobraList, err := buildCobra(listCmd)
	if err != nil {
		return err
	}

	infoCmd, err := NewNrfInfoCommand(defaults)
	if err != nil {
		return err
	}
	cobraInfo, err := buildCobra(infoCmd)
	if err != nil {
		return err
	}

	channelCmd, err := NewNrfChannelCommand(defaults)
	if err != nil {
		return err
	}
	cobraChannel, err := buildCobra(channelCmd)
	if err != nil {
		return err
	}

	doctorCmd, err := NewNrfDoctorCommand(defaults)
	if err != nil {
		return err
	}
	cobraDoctor, err := buildCobra(doctorCmd)
	if err != nil {
		return err
	}

	captureCmd, err := NewNrfCaptureCommand(defaults)
	if err != nil {
		return err
	}
	cobraCapture, err := buildCobra(captureCmd)
	if err != nil {
		return err
	}

	liveCmd, err := NewNrfLiveCommand(defaults)
	if err != nil {
		return err
	}
	cobraLive, err := buildCobra(liveCmd)
	if err != nil {
		return err
	}

	bootCmd, err := NewNrfBootloaderCommand(defaults)
	if err != nil {
		return err
	}
	cobraBoot, err := buildCobra(bootCmd)
	if err != nil {
		return err
	}

	nrfCmd.AddCommand(cobraList, cobraInfo, cobraChannel, cobraDoctor, cobraCapture, cobraLive, cobraBoot)
	sniffCmd.AddCommand(nrfCmd)
	root.AddCommand(sniffCmd)
	return nil
}

func buildCobra(cmd cmds.Command) (*cobra.Command, error) {
	return cli.BuildCobraCommand(cmd,
		cli.WithParserConfig(cli.CobraParserConfig{
			ShortHelpLayers: []string{schema.DefaultSlug, zigbee.SnifferLayerSlug},
			MiddlewaresFunc: cli.CobraCommandDefaultMiddlewares,
		}),
	)
}
