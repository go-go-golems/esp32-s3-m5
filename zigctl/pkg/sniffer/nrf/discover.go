package nrf

import (
	"fmt"
	"strings"

	"go.bug.st/serial/enumerator"
)

const (
	NordicVID  = "1915"
	SnifferPID = "154B"
)

func ListDevices() ([]DeviceInfo, error) {
	ports, err := enumerator.GetDetailedPortsList()
	if err != nil {
		return nil, err
	}

	devices := make([]DeviceInfo, 0)
	for _, port := range ports {
		if !port.IsUSB {
			continue
		}
		if !strings.EqualFold(port.VID, NordicVID) || !strings.EqualFold(port.PID, SnifferPID) {
			continue
		}
		devices = append(devices, DeviceInfo{
			PortName:     port.Name,
			VID:          port.VID,
			PID:          port.PID,
			Product:      port.Product,
			SerialNumber: port.SerialNumber,
		})
	}

	return devices, nil
}

func ResolvePort(explicit string) (string, []DeviceInfo, error) {
	devices, err := ListDevices()
	if err != nil {
		return "", nil, err
	}

	if explicit != "" {
		return explicit, devices, nil
	}

	if len(devices) == 1 {
		return devices[0].PortName, devices, nil
	}
	if len(devices) == 0 {
		return "", devices, fmt.Errorf("no nRF 802.15.4 sniffer devices found")
	}
	return "", devices, fmt.Errorf("multiple nRF sniffers detected; specify --port")
}
