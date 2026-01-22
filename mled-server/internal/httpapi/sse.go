package httpapi

import (
	"encoding/json"
	"fmt"
	"net/http"
	"sync"
	"time"

	"github.com/rs/zerolog/log"

	"mled-server/internal/mledhost"
)

type sseMessage struct {
	event string
	data  []byte
}

type sseHub struct {
	mu      sync.RWMutex
	clients map[chan sseMessage]struct{}
}

func newSSEHub() *sseHub {
	return &sseHub{clients: make(map[chan sseMessage]struct{})}
}

func (h *sseHub) addClient() chan sseMessage {
	ch := make(chan sseMessage, 64)
	h.mu.Lock()
	h.clients[ch] = struct{}{}
	h.mu.Unlock()
	return ch
}

func (h *sseHub) removeClient(ch chan sseMessage) {
	h.mu.Lock()
	if _, ok := h.clients[ch]; ok {
		delete(h.clients, ch)
		close(ch)
	}
	h.mu.Unlock()
}

func (h *sseHub) broadcast(msg sseMessage) {
	h.mu.RLock()
	defer h.mu.RUnlock()
	for ch := range h.clients {
		select {
		case ch <- msg:
		default:
			// Drop if the client isn't consuming; don't block the system.
		}
	}
}

func (s *Server) onEngineEvent(evt mledhost.Event) {
	if s == nil || s.sse == nil {
		return
	}

	var (
		eventName string
		payload   any
	)

	switch evt.Type {
	case mledhost.EventNodeUpdate:
		// Frontend expects event name "node"
		eventName = "node"
		payload = evt.Payload
	case mledhost.EventNodeOffline:
		eventName = "node.offline"
		payload = evt.Payload
	case mledhost.EventApplyAck:
		// Frontend expects event name "ack"
		eventName = "ack"
		payload = evt.Payload
	case mledhost.EventError:
		// Frontend expects event name "log" with {message}
		eventName = "log"
		payload = map[string]any{"message": evt.Error.Error()}
	default:
		return
	}

	b, err := json.Marshal(payload)
	if err != nil {
		return
	}
	s.sse.broadcast(sseMessage{event: eventName, data: b})
}

func (s *Server) handleEvents(w http.ResponseWriter, r *http.Request) {
	flusher, ok := w.(http.Flusher)
	if !ok {
		writeError(w, http.StatusInternalServerError, fmt.Errorf("streaming not supported"))
		return
	}

	w.Header().Set("content-type", "text/event-stream; charset=utf-8")
	w.Header().Set("cache-control", "no-cache")
	w.Header().Set("connection", "keep-alive")
	w.WriteHeader(http.StatusOK)
	_, _ = w.Write([]byte(": ok\n\n"))
	flusher.Flush()

	ch := s.sse.addClient()
	defer s.sse.removeClient(ch)

	log.Info().Str("remote_addr", r.RemoteAddr).Msg("sse connect")
	defer log.Info().Str("remote_addr", r.RemoteAddr).Msg("sse disconnect")

	keepAlive := time.NewTicker(20 * time.Second)
	defer keepAlive.Stop()

	for {
		select {
		case <-r.Context().Done():
			return
		case <-keepAlive.C:
			_, err := w.Write([]byte(": keep-alive\n\n"))
			if err != nil {
				return
			}
			flusher.Flush()
		case msg, ok := <-ch:
			if !ok {
				return
			}
			// SSE frame: event + data
			_, err := fmt.Fprintf(w, "event: %s\ndata: %s\n\n", msg.event, msg.data)
			if err != nil {
				return
			}
			flusher.Flush()
		}
	}
}
