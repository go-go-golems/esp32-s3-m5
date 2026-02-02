package zigbee

import (
	"context"
	"fmt"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

func JoinTopic(base string, parts ...string) string {
	if base == "" {
		base = "zigbee2mqtt"
	}
	topic := base
	for _, part := range parts {
		if part == "" {
			continue
		}
		if topic == "" {
			topic = part
			continue
		}
		topic = topic + "/" + part
	}
	return topic
}

func Publish(ctx context.Context, client mqtt.Client, topic string, qos byte, payload []byte, timeout time.Duration) error {
	token := client.Publish(topic, qos, false, payload)
	if !token.WaitTimeout(timeout) {
		return fmt.Errorf("publish timeout after %s", timeout)
	}
	if err := token.Error(); err != nil {
		return err
	}
	return ctx.Err()
}

func RequestOnce(
	ctx context.Context,
	client mqtt.Client,
	qos byte,
	requestTopic string,
	payload []byte,
	responseTopic string,
	timeout time.Duration,
) ([]byte, error) {
	ch := make(chan mqtt.Message, 1)
	callback := func(_ mqtt.Client, msg mqtt.Message) {
		select {
		case ch <- msg:
		default:
		}
	}

	subToken := client.Subscribe(responseTopic, qos, callback)
	if !subToken.WaitTimeout(timeout) {
		return nil, fmt.Errorf("subscribe timeout after %s", timeout)
	}
	if err := subToken.Error(); err != nil {
		return nil, err
	}
	defer client.Unsubscribe(responseTopic)

	if err := Publish(ctx, client, requestTopic, qos, payload, timeout); err != nil {
		return nil, err
	}

	select {
	case msg := <-ch:
		return msg.Payload(), nil
	case <-ctx.Done():
		return nil, ctx.Err()
	case <-time.After(timeout):
		return nil, fmt.Errorf("response timeout after %s", timeout)
	}
}
