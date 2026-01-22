package mledhost

import (
	"encoding/hex"
	"errors"
	"fmt"
	"strconv"
	"strings"

	"mled-server/internal/mledproto"
	"mled-server/internal/storage"
)

// BuildWirePattern converts the UI PatternConfig into the fixed-size MLED/1
// PatternConfig payload (20 bytes).
//
// The current UI supports the MLED/1 wire types directly:
// - off, rainbow, chase, breathing, sparkle
//
// For backward compatibility with older presets:
// - solid -> breathing (min=max brightness)
// - pulse -> breathing (min=0, max=brightness)
// - gradient -> not supported (returns error)
func BuildWirePattern(cfg storage.PatternConfig) (mledproto.PatternConfig, error) {
	var p mledproto.PatternConfig

	brightness := clampU8(cfg.Brightness, 0, 100)

	switch strings.ToLower(cfg.Type) {
	case "off":
		p.PatternType = mledproto.PatternOff
		p.BrightnessPct = brightness
		return p, nil

	case "rainbow":
		p.PatternType = mledproto.PatternRainbow
		p.BrightnessPct = brightness
		// data[0]=speed(0..20), data[1]=saturation(0..100), data[2]=spread_x10(1..50)
		speed := getIntParam(cfg.Params, "speed", 10)
		if speed > 20 {
			// Accept older 0..100 style inputs by scaling down.
			speed = (speed * 20) / 100
		}
		saturation := getIntParam(cfg.Params, "saturation", 100)
		spread := getIntParam(cfg.Params, "spread_x10", 10)
		p.Data[0] = clampU8(speed, 0, 20)
		p.Data[1] = clampU8(saturation, 0, 100)
		p.Data[2] = clampU8(spread, 1, 50)
		return p, nil

	case "chase":
		p.PatternType = mledproto.PatternChase
		p.BrightnessPct = brightness
		// data[0]=speed, data[1]=tail_len, data[2]=gap_len, data[3]=trains
		// data[4..6]=fg RGB, data[7..9]=bg RGB, data[10]=dir, data[11]=fade_tail
		speed := getIntParam(cfg.Params, "speed", 30)
		tailLen := getIntParam(cfg.Params, "tail_len", 5)
		gapLen := getIntParam(cfg.Params, "gap_len", 10)
		trains := getIntParam(cfg.Params, "trains", 1)
		fgR, fgG, fgB, err := getHexColor(cfg.Params, "fg_color", "#FFFFFF")
		if err != nil {
			return p, err
		}
		bgR, bgG, bgB, err := getHexColor(cfg.Params, "bg_color", "#000000")
		if err != nil {
			return p, err
		}
		dir := getEnumParam(cfg.Params, "direction", map[string]uint8{
			"forward": 0,
			"reverse": 1,
			"bounce":  2,
		}, 0)
		fadeTail := uint8(0)
		if getBoolParam(cfg.Params, "fade_tail", true) {
			fadeTail = 1
		}
		p.Data[0] = clampU8(speed, 0, 255)
		p.Data[1] = clampU8(tailLen, 1, 255)
		p.Data[2] = clampU8(gapLen, 0, 255)
		p.Data[3] = clampU8(trains, 1, 255)
		p.Data[4], p.Data[5], p.Data[6] = fgR, fgG, fgB
		p.Data[7], p.Data[8], p.Data[9] = bgR, bgG, bgB
		p.Data[10] = dir
		p.Data[11] = fadeTail
		return p, nil

	case "breathing":
		p.PatternType = mledproto.PatternBreathing
		p.BrightnessPct = brightness
		// data[0]=speed, data[1..3]=rgb, data[4]=min_bri, data[5]=max_bri, data[6]=curve
		speed := getIntParam(cfg.Params, "speed", 6)
		if speed > 20 {
			speed = (speed * 20) / 100
		}
		r, g, b, err := getHexColor(cfg.Params, "color", "#FFFFFF")
		if err != nil {
			return p, err
		}
		minBri := getIntParam(cfg.Params, "min_bri", 10)
		maxBri := getIntParam(cfg.Params, "max_bri", int(brightness))
		curve := getEnumParam(cfg.Params, "curve", map[string]uint8{
			"sine":   0,
			"linear": 1,
			"ease":   2,
		}, 0)
		p.Data[0] = clampU8(speed, 0, 20)
		p.Data[1], p.Data[2], p.Data[3] = r, g, b
		p.Data[4] = clampU8(minBri, 0, 255)
		p.Data[5] = clampU8(maxBri, 0, 255)
		p.Data[6] = curve
		return p, nil

	case "sparkle":
		p.PatternType = mledproto.PatternSparkle
		p.BrightnessPct = brightness
		// data[0]=speed, data[1..3]=color, data[4]=density_pct, data[5]=fade_speed
		// data[6]=color_mode, data[7..9]=bg RGB
		speed := getIntParam(cfg.Params, "speed", 10)
		if speed > 20 {
			speed = (speed * 20) / 100
		}
		r, g, b, err := getHexColor(cfg.Params, "color", "#FFFFFF")
		if err != nil {
			return p, err
		}
		density := getIntParam(cfg.Params, "density_pct", 30)
		fadeSpeed := getIntParam(cfg.Params, "fade_speed", 50)
		mode := getEnumParam(cfg.Params, "color_mode", map[string]uint8{
			"fixed":   0,
			"random":  1,
			"rainbow": 2,
		}, 0)
		bgR, bgG, bgB, err := getHexColor(cfg.Params, "bg_color", "#000000")
		if err != nil {
			return p, err
		}
		p.Data[0] = clampU8(speed, 0, 20)
		p.Data[1], p.Data[2], p.Data[3] = r, g, b
		p.Data[4] = clampU8(density, 0, 100)
		p.Data[5] = clampU8(fadeSpeed, 1, 255)
		p.Data[6] = mode
		p.Data[7], p.Data[8], p.Data[9] = bgR, bgG, bgB
		return p, nil

	case "solid":
		p.PatternType = mledproto.PatternBreathing
		p.BrightnessPct = brightness
		r, g, b, err := getHexColor(cfg.Params, "color", "#FFFFFF")
		if err != nil {
			return p, err
		}
		// BREATHING mapping from protocol notes:
		// data[0]=speed, data[1..3]=rgb, data[4]=min_bri, data[5]=max_bri, data[6]=curve
		p.Data[0] = 0
		p.Data[1], p.Data[2], p.Data[3] = r, g, b
		p.Data[4] = brightness
		p.Data[5] = brightness
		p.Data[6] = 1 // LINEAR
		return p, nil

	case "pulse":
		p.PatternType = mledproto.PatternBreathing
		p.BrightnessPct = brightness
		r, g, b, err := getHexColor(cfg.Params, "color", "#FFFFFF")
		if err != nil {
			return p, err
		}
		speed := getIntParam(cfg.Params, "speed", 50)
		p.Data[0] = uint8((speed * 20) / 100)
		p.Data[1], p.Data[2], p.Data[3] = r, g, b
		p.Data[4] = 0
		p.Data[5] = brightness
		p.Data[6] = 0 // SINE
		return p, nil

	case "gradient":
		return p, errors.New("pattern type 'gradient' is not supported by the current MLED/1 wire patterns")

	default:
		return p, fmt.Errorf("unknown pattern type: %q", cfg.Type)
	}
}

