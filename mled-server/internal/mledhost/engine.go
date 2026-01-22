package mledhost

import (
	"context"
	"crypto/rand"
	"encoding/binary"
	"errors"
	"fmt"
	"net"
	"strings"
	"sync"
	"sync/atomic"
	"time"

	"golang.org/x/net/ipv4"

	"mled-server/internal/mledproto"
)

type Config struct {
	BindIP         string
	Interface      string
	MulticastGroup string
	MulticastPort  int
	MulticastTTL   int

	DiscoveryInterval time.Duration
	OfflineThreshold  time.Duration
	BeaconInterval    time.Duration

	WeakRSSIDbm int
}

type Engine struct {
	cfgMu sync.RWMutex
	cfg   Config

	epochID uint32
	msgID   atomic.Uint32

	startMono time.Time

	conn  net.PacketConn
	pconn *ipv4.PacketConn
	group *net.UDPAddr
	iface *net.Interface

	ctx    context.Context
	cancel context.CancelFunc
	wg     sync.WaitGroup

	mu    sync.RWMutex
	nodes map[uint32]*NodeRecord

	ackMu sync.Mutex
	acks  map[uint32]uint32 // controller_msg_id -> node_id

	onEventMu sync.RWMutex
	onEvent   []func(Event)

	restartMu sync.Mutex
}

func NewEngine(cfg Config) *Engine {
	var epoch [4]byte
	_, _ = rand.Read(epoch[:])
	epochID := binary.LittleEndian.Uint32(epoch[:])

	e := &Engine{
		cfg:       cfg,
		epochID:   epochID,
		startMono: time.Now(),
		nodes:     make(map[uint32]*NodeRecord),
		acks:      make(map[uint32]uint32),
	}
	return e
}

func (e *Engine) OnEvent(fn func(Event)) {
	e.onEventMu.Lock()
	e.onEvent = append(e.onEvent, fn)
	e.onEventMu.Unlock()
}

func (e *Engine) emit(evt Event) {
	e.onEventMu.RLock()
	fns := append([]func(Event){}, e.onEvent...)
	e.onEventMu.RUnlock()
	for _, fn := range fns {
		fn(evt)
	}
}

func (e *Engine) EpochID() uint32 { return e.epochID }

func (e *Engine) ShowMS() uint32 {
	ms := time.Since(e.startMono) / time.Millisecond
	return uint32(uint64(ms) & 0xFFFFFFFF)
}

func (e *Engine) Start(ctx context.Context) error {
	e.restartMu.Lock()
	defer e.restartMu.Unlock()
	return e.startUnlocked(ctx)
}

func (e *Engine) Stop() {
	e.restartMu.Lock()
	defer e.restartMu.Unlock()

	if e.cancel != nil {
		e.cancel()
	}
	if e.conn != nil {
		_ = e.conn.Close()
		e.conn = nil
	}
	e.wg.Wait()
}

func (e *Engine) Restart(ctx context.Context, cfg Config) error {
	e.restartMu.Lock()
	defer e.restartMu.Unlock()

	// Stop existing loops/sockets.
	if e.cancel != nil {
		e.cancel()
	}
	if e.conn != nil {
		_ = e.conn.Close()
	}
	e.wg.Wait()

	e.cfgMu.Lock()
	e.cfg = cfg
	e.cfgMu.Unlock()

	// Start with new config.
	return e.startUnlocked(ctx)
}

func (e *Engine) startUnlocked(ctx context.Context) error {
	e.cfgMu.RLock()
	cfg := e.cfg
	e.cfgMu.RUnlock()

	if cfg.OfflineThreshold <= 0 {
		cfg.OfflineThreshold = 30 * time.Second
	}

	engineCtx, cancel := context.WithCancel(ctx)
	e.ctx = engineCtx
	e.cancel = cancel

	if err := e.openSockets(cfg); err != nil {
		cancel()
		return err
	}

	e.wg.Add(1)
	go func() {
		defer e.wg.Done()
		e.recvLoop()
	}()

	if cfg.DiscoveryInterval > 0 {
		e.wg.Add(1)
		go func() {
			defer e.wg.Done()
			e.pingLoop(cfg.DiscoveryInterval)
		}()
	}

	if cfg.BeaconInterval > 0 {
		e.wg.Add(1)
		go func() {
			defer e.wg.Done()
			e.beaconLoop(cfg.BeaconInterval)
		}()
	}

	e.wg.Add(1)
	go func() {
		defer e.wg.Done()
		e.offlineLoop(cfg.OfflineThreshold)
	}()

	return nil
}

