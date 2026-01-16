package main

import (
	"bytes"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"log"
	"net"
	"net/http"
	"os"
	"os/exec"
	"os/signal"
	"strings"
	"sync"
	"syscall"
	"time"

	"github.com/gorilla/websocket"
)

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool {
		return true // Allow all origins for development
	},
	ReadBufferSize:  10 * 1024 * 1024, // 10MB
	WriteBufferSize: 1024 * 1024,      // 1MB
}

type StreamServer struct {
	cameraClients sync.Map
	viewerClients sync.Map
	ffmpegCmd     *exec.Cmd
	ffmpegStdin   io.WriteCloser
	h264Buffer    *bytes.Buffer
	bufferMutex   sync.RWMutex
	isStreaming   bool
	streamMutex   sync.Mutex
	lastCamLog    time.Time
	camFrameCount uint64
	camByteCount  uint64
	lastViewLog   time.Time
	viewByteCount uint64
}

func NewStreamServer() *StreamServer {
	return &StreamServer{
		h264Buffer: bytes.NewBuffer(make([]byte, 0, 2*1024*1024)),
	}
}

func countClients(m *sync.Map) int {
	count := 0
	m.Range(func(key, value interface{}) bool {
		count++
		return true
	})
	return count
}

func (s *StreamServer) recordCameraFrame(size int) {
	if size <= 0 {
		return
	}
	s.camFrameCount++
	s.camByteCount += uint64(size)
	now := time.Now()
	if s.lastCamLog.IsZero() {
		s.lastCamLog = now
		return
	}
	elapsed := now.Sub(s.lastCamLog)
	if elapsed < 5*time.Second {
		return
	}
	fps := float64(s.camFrameCount) / elapsed.Seconds()
	avg := float64(s.camByteCount) / float64(s.camFrameCount)
	log.Printf("Camera stats: frames=%d fps=%.1f avg_bytes=%.0f connected=%d",
		s.camFrameCount, fps, avg, countClients(&s.cameraClients))
	s.camFrameCount = 0
	s.camByteCount = 0
	s.lastCamLog = now
}

func (s *StreamServer) recordViewerBytes(size int) {
	if size <= 0 {
		return
	}
	s.viewByteCount += uint64(size)
	now := time.Now()
	if s.lastViewLog.IsZero() {
		s.lastViewLog = now
		return
	}
	elapsed := now.Sub(s.lastViewLog)
	if elapsed < 5*time.Second {
		return
	}
	log.Printf("Viewer stats: bytes=%d viewers=%d",
		s.viewByteCount, countClients(&s.viewerClients))
	s.viewByteCount = 0
	s.lastViewLog = now
}

func (s *StreamServer) handleCameraWebSocket(w http.ResponseWriter, r *http.Request) {
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Printf("Camera WebSocket upgrade error: %v", err)
		return
	}
	defer conn.Close()

	clientAddr := conn.RemoteAddr().String()
	userAgent := r.UserAgent()
	if userAgent == "" {
		userAgent = "-"
	}
	log.Printf("Camera connected from %s ua=%s", clientAddr, userAgent)

	s.cameraClients.Store(conn, true)
	log.Printf("Camera clients: %d", countClients(&s.cameraClients))
	defer func() {
		s.cameraClients.Delete(conn)
		log.Printf("Camera clients: %d", countClients(&s.cameraClients))
	}()

	// Start FFmpeg if not already running
	if err := s.startFFmpeg(); err != nil {
		log.Printf("Failed to start FFmpeg: %v", err)
		return
	}

	for {
		messageType, message, err := conn.ReadMessage()
		if err != nil {
			if websocket.IsUnexpectedCloseError(err, websocket.CloseGoingAway, websocket.CloseAbnormalClosure) {
				log.Printf("Camera WebSocket error: %v", err)
			} else if closeErr, ok := err.(*websocket.CloseError); ok {
				log.Printf("Camera WebSocket closed: code=%d text=%s", closeErr.Code, closeErr.Text)
			} else {
				log.Printf("Camera WebSocket read error: %v", err)
			}
			break
		}

		if messageType == websocket.BinaryMessage {
			s.recordCameraFrame(len(message))
			if err := s.processJPEGFrame(message); err != nil {
				log.Printf("Error processing JPEG frame: %v", err)
			}
		} else {
			log.Printf("Camera message type %d (%d bytes)", messageType, len(message))
		}
	}

	log.Printf("Camera disconnected from %s", clientAddr)

	// Check if any cameras are still connected
	hasCamera := false
	s.cameraClients.Range(func(key, value interface{}) bool {
		hasCamera = true
		return false
	})

	if !hasCamera {
		s.stopFFmpeg()
	}
}

