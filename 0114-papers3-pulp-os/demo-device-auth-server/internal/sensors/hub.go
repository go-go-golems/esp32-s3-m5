package sensors

import (
	"context"
	"math"
	"sync"
	"time"
)

// Sample is the bounded versioned payload sent to devices.
type Sample struct {
	Version     int     `json:"v"`
	Type        string  `json:"type"`
	Sequence    uint64  `json:"seq"`
	TimestampMS int64   `json:"ts_ms"`
	Temperature float64 `json:"temp_c"`
	Humidity    float64 `json:"humidity_pct"`
	Pressure    float64 `json:"pressure_hpa"`
}

type Subscription struct {
	C       <-chan Sample
	closeFn func()
}

func (s Subscription) Close() {
	if s.closeFn != nil {
		s.closeFn()
	}
}

type subscriber struct {
	ch      chan Sample
	dropped uint64
}

// Hub owns one deterministic fake-sensor producer and latest-wins subscribers.
type Hub struct {
	interval time.Duration
	now      func() time.Time

	mu          sync.RWMutex
	latest      Sample
	nextID      uint64
	subscribers map[uint64]*subscriber
	dropped     uint64
}

func NewHub(interval time.Duration) *Hub {
	if interval <= 0 {
		interval = 500 * time.Millisecond
	}
	return &Hub{interval: interval, now: time.Now, subscribers: make(map[uint64]*subscriber)}
}

func (h *Hub) Run(ctx context.Context) error {
	ticker := time.NewTicker(h.interval)
	defer ticker.Stop()
	for {
		select {
		case <-ctx.Done():
			return ctx.Err()
		case now := <-ticker.C:
			h.Publish(h.makeSample(now))
		}
	}
}

func (h *Hub) makeSample(now time.Time) Sample {
	h.mu.RLock()
	seq := h.latest.Sequence + 1
	h.mu.RUnlock()
	x := float64(seq)
	return Sample{
		Version: 1, Type: "sensor.sample", Sequence: seq, TimestampMS: now.UnixMilli(),
		Temperature: round2(22 + 2.5*math.Sin(x/24) + 0.08*math.Sin(x*1.71)),
		Humidity:    round2(48 + 7*math.Sin(x/37+1.2) + 0.2*math.Sin(x*0.93)),
		Pressure:    round2(1013 + 3*math.Sin(x/90)),
	}
}

func round2(v float64) float64 { return math.Round(v*100) / 100 }

func (h *Hub) Publish(sample Sample) {
	h.mu.Lock()
	defer h.mu.Unlock()
	h.latest = sample
	for _, sub := range h.subscribers {
		select {
		case sub.ch <- sample:
		default:
			// Keep only the most recent sample. Draining is non-blocking because
			// this channel has capacity one and Publish holds the only send path.
			select {
			case <-sub.ch:
			default:
			}
			select {
			case sub.ch <- sample:
			default:
			}
			sub.dropped++
			h.dropped++
		}
	}
}

func (h *Hub) Snapshot() Sample {
	h.mu.RLock()
	defer h.mu.RUnlock()
	return h.latest
}

func (h *Hub) Subscribe() Subscription {
	h.mu.Lock()
	defer h.mu.Unlock()
	h.nextID++
	id := h.nextID
	sub := &subscriber{ch: make(chan Sample, 1)}
	h.subscribers[id] = sub
	if h.latest.Sequence != 0 {
		sub.ch <- h.latest
	}
	var once sync.Once
	return Subscription{C: sub.ch, closeFn: func() {
		once.Do(func() {
			h.mu.Lock()
			delete(h.subscribers, id)
			close(sub.ch)
			h.mu.Unlock()
		})
	}}
}

func (h *Hub) Stats() (clients int, dropped uint64) {
	h.mu.RLock()
	defer h.mu.RUnlock()
	return len(h.subscribers), h.dropped
}
