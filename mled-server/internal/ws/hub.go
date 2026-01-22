package ws

import (
	"context"
	"encoding/json"
	"sync"

	"nhooyr.io/websocket"
)

type Hub struct {
	mu        sync.RWMutex
	clients   map[*client]struct{}
	onRefresh func()
}

func NewHub() *Hub {
	return &Hub{clients: make(map[*client]struct{})}
}

func (h *Hub) ServeWS(wsc *websocket.Conn) {
	c := &client{
		conn: wsc,
		send: make(chan []byte, 64),
	}

	h.mu.Lock()
	h.clients[c] = struct{}{}
	h.mu.Unlock()

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	go func() {
		defer cancel()
		for {
			_, b, err := wsc.Read(ctx)
			if err != nil {
				return
			}
			// We only support a tiny client->server surface (per imported spec).
			var msg inbound
			if err := json.Unmarshal(b, &msg); err != nil {
				continue
			}
			if msg.Event == "refresh" && h.onRefresh != nil {
				h.onRefresh()
			}
		}
	}()

	for {
		select {
		case <-ctx.Done():
			h.removeClient(c)
			return
		case b := <-c.send:
			_ = wsc.Write(ctx, websocket.MessageText, b)
		}
	}
}

type inbound struct {
	Event string `json:"event"`
}

type client struct {
	conn *websocket.Conn
	send chan []byte
}

func (h *Hub) removeClient(c *client) {
	h.mu.Lock()
	delete(h.clients, c)
	h.mu.Unlock()
	close(c.send)
	_ = c.conn.Close(websocket.StatusNormalClosure, "")
}

type envelope struct {
	Event   string `json:"event"`
	Payload any    `json:"payload"`
}

func (h *Hub) Broadcast(event string, payload any) {
	b, err := json.Marshal(envelope{Event: event, Payload: payload})
	if err != nil {
		return
	}

	h.mu.RLock()
	defer h.mu.RUnlock()
	for c := range h.clients {
		select {
		case c.send <- b:
		default:
			// Drop if the client isn't consuming; don't block the whole system.
		}
	}
}

func (h *Hub) OnRefresh(fn func()) {
	h.onRefresh = fn
}
