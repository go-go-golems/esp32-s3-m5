---
Title: Imported source note - esp32 knob web
Ticket: ESP-28-M5DIAL-WEB-REMOTE
Status: active
Topics:
    - esp32-s3
    - esp32s3
    - firmware
    - http
    - m5stack
    - ui
    - webserver
    - websocket
    - wifi
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Imported external note describing an event-pipeline design using device ingress, normalization, and a single browser WebSocket stream.
LastUpdated: 2026-03-11T22:55:00-04:00
WhatFor: Source material imported into the ticket before analysis.
WhenToUse: Read when comparing the original external-server proposal with the self-contained firmware-hosted design in the ticket.
---

Yes — I’d structure it as **device ingress → Watermill bus → projector/aggregator → one browser WebSocket stream**.

That is a good fit for Watermill because it gives you a router plus middleware, and it keeps the app code mostly independent from the concrete Pub/Sub backend. Watermill’s own docs frame it as a common Publisher/Subscriber abstraction across brokers like GoChannel, NATS Jetstream, RabbitMQ, Redis Stream, SQL, and SQLite, and its delivery model is at-least-once, so handlers should be idempotent. ([Watermill][1])

For the WebSocket server in Go, I’d use `github.com/coder/websocket`: it is actively maintained, has first-class `context.Context` support, and the `golang.org/x/net/websocket` docs explicitly point people toward more actively maintained alternatives such as `coder/websocket` and Gorilla. ([GitHub][2])

## Shape of the server

```text
ESP32 #1 ---\
ESP32 #2 ----> /device/ws  --->  Watermill topic: device.ingress  --->  normalize/project  --->  browser.mux  --->  /ws  ---> frontend bus/store
ESP32 #3 ---/                                |                              |
                                             └---- presence/device state ---┘
```

I would keep exactly three logical layers:

1. **Ingress layer**
   Accept device connections and turn incoming frames into Watermill messages.

2. **Event pipeline layer**
   Normalize raw device messages, enrich with metadata, maybe resolve room/session mappings, and project them into browser-facing envelopes.

3. **Bridge layer**
   Subscribe once to the browser-outbound topic and fan that out to all connected browser clients over one WebSocket per browser tab.

The nice thing is that the frontend then only needs one multiplexed socket, and it can redistribute messages internally by `channel`, `room`, `view`, or `device_id`.

## Topic model

For a first pass, I would **not** create one Watermill topic per device instance. I’d use a small number of semantic topics and keep `device_id` in metadata/payload.

```go
const (
    TopicDeviceIngress = "device.ingress" // raw-ish device events
    TopicUIIntent      = "ui.intent"      // normalized app intents
    TopicBrowserMux    = "browser.mux"    // everything sent to browsers
    TopicDeviceRender  = "device.render"  // optional: updates back to ESP32 display
    TopicPresence      = "device.presence"
)
```

That keeps routing simple. If later you want separate input topics like `device.encoder`, `device.button`, and `device.touch`, Watermill has a `FanIn` component specifically for merging source topics into one target topic, and `FanOut` for multiplying one subscription to multiple internal consumers. ([Watermill][3])

## The concrete server sketch

Here’s the skeleton I’d start from.

