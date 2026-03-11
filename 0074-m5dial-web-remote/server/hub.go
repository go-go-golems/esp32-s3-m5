package main

import (
	"encoding/json"
	"fmt"
	"log"
	"sort"
	"sync"
	"time"

	"github.com/gorilla/websocket"
)

const maxHistory = 200

type Hub struct {
	log *log.Logger

	mu            sync.RWMutex
	nextHistoryID int64
	browsers      map[*websocket.Conn]struct{}
	devices       map[string]*DeviceState
	history       []HistoryEntry
}

type DeviceState struct {
	DeviceID   string    `json:"device_id"`
	Connected  bool      `json:"connected"`
	RemoteAddr string    `json:"remote_addr"`
	LastType   string    `json:"last_type"`
	LastButton string    `json:"last_button,omitempty"`
	LastSeq    uint32    `json:"last_seq"`
	Position   int32     `json:"position"`
	LastDelta  int32     `json:"last_delta"`
	LastSeen   time.Time `json:"last_seen"`
	RxCount    uint32    `json:"rx_count"`
}

type HistoryEntry struct {
	ID         int64     `json:"id"`
	ReceivedAt time.Time `json:"received_at"`
	DeviceID   string    `json:"device_id"`
	Type       string    `json:"type"`
	Raw        string    `json:"raw"`
}

type ServerSnapshot struct {
	Type        string         `json:"type"`
	GeneratedAt time.Time      `json:"generated_at"`
	Devices     []DeviceState  `json:"devices"`
	History     []HistoryEntry `json:"history"`
}

type BaseMessage struct {
	Type     string `json:"type"`
	DeviceID string `json:"device_id"`
	Seq      uint32 `json:"seq"`
}

type EncoderMessage struct {
	BaseMessage
	Position int32 `json:"pos"`
	Delta    int32 `json:"delta"`
}

type ButtonMessage struct {
	BaseMessage
	Kind     string `json:"kind"`
	Position int32  `json:"pos"`
}

func NewHub(logger *log.Logger) *Hub {
	return &Hub{
		log:      logger,
		browsers: map[*websocket.Conn]struct{}{},
		devices:  map[string]*DeviceState{},
	}
}

func (h *Hub) Snapshot() ServerSnapshot {
	h.mu.RLock()
	defer h.mu.RUnlock()
	return h.snapshotLocked("server_snapshot")
}

func (h *Hub) HandleBrowserConn(conn *websocket.Conn) {
	h.mu.Lock()
	h.browsers[conn] = struct{}{}
	initial := h.snapshotLocked("server_snapshot")
	h.mu.Unlock()

	if err := conn.WriteJSON(initial); err != nil {
		h.removeBrowser(conn)
		return
	}

	for {
		if _, _, err := conn.ReadMessage(); err != nil {
			h.removeBrowser(conn)
			return
		}
	}
}

func (h *Hub) HandleDeviceConn(conn *websocket.Conn, remoteAddr string) {
	deviceID := ""
	for {
		_, payload, err := conn.ReadMessage()
		if err != nil {
			h.disconnectDevice(deviceID)
			return
		}

		var base BaseMessage
		if err := json.Unmarshal(payload, &base); err != nil {
			h.log.Printf("invalid device json from %s: %v", remoteAddr, err)
			continue
		}

		if base.DeviceID != "" {
			deviceID = base.DeviceID
		}
		if deviceID == "" {
			deviceID = fmt.Sprintf("anon-%d", time.Now().UnixNano())
		}

		h.recordDeviceMessage(deviceID, remoteAddr, payload, base)
	}
}

func (h *Hub) removeBrowser(conn *websocket.Conn) {
	h.mu.Lock()
	delete(h.browsers, conn)
	h.mu.Unlock()
}

func (h *Hub) disconnectDevice(deviceID string) {
	if deviceID == "" {
		return
	}
	h.mu.Lock()
	device, ok := h.devices[deviceID]
	if ok {
		device.Connected = false
		device.LastSeen = time.Now()
	}
	status := h.snapshotLocked("server_status")
	h.mu.Unlock()
	h.broadcast(status)
}

func (h *Hub) recordDeviceMessage(deviceID string, remoteAddr string, payload []byte, base BaseMessage) {
	h.mu.Lock()
	device := h.devices[deviceID]
	if device == nil {
		device = &DeviceState{DeviceID: deviceID}
		h.devices[deviceID] = device
	}

	now := time.Now()
	device.Connected = true
	device.RemoteAddr = remoteAddr
	device.LastType = base.Type
	device.LastSeq = base.Seq
	device.LastSeen = now
	device.RxCount++

	switch base.Type {
	case "encoder":
		var msg EncoderMessage
		if err := json.Unmarshal(payload, &msg); err == nil {
			device.Position = msg.Position
			device.LastDelta = msg.Delta
		}
	case "button":
		var msg ButtonMessage
		if err := json.Unmarshal(payload, &msg); err == nil {
			device.LastButton = msg.Kind
			device.Position = msg.Position
		}
	}

	h.nextHistoryID++
	entry := HistoryEntry{
		ID:         h.nextHistoryID,
		ReceivedAt: now,
		DeviceID:   deviceID,
		Type:       base.Type,
		Raw:        string(payload),
	}
	h.history = append(h.history, entry)
	if len(h.history) > maxHistory {
		h.history = h.history[len(h.history)-maxHistory:]
	}

	status := h.snapshotLocked("server_status")
	event := map[string]any{
		"type":      "device_event",
		"device_id": deviceID,
		"payload":   json.RawMessage(payload),
	}
	h.mu.Unlock()

	h.broadcast(event)
	h.broadcast(status)
}

func (h *Hub) snapshotLocked(eventType string) ServerSnapshot {
	devices := make([]DeviceState, 0, len(h.devices))
	for _, device := range h.devices {
		devices = append(devices, *device)
	}
	sort.Slice(devices, func(i, j int) bool {
		return devices[i].DeviceID < devices[j].DeviceID
	})

	history := append([]HistoryEntry(nil), h.history...)
	return ServerSnapshot{
		Type:        eventType,
		GeneratedAt: time.Now(),
		Devices:     devices,
		History:     history,
	}
}

func (h *Hub) broadcast(message any) {
	h.mu.RLock()
	conns := make([]*websocket.Conn, 0, len(h.browsers))
	for conn := range h.browsers {
		conns = append(conns, conn)
	}
	h.mu.RUnlock()

	for _, conn := range conns {
		if err := conn.WriteJSON(message); err != nil {
			h.mu.Lock()
			delete(h.browsers, conn)
			h.mu.Unlock()
		}
	}
}