func (e *Engine) NodesSnapshot() []NodeDTO {
	e.mu.RLock()
	defer e.mu.RUnlock()
	now := time.Now()
	e.cfgMu.RLock()
	cfg := e.cfg
	e.cfgMu.RUnlock()

	out := make([]NodeDTO, 0, len(e.nodes))
	for _, n := range e.nodes {
		out = append(out, n.ToDTO(now, cfg.WeakRSSIDbm, cfg.OfflineThreshold))
	}
	return out
}

func (e *Engine) SendPing() error {
	h := mledproto.NewHeader(mledproto.MsgPing)
	h.EpochID = e.epochID
	h.MsgID = e.msgID.Add(1)
	h.SenderID = 0
	h.SetTargetMode(mledproto.TargetAll)
	h.PayloadLen = 0

	var pkt [mledproto.HeaderSize]byte
	_ = h.MarshalTo(pkt[:])
	return e.sendMulticast(pkt[:])
}

func (e *Engine) SendBeacon() error {
	h := mledproto.NewHeader(mledproto.MsgBeacon)
	h.EpochID = e.epochID
	h.MsgID = e.msgID.Add(1)
	h.SenderID = 0
	h.SetTargetMode(mledproto.TargetAll)
	h.ExecuteAtMS = e.ShowMS()
	h.PayloadLen = 0

	var pkt [mledproto.HeaderSize]byte
	_ = h.MarshalTo(pkt[:])
	return e.sendMulticast(pkt[:])
}

type ApplyResult struct {
	SentTo []string `json:"sent_to"`
	Failed []string `json:"failed"`
}

type ApplyOptions struct {
	NodeIDsHex []string
	All        bool
	Pattern    mledproto.PatternConfig
}

// Apply maps the UI-level operation to the on-wire protocol by issuing:
// - targeted CUE_PREPARE to each node (requesting ACK),
// - followed by a multicast CUE_FIRE scheduled shortly in the future.
func (e *Engine) Apply(opts ApplyOptions) (ApplyResult, error) {
	targets, failed, err := e.resolveTargets(opts.NodeIDsHex, opts.All)
	if err != nil {
		return ApplyResult{}, err
	}
	if len(targets) == 0 {
		return ApplyResult{SentTo: nil, Failed: failed}, nil
	}

	cueID, err := randomU32()
	if err != nil {
		return ApplyResult{}, err
	}

	for _, nodeID := range targets {
		if err := e.sendCuePrepare(nodeID, cueID, opts.Pattern, true); err != nil {
			e.emit(Event{Type: EventError, Error: err})
		}
	}

	execAt := e.ShowMS() + 50
	for i := 0; i < 3; i++ {
		_ = e.sendCueFire(cueID, execAt)
		time.Sleep(1 * time.Millisecond)
	}

	sentTo := make([]string, 0, len(targets))
	for _, id := range targets {
		sentTo = append(sentTo, fmt.Sprintf("%08X", id))
	}

	return ApplyResult{SentTo: sentTo, Failed: failed}, nil
}

func (e *Engine) openSockets(cfg Config) error {
	groupIP := net.ParseIP(cfg.MulticastGroup)
	if groupIP == nil {
		return fmt.Errorf("invalid multicast group: %q", cfg.MulticastGroup)
	}
	e.group = &net.UDPAddr{IP: groupIP, Port: cfg.MulticastPort}

	bindAddr := fmt.Sprintf(":%d", cfg.MulticastPort)
	if ip := normalizeBindIP(cfg.BindIP); ip != "" {
		bindAddr = net.JoinHostPort(ip, fmt.Sprintf("%d", cfg.MulticastPort))
	}

	conn, err := net.ListenPacket("udp4", bindAddr)
	if err != nil {
		return err
	}
	e.conn = conn

	pconn := ipv4.NewPacketConn(conn)
	e.pconn = pconn

	iface, err := selectInterface(cfg.Interface, normalizeBindIP(cfg.BindIP))
	if err != nil {
		_ = conn.Close()
		return err
	}
	e.iface = iface

	if err := pconn.JoinGroup(iface, e.group); err != nil {
		_ = conn.Close()
		return err
	}
	if err := pconn.SetMulticastTTL(cfg.MulticastTTL); err != nil {
		_ = conn.Close()
		return err
	}
	if err := pconn.SetMulticastInterface(iface); err != nil {
		_ = conn.Close()
		return err
	}
	return nil
}