```go
package main

import (
	"context"
	"encoding/json"
	"log"
	"net/http"
	"sync"
	"time"

	"github.com/ThreeDotsLabs/watermill"
	"github.com/ThreeDotsLabs/watermill/message"
	"github.com/ThreeDotsLabs/watermill/message/router/middleware"
	"github.com/ThreeDotsLabs/watermill/pubsub/gochannel"
	"github.com/coder/websocket"
	"github.com/coder/websocket/wsjson"
	"github.com/google/uuid"
)

const (
	TopicDeviceIngress = "device.ingress"
	TopicUIIntent      = "ui.intent"
	TopicBrowserMux    = "browser.mux"
	TopicDeviceRender  = "device.render"
	TopicPresence      = "device.presence"
)

type DeviceEvent struct {
	DeviceID  string         `json:"device_id"`
	RoomID    string         `json:"room_id,omitempty"`
	Kind      string         `json:"kind"` // rotate, press, release, touch, ...
	Delta     int            `json:"delta,omitempty"`
	Value     any            `json:"value,omitempty"`
	Timestamp time.Time      `json:"timestamp"`
	Meta      map[string]any `json:"meta,omitempty"`
}

type UIIntent struct {
	RoomID    string         `json:"room_id"`
	DeviceID  string         `json:"device_id"`
	Action    string         `json:"action"` // focus.next, slider.adjust, menu.open...
	Args      map[string]any `json:"args,omitempty"`
	Timestamp time.Time      `json:"timestamp"`
}

type BrowserEnvelope struct {
	Channel   string         `json:"channel"`   // ui, device, system
	Topic     string         `json:"topic"`     // ui.patch, ui.intent, ...
	RoomID    string         `json:"room_id"`   // frontend can demux by room/view
	DeviceID  string         `json:"device_id"` // optional
	Seq       uint64         `json:"seq"`
	Timestamp time.Time      `json:"timestamp"`
	Payload   map[string]any `json:"payload"`
}

func main() {
	logger := watermill.NewStdLogger(false, false)

	// In-process bus for v1.
	pubSub := gochannel.NewGoChannel(gochannel.Config{
		OutputChannelBuffer:            256,
		Persistent:                     false,
		BlockPublishUntilSubscriberAck: false,
	}, logger)

	router, err := message.NewRouter(message.RouterConfig{}, logger)
	if err != nil {
		log.Fatal(err)
	}

	router.AddMiddleware(
		middleware.Recoverer,
		middleware.CorrelationID,
		middleware.Retry{
			MaxRetries:      3,
			InitialInterval: 100 * time.Millisecond,
			Logger:          logger,
		}.Middleware,
	)

	router.AddHandler(
		"normalize-device-events",
		TopicDeviceIngress,
		pubSub,
		TopicUIIntent,
		pubSub,
		normalizeDeviceEvent,
	)

	router.AddHandler(
		"project-ui-intents-to-browser",
		TopicUIIntent,
		pubSub,
		TopicBrowserMux,
		pubSub,
		projectBrowserEnvelope,
	)

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	go func() {
		if err := router.Run(ctx); err != nil {
			log.Printf("router stopped: %v", err)
		}
	}()

	hub := NewBrowserHub()

	go func() {
		if err := hub.Run(ctx, pubSub, TopicBrowserMux); err != nil {
			log.Printf("browser hub stopped: %v", err)
		}
	}()

	http.HandleFunc("/device/ws", makeDeviceIngressHandler(pubSub))
	http.HandleFunc("/ws", hub.HandleWS)

	log.Println("listening on :8080")
	log.Fatal(http.ListenAndServe(":8080", nil))
}

func normalizeDeviceEvent(msg *message.Message) ([]*message.Message, error) {
	var ev DeviceEvent
	if err := json.Unmarshal(msg.Payload, &ev); err != nil {
		return nil, err
	}

	intent := UIIntent{
		RoomID:    ev.RoomID,
		DeviceID:  ev.DeviceID,
		Timestamp: time.Now(),
		Args:      map[string]any{},
	}

	switch ev.Kind {
	case "rotate":
		intent.Action = "focus.adjust"
		intent.Args["delta"] = ev.Delta
	case "press":
		intent.Action = "activate.current"
	case "long_press":
		intent.Action = "open.palette"
	default:
		intent.Action = "device.unknown"
		intent.Args["kind"] = ev.Kind
	}

	payload, err := json.Marshal(intent)
	if err != nil {
		return nil, err
	}

	out := message.NewMessage(uuid.NewString(), payload)
	out.Metadata.Set("device_id", ev.DeviceID)
	out.Metadata.Set("room_id", ev.RoomID)

	return []*message.Message{out}, nil
}

var seqMu sync.Mutex
var seq uint64

func nextSeq() uint64 {
	seqMu.Lock()
	defer seqMu.Unlock()
	seq++
	return seq
}

func projectBrowserEnvelope(msg *message.Message) ([]*message.Message, error) {
	var intent UIIntent
	if err := json.Unmarshal(msg.Payload, &intent); err != nil {
		return nil, err
	}

	env := BrowserEnvelope{
		Channel:   "ui",
		Topic:     "ui.intent",
		RoomID:    intent.RoomID,
		DeviceID:  intent.DeviceID,
		Seq:       nextSeq(),
		Timestamp: time.Now(),
		Payload: map[string]any{
			"action": intent.Action,
			"args":   intent.Args,
		},
	}

	payload, err := json.Marshal(env)
	if err != nil {
		return nil, err
	}

	out := message.NewMessage(uuid.NewString(), payload)
	out.Metadata.Set("room_id", intent.RoomID)
	return []*message.Message{out}, nil
}

func makeDeviceIngressHandler(pub message.Publisher) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		c, err := websocket.Accept(w, r, nil)
		if err != nil {
			log.Printf("device websocket accept error: %v", err)
			return
		}
		defer c.CloseNow()

		ctx := r.Context()

		for {
			var ev DeviceEvent
			if err := wsjson.Read(ctx, c, &ev); err != nil {
				return
			}

			if ev.Timestamp.IsZero() {
				ev.Timestamp = time.Now()
			}

			payload, err := json.Marshal(ev)
			if err != nil {
				log.Printf("marshal device event: %v", err)
				continue
			}

			msg := message.NewMessage(uuid.NewString(), payload)
			msg.Metadata.Set("device_id", ev.DeviceID)
			msg.Metadata.Set("room_id", ev.RoomID)

			if err := pub.Publish(TopicDeviceIngress, msg); err != nil {
				log.Printf("publish device event: %v", err)
			}
		}
	}
}

type BrowserHub struct {
	mu      sync.RWMutex
	clients map[*websocket.Conn]chan BrowserEnvelope
}

func NewBrowserHub() *BrowserHub {
	return &BrowserHub{
		clients: map[*websocket.Conn]chan BrowserEnvelope{},
	}
}

func (h *BrowserHub) Run(ctx context.Context, sub message.Subscriber, topic string) error {
	ch, err := sub.Subscribe(ctx, topic)
	if err != nil {
		return err
	}

	for msg := range ch {
		var env BrowserEnvelope
		if err := json.Unmarshal(msg.Payload, &env); err != nil {
			msg.Nack()
			continue
		}

		h.broadcast(env)
		msg.Ack()
	}

	return nil
}

func (h *BrowserHub) HandleWS(w http.ResponseWriter, r *http.Request) {
	c, err := websocket.Accept(w, r, nil)
	if err != nil {
		return
	}
	defer c.CloseNow()

	sendQ := make(chan BrowserEnvelope, 64)

	h.mu.Lock()
	h.clients[c] = sendQ
	h.mu.Unlock()

	defer func() {
		h.mu.Lock()
		delete(h.clients, c)
		h.mu.Unlock()
		close(sendQ)
	}()

	ctx := r.Context()

	// Write pump
	go func() {
		for env := range sendQ {
			_ = wsjson.Write(ctx, c, env)
		}
	}()

	// Optional read loop if the browser will send commands later.
	for {
		var ignore map[string]any
		if err := wsjson.Read(ctx, c, &ignore); err != nil {
			return
		}
	}
}

func (h *BrowserHub) broadcast(env BrowserEnvelope) {
	h.mu.RLock()
	defer h.mu.RUnlock()

	for _, q := range h.clients {
		select {
		case q <- env:
		default:
			// Drop or meter slow clients in v1.
		}
	}
}
```

