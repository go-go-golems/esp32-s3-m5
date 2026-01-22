package storage

import (
	"encoding/json"
	"errors"
	"os"
	"sync"
)

type PresetStore struct {
	path string

	mu      sync.RWMutex
	loaded  bool
	presets []Preset
}

func NewPresetStore(path string) *PresetStore {
	return &PresetStore{path: path}
}

func (s *PresetStore) Path() string { return s.path }

func (s *PresetStore) Load() ([]Preset, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	b, err := os.ReadFile(s.path)
	if err != nil {
		if errors.Is(err, os.ErrNotExist) {
			s.loaded = true
			s.presets = nil
			return nil, nil
		}
		return nil, err
	}
	var out []Preset
	if err := json.Unmarshal(b, &out); err != nil {
		return nil, err
	}
	s.loaded = true
	s.presets = out
	return append([]Preset(nil), out...), nil
}

func (s *PresetStore) List() ([]Preset, error) {
	if err := s.ensureLoaded(); err != nil {
		return nil, err
	}
	s.mu.RLock()
	defer s.mu.RUnlock()
	return append([]Preset(nil), s.presets...), nil
}

func (s *PresetStore) Put(p Preset) error {
	if err := s.ensureLoaded(); err != nil {
		return err
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	for i := range s.presets {
		if s.presets[i].ID == p.ID {
			s.presets[i] = p
			return s.flushLocked()
		}
	}
	s.presets = append(s.presets, p)
	return s.flushLocked()
}

func (s *PresetStore) Delete(id string) error {
	if err := s.ensureLoaded(); err != nil {
		return err
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	w := 0
	for _, p := range s.presets {
		if p.ID == id {
			continue
		}
		s.presets[w] = p
		w++
	}
	s.presets = s.presets[:w]
	return s.flushLocked()
}

func (s *PresetStore) ensureLoaded() error {
	s.mu.RLock()
	loaded := s.loaded
	s.mu.RUnlock()
	if loaded {
		return nil
	}
	_, err := s.Load()
	return err
}

func (s *PresetStore) flushLocked() error {
	b, err := json.MarshalIndent(s.presets, "", "  ")
	if err != nil {
		return err
	}
	return os.WriteFile(s.path, b, 0o644)
}