func (e *Engine) sendMulticast(b []byte) error {
	if e.pconn == nil {
		return errors.New("not started")
	}
	_, err := e.pconn.WriteTo(b, nil, e.group)
	return err
}

func (e *Engine) sendUnicast(b []byte, dest *net.UDPAddr) error {
	if e.conn == nil {
		return errors.New("not started")
	}
	_, err := e.conn.WriteTo(b, dest)
	return err
}

func (e *Engine) recvLoop() {
	buf := make([]byte, 4096)
	for {
		select {
		case <-e.ctx.Done():
			return
		default:
		}

		_ = e.conn.SetReadDeadline(time.Now().Add(500 * time.Millisecond))
		n, addr, err := e.conn.ReadFrom(buf)
		if err != nil {
			var ne net.Error
			if errors.As(err, &ne) && ne.Timeout() {
				continue
			}
			if e.ctx.Err() != nil {
				return
			}
			e.emit(Event{Type: EventError, Error: err})
			continue
		}

		udpAddr, ok := addr.(*net.UDPAddr)
		if !ok {
			continue
		}

		if n < mledproto.HeaderSize {
			continue
		}
		h, err := mledproto.UnmarshalHeader(buf[:mledproto.HeaderSize])
		if err != nil {
			continue
		}
		if int(mledproto.HeaderSize+h.PayloadLen) > n {
			continue
		}
		payload := buf[mledproto.HeaderSize : mledproto.HeaderSize+int(h.PayloadLen)]

		switch h.Type {
		case mledproto.MsgPong:
			e.handlePong(h, payload, udpAddr)
		case mledproto.MsgAck:
			e.handleAck(h, payload)
		case mledproto.MsgTimeReq:
			e.handleTimeReq(h, udpAddr)
		default:
		}
	}
}

func (e *Engine) handlePong(h mledproto.Header, payload []byte, addr *net.UDPAddr) {
	p, err := mledproto.UnmarshalPong(payload)
	if err != nil {
		return
	}

	now := time.Now()

	e.mu.Lock()
	rec, ok := e.nodes[h.SenderID]
	if !ok {
		rec = &NodeRecord{NodeID: h.SenderID}
		e.nodes[h.SenderID] = rec
	}
	rec.Pong = p
	rec.Addr = *addr
	rec.LastSeen = now
	wasOffline := rec.Offline
	rec.Offline = false
	e.mu.Unlock()

	if wasOffline {
		// An offline->online transition should be visible to the UI.
	}

	e.cfgMu.RLock()
	cfg := e.cfg
	e.cfgMu.RUnlock()
	dto := rec.ToDTO(now, cfg.WeakRSSIDbm, cfg.OfflineThreshold)
	e.emit(Event{Type: EventNodeUpdate, Payload: dto})
}

func (e *Engine) handleAck(h mledproto.Header, payload []byte) {
	ack, err := mledproto.UnmarshalAck(payload)
	if err != nil {
		return
	}

	e.ackMu.Lock()
	nodeID, ok := e.acks[ack.AckForMsgID]
	if ok {
		delete(e.acks, ack.AckForMsgID)
	}
	e.ackMu.Unlock()
	if !ok {
		return
	}

	success := ack.Code == 0
	e.emit(Event{
		Type: EventApplyAck,
		Payload: map[string]any{
			"node_id": fmt.Sprintf("%08X", nodeID),
			"success": success,
		},
	})
}

