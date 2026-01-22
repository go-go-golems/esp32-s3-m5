package storage

import (
	"encoding/json"
	"errors"
	"os"
)

type SettingsStore struct {
	path string
}

func NewSettingsStore(path string) *SettingsStore {
	return &SettingsStore{path: path}
}

func (s *SettingsStore) Path() string { return s.path }

func (s *SettingsStore) Default() Settings {
	return Settings{
		BindIP:              "auto",
		MulticastGroup:      "239.255.32.6",
		MulticastPort:       4626,
		DiscoveryIntervalMS: 1000,
		OfflineThresholdS:   30,
	}
}

func (s *SettingsStore) Load() (Settings, error) {
	b, err := os.ReadFile(s.path)
	if err != nil {
		if errors.Is(err, os.ErrNotExist) {
			return Settings{}, os.ErrNotExist
		}
		return Settings{}, err
	}
	var out Settings
	if err := json.Unmarshal(b, &out); err != nil {
		return Settings{}, err
	}
	return out, nil
}

func (s *SettingsStore) Save(v Settings) error {
	b, err := json.MarshalIndent(v, "", "  ")
	if err != nil {
		return err
	}
	return os.WriteFile(s.path, b, 0o644)
}
