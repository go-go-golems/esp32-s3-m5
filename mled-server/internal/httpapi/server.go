package httpapi

import (
	"context"
	"encoding/json"
	"errors"
	"io"
	"net/http"
	"strings"
	"time"

	"github.com/rs/zerolog/log"
	"nhooyr.io/websocket"

	"mled-server/internal/mledhost"
	"mled-server/internal/storage"
	"mled-server/internal/ws"
)

type ServerDeps struct {
	Engine   *mledhost.Engine
	Presets  *storage.PresetStore
	Settings *storage.SettingsStore
	WS       *ws.Hub

	WeakRSSIDbm    int
	Interface      string
	MulticastTTL   int
	BeaconInterval time.Duration
}

type Server struct {
	deps     ServerDeps
	settings storage.Settings

	sse *sseHub
}

func NewServer(deps ServerDeps) *Server {
	s := &Server{
		deps:     deps,
		settings: deps.Settings.Default(),
		sse:      newSSEHub(),
	}
	if loaded, err := deps.Settings.Load(); err == nil {
		s.settings = loaded
		log.Info().
			Str("settings_path", deps.Settings.Path()).
			Msg("loaded settings")
	} else {
		log.Info().
			Str("settings_path", deps.Settings.Path()).
			Msg("using default settings")
	}
	if deps.WS != nil && deps.Engine != nil {
		deps.WS.OnRefresh(func() { _ = deps.Engine.SendPing() })
	}
	if deps.Engine != nil {
		deps.Engine.OnEvent(s.onEngineEvent)
	}
	return s
}

func (s *Server) Register(mux *http.ServeMux) {
	mux.HandleFunc("GET /api/nodes", s.handleGetNodes)
	mux.HandleFunc("GET /api/presets", s.handleGetPresets)
	mux.HandleFunc("POST /api/presets", s.handlePostPreset)
	mux.HandleFunc("PUT /api/presets/", s.handlePutPreset)
	mux.HandleFunc("DELETE /api/presets/", s.handleDeletePreset)
	mux.HandleFunc("POST /api/apply", s.handleApply)
	mux.HandleFunc("GET /api/settings", s.handleGetSettings)
	mux.HandleFunc("PUT /api/settings", s.handlePutSettings)
	mux.HandleFunc("GET /api/status", s.handleStatus)
	mux.HandleFunc("GET /api/events", s.handleEvents)
	mux.HandleFunc("GET /ws", s.handleWS)
}

func (s *Server) CurrentSettings() storage.Settings {
	return s.settings
}

func (s *Server) handleGetNodes(w http.ResponseWriter, r *http.Request) {
	writeJSON(w, http.StatusOK, s.deps.Engine.NodesSnapshot())
}

func (s *Server) handleGetPresets(w http.ResponseWriter, r *http.Request) {
	ps, err := s.deps.Presets.List()
	if err != nil {
		writeError(w, http.StatusInternalServerError, err)
		return
	}
	writeJSON(w, http.StatusOK, ps)
}

func (s *Server) handlePostPreset(w http.ResponseWriter, r *http.Request) {
	var p storage.Preset
	if err := decodeJSON(r.Body, &p); err != nil {
		writeError(w, http.StatusBadRequest, err)
		return
	}
	if p.ID == "" {
		p.ID = newID()
	}
	log.Info().
		Str("preset_id", p.ID).
		Str("preset_name", p.Name).
		Str("pattern_type", p.Config.Type).
		Int("brightness", p.Config.Brightness).
		Msg("preset create")
	if err := s.deps.Presets.Put(p); err != nil {
		writeError(w, http.StatusInternalServerError, err)
		return
	}
	writeJSON(w, http.StatusOK, p)
}

func (s *Server) handlePutPreset(w http.ResponseWriter, r *http.Request) {
	id := strings.TrimPrefix(r.URL.Path, "/api/presets/")
	id = strings.Trim(id, "/")
	if id == "" {
		writeError(w, http.StatusBadRequest, errors.New("missing preset id"))
		return
	}
	var p storage.Preset
	if err := decodeJSON(r.Body, &p); err != nil {
		writeError(w, http.StatusBadRequest, err)
		return
	}
	p.ID = id
	log.Info().
		Str("preset_id", p.ID).
		Str("preset_name", p.Name).
		Str("pattern_type", p.Config.Type).
		Int("brightness", p.Config.Brightness).
		Msg("preset update")
	if err := s.deps.Presets.Put(p); err != nil {
		writeError(w, http.StatusInternalServerError, err)
		return
	}
	writeJSON(w, http.StatusOK, p)
}

func (s *Server) handleDeletePreset(w http.ResponseWriter, r *http.Request) {
	id := strings.TrimPrefix(r.URL.Path, "/api/presets/")
	id = strings.Trim(id, "/")
	if id == "" {
		writeError(w, http.StatusBadRequest, errors.New("missing preset id"))
		return
	}
	log.Info().
		Str("preset_id", id).
		Msg("preset delete")
	if err := s.deps.Presets.Delete(id); err != nil {
		writeError(w, http.StatusInternalServerError, err)
		return
	}
	writeJSON(w, http.StatusOK, map[string]any{"ok": true})
}

type applyRequest struct {
	NodeIDs any                   `json:"node_ids"`
	Config  storage.PatternConfig `json:"config"`
}