func (e *Engine) handleTimeReq(h mledproto.Header, addr *net.UDPAddr) {
	// TIME_RESP is epoch-gated by the C node; only respond to requests for our epoch.
	if h.EpochID != e.epochID {
		return
	}

	rx := e.ShowMS()
	tx := e.ShowMS()

	resp := mledproto.TimeResp{
		ReqMsgID:       h.MsgID,
		MasterRxShowMS: rx,
		MasterTxShowMS: tx,
	}
	payload := make([]byte, mledproto.TimeRespSize)
	_ = resp.MarshalTo(payload)

	hdr := mledproto.NewHeader(mledproto.MsgTimeResp)
	hdr.EpochID = e.epochID
	hdr.MsgID = e.msgID.Add(1)
	hdr.SenderID = 0
	hdr.SetTargetMode(mledproto.TargetNode)
	hdr.Target = h.SenderID
	hdr.PayloadLen = mledproto.TimeRespSize

	pkt := make([]byte, mledproto.HeaderSize+mledproto.TimeRespSize)
	_ = hdr.MarshalTo(pkt[:mledproto.HeaderSize])
	copy(pkt[mledproto.HeaderSize:], payload)

	_ = e.sendUnicast(pkt, addr)
}

func (e *Engine) sendCuePrepare(nodeID, cueID uint32, pattern mledproto.PatternConfig, ackReq bool) error {
	cp := mledproto.CuePrepare{
		CueID:   cueID,
		Pattern: pattern,
	}
	payload := make([]byte, mledproto.CuePrepareSize)
	if err := cp.MarshalTo(payload); err != nil {
		return err
	}

	h := mledproto.NewHeader(mledproto.MsgCuePrepare)
	h.EpochID = e.epochID
	h.MsgID = e.msgID.Add(1)
	h.SenderID = 0
	h.SetTargetMode(mledproto.TargetNode)
	h.Target = nodeID
	h.SetAckReq(ackReq)
	h.PayloadLen = mledproto.CuePrepareSize

	pkt := make([]byte, mledproto.HeaderSize+mledproto.CuePrepareSize)
	_ = h.MarshalTo(pkt[:mledproto.HeaderSize])
	copy(pkt[mledproto.HeaderSize:], payload)

	if ackReq {
		e.ackMu.Lock()
		e.acks[h.MsgID] = nodeID
		e.ackMu.Unlock()
	}

	return e.sendMulticast(pkt)
}

func (e *Engine) sendCueFire(cueID uint32, executeAt uint32) error {
	cf := mledproto.CueFire{CueID: cueID}
	payload := make([]byte, mledproto.CueFireSize)
	_ = cf.MarshalTo(payload)

	h := mledproto.NewHeader(mledproto.MsgCueFire)
	h.EpochID = e.epochID
	h.MsgID = e.msgID.Add(1)
	h.SenderID = 0
	h.SetTargetMode(mledproto.TargetAll)
	h.ExecuteAtMS = executeAt
	h.PayloadLen = mledproto.CueFireSize

	pkt := make([]byte, mledproto.HeaderSize+mledproto.CueFireSize)
	_ = h.MarshalTo(pkt[:mledproto.HeaderSize])
	copy(pkt[mledproto.HeaderSize:], payload)
	return e.sendMulticast(pkt)
}

func (e *Engine) pingLoop(interval time.Duration) {
	t := time.NewTicker(interval)
	defer t.Stop()
	for {
		select {
		case <-e.ctx.Done():
			return
		case <-t.C:
			_ = e.SendPing()
		}
	}
}

func (e *Engine) beaconLoop(interval time.Duration) {
	t := time.NewTicker(interval)
	defer t.Stop()
	for {
		select {
		case <-e.ctx.Done():
			return
		case <-t.C:
			_ = e.SendBeacon()
		}
	}
}

func (e *Engine) offlineLoop(threshold time.Duration) {
	t := time.NewTicker(1 * time.Second)
	defer t.Stop()
	for {
		select {
		case <-e.ctx.Done():
			return
		case <-t.C:
		}

		now := time.Now()
		var offlineIDs []uint32

		e.mu.Lock()
		for id, n := range e.nodes {
			if n.Offline {
				continue
			}
			if now.Sub(n.LastSeen) >= threshold {
				n.Offline = true
				offlineIDs = append(offlineIDs, id)
			}
		}
		e.mu.Unlock()

		for _, id := range offlineIDs {
			e.emit(Event{
				Type:    EventNodeOffline,
				Payload: map[string]any{"node_id": fmt.Sprintf("%08X", id)},
			})
		}
	}
}

