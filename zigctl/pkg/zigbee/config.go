package zigbee

import (
	"errors"
	"os"
	"path/filepath"

	"gopkg.in/yaml.v3"
)

type Config struct {
	Broker    string `yaml:"broker"`
	BaseTopic string `yaml:"base_topic"`
	TLS       bool   `yaml:"tls"`
	CAFile    string `yaml:"cafile"`
	CertFile  string `yaml:"cert"`
	KeyFile   string `yaml:"key"`
	QOS       int    `yaml:"qos"`
	Timeout   string `yaml:"timeout"`
}

func DefaultConfig() Config {
	return Config{
		Broker:    "mqtt://localhost:1883",
		BaseTopic: "zigbee2mqtt",
		TLS:       false,
		QOS:       0,
		Timeout:   "10s",
	}
}

func DefaultConfigPath() (string, error) {
	base, err := os.UserConfigDir()
	if err != nil {
		return "", err
	}
	return filepath.Join(base, "zigctl", "config.yaml"), nil
}

func LoadConfig(path string) (Config, bool, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		if errors.Is(err, os.ErrNotExist) {
			return Config{}, false, nil
		}
		return Config{}, false, err
	}

	cfg := DefaultConfig()
	if err := yaml.Unmarshal(data, &cfg); err != nil {
		return Config{}, false, err
	}
	return cfg, true, nil
}

func LoadDefaultConfig() (Config, bool, error) {
	path, err := DefaultConfigPath()
	if err != nil {
		return Config{}, false, err
	}
	return LoadConfig(path)
}

func (c Config) WithDefaults() Config {
	defaults := DefaultConfig()
	out := c
	if out.Broker == "" {
		out.Broker = defaults.Broker
	}
	if out.BaseTopic == "" {
		out.BaseTopic = defaults.BaseTopic
	}
	if out.Timeout == "" {
		out.Timeout = defaults.Timeout
	}
	return out
}
