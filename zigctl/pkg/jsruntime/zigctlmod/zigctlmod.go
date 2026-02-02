package zigctlmod

import (
	"context"
	"encoding/json"
	"fmt"
	"time"

	"github.com/dop251/goja"
	"github.com/go-go-golems/go-go-goja/modules"
)

type Module struct{}

var _ modules.NativeModule = (*Module)(nil)

func (m *Module) Name() string {
	return "zigctl"
}

func (m *Module) Doc() string {
	return `
Zigctl module provides JS access to Zigbee2MQTT via zigctl.

Usage:
  const zigctl = require('zigctl');
  const client = zigctl.connect({ broker: 'mqtt://localhost:1884' });
  const info = client.bridgeInfo();

Functions:
  connect(config?): Connect to the MQTT broker and return a client object.

Client methods:
  bridgeInfo()
  devices()
  permitJoin({ seconds, device? })
  publish(topic, payload)
  request(topic, payload, responseTopic, timeout?)
  watch({ topics, duration }) -> stream (next(), stop())
  close()
`
}

func (m *Module) Loader(vm *goja.Runtime, moduleObj *goja.Object) {
	exports := moduleObj.Get("exports").(*goja.Object)
	_ = exports.Set("connect", func(cfg goja.Value) (*goja.Object, error) {
		settings, err := settingsFromValue(vm, cfg)
		if err != nil {
			return nil, err
		}
		client, err := newClient(context.Background(), settings)
		if err != nil {
			return nil, err
		}
		return buildClientObject(vm, client), nil
	})
}

func buildClientObject(vm *goja.Runtime, client *Client) *goja.Object {
	obj := vm.NewObject()

	_ = obj.Set("close", func() error {
		return client.Close()
	})
	_ = obj.Set("bridgeInfo", func() (map[string]any, error) {
		return client.BridgeInfo(context.Background())
	})
	_ = obj.Set("devices", func() ([]map[string]any, error) {
		return client.Devices(context.Background())
	})
	_ = obj.Set("permitJoin", func(opts goja.Value) (map[string]any, error) {
		seconds := 60
		device := ""
		if opts != nil && !goja.IsUndefined(opts) && !goja.IsNull(opts) {
			var raw map[string]any
			if err := vm.ExportTo(opts, &raw); err != nil {
				return nil, err
			}
			if value, ok := intFromMap(raw, "seconds", "time"); ok {
				seconds = value
			}
			if value, ok := stringFromMap(raw, "device"); ok {
				device = value
			}
		}
		return client.PermitJoin(context.Background(), seconds, device)
	})
	_ = obj.Set("publish", func(topic string, payload goja.Value) error {
		encoded, err := payloadFromValue(vm, payload)
		if err != nil {
			return err
		}
		return client.Publish(context.Background(), topic, encoded)
	})
	_ = obj.Set("request", func(topic string, payload goja.Value, responseTopic string, timeout goja.Value) (map[string]any, error) {
		encoded, err := payloadFromValue(vm, payload)
		if err != nil {
			return nil, err
		}
		timeoutOverride, err := durationFromValue(vm, timeout)
		if err != nil {
			return nil, err
		}
		return client.Request(context.Background(), topic, encoded, responseTopic, timeoutOverride)
	})
	_ = obj.Set("watch", func(opts goja.Value) (*goja.Object, error) {
		options, err := watchOptionsFromValue(vm, opts)
		if err != nil {
			return nil, err
		}
		stream, err := client.Watch(context.Background(), options)
		if err != nil {
			return nil, err
		}
		return buildStreamObject(vm, stream), nil
	})

	return obj
}

func buildStreamObject(vm *goja.Runtime, stream *WatchStream) *goja.Object {
	obj := vm.NewObject()
	_ = obj.Set("next", func() (map[string]any, error) {
		return stream.Next(context.Background())
	})
	_ = obj.Set("stop", func() {
		stream.Stop()
	})
	return obj
}

func payloadFromValue(vm *goja.Runtime, value goja.Value) ([]byte, error) {
	if value == nil || goja.IsUndefined(value) || goja.IsNull(value) {
		return []byte("null"), nil
	}
	if s, ok := value.Export().(string); ok {
		return []byte(s), nil
	}
	var raw any
	if err := vm.ExportTo(value, &raw); err != nil {
		return nil, err
	}
	return json.Marshal(raw)
}

func durationFromValue(vm *goja.Runtime, value goja.Value) (time.Duration, error) {
	if value == nil || goja.IsUndefined(value) || goja.IsNull(value) {
		return 0, nil
	}
	var raw any
	if err := vm.ExportTo(value, &raw); err != nil {
		return 0, err
	}
	switch typed := raw.(type) {
	case string:
		if typed == "" {
			return 0, nil
		}
		return time.ParseDuration(typed)
	case float64:
		return time.Duration(typed) * time.Second, nil
	case int:
		return time.Duration(typed) * time.Second, nil
	case int64:
		return time.Duration(typed) * time.Second, nil
	default:
		return 0, fmt.Errorf("unsupported timeout value: %T", raw)
	}
}

func init() {
	modules.Register(&Module{})
}
