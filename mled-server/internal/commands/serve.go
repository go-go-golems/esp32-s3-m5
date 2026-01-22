package commands

import (
	"context"
	"errors"
	"fmt"
	"net/http"
	"os"
	"os/signal"
	"path/filepath"
	"syscall"
	"time"

	"github.com/go-go-golems/glazed/pkg/cli"
	"github.com/go-go-golems/glazed/pkg/cmds"
	"github.com/go-go-golems/glazed/pkg/cmds/fields"
	"github.com/go-go-golems/glazed/pkg/cmds/layers"
	"github.com/go-go-golems/glazed/pkg/cmds/schema"
	"github.com/rs/zerolog/log"

	"mled-server/internal/httpapi"
	"mled-server/internal/mledhost"
	"mled-server/internal/storage"
	"mled-server/internal/ws"
)

type ServeCommand struct {
	*cmds.CommandDescription
}

type ServeSettings struct {
	HTTPAddr string `glazed.parameter:"http-addr"`
	DataDir  string `glazed.parameter:"data-dir"`

	BindIP         string `glazed.parameter:"bind-ip"`
	Interface      string `glazed.parameter:"iface"`
	MulticastGroup string `glazed.parameter:"mcast-group"`
	MulticastPort  int    `glazed.parameter:"mcast-port"`
	MulticastTTL   int    `glazed.parameter:"mcast-ttl"`

	DiscoveryInterval string `glazed.parameter:"discovery-interval"`
	OfflineThreshold  string `glazed.parameter:"offline-threshold"`
	BeaconInterval    string `glazed.parameter:"beacon-interval"`
}

var _ cmds.BareCommand = &ServeCommand{}

func NewServeCommand() (*ServeCommand, error) {
	commandSettingsLayer, err := cli.NewCommandSettingsLayer()
	if err != nil {
		return nil, err
	}

	cmdDesc := cmds.NewCommandDescription(
		"serve",
		cmds.WithShort("Run HTTP + WS API server"),
		cmds.WithLong(`Run the MLED/1 host controller HTTP+WS server.

The server exposes:
  - REST API under /api/*
  - WebSocket under /ws

Examples:
  mled-server serve
  mled-server serve --http-addr 0.0.0.0:8765
  mled-server serve --data-dir ./var
  mled-server serve --bind-ip 192.168.1.50
  mled-server serve --iface en0
`),
		cmds.WithFlags(
			fields.New("http-addr", fields.TypeString, fields.WithDefault("localhost:8765"), fields.WithHelp("HTTP listen address")),
			fields.New("data-dir", fields.TypeString, fields.WithDefault("./var"), fields.WithHelp("Directory for settings/presets JSON files")),

			fields.New("bind-ip", fields.TypeString, fields.WithDefault(""), fields.WithHelp("Override bind IP for UDP socket (empty uses stored setting)")),
			fields.New("iface", fields.TypeString, fields.WithDefault(""), fields.WithHelp("Network interface for multicast (empty means OS default)")),
			fields.New("mcast-group", fields.TypeString, fields.WithDefault(""), fields.WithHelp("Override multicast group IP (empty uses stored setting)")),
			fields.New("mcast-port", fields.TypeInteger, fields.WithDefault(0), fields.WithHelp("Override multicast UDP port (0 uses stored setting)")),
			fields.New("mcast-ttl", fields.TypeInteger, fields.WithDefault(1), fields.WithHelp("Multicast TTL")),

			fields.New("discovery-interval", fields.TypeString, fields.WithDefault(""), fields.WithHelp("Override PING interval (Go duration, e.g. 1s). Empty uses stored setting; 0 disables background ping")),
			fields.New("offline-threshold", fields.TypeString, fields.WithDefault(""), fields.WithHelp("Override offline threshold (Go duration, e.g. 30s). Empty uses stored setting")),
			fields.New("beacon-interval", fields.TypeString, fields.WithDefault("500ms"), fields.WithHelp("BEACON interval (Go duration, e.g. 500ms). 0 disables beacons")),
		),
		cmds.WithLayersList(commandSettingsLayer),
	)

	return &ServeCommand{CommandDescription: cmdDesc}, nil
}

