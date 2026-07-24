package sensors

import (
	"context"
	"testing"
	"time"
)

func TestHubLatestWinsAndSequences(t *testing.T) {
	hub := NewHub(time.Hour)
	sub := hub.Subscribe()
	defer sub.Close()
	for i := uint64(1); i <= 3; i++ {
		hub.Publish(Sample{Version: 1, Type: "sensor.sample", Sequence: i})
	}
	select {
	case got := <-sub.C:
		if got.Sequence != 3 {
			t.Fatalf("latest sequence = %d, want 3", got.Sequence)
		}
	case <-time.After(time.Second):
		t.Fatal("timed out waiting for sample")
	}
	clients, dropped := hub.Stats()
	if clients != 1 || dropped != 2 {
		t.Fatalf("stats clients=%d dropped=%d, want 1/2", clients, dropped)
	}
}

func TestHubRunStopsWithContext(t *testing.T) {
	hub := NewHub(time.Millisecond)
	ctx, cancel := context.WithCancel(context.Background())
	done := make(chan error, 1)
	go func() { done <- hub.Run(ctx) }()
	time.Sleep(5 * time.Millisecond)
	cancel()
	if err := <-done; err != context.Canceled {
		t.Fatalf("Run error = %v, want context.Canceled", err)
	}
	if hub.Snapshot().Sequence == 0 {
		t.Fatal("producer emitted no samples")
	}
}
