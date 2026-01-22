package storage

import "encoding/json"

// PatternConfig and related models intentionally follow the imported UI spec in
// fw-screens.md (do not rename JSON fields).

type PatternConfig struct {
	Type       string          `json:"type"`
	Brightness int             `json:"brightness"`
	Params     map[string]any  `json:"params"`
	_          json.RawMessage `json:"-"` // forward-compat padding
}

type Preset struct {
	ID     string        `json:"id"`
	Name   string        `json:"name"`
	Icon   string        `json:"icon"`
	Config PatternConfig `json:"config"`
}

type Settings struct {
	BindIP              string `json:"bind_ip"`
	MulticastGroup      string `json:"multicast_group"`
	MulticastPort       int    `json:"multicast_port"`
	DiscoveryIntervalMS int    `json:"discovery_interval_ms"`
	OfflineThresholdS   int    `json:"offline_threshold_s"`
}
