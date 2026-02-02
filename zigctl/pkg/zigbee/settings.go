package zigbee

import (
	"fmt"
	"net/url"
	"time"
)

type Settings struct {
	Broker    string `glazed.parameter:"broker"`
	BaseTopic string `glazed.parameter:"base-topic"`
	TLS       bool   `glazed.parameter:"tls"`
	CAFile    string `glazed.parameter:"cafile"`
	CertFile  string `glazed.parameter:"cert"`
	KeyFile   string `glazed.parameter:"key"`
	QOS       int    `glazed.parameter:"qos"`
	Timeout   string `glazed.parameter:"timeout"`
}

func (s Settings) TimeoutDuration() (time.Duration, error) {
	if s.Timeout == "" {
		return 0, fmt.Errorf("timeout is empty")
	}
	return time.ParseDuration(s.Timeout)
}

func (s Settings) QOSByte() (byte, error) {
	if s.QOS < 0 || s.QOS > 2 {
		return 0, fmt.Errorf("qos must be 0, 1, or 2 (got %d)", s.QOS)
	}
	return byte(s.QOS), nil
}

func (s Settings) BasePrefix() string {
	if s.BaseTopic == "" {
		return "zigbee2mqtt"
	}
	return s.BaseTopic
}

func (s Settings) TLSEnabled() bool {
	if s.TLS {
		return true
	}
	parsed, err := url.Parse(s.Broker)
	if err != nil {
		return false
	}
	return parsed.Scheme == "mqtts" || parsed.Scheme == "ssl" || parsed.Scheme == "tls"
}