func clampU8(v int, lo int, hi int) uint8 {
	if v < lo {
		return uint8(lo)
	}
	if v > hi {
		return uint8(hi)
	}
	return uint8(v)
}

func getIntParam(m map[string]any, key string, def int) int {
	if m == nil {
		return def
	}
	v, ok := m[key]
	if !ok {
		return def
	}
	switch t := v.(type) {
	case float64:
		return int(t)
	case int:
		return t
	case string:
		if n, err := strconv.Atoi(t); err == nil {
			return n
		}
	}
	return def
}

func getBoolParam(m map[string]any, key string, def bool) bool {
	if m == nil {
		return def
	}
	v, ok := m[key]
	if !ok {
		return def
	}
	switch t := v.(type) {
	case bool:
		return t
	case string:
		switch strings.ToLower(strings.TrimSpace(t)) {
		case "true", "1", "yes", "on":
			return true
		case "false", "0", "no", "off":
			return false
		}
	case float64:
		return t != 0
	case int:
		return t != 0
	}
	return def
}

func getEnumParam(m map[string]any, key string, lookup map[string]uint8, def uint8) uint8 {
	if m == nil {
		return def
	}
	v, ok := m[key]
	if !ok {
		return def
	}
	switch t := v.(type) {
	case string:
		if out, ok := lookup[strings.ToLower(strings.TrimSpace(t))]; ok {
			return out
		}
	case float64:
		return uint8(t)
	case int:
		return uint8(t)
	}
	return def
}

func getHexColor(m map[string]any, key string, def string) (uint8, uint8, uint8, error) {
	if m == nil {
		return parseHexColor(def)
	}
	v, ok := m[key]
	if !ok {
		return parseHexColor(def)
	}
	s, ok := v.(string)
	if !ok || s == "" {
		return parseHexColor(def)
	}
	return parseHexColor(s)
}

func parseHexColor(s string) (uint8, uint8, uint8, error) {
	s = strings.TrimSpace(s)
	s = strings.TrimPrefix(s, "#")
	if len(s) != 6 {
		return 0, 0, 0, fmt.Errorf("invalid hex color: %q", s)
	}
	b, err := hex.DecodeString(s)
	if err != nil || len(b) != 3 {
		return 0, 0, 0, fmt.Errorf("invalid hex color: %q", s)
	}
	return b[0], b[1], b[2], nil
}

