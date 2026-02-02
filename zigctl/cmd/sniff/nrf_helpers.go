package sniff

import (
	"github.com/go-go-golems/zigctl/pkg/sniffer/nrf"
)

func resolvePort(explicit string, defaults SnifferLayerSettings) (string, []nrf.DeviceInfo, error) {
	port := explicit
	if port == "" {
		port = defaults.NrfDefaultPort
	}
	return nrf.ResolvePort(port)
}

func findDevice(devices []nrf.DeviceInfo, port string) *nrf.DeviceInfo {
	for _, device := range devices {
		if device.PortName == port {
			copy := device
			return &copy
		}
	}
	return nil
}