## Why I’d do it this way

The important design choice is that the **browser hub subscribes once** to `browser.mux`, and then your own in-memory hub fans out to connected browser sockets. I would not create a distinct Watermill subscription per browser tab in the first version. With GoChannel, every subscriber receives every produced message, and non-persistent messages are dropped if nobody is subscribed; it is explicitly an in-process Pub/Sub with no global state, so you want to use the same instance for publish and subscribe. That makes it perfect for a single-process bridge, but it is not the thing I’d use as the browser-connection manager itself. ([Go Packages][4])

In other words: use Watermill for the **event graph**, and use your own hub for the **socket fanout**.

## Frontend contract

On the frontend, I’d keep one socket and redistribute locally.

```ts
type BrowserEnvelope = {
  channel: string;
  topic: string;
  room_id: string;
  device_id?: string;
  seq: number;
  timestamp: string;
  payload: Record<string, unknown>;
};

const ws = new WebSocket(`ws://${location.host}/ws`);
const bus = new EventTarget();

ws.onmessage = (ev) => {
  const msg: BrowserEnvelope = JSON.parse(ev.data);
  bus.dispatchEvent(new CustomEvent(msg.topic, { detail: msg }));
};

// elsewhere
bus.addEventListener("ui.intent", (ev: Event) => {
  const msg = (ev as CustomEvent<BrowserEnvelope>).detail;
  // route into Redux/Zustand/RxJS/custom store
});
```

That gives you your “one WS channel to the frontend, where it can be distributed further”.

## How I’d model the device side

For the ESP32, I’d send events that are already a little semantic:

```json
{ "device_id": "knob-01", "room_id": "lab", "kind": "rotate", "delta": 1 }
{ "device_id": "knob-01", "room_id": "lab", "kind": "press" }
{ "device_id": "knob-01", "room_id": "lab", "kind": "long_press" }
```

Not raw GPIO edges. The server should receive **device events**, not hardware noise.

If the ESP32 also has a display, add the reverse path:

* server publishes `device.render`
* a device-specific bridge subscribes and writes frames back to that device WebSocket

That way the browser and the knob/display can both be projections of the same Watermill event stream.

## What I would change in v2

For local development, `gochannel` is great because it is trivial to wire up. But Watermill’s own docs are clear that GoChannel is in-process only, and Watermill supports swapping to real brokers with the same general API. So once you want multiple server processes, persistence, replay, or durable device presence, I’d move the Watermill backend to **NATS JetStream, RabbitMQ, Redis Streams, SQL, or SQLite**, while keeping the same handlers and bridge shape. ([Watermill][5])

The other v2 thing I’d add quickly is metrics. Watermill has a metrics layer built around Prometheus decorators and handler middleware, so it’s straightforward to instrument publish/subscribe/handler behavior. ([Watermill][6])

## The main refinement I’d suggest

I would split the code into four packages almost immediately:

```text
/internal/devices     // ws ingress, device connection tracking
/internal/events      // topic names, message envelopes, serializers
/internal/projector   // normalize + project handlers
/internal/frontend    // browser hub and ws bridge
```

That keeps the transport, event model, and UI projection separate.

The next useful step is a slightly more complete version with:

* per-room subscriptions
* connect/disconnect presence events
* optional reverse channel for rendering back to the ESP32
* a tiny React/Zustand example that consumes `browser.mux` cleanly

[1]: https://watermill.io/learn/getting-started/?utm_source=chatgpt.com "Getting started | Watermill | Event-Driven in Go"
[2]: https://github.com/coder/websocket?utm_source=chatgpt.com "GitHub - coder/websocket: Minimal and idiomatic WebSocket library for Go"
[3]: https://watermill.io/advanced/fanin/?utm_source=chatgpt.com "FanIn (merging topics) | Watermill | Event-Driven in Go"
[4]: https://pkg.go.dev/github.com/ThreeDotsLabs/watermill/pubsub/gochannel?utm_source=chatgpt.com "gochannel package - github.com/ThreeDotsLabs/watermill/pubsub/gochannel - Go Packages"
[5]: https://watermill.io/?utm_source=chatgpt.com "Watermill"
[6]: https://watermill.io/advanced/metrics/?utm_source=chatgpt.com "Metrics | Watermill | Event-Driven in Go"