func (s *Server) handleApply(w http.ResponseWriter, r *http.Request) {
	var req applyRequest
	if err := decodeJSON(r.Body, &req); err != nil {
		writeError(w, http.StatusBadRequest, err)
		return
	}

	var all bool
	var nodeIDs []string
	switch v := req.NodeIDs.(type) {
	case string:
		if v != "all" {
			writeError(w, http.StatusBadRequest, errors.New("node_ids must be 'all' or string[]"))
			return
		}
		all = true
	case []any:
		for _, item := range v {
			s, ok := item.(string)
			if !ok {
				writeError(w, http.StatusBadRequest, errors.New("node_ids must be 'all' or string[]"))
				return
			}
			nodeIDs = append(nodeIDs, s)
		}
	default:
		writeError(w, http.StatusBadRequest, errors.New("node_ids must be 'all' or string[]"))
		return
	}

	log.Info().
		Bool("all", all).
		Int("node_count", len(nodeIDs)).
		Str("pattern_type", req.Config.Type).
		Int("brightness", req.Config.Brightness).
		Msg("apply request")

	wirePattern, err := mledhost.BuildWirePattern(req.Config)
	if err != nil {
		writeError(w, http.StatusBadRequest, err)
		return
	}

	res, err := s.deps.Engine.Apply(mledhost.ApplyOptions{
		NodeIDsHex: nodeIDs,
		All:        all,
		Pattern:    wirePattern,
	})
	if err != nil {
		writeError(w, http.StatusInternalServerError, err)
		return
	}
	log.Info().
		Int("sent_to", len(res.SentTo)).
		Int("failed", len(res.Failed)).
		Msg("apply result")
	writeJSON(w, http.StatusOK, res)
}

func (s *Server) handleGetSettings(w http.ResponseWriter, r *http.Request) {
	writeJSON(w, http.StatusOK, s.settings)
}

func (s *Server) handlePutSettings(w http.ResponseWriter, r *http.Request) {
	var next storage.Settings
	if err := decodeJSON(r.Body, &next); err != nil {
		writeError(w, http.StatusBadRequest, err)
		return
	}
	prev := s.settings
	s.settings = next
	if err := s.deps.Settings.Save(next); err != nil {
		writeError(w, http.StatusInternalServerError, err)
		return
	}

	log.Info().
		Str("bind_ip_prev", prev.BindIP).
		Str("bind_ip_next", next.BindIP).
		Str("mcast_group_prev", prev.MulticastGroup).
		Str("mcast_group_next", next.MulticastGroup).
		Int("mcast_port_prev", prev.MulticastPort).
		Int("mcast_port_next", next.MulticastPort).
		Int("discovery_interval_ms_prev", prev.DiscoveryIntervalMS).
		Int("discovery_interval_ms_next", next.DiscoveryIntervalMS).
		Int("offline_threshold_s_prev", prev.OfflineThresholdS).
		Int("offline_threshold_s_next", next.OfflineThresholdS).
		Msg("settings updated; restarting engine")

	if err := s.deps.Engine.Restart(context.Background(), mledhost.Config{
		BindIP:            next.BindIP,
		Interface:         s.deps.Interface,
		MulticastGroup:    next.MulticastGroup,
		MulticastPort:     next.MulticastPort,
		MulticastTTL:      s.deps.MulticastTTL,
		DiscoveryInterval: time.Duration(next.DiscoveryIntervalMS) * time.Millisecond,
		OfflineThreshold:  time.Duration(next.OfflineThresholdS) * time.Second,
		BeaconInterval:    s.deps.BeaconInterval,
		WeakRSSIDbm:       s.deps.WeakRSSIDbm,
	}); err != nil {
		writeError(w, http.StatusInternalServerError, err)
		return
	}

	writeJSON(w, http.StatusOK, next)
}

func (s *Server) handleStatus(w http.ResponseWriter, r *http.Request) {
	writeJSON(w, http.StatusOK, map[string]any{
		"epoch_id": s.deps.Engine.EpochID(),
		"show_ms":  s.deps.Engine.ShowMS(),
		"running":  true,
	})
}

func (s *Server) handleWS(w http.ResponseWriter, r *http.Request) {
	log.Info().Str("remote_addr", r.RemoteAddr).Msg("ws connect")
	c, err := websocket.Accept(w, r, &websocket.AcceptOptions{
		InsecureSkipVerify: true,
	})
	if err != nil {
		return
	}
	defer c.Close(websocket.StatusNormalClosure, "")
	s.deps.WS.ServeWS(c)
	log.Info().Str("remote_addr", r.RemoteAddr).Msg("ws disconnect")
}

func decodeJSON(body io.ReadCloser, v any) error {
	defer body.Close()
	b, err := io.ReadAll(io.LimitReader(body, 1<<20))
	if err != nil {
		return err
	}
	dec := json.NewDecoder(strings.NewReader(string(b)))
	dec.DisallowUnknownFields()
	return dec.Decode(v)
}

func writeJSON(w http.ResponseWriter, status int, v any) {
	w.Header().Set("content-type", "application/json; charset=utf-8")
	w.WriteHeader(status)
	_ = json.NewEncoder(w).Encode(v)
}

func writeError(w http.ResponseWriter, status int, err error) {
	writeJSON(w, status, map[string]any{"error": err.Error()})
}
