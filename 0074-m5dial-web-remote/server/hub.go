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
	browsers      map[*websocket.Conn]*BrowserSocket
	devices       map[string]*DeviceState
	deviceSockets map[string]*DeviceSocket
	history       []HistoryEntry
}

type BrowserSocket struct {
	conn       *websocket.Conn
	remoteAddr string
	mu         sync.Mutex
}

type DeviceSocket struct {
	conn       *websocket.Conn
	remoteAddr string
	deviceID   string
	mu         sync.Mutex
}

type DeviceState struct {
	DeviceID          string    `json:"device_id"`
	Connected         bool      `json:"connected"`
	RemoteAddr        string    `json:"remote_addr"`
	LastType          string    `json:"last_type"`
	LastButton        string    `json:"last_button,omitempty"`
	LastSeq           uint32    `json:"last_seq"`
	Position          int32     `json:"position"`
	LastDelta         int32     `json:"last_delta"`
	LastText          string    `json:"last_text,omitempty"`
	LastCommand       string    `json:"last_command,omitempty"`
	LastCommandStatus string    `json:"last_command_status,omitempty"`
	LastRequestID     uint32    `json:"last_request_id,omitempty"`
	LastSeen          time.Time `json:"last_seen"`
	RxCount           uint32    `json:"rx_count"`
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
	Type      string `json:"type"`
	DeviceID  string `json:"device_id"`
	Seq       uint32 `json:"seq"`
	RequestID uint32 `json:"request_id,omitempty"`
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

type UiCommandMessage struct {
	Type      string `json:"type"`
	DeviceID  string `json:"device_id"`
	Command   string `json:"command"`
	RequestID uint32 `json:"request_id"`
	Text      string `json:"text,omitempty"`
	Value     int32  `json:"value,omitempty"`
}

type UiCommandAckMessage struct {
	BaseMessage
	Command  string `json:"command"`
	Status   string `json:"status"`
	Position int32  `json:"pos"`
	Text     string `json:"text,omitempty"`
}

type UiCommandResultMessage struct {
	Type        string    `json:"type"`
	GeneratedAt time.Time `json:"generated_at"`
	DeviceID    string    `json:"device_id"`
	Command     string    `json:"command"`
	RequestID   uint32    `json:"request_id"`
	Status      string    `json:"status"`
	Reason      string    `json:"reason,omitempty"`
}

func NewHub(logger *log.Logger) *Hub {
	return &Hub{
		log:           logger,
		browsers:      map[*websocket.Conn]*BrowserSocket{},
		devices:       map[string]*DeviceState{},
		deviceSockets: map[string]*DeviceSocket{},
	}
}

func (h *Hub) Snapshot() ServerSnapshot {
	h.mu.RLock()
	defer h.mu.RUnlock()
	return h.snapshotLocked("server_snapshot")
}

func (b *BrowserSocket) writeJSON(message any) error {
	b.mu.Lock()
	defer b.mu.Unlock()
	return b.conn.WriteJSON(message)
}

func (d *DeviceSocket) writeText(payload []byte) error {
	d.mu.Lock()
	defer d.mu.Unlock()
	return d.conn.WriteMessage(websocket.TextMessage, payload)
}

func (h *Hub) HandleBrowserConn(conn *websocket.Conn, remoteAddr string) {
	browser := &BrowserSocket{
		conn:       conn,
		remoteAddr: remoteAddr,
	}

	h.mu.Lock()
	h.browsers[conn] = browser
	initial := h.snapshotLocked("server_snapshot")
	browserCount := len(h.browsers)
	h.mu.Unlock()
	h.log.Printf("browser connected remote=%s browsers=%d devices=%d history=%d", remoteAddr, browserCount, len(initial.Devices), len(initial.History))

	if err := browser.writeJSON(initial); err != nil {
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

		h.log.Printf("browser frame remote=%s type=%d payload=%q", remoteAddr, msgType, abbreviatePayload(payload))
		h.handleBrowserMessage(browser, msgType, payload)
	}
}

func (h *Hub) HandleDeviceConn(conn *websocket.Conn, remoteAddr string) {
	deviceSocket := &DeviceSocket{
		conn:       conn,
		remoteAddr: remoteAddr,
	}
	deviceID := ""

	h.log.Printf("device connected remote=%s", remoteAddr)
	for {
		msgType, payload, err := conn.ReadMessage()
		if err != nil {
			h.log.Printf("device read closed remote=%s device_id=%s err=%v", remoteAddr, deviceID, err)
			h.disconnectDevice(deviceID, conn)
			return
		}

		h.log.Printf("device frame remote=%s type=%d payload=%q", remoteAddr, msgType, abbreviatePayload(payload))
		if msgType != websocket.TextMessage {
			h.log.Printf("ignoring non-text device frame remote=%s type=%d", remoteAddr, msgType)
			continue
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
		h.log.Printf("device identified remote=%s device_id=%s msg_type=%s seq=%d request_id=%d", remoteAddr, deviceID, base.Type, base.Seq, base.RequestID)

		if deviceSocket.deviceID != deviceID {
			deviceSocket.deviceID = deviceID
			h.registerDeviceSocket(deviceID, deviceSocket)
		}

		h.recordDeviceMessage(deviceID, remoteAddr, payload, base)
	}
}

func abbreviatePayload(payload []byte) string {
	if len(payload) <= maxPayloadLogLen {
		return string(payload)
	}
	return string(payload[:maxPayloadLogLen]) + "..."
}

func (h *Hub) handleBrowserMessage(browser *BrowserSocket, msgType int, payload []byte) {
	if msgType != websocket.TextMessage {
		h.log.Printf("rejecting non-text browser frame remote=%s type=%d", browser.remoteAddr, msgType)
		h.sendCommandResult(browser, UiCommandResultMessage{
			Type:        "ui_command_result",
			GeneratedAt: time.Now(),
			Status:      "rejected",
			Reason:      "non-text websocket frames are unsupported",
		})
		return
	}

	var msg UiCommandMessage
	if err := json.Unmarshal(payload, &msg); err != nil {
		h.log.Printf("invalid browser json remote=%s err=%v", browser.remoteAddr, err)
		h.sendCommandResult(browser, UiCommandResultMessage{
			Type:        "ui_command_result",
			GeneratedAt: time.Now(),
			Status:      "rejected",
			Reason:      "invalid json payload",
		})
		return
	}

	if msg.Type != "ui_command" {
		h.log.Printf("unexpected browser message remote=%s type=%q", browser.remoteAddr, msg.Type)
		h.sendCommandResult(browser, UiCommandResultMessage{
			Type:        "ui_command_result",
			GeneratedAt: time.Now(),
			DeviceID:    msg.DeviceID,
			Command:     msg.Command,
			RequestID:   msg.RequestID,
			Status:      "rejected",
			Reason:      "only ui_command messages are accepted from browsers",
		})
		return
	}

	if msg.DeviceID == "" || msg.Command == "" {
		h.log.Printf("rejecting browser command remote=%s missing device_id or command payload=%q", browser.remoteAddr, abbreviatePayload(payload))
		h.sendCommandResult(browser, UiCommandResultMessage{
			Type:        "ui_command_result",
			GeneratedAt: time.Now(),
			DeviceID:    msg.DeviceID,
			Command:     msg.Command,
			RequestID:   msg.RequestID,
			Status:      "rejected",
			Reason:      "device_id and command are required",
		})
		return
	}

	if err := h.routeCommandToDevice(browser, payload, msg); err != nil {
		h.log.Printf("browser command route failed remote=%s device_id=%s command=%s request_id=%d err=%v",
			browser.remoteAddr,
			msg.DeviceID,
			msg.Command,
			msg.RequestID,
			err,
		)
		h.sendCommandResult(browser, UiCommandResultMessage{
			Type:        "ui_command_result",
			GeneratedAt: time.Now(),
			DeviceID:    msg.DeviceID,
			Command:     msg.Command,
			RequestID:   msg.RequestID,
			Status:      "rejected",
			Reason:      err.Error(),
		})
		return
	}

	h.sendCommandResult(browser, UiCommandResultMessage{
		Type:        "ui_command_result",
		GeneratedAt: time.Now(),
		DeviceID:    msg.DeviceID,
		Command:     msg.Command,
		RequestID:   msg.RequestID,
		Status:      "queued",
	})
}

func (h *Hub) routeCommandToDevice(browser *BrowserSocket, payload []byte, msg UiCommandMessage) error {
	h.mu.RLock()
	deviceSocket := h.deviceSockets[msg.DeviceID]
	h.mu.RUnlock()

	if deviceSocket == nil {
		return fmt.Errorf("device %s is not connected", msg.DeviceID)
	}

	if err := deviceSocket.writeText(payload); err != nil {
		h.unregisterDeviceSocket(msg.DeviceID, deviceSocket.conn)
		return fmt.Errorf("device write failed: %w", err)
	}

	h.log.Printf("browser command forwarded remote=%s device_id=%s command=%s request_id=%d payload=%q",
		browser.remoteAddr,
		msg.DeviceID,
		msg.Command,
		msg.RequestID,
		abbreviatePayload(payload),
	)
	return nil
}

func (h *Hub) sendCommandResult(browser *BrowserSocket, result UiCommandResultMessage) {
	if err := browser.writeJSON(result); err != nil {
		h.log.Printf("browser command result write failed remote=%s request_id=%d err=%v", browser.remoteAddr, result.RequestID, err)
		h.removeBrowser(browser.conn)
		return
	}

	h.log.Printf("browser command result remote=%s device_id=%s command=%s request_id=%d status=%s reason=%q",
		browser.remoteAddr,
		result.DeviceID,
		result.Command,
		result.RequestID,
		result.Status,
		result.Reason,
	)
}

func (h *Hub) removeBrowser(conn *websocket.Conn) {
	h.mu.Lock()
	browser := h.browsers[conn]
	delete(h.browsers, conn)
	remaining := len(h.browsers)
	h.mu.Unlock()

	if browser == nil {
		h.log.Printf("browser disconnected remote=%s browsers=%d", "", remaining)
		return
	}
	h.log.Printf("browser disconnected remote=%s browsers=%d", browser.remoteAddr, remaining)
}

func (h *Hub) registerDeviceSocket(deviceID string, deviceSocket *DeviceSocket) {
	h.mu.Lock()
	previous := h.deviceSockets[deviceID]
	h.deviceSockets[deviceID] = deviceSocket

	device := h.devices[deviceID]
	if device == nil {
		device = &DeviceState{DeviceID: deviceID}
		h.devices[deviceID] = device
	}
	device.Connected = true
	device.RemoteAddr = deviceSocket.remoteAddr
	device.LastSeen = time.Now()
	h.mu.Unlock()

	if previous != nil && previous.conn != deviceSocket.conn {
		h.log.Printf("device socket replaced device_id=%s old_remote=%s new_remote=%s", deviceID, previous.remoteAddr, deviceSocket.remoteAddr)
		return
	}

	h.log.Printf("device socket registered device_id=%s remote=%s", deviceID, deviceSocket.remoteAddr)
}

func (h *Hub) unregisterDeviceSocket(deviceID string, conn *websocket.Conn) {
	h.mu.Lock()
	current := h.deviceSockets[deviceID]
	if current == nil || current.conn != conn {
		h.mu.Unlock()
		return
	}
	delete(h.deviceSockets, deviceID)
	h.mu.Unlock()
}

func (h *Hub) disconnectDevice(deviceID string, conn *websocket.Conn) {
	if deviceID == "" {
		return
	}

	h.mu.Lock()
	current := h.deviceSockets[deviceID]
	if current != nil && current.conn != conn {
		h.mu.Unlock()
		h.log.Printf("ignoring stale device disconnect device_id=%s remote=%s", deviceID, current.remoteAddr)
		return
	}
	delete(h.deviceSockets, deviceID)

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
	case "ui_command_ack":
		var msg UiCommandAckMessage
		if err := json.Unmarshal(payload, &msg); err == nil {
			device.Position = msg.Position
			device.LastText = msg.Text
			device.LastCommand = msg.Command
			device.LastCommandStatus = msg.Status
			device.LastRequestID = msg.RequestID
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
	lastCommand := device.LastCommand
	lastCommandStatus := device.LastCommandStatus
	lastRequestID := device.LastRequestID
	h.mu.Unlock()

	h.log.Printf("device state updated device_id=%s seq=%d type=%s pos=%d delta=%d rx_count=%d last_command=%s command_status=%s request_id=%d browsers=%d",
		deviceID,
		lastSeq,
		lastType,
		position,
		lastDelta,
		rxCount,
		lastCommand,
		lastCommandStatus,
		lastRequestID,
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
	browsers := make([]*BrowserSocket, 0, len(h.browsers))
	for _, browser := range h.browsers {
		browsers = append(browsers, browser)
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
	h.log.Printf("broadcast start type=%s browsers=%d", msgType, len(browsers))

	for _, browser := range browsers {
		if err := browser.writeJSON(message); err != nil {
			h.removeBrowser(browser.conn)
			h.log.Printf("broadcast failed type=%s remote=%s err=%v", msgType, browser.remoteAddr, err)
			continue
		}
		h.log.Printf("broadcast ok type=%s remote=%s", msgType, browser.remoteAddr)
	}
}
