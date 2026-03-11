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
const maxPayloadLogLen = 240

type Hub struct {
	log *log.Logger

	mu            sync.RWMutex
	nextHistoryID int64
	browsers      map[*websocket.Conn]string
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
		browsers: map[*websocket.Conn]string{},
		devices:  map[string]*DeviceState{},
	}
}

func (h *Hub) Snapshot() ServerSnapshot {
	h.mu.RLock()
	defer h.mu.RUnlock()
	return h.snapshotLocked("server_snapshot")
}

func (h *Hub) HandleBrowserConn(conn *websocket.Conn, remoteAddr string) {
	h.mu.Lock()
	h.browsers[conn] = remoteAddr
	initial := h.snapshotLocked("server_snapshot")
	browserCount := len(h.browsers)
	h.mu.Unlock()
	h.log.Printf("browser connected remote=%s browsers=%d devices=%d history=%d", remoteAddr, browserCount, len(initial.Devices), len(initial.History))

	if err := conn.WriteJSON(initial); err != nil {
		h.log.Printf("browser initial snapshot write failed remote=%s err=%v", remoteAddr, err)
		h.removeBrowser(conn)
		return
	}
	h.log.Printf("browser initial snapshot sent remote=%s devices=%d history=%d", remoteAddr, len(initial.Devices), len(initial.History))

	for {
		msgType, payload, err := conn.ReadMessage()
		if err != nil {
			h.log.Printf("browser read closed remote=%s err=%v", remoteAddr, err)
			h.removeBrowser(conn)
			return
		}
		h.log.Printf("browser unexpected message remote=%s type=%d payload=%q", remoteAddr, msgType, abbreviatePayload(payload))
	}
}

func (h *Hub) HandleDeviceConn(conn *websocket.Conn, remoteAddr string) {
	deviceID := ""
	h.log.Printf("device connected remote=%s", remoteAddr)
	for {
		msgType, payload, err := conn.ReadMessage()
		if err != nil {
			h.log.Printf("device read closed remote=%s device_id=%s err=%v", remoteAddr, deviceID, err)
			h.disconnectDevice(deviceID)
			return
		}
		h.log.Printf("device frame remote=%s type=%d payload=%q", remoteAddr, msgType, abbreviatePayload(payload))

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
		h.log.Printf("device identified remote=%s device_id=%s msg_type=%s seq=%d", remoteAddr, deviceID, base.Type, base.Seq)

		h.recordDeviceMessage(deviceID, remoteAddr, payload, base)
	}
}

func abbreviatePayload(payload []byte) string {
	if len(payload) <= maxPayloadLogLen {
		return string(payload)
	}
	return string(payload[:maxPayloadLogLen]) + "..."
}

func (h *Hub) removeBrowser(conn *websocket.Conn) {
	h.mu.Lock()
	remoteAddr := h.browsers[conn]
	delete(h.browsers, conn)
	remaining := len(h.browsers)
	h.mu.Unlock()
	h.log.Printf("browser disconnected remote=%s browsers=%d", remoteAddr, remaining)
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
	h.log.Printf("device disconnected device_id=%s connected_devices=%d", deviceID, len(status.Devices))
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
	browserCount := len(h.browsers)
	lastSeq := device.LastSeq
	lastType := device.LastType
	position := device.Position
	lastDelta := device.LastDelta
	rxCount := device.RxCount
	h.mu.Unlock()
	h.log.Printf("device state updated device_id=%s seq=%d type=%s pos=%d delta=%d rx_count=%d browsers=%d",
		deviceID,
		lastSeq,
		lastType,
		position,
		lastDelta,
		rxCount,
		browserCount,
	)

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

	msgType := "unknown"
	switch msg := message.(type) {
	case map[string]any:
		if t, ok := msg["type"].(string); ok {
			msgType = t
		}
	case ServerSnapshot:
		msgType = msg.Type
	}
	h.log.Printf("broadcast start type=%s browsers=%d", msgType, len(conns))

	for _, conn := range conns {
		if err := conn.WriteJSON(message); err != nil {
			h.mu.Lock()
			remoteAddr := h.browsers[conn]
			delete(h.browsers, conn)
			h.mu.Unlock()
			h.log.Printf("broadcast failed type=%s remote=%s err=%v", msgType, remoteAddr, err)
			continue
		}
		h.mu.RLock()
		remoteAddr := h.browsers[conn]
		h.mu.RUnlock()
		h.log.Printf("broadcast ok type=%s remote=%s", msgType, remoteAddr)
	}
}
