package zigctlmod

import (
	"context"
	"encoding/json"
	"fmt"
	"strings"
	"sync"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
	"github.com/go-go-golems/zigctl/pkg/zigbee"
)

type Client struct {
	settings zigbee.Settings
	client   mqtt.Client
	timeout  time.Duration
	qos      byte
	mu       sync.Mutex
	closed   bool
}

func newClient(ctx context.Context, settings zigbee.Settings) (*Client, error) {
	client, timeout, qos, err := zigbee.Connect(ctx, settings)
	if err != nil {
		return nil, err
	}

	return &Client{
		settings: settings,
		client:   client,
		timeout:  timeout,
		qos:      qos,
	}, nil
}

func (c *Client) Close() error {
	c.mu.Lock()
	defer c.mu.Unlock()
	if c.closed {
		return nil
	}
	c.closed = true
	c.client.Disconnect(250)
	return nil
}

func (c *Client) BridgeInfo(ctx context.Context) (map[string]any, error) {
	base := c.settings.BasePrefix()
	responseTopic := zigbee.JoinTopic(base, "bridge", "info")
	payload, err := zigbee.RequestOnce(
		ctx,
		c.client,
		c.qos,
		zigbee.JoinTopic(base, "bridge", "request", "info"),
		[]byte("{}"),
		responseTopic,
		c.timeout,
	)
	if err != nil {
		return nil, err
	}

	return map[string]any{
		"topic":   responseTopic,
		"payload": decodePayload(payload),
	}, nil
}

func (c *Client) Devices(ctx context.Context) ([]map[string]any, error) {
	base := c.settings.BasePrefix()
	responseTopic := zigbee.JoinTopic(base, "bridge", "devices")
	payload, err := zigbee.RequestOnce(
		ctx,
		c.client,
		c.qos,
		zigbee.JoinTopic(base, "bridge", "request", "devices"),
		[]byte("{}"),
		responseTopic,
		c.timeout,
	)
	if err != nil {
		return nil, err
	}

	var devices []map[string]any
	if err := json.Unmarshal(payload, &devices); err != nil {
		return nil, fmt.Errorf("decode devices: %w", err)
	}
	return devices, nil
}

func (c *Client) PermitJoin(ctx context.Context, seconds int, device string) (map[string]any, error) {
	base := c.settings.BasePrefix()
	payload := map[string]any{
		"value": true,
		"time":  seconds,
	}
	if device != "" {
		payload["device"] = device
	}
	encoded, err := json.Marshal(payload)
	if err != nil {
		return nil, err
	}

	responseTopic := zigbee.JoinTopic(base, "bridge", "response", "permit_join")
	resp, err := zigbee.RequestOnce(
		ctx,
		c.client,
		c.qos,
		zigbee.JoinTopic(base, "bridge", "request", "permit_join"),
		encoded,
		responseTopic,
		c.timeout,
	)
	if err != nil {
		return nil, err
	}

	return map[string]any{
		"topic":   responseTopic,
		"payload": decodePayload(resp),
	}, nil
}

func (c *Client) Publish(ctx context.Context, topic string, payload []byte) error {
	return zigbee.Publish(ctx, c.client, c.ensureTopic(topic), c.qos, payload, c.timeout)
}

func (c *Client) Request(ctx context.Context, topic string, payload []byte, responseTopic string, timeoutOverride time.Duration) (map[string]any, error) {
	timeout := c.timeout
	if timeoutOverride > 0 {
		timeout = timeoutOverride
	}
	response := c.ensureTopic(responseTopic)
	resp, err := zigbee.RequestOnce(
		ctx,
		c.client,
		c.qos,
		c.ensureTopic(topic),
		payload,
		response,
		timeout,
	)
	if err != nil {
		return nil, err
	}

	return map[string]any{
		"topic":   response,
		"payload": decodePayload(resp),
	}, nil
}

func (c *Client) Watch(ctx context.Context, options watchOptions) (*WatchStream, error) {
	duration := time.Duration(0)
	if options.Duration != "" {
		parsed, err := time.ParseDuration(options.Duration)
		if err != nil {
			return nil, err
		}
		duration = parsed
	}

	base := c.settings.BasePrefix()
	topics := make([]string, 0, len(options.Topics))
	for _, topic := range options.Topics {
		topics = append(topics, ensureTopic(base, topic))
	}

	return newWatchStream(ctx, c.client, c.qos, topics, duration)
}

func (c *Client) ensureTopic(topic string) string {
	return ensureTopic(c.settings.BasePrefix(), topic)
}

func ensureTopic(base, topic string) string {
	if topic == "" {
		return base
	}
	if base == "" {
		return topic
	}
	if topic == base || strings.HasPrefix(topic, base+"/") {
		return topic
	}
	return zigbee.JoinTopic(base, topic)
}

func decodePayload(payload []byte) any {
	if len(payload) == 0 {
		return nil
	}
	var decoded any
	if err := json.Unmarshal(payload, &decoded); err != nil {
		return string(payload)
	}
	return decoded
}