func (s *StreamServer) handleViewerWebSocket(w http.ResponseWriter, r *http.Request) {
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Printf("Viewer WebSocket upgrade error: %v", err)
		return
	}
	defer conn.Close()

	clientAddr := conn.RemoteAddr().String()
	userAgent := r.UserAgent()
	if userAgent == "" {
		userAgent = "-"
	}
	log.Printf("Viewer connected from %s ua=%s", clientAddr, userAgent)

	s.viewerClients.Store(conn, true)
	log.Printf("Viewer clients: %d", countClients(&s.viewerClients))
	defer func() {
		s.viewerClients.Delete(conn)
		log.Printf("Viewer clients: %d", countClients(&s.viewerClients))
	}()

	// Send buffered data first
	s.bufferMutex.RLock()
	if s.h264Buffer.Len() > 0 {
		bufferedData := make([]byte, s.h264Buffer.Len())
		copy(bufferedData, s.h264Buffer.Bytes())
		s.bufferMutex.RUnlock()

		if err := conn.WriteMessage(websocket.BinaryMessage, bufferedData); err != nil {
			log.Printf("Error sending buffered data: %v", err)
			return
		}
	} else {
		s.bufferMutex.RUnlock()
	}

	// Keep connection alive
	for {
		messageType, message, err := conn.ReadMessage()
		if err != nil {
			if closeErr, ok := err.(*websocket.CloseError); ok {
				log.Printf("Viewer WebSocket closed: code=%d text=%s", closeErr.Code, closeErr.Text)
			} else {
				log.Printf("Viewer WebSocket read error: %v", err)
			}
			break
		}
		if messageType != websocket.BinaryMessage {
			log.Printf("Viewer message type %d (%d bytes)", messageType, len(message))
		}
	}

	log.Printf("Viewer disconnected from %s", clientAddr)
}

func (s *StreamServer) startFFmpeg() error {
	s.streamMutex.Lock()
	defer s.streamMutex.Unlock()

	if s.isStreaming {
		return nil
	}

	log.Println("Starting FFmpeg transcoding pipeline...")

	s.ffmpegCmd = exec.Command("ffmpeg",
		"-f", "image2pipe",
		"-c:v", "mjpeg",
		"-i", "-",
		"-c:v", "libx264",
		"-preset", "ultrafast",
		"-tune", "zerolatency",
		"-pix_fmt", "yuv420p",
		"-r", "30",
		"-g", "60",
		"-f", "mpegts",
		"-",
	)
	log.Printf("FFmpeg command: %s", strings.Join(s.ffmpegCmd.Args, " "))

	stdin, err := s.ffmpegCmd.StdinPipe()
	if err != nil {
		return fmt.Errorf("failed to create stdin pipe: %w", err)
	}
	s.ffmpegStdin = stdin

	stdout, err := s.ffmpegCmd.StdoutPipe()
	if err != nil {
		return fmt.Errorf("failed to create stdout pipe: %w", err)
	}

	stderr, err := s.ffmpegCmd.StderrPipe()
	if err != nil {
		return fmt.Errorf("failed to create stderr pipe: %w", err)
	}

	if err := s.ffmpegCmd.Start(); err != nil {
		return fmt.Errorf("failed to start FFmpeg: %w", err)
	}

	s.isStreaming = true

	// Read H.264 output
	go s.readH264Output(stdout)

	// Log FFmpeg stderr
	go s.logFFmpegStderr(stderr)

	log.Println("FFmpeg pipeline started")
	return nil
}

func (s *StreamServer) stopFFmpeg() {
	s.streamMutex.Lock()
	defer s.streamMutex.Unlock()

	if !s.isStreaming {
		return
	}

	log.Println("Stopping FFmpeg pipeline...")
	s.isStreaming = false

	if s.ffmpegStdin != nil {
		s.ffmpegStdin.Close()
	}

	if s.ffmpegCmd != nil && s.ffmpegCmd.Process != nil {
		s.ffmpegCmd.Process.Kill()
		s.ffmpegCmd.Wait()
	}

	log.Println("FFmpeg pipeline stopped")
}

func (s *StreamServer) processJPEGFrame(jpegData []byte) error {
	if !s.isStreaming || s.ffmpegStdin == nil {
		return nil
	}

	_, err := s.ffmpegStdin.Write(jpegData)
	return err
}

func (s *StreamServer) readH264Output(stdout io.Reader) {
	log.Println("Started reading H.264 output from FFmpeg")
	buffer := make([]byte, 4096)

	for s.isStreaming {
		n, err := stdout.Read(buffer)
		if err != nil {
			if err != io.EOF {
				log.Printf("Error reading H.264 output: %v", err)
			}
			break
		}

		if n > 0 {
			chunk := buffer[:n]

			// Add to buffer
			s.bufferMutex.Lock()
			s.h264Buffer.Write(chunk)
			// Keep buffer size manageable (last 2MB)
			if s.h264Buffer.Len() > 2*1024*1024 {
				// Keep last 1MB
				data := s.h264Buffer.Bytes()
				s.h264Buffer.Reset()
				s.h264Buffer.Write(data[len(data)-1024*1024:])
			}
			s.bufferMutex.Unlock()

			// Broadcast to viewers
			s.broadcastH264(chunk)
		}
	}
}

