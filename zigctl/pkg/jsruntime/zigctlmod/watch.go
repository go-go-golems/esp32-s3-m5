package zigctlmod

import (
	"context"
	"sync"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

type WatchStream struct {
	client   mqtt.Client
	qos      byte
	topics   []string
	msgCh    chan mqtt.Message
	doneCh   chan struct{}
	stopOnce sync.Once
	timer    *time.Timer
}

func newWatchStream(ctx context.Context, client mqtt.Client, qos byte, topics []string, duration time.Duration) (*WatchStream, error) {
	stream := &WatchStream{
		client: client,
		qos:    qos,
		topics: topics,
		msgCh:  make(chan mqtt.Message, 64),
		doneCh: make(chan struct{}),
	}

	handler := func(_ mqtt.Client, msg mqtt.Message) {
		select {
		case stream.msgCh <- msg:
		default:
		}
	}

	for _, topic := range topics {
		token := client.Subscribe(topic, qos, handler)
		if !token.WaitTimeout(5 * time.Second) {
			stream.Stop()
			return nil, context.DeadlineExceeded
		}
		if err := token.Error(); err != nil {
			stream.Stop()
			return nil, err
		}
	}

	if duration > 0 {
		stream.timer = time.AfterFunc(duration, stream.Stop)
	}

	if ctx.Err() != nil {
		stream.Stop()
		return nil, ctx.Err()
	}

	return stream, nil
}

func (s *WatchStream) Stop() {
	s.stopOnce.Do(func() {
		if s.timer != nil {
			s.timer.Stop()
		}
		if len(s.topics) > 0 {
			s.client.Unsubscribe(s.topics...)
		}
		close(s.doneCh)
		close(s.msgCh)
	})
}

func (s *WatchStream) Next(ctx context.Context) (map[string]any, error) {
	select {
	case <-ctx.Done():
		return nil, ctx.Err()
	case <-s.doneCh:
		return map[string]any{"done": true}, nil
	case msg, ok := <-s.msgCh:
		if !ok {
			return map[string]any{"done": true}, nil
		}
		return map[string]any{
			"done":  false,
			"value": map[string]any{"topic": msg.Topic(), "payload": decodePayload(msg.Payload())},
		}, nil
	}
}
