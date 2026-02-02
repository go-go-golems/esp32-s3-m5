package zigctlmod

import (
	"fmt"

	"github.com/dop251/goja"
	"github.com/go-go-golems/zigctl/pkg/zigbee"
)

type watchOptions struct {
	Topics   []string
	Duration string
}

func settingsFromValue(vm *goja.Runtime, value goja.Value) (zigbee.Settings, error) {
	defaults := zigbee.DefaultConfig()
	settings := zigbee.Settings{
		Broker:    defaults.Broker,
		BaseTopic: defaults.BaseTopic,
		TLS:       defaults.TLS,
		CAFile:    defaults.CAFile,
		CertFile:  defaults.CertFile,
		KeyFile:   defaults.KeyFile,
		QOS:       defaults.QOS,
		Timeout:   defaults.Timeout,
	}

	if value == nil || goja.IsUndefined(value) || goja.IsNull(value) {
		return settings, nil
	}

	var raw map[string]any
	if err := vm.ExportTo(value, &raw); err != nil {
		return settings, err
	}

	if v, ok := stringFromMap(raw, "broker"); ok {
		settings.Broker = v
	}
	if v, ok := stringFromMap(raw, "baseTopic", "base_topic", "base-topic"); ok {
		settings.BaseTopic = v
	}
	if v, ok := boolFromMap(raw, "tls"); ok {
		settings.TLS = v
	}
	if v, ok := stringFromMap(raw, "cafile", "caFile", "ca_file"); ok {
		settings.CAFile = v
	}
	if v, ok := stringFromMap(raw, "cert", "certFile", "cert_file"); ok {
		settings.CertFile = v
	}
	if v, ok := stringFromMap(raw, "key", "keyFile", "key_file"); ok {
		settings.KeyFile = v
	}
	if v, ok := intFromMap(raw, "qos"); ok {
		settings.QOS = v
	}
	if v, ok := stringFromMap(raw, "timeout"); ok {
		settings.Timeout = v
	} else if v, ok := intFromMap(raw, "timeoutSeconds", "timeout_seconds"); ok {
		settings.Timeout = fmt.Sprintf("%ds", v)
	}

	return settings, nil
}

func watchOptionsFromValue(vm *goja.Runtime, value goja.Value) (watchOptions, error) {
	options := watchOptions{
		Topics:   []string{"bridge/event"},
		Duration: "",
	}
	if value == nil || goja.IsUndefined(value) || goja.IsNull(value) {
		return options, nil
	}

	var raw map[string]any
	if err := vm.ExportTo(value, &raw); err != nil {
		return options, err
	}

	if topics, ok := raw["topics"]; ok {
		if list, ok := topics.([]any); ok {
			options.Topics = []string{}
			for _, item := range list {
				if s, ok := item.(string); ok {
					options.Topics = append(options.Topics, s)
				}
			}
		}
	}
	if topic, ok := stringFromMap(raw, "topic"); ok {
		options.Topics = []string{topic}
	}
	if duration, ok := stringFromMap(raw, "duration"); ok {
		options.Duration = duration
	} else if seconds, ok := intFromMap(raw, "seconds", "durationSeconds", "duration_seconds"); ok {
		options.Duration = fmt.Sprintf("%ds", seconds)
	}

	return options, nil
}

func stringFromMap(raw map[string]any, keys ...string) (string, bool) {
	for _, key := range keys {
		if val, ok := raw[key]; ok {
			switch typed := val.(type) {
			case string:
				return typed, true
			default:
				return fmt.Sprint(typed), true
			}
		}
	}
	return "", false
}

func intFromMap(raw map[string]any, keys ...string) (int, bool) {
	for _, key := range keys {
		if val, ok := raw[key]; ok {
			switch typed := val.(type) {
			case int:
				return typed, true
			case int64:
				return int(typed), true
			case float64:
				return int(typed), true
			case float32:
				return int(typed), true
			case string:
				var parsed int
				if _, err := fmt.Sscanf(typed, "%d", &parsed); err == nil {
					return parsed, true
				}
			}
		}
	}
	return 0, false
}

func boolFromMap(raw map[string]any, keys ...string) (bool, bool) {
	for _, key := range keys {
		if val, ok := raw[key]; ok {
			switch typed := val.(type) {
			case bool:
				return typed, true
			case string:
				if typed == "true" {
					return true, true
				}
				if typed == "false" {
					return false, true
				}
			}
		}
	}
	return false, false
}
