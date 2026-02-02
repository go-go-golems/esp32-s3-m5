package nrf

import (
	"encoding/hex"
	"fmt"
	"regexp"
	"strconv"
	"strings"
)

var packetRegex = regexp.MustCompile(`received:\s+([0-9a-fA-F]+)\s+power:\s+(-?\d+)\s+lqi:\s+(\d+)\s+time:\s+(-?\d+)`)

func ParseLine(line []byte) (Packet, error) {
	text := strings.TrimSpace(string(line))
	matches := packetRegex.FindStringSubmatch(text)
	if matches == nil {
		return Packet{}, fmt.Errorf("line did not match packet format")
	}

	hexPayload := matches[1]
	if len(hexPayload) < 4 {
		return Packet{}, fmt.Errorf("payload too short for FCS stripping")
	}

	payload, err := hex.DecodeString(hexPayload[:len(hexPayload)-4])
	if err != nil {
		return Packet{}, fmt.Errorf("decode payload: %w", err)
	}

	rssi, err := strconv.Atoi(matches[2])
	if err != nil {
		return Packet{}, fmt.Errorf("parse rssi: %w", err)
	}

	lqi, err := strconv.Atoi(matches[3])
	if err != nil {
		return Packet{}, fmt.Errorf("parse lqi: %w", err)
	}

	timestamp, err := strconv.ParseInt(matches[4], 10, 64)
	if err != nil {
		return Packet{}, fmt.Errorf("parse timestamp: %w", err)
	}

	return Packet{
		Payload:         payload,
		RSSI:            rssi,
		LQI:             lqi,
		TimestampMicros: timestamp,
		RawLine:         text,
	}, nil
}