func (e *Engine) resolveTargets(nodeIDsHex []string, all bool) ([]uint32, []string, error) {
	e.mu.RLock()
	defer e.mu.RUnlock()
	e.cfgMu.RLock()
	cfg := e.cfg
	e.cfgMu.RUnlock()

	now := time.Now()
	if all {
		var ids []uint32
		for id, n := range e.nodes {
			if deriveStatus(now, n.LastSeen, int(n.Pong.RSSIDbm), cfg.WeakRSSIDbm, cfg.OfflineThreshold) == NodeOffline {
				continue
			}
			ids = append(ids, id)
		}
		return ids, nil, nil
	}

	var ids []uint32
	var failed []string
	for _, hexID := range nodeIDsHex {
		id, err := parseNodeID(hexID)
		if err != nil {
			return nil, nil, err
		}
		n, ok := e.nodes[id]
		if !ok || deriveStatus(now, n.LastSeen, int(n.Pong.RSSIDbm), cfg.WeakRSSIDbm, cfg.OfflineThreshold) == NodeOffline {
			failed = append(failed, fmt.Sprintf("%08X", id))
			continue
		}
		ids = append(ids, id)
	}
	return ids, failed, nil
}

func normalizeBindIP(v string) string {
	if v == "" || v == "auto" {
		return ""
	}
	// Accept UI-ish strings like "eth0: 192.168.1.112".
	if idx := lastSpace(v); idx >= 0 {
		cand := v[idx+1:]
		if net.ParseIP(cand) != nil {
			return cand
		}
	}
	if strings.Contains(v, ":") && net.ParseIP(v) == nil {
		// Possibly "iface:ip" without spaces.
		if parts := strings.Split(v, ":"); len(parts) == 2 && net.ParseIP(parts[1]) != nil {
			return parts[1]
		}
	}
	return v
}

func lastSpace(s string) int {
	for i := len(s) - 1; i >= 0; i-- {
		if s[i] == ' ' {
			return i
		}
	}
	return -1
}

func selectInterface(name string, bindIP string) (*net.Interface, error) {
	if name != "" {
		iface, err := net.InterfaceByName(name)
		if err != nil {
			return nil, err
		}
		return iface, nil
	}
	if bindIP != "" {
		iface, err := interfaceForIP(bindIP)
		if err == nil && iface != nil {
			return iface, nil
		}
	}

	ifaces, err := net.Interfaces()
	if err != nil {
		return nil, err
	}
	for i := range ifaces {
		if (ifaces[i].Flags&net.FlagUp) == 0 || (ifaces[i].Flags&net.FlagMulticast) == 0 {
			continue
		}
		if (ifaces[i].Flags & net.FlagLoopback) != 0 {
			continue
		}
		addrs, _ := ifaces[i].Addrs()
		for _, a := range addrs {
			ipNet, ok := a.(*net.IPNet)
			if !ok || ipNet.IP == nil {
				continue
			}
			if ipNet.IP.To4() == nil {
				continue
			}
			return &ifaces[i], nil
		}
	}

	return nil, errors.New("no suitable multicast interface found (set --iface)")
}

func interfaceForIP(bindIP string) (*net.Interface, error) {
	ip := net.ParseIP(bindIP)
	if ip == nil {
		return nil, errors.New("invalid bind ip")
	}
	ifaces, err := net.Interfaces()
	if err != nil {
		return nil, err
	}
	for i := range ifaces {
		addrs, _ := ifaces[i].Addrs()
		for _, a := range addrs {
			ipNet, ok := a.(*net.IPNet)
			if !ok || ipNet.IP == nil {
				continue
			}
			if ipNet.IP.Equal(ip) {
				return &ifaces[i], nil
			}
		}
	}
	return nil, errors.New("no interface found for bind ip")
}

func parseNodeID(hexID string) (uint32, error) {
	var id uint32
	_, err := fmt.Sscanf(hexID, "%x", &id)
	if err != nil {
		return 0, fmt.Errorf("invalid node_id: %q", hexID)
	}
	return id, nil
}

func randomU32() (uint32, error) {
	var b [4]byte
	if _, err := rand.Read(b[:]); err != nil {
		return 0, err
	}
	return binary.LittleEndian.Uint32(b[:]), nil
}
