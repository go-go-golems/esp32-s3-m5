package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
	"os/exec"
	"os/signal"
	"sync"
	"syscall"

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
}

func NewStreamServer() *StreamServer {
	return &StreamServer{
		h264Buffer: bytes.NewBuffer(make([]byte, 0, 2*1024*1024)),
	}
}

func (s *StreamServer) handleCameraWebSocket(w http.ResponseWriter, r *http.Request) {
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Printf("Camera WebSocket upgrade error: %v", err)
		return
	}
	defer conn.Close()

	clientAddr := conn.RemoteAddr().String()
	log.Printf("Camera connected from %s", clientAddr)

	s.cameraClients.Store(conn, true)
	defer s.cameraClients.Delete(conn)

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
			}
			break
		}

		if messageType == websocket.BinaryMessage {
			if err := s.processJPEGFrame(message); err != nil {
				log.Printf("Error processing JPEG frame: %v", err)
			}
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
	log.Printf("Viewer connected from %s", clientAddr)

	s.viewerClients.Store(conn, true)
	defer s.viewerClients.Delete(conn)

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
		_, _, err := conn.ReadMessage()
		if err != nil {
			break
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
	s.viewerClients.Range(func(key, value interface{}) bool {
		conn := key.(*websocket.Conn)
		if err := conn.WriteMessage(websocket.BinaryMessage, data); err != nil {
			log.Printf("Error broadcasting to viewer: %v", err)
			s.viewerClients.Delete(conn)
		}
		return true
	})
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

func main() {
	server := NewStreamServer()

	// HTTP handlers
	http.HandleFunc("/", server.serveFile("client.html"))
	http.HandleFunc("/mjpeg", server.serveFile("client_mjpeg.html"))
	http.HandleFunc("/status", server.handleStatus)

	// WebSocket handlers
	http.HandleFunc("/ws/camera", server.handleCameraWebSocket)
	http.HandleFunc("/ws/viewer", server.handleViewerWebSocket)

	// Start HTTP server
	go func() {
		log.Println("HTTP server started on port 8080")
		if err := http.ListenAndServe(":8080", nil); err != nil {
			log.Fatalf("HTTP server error: %v", err)
		}
	}()

	log.Println("Server is ready!")
	log.Println("- Camera should connect to: ws://localhost:8080/ws/camera")
	log.Println("- Viewer should connect to: ws://localhost:8080/ws/viewer")
	log.Println("- Web client available at: http://localhost:8080")

	// Wait for interrupt signal
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, os.Interrupt, syscall.SIGTERM)
	<-sigChan

	log.Println("Shutting down...")
	server.stopFFmpeg()
}
