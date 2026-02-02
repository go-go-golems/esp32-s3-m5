package zigbee

type SnifferConfig struct {
	Nrf NrfSnifferConfig `yaml:"nrf"`
}

type NrfSnifferConfig struct {
	DefaultPort     string `yaml:"default_port"`
	DefaultChannel  int    `yaml:"default_channel"`
	DefaultFormat   string `yaml:"default_format"`
	AutoSleepOnExit bool   `yaml:"auto_sleep_on_exit"`
	SerialBaud      int    `yaml:"serial_baud"`
}

func DefaultSnifferConfig() SnifferConfig {
	return SnifferConfig{
		Nrf: DefaultNrfSnifferConfig(),
	}
}

func DefaultNrfSnifferConfig() NrfSnifferConfig {
	return NrfSnifferConfig{
		DefaultPort:     "",
		DefaultChannel:  11,
		DefaultFormat:   "pcapng-tap",
		AutoSleepOnExit: true,
		SerialBaud:      115200,
	}
}

func (c SnifferConfig) WithDefaults(defaults SnifferConfig) SnifferConfig {
	out := c
	out.Nrf = out.Nrf.WithDefaults(defaults.Nrf)
	return out
}

func (c NrfSnifferConfig) WithDefaults(defaults NrfSnifferConfig) NrfSnifferConfig {
	out := c
	if out.DefaultPort == "" {
		out.DefaultPort = defaults.DefaultPort
	}
	if out.DefaultChannel == 0 {
		out.DefaultChannel = defaults.DefaultChannel
	}
	if out.DefaultFormat == "" {
		out.DefaultFormat = defaults.DefaultFormat
	}
	if out.SerialBaud == 0 {
		out.SerialBaud = defaults.SerialBaud
	}
	return out
}
