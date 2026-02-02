package sniff

import (
	"github.com/go-go-golems/glazed/pkg/cmds/layers"
	"github.com/go-go-golems/glazed/pkg/cmds/values"
	"github.com/go-go-golems/zigctl/pkg/zigbee"
)

type SnifferLayerSettings struct {
	NrfDefaultPort     string `glazed.parameter:"nrf-default-port"`
	NrfDefaultChannel  int    `glazed.parameter:"nrf-default-channel"`
	NrfDefaultFormat   string `glazed.parameter:"nrf-default-format"`
	NrfAutoSleepOnExit bool   `glazed.parameter:"nrf-auto-sleep-on-exit"`
	NrfSerialBaud      int    `glazed.parameter:"nrf-serial-baud"`
}

func decodeSnifferLayer(vals *values.Values) (SnifferLayerSettings, error) {
	settings := SnifferLayerSettings{}
	if err := values.DecodeSectionInto(vals, zigbee.SnifferLayerSlug, &settings); err != nil {
		return SnifferLayerSettings{}, err
	}
	return settings, nil
}

func initSnifferLayer(parsed *layers.ParsedLayers) (SnifferLayerSettings, error) {
	settings := SnifferLayerSettings{}
	if err := parsed.InitializeStruct(zigbee.SnifferLayerSlug, &settings); err != nil {
		return SnifferLayerSettings{}, err
	}
	return settings, nil
}
