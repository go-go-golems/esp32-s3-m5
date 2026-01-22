package mledhost

import (
	"fmt"
	"net"
	"strings"
	"time"

	"mled-server/internal/mledproto"
)

type NodeStatus string

const (
	NodeOnline  NodeStatus = "online"
	NodeWeak    NodeStatus = "weak"
	NodeOffline NodeStatus = "offline"
)

type NodeRecord struct {
	NodeID uint32

	Addr     net.UDPAddr
	LastSeen time.Time

	Pong mledproto.Pong

	Offline bool
}

type NodeDTO struct {
	NodeID         string         `json:"node_id"`
	Name           string         `json:"name"`
	IP             string         `json:"ip"`
	Port           int            `json:"port"`
	RSSI           int            `json:"rssi"`
	UptimeMS       uint32         `json:"uptime_ms"`
	LastSeen       int64          `json:"last_seen"`
	CurrentPattern map[string]any `json:"current_pattern"`
	Status         NodeStatus     `json:"status"`
}

func (n NodeRecord) ToDTO(now time.Time, weakRSSIDbm int, offlineAfter time.Duration) NodeDTO {
	name := strings.TrimRight(string(n.Pong.Name[:]), "\x00")
	if name == "" {
		name = fmt.Sprintf("%08X", n.NodeID)
	}

	status := deriveStatus(now, n.LastSeen, int(n.Pong.RSSIDbm), weakRSSIDbm, offlineAfter)

	var currentPattern map[string]any
	switch n.Pong.PatternType {
	case mledproto.PatternOff:
		currentPattern = map[string]any{"type": "off", "brightness": int(n.Pong.BrightnessPct), "params": map[string]any{}}
	case mledproto.PatternRainbow:
		currentPattern = map[string]any{"type": "rainbow", "brightness": int(n.Pong.BrightnessPct), "params": map[string]any{}}
	case mledproto.PatternChase:
		currentPattern = map[string]any{"type": "chase", "brightness": int(n.Pong.BrightnessPct), "params": map[string]any{}}
	case mledproto.PatternBreathing:
		currentPattern = map[string]any{"type": "breathing", "brightness": int(n.Pong.BrightnessPct), "params": map[string]any{}}
	case mledproto.PatternSparkle:
		currentPattern = map[string]any{"type": "sparkle", "brightness": int(n.Pong.BrightnessPct), "params": map[string]any{}}
	default:
		// Unknown/unsupported mapping; keep minimal.
		currentPattern = nil
	}

	return NodeDTO{
		NodeID:         fmt.Sprintf("%08X", n.NodeID),
		Name:           name,
		IP:             n.Addr.IP.String(),
		Port:           n.Addr.Port,
		RSSI:           int(n.Pong.RSSIDbm),
		UptimeMS:       n.Pong.UptimeMS,
		LastSeen:       n.LastSeen.UnixMilli(),
		CurrentPattern: currentPattern,
		Status:         status,
	}
}

func deriveStatus(now, lastSeen time.Time, rssi, weakRSSIDbm int, offlineAfter time.Duration) NodeStatus {
	age := now.Sub(lastSeen)
	if age >= offlineAfter {
		return NodeOffline
	}
	if age >= 5*time.Second {
		return NodeWeak
	}
	if rssi < weakRSSIDbm {
		return NodeWeak
	}
	return NodeOnline
}
