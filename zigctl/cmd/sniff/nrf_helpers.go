package sniff

import (
	"fmt"

	"github.com/go-go-golems/zigctl/pkg/sniffer/nrf"
)

const (
	formatPcapngTap = "pcapng-tap"
	formatPcapNoFCS = "pcap-nofcs"
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

func resolveFormat(explicit string, defaults SnifferLayerSettings) (string, error) {
	format := explicit
	if format == "" {
		format = defaults.NrfDefaultFormat
	}
	switch format {
	case formatPcapngTap, formatPcapNoFCS:
		return format, nil
	default:
		return "", fmt.Errorf("unknown format %q (expected %q or %q)", format, formatPcapngTap, formatPcapNoFCS)
	}
}