func (c *ServeCommand) Run(ctx context.Context, parsedLayers *layers.ParsedLayers) error {
	settings := &ServeSettings{}
	if err := parsedLayers.InitializeStruct(schema.DefaultSlug, settings); err != nil {
		return fmt.Errorf("failed to parse settings: %w", err)
	}

	log.Info().
		Str("http_addr", settings.HTTPAddr).
		Str("data_dir", settings.DataDir).
		Str("bind_ip_override", settings.BindIP).
		Str("iface", settings.Interface).
		Str("mcast_group_override", settings.MulticastGroup).
		Int("mcast_port_override", settings.MulticastPort).
		Int("mcast_ttl", settings.MulticastTTL).
		Str("discovery_interval_override", settings.DiscoveryInterval).
		Str("offline_threshold_override", settings.OfflineThreshold).
		Str("beacon_interval", settings.BeaconInterval).
		Msg("starting server")

	if err := os.MkdirAll(settings.DataDir, 0o755); err != nil {
		return err
	}
	absDataDir, err := filepath.Abs(settings.DataDir)
	if err != nil {
		return err
	}

	settingsStore := storage.NewSettingsStore(filepath.Join(absDataDir, "settings.json"))
	presetStore := storage.NewPresetStore(filepath.Join(absDataDir, "presets.json"))

	dirtySettings := false
	effective := settingsStore.Default()
	if loaded, err := settingsStore.Load(); err == nil {
		effective = loaded
	} else if errors.Is(err, os.ErrNotExist) {
		dirtySettings = true
	} else {
		return err
	}

	if settings.BindIP != "" {
		effective.BindIP = settings.BindIP
		dirtySettings = true
	}
	if settings.MulticastGroup != "" {
		effective.MulticastGroup = settings.MulticastGroup
		dirtySettings = true
	}
	if settings.MulticastPort != 0 {
		effective.MulticastPort = settings.MulticastPort
		dirtySettings = true
	}

	if settings.DiscoveryInterval != "" {
		d, err := time.ParseDuration(settings.DiscoveryInterval)
		if err != nil {
			return fmt.Errorf("invalid --discovery-interval: %w", err)
		}
		effective.DiscoveryIntervalMS = int(d / time.Millisecond)
		dirtySettings = true
	}
	if settings.OfflineThreshold != "" {
		d, err := time.ParseDuration(settings.OfflineThreshold)
		if err != nil {
			return fmt.Errorf("invalid --offline-threshold: %w", err)
		}
		effective.OfflineThresholdS = int(d / time.Second)
		dirtySettings = true
	}

	beaconInterval := 500 * time.Millisecond
	if settings.BeaconInterval != "" {
		d, err := time.ParseDuration(settings.BeaconInterval)
		if err != nil {
			return fmt.Errorf("invalid --beacon-interval: %w", err)
		}
		beaconInterval = d
	}

	if dirtySettings {
		if err := settingsStore.Save(effective); err != nil {
			return err
		}
		log.Info().Str("settings_path", settingsStore.Path()).Msg("saved settings")
	}

	hub := ws.NewHub()

	engine := mledhost.NewEngine(mledhost.Config{
		BindIP:            effective.BindIP,
		Interface:         settings.Interface,
		MulticastGroup:    effective.MulticastGroup,
		MulticastPort:     effective.MulticastPort,
		MulticastTTL:      settings.MulticastTTL,
		DiscoveryInterval: time.Duration(effective.DiscoveryIntervalMS) * time.Millisecond,
		OfflineThreshold:  time.Duration(effective.OfflineThresholdS) * time.Second,
		BeaconInterval:    beaconInterval,
		WeakRSSIDbm:       -70,
	})

	engine.OnEvent(func(evt mledhost.Event) {
		switch evt.Type {
		case mledhost.EventNodeUpdate:
			if dto, ok := evt.Payload.(mledhost.NodeDTO); ok {
				log.Debug().
					Str("event", string(evt.Type)).
					Str("node_id", dto.NodeID).
					Str("name", dto.Name).
					Str("ip", dto.IP).
					Int("rssi", dto.RSSI).
					Str("status", string(dto.Status)).
					Msg("engine event")
			} else {
				log.Debug().Str("event", string(evt.Type)).Msg("engine event")
			}
			hub.Broadcast("node.update", evt.Payload)
		case mledhost.EventNodeOffline:
			if m, ok := evt.Payload.(map[string]any); ok {
				if nodeID, ok := m["node_id"].(string); ok {
					log.Info().Str("event", string(evt.Type)).Str("node_id", nodeID).Msg("engine event")
				} else {
					log.Info().Str("event", string(evt.Type)).Msg("engine event")
				}
			} else {
				log.Info().Str("event", string(evt.Type)).Msg("engine event")
			}
			hub.Broadcast("node.offline", evt.Payload)
		case mledhost.EventApplyAck:
			if m, ok := evt.Payload.(map[string]any); ok {
				nodeID, _ := m["node_id"].(string)
				success, _ := m["success"].(bool)
				log.Info().Str("event", string(evt.Type)).Str("node_id", nodeID).Bool("success", success).Msg("engine event")
			} else {
				log.Info().Str("event", string(evt.Type)).Msg("engine event")
			}
			hub.Broadcast("apply.ack", evt.Payload)
		case mledhost.EventError:
			log.Error().Err(evt.Error).Msg("engine error")
			hub.Broadcast("error", map[string]any{"message": evt.Error.Error()})
		}
	})

	log.Info().
		Str("bind_ip", effective.BindIP).
		Str("mcast_group", effective.MulticastGroup).
		Int("mcast_port", effective.MulticastPort).
		Int("mcast_ttl", settings.MulticastTTL).
		Dur("discovery_interval", time.Duration(effective.DiscoveryIntervalMS)*time.Millisecond).
		Dur("offline_threshold", time.Duration(effective.OfflineThresholdS)*time.Second).
		Dur("beacon_interval", beaconInterval).
		Msg("engine config")

	if err := engine.Start(ctx); err != nil {
		return err
	}
	defer engine.Stop()

	api := httpapi.NewServer(httpapi.ServerDeps{
		Engine:         engine,
		Presets:        presetStore,
		Settings:       settingsStore,
		WS:             hub,
		WeakRSSIDbm:    -70,
		Interface:      settings.Interface,
		MulticastTTL:   settings.MulticastTTL,
		BeaconInterval: beaconInterval,
	})

	mux := http.NewServeMux()
	api.Register(mux)
	mux.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("content-type", "text/plain; charset=utf-8")
		_, _ = w.Write([]byte("mled-server running (UI not built here). Use /api/* and /ws.\n"))
	})

	handler := httpapi.LogRequests(mux)

	srv := &http.Server{
		Addr:              settings.HTTPAddr,
		Handler:           handler,
		ReadHeaderTimeout: 5 * time.Second,
	}

	go func() {
		log.Info().Str("http_addr", settings.HTTPAddr).Msg("http listening")
		if err := srv.ListenAndServe(); err != nil && !errors.Is(err, http.ErrServerClosed) {
			log.Error().Err(err).Msg("http serve error")
		}
	}()

	stopCh := make(chan os.Signal, 2)
	signal.Notify(stopCh, syscall.SIGINT, syscall.SIGTERM)
	select {
	case <-ctx.Done():
	case <-stopCh:
	}

	log.Info().Msg("shutting down")
	shutdownCtx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()
	_ = srv.Shutdown(shutdownCtx)
	return nil
}