func (s *StreamServer) logFFmpegStderr(stderr io.Reader) {
	scanner := io.Reader(stderr)
	buffer := make([]byte, 1024)
	for s.isStreaming {
		n, err := scanner.Read(buffer)
		if err != nil {
			break
		}
		if n > 0 {
			log.Printf("FFmpeg: %s", string(buffer[:n]))
		}
	}
}

func (s *StreamServer) broadcastH264(data []byte) {
	viewerCount := 0
	s.viewerClients.Range(func(key, value interface{}) bool {
		viewerCount++
		conn := key.(*websocket.Conn)
		if err := conn.WriteMessage(websocket.BinaryMessage, data); err != nil {
			log.Printf("Error broadcasting to viewer: %v", err)
			s.viewerClients.Delete(conn)
		}
		return true
	})
	if viewerCount > 0 {
		s.recordViewerBytes(len(data) * viewerCount)
	}
}

func (s *StreamServer) handleStatus(w http.ResponseWriter, r *http.Request) {
	cameraCount := 0
	s.cameraClients.Range(func(key, value interface{}) bool {
		cameraCount++
		return true
	})

	viewerCount := 0
	s.viewerClients.Range(func(key, value interface{}) bool {
		viewerCount++
		return true
	})

	status := map[string]interface{}{
		"camera_clients": cameraCount,
		"viewer_clients": viewerCount,
		"streaming":      s.isStreaming,
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(status)
}

func (s *StreamServer) serveFile(filename string) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		http.ServeFile(w, r, filename)
	}
}

func splitAddr(addr string, defaultPort string) (string, string, error) {
	if addr == "" {
		addr = ":" + defaultPort
	}
	if strings.HasPrefix(addr, ":") {
		addr = "0.0.0.0" + addr
	}
	host, port, err := net.SplitHostPort(addr)
	if err == nil {
		return host, port, nil
	}
	if strings.Contains(addr, ":") {
		return "", "", err
	}
	addr = net.JoinHostPort(addr, defaultPort)
	host, port, err = net.SplitHostPort(addr)
	if err != nil {
		return "", "", err
	}
	return host, port, nil
}

func localIPv4s() []string {
	ifaces, err := net.Interfaces()
	if err != nil {
		return nil
	}
	ips := []string{}
	for _, iface := range ifaces {
		if iface.Flags&net.FlagUp == 0 || iface.Flags&net.FlagLoopback != 0 {
			continue
		}
		addrs, err := iface.Addrs()
		if err != nil {
			continue
		}
		for _, addr := range addrs {
			var ip net.IP
			switch v := addr.(type) {
			case *net.IPNet:
				ip = v.IP
			case *net.IPAddr:
				ip = v.IP
			}
			if ip == nil {
				continue
			}
			if ipv4 := ip.To4(); ipv4 != nil {
				ips = append(ips, ipv4.String())
			}
		}
	}
	return ips
}

func main() {
	server := NewStreamServer()

	addrFlag := flag.String("addr", "127.0.0.1:8080", "HTTP bind address (host:port)")
	flag.Parse()

	// HTTP handlers
	http.HandleFunc("/", server.serveFile("client.html"))
	http.HandleFunc("/mjpeg", server.serveFile("client_mjpeg.html"))
	http.HandleFunc("/status", server.handleStatus)

	// WebSocket handlers
	http.HandleFunc("/ws/camera", server.handleCameraWebSocket)
	http.HandleFunc("/ws/viewer", server.handleViewerWebSocket)

	host, port, err := splitAddr(*addrFlag, "8080")
	if err != nil {
		log.Fatalf("Invalid -addr %q: %v", *addrFlag, err)
	}

	// Start HTTP server
	go func() {
		log.Printf("HTTP server started on %s", net.JoinHostPort(host, port))
		if err := http.ListenAndServe(net.JoinHostPort(host, port), nil); err != nil {
			log.Fatalf("HTTP server error: %v", err)
		}
	}()

	log.Println("Server is ready!")
	if host == "0.0.0.0" || host == "::" || host == "" {
		ips := localIPv4s()
		if len(ips) > 0 {
			for _, ip := range ips {
				log.Printf("- Camera should connect to: ws://%s:%s/ws/camera", ip, port)
				log.Printf("- Viewer should connect to: ws://%s:%s/ws/viewer", ip, port)
				log.Printf("- Web client available at: http://%s:%s", ip, port)
			}
		} else {
			log.Printf("- Camera should connect to: ws://<host>:%s/ws/camera", port)
			log.Printf("- Viewer should connect to: ws://<host>:%s/ws/viewer", port)
			log.Printf("- Web client available at: http://<host>:%s", port)
		}
	} else {
		log.Printf("- Camera should connect to: ws://%s:%s/ws/camera", host, port)
		log.Printf("- Viewer should connect to: ws://%s:%s/ws/viewer", host, port)
		log.Printf("- Web client available at: http://%s:%s", host, port)
	}

	// Wait for interrupt signal
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, os.Interrupt, syscall.SIGTERM)
	<-sigChan

	log.Println("Shutting down...")
	server.stopFFmpeg()
}
