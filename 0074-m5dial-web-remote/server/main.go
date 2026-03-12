package main

import (
	"embed"
	"encoding/json"
	"flag"
	"fmt"
	"io/fs"
	"log"
	"net/http"
	"os"
	"strings"
	"time"

	"github.com/gorilla/websocket"
)

//go:embed static
var staticFS embed.FS

var upgrader = websocket.Upgrader{
	ReadBufferSize:  4096,
	WriteBufferSize: 4096,
	CheckOrigin: func(r *http.Request) bool {
		return true
	},
}

func main() {
	if len(os.Args) > 1 && os.Args[1] == "script-eval" {
		if err := runScriptEvalCLI(os.Args[2:]); err != nil {
			fmt.Fprintf(os.Stderr, "script-eval: %v\n", err)
			os.Exit(1)
		}
		return
	}

	listenAddr := flag.String("listen", ":8080", "HTTP listen address")
	flag.Parse()

	logger := log.New(os.Stdout, "m5dial-server ", log.LstdFlags|log.Lmicroseconds)
	hub := NewHub(logger)

	staticSub, err := fs.Sub(staticFS, "static")
	if err != nil {
		logger.Fatalf("open embedded static dir: %v", err)
	}

	mux := http.NewServeMux()
	mux.HandleFunc("/api/status", func(w http.ResponseWriter, r *http.Request) {
		logger.Printf("api status requested remote=%s ua=%q", r.RemoteAddr, r.UserAgent())
		writeJSON(w, http.StatusOK, hub.Snapshot())
	})
	mux.HandleFunc("/api/script-eval", func(w http.ResponseWriter, r *http.Request) {
		logger.Printf("api script-eval requested remote=%s ua=%q", r.RemoteAddr, r.UserAgent())
		if r.Method != http.MethodPost {
			w.Header().Set("Allow", http.MethodPost)
			http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
			return
		}

		defer r.Body.Close()

		var msg ScriptEvalMessage
		dec := json.NewDecoder(r.Body)
		dec.DisallowUnknownFields()
		if err := dec.Decode(&msg); err != nil {
			logger.Printf("api script-eval decode failed remote=%s err=%v", r.RemoteAddr, err)
			writeJSON(w, http.StatusBadRequest, ScriptEvalResultMessage{
				Type:        "script_eval_result",
				GeneratedAt: time.Now(),
				Status:      "rejected",
				Reason:      "invalid json payload",
			})
			return
		}

		result := hub.queueScriptEval("http:"+r.RemoteAddr, msg)
		status := http.StatusAccepted
		if result.Status != "queued" {
			switch result.Reason {
			case "device_id and code are required", "invalid script_eval payload":
				status = http.StatusBadRequest
			default:
				status = http.StatusConflict
			}
		}
		writeJSON(w, status, result)
	})
	mux.HandleFunc("/ws/device", func(w http.ResponseWriter, r *http.Request) {
		logger.Printf("device ws upgrade requested remote=%s ua=%q origin=%q", r.RemoteAddr, r.UserAgent(), r.Header.Get("Origin"))
		conn, err := upgrader.Upgrade(w, r, nil)
		if err != nil {
			logger.Printf("upgrade device ws: %v", err)
			return
		}
		defer conn.Close()
		hub.HandleDeviceConn(conn, r.RemoteAddr)
	})
	mux.HandleFunc("/ws/browser", func(w http.ResponseWriter, r *http.Request) {
		logger.Printf("browser ws upgrade requested remote=%s ua=%q origin=%q", r.RemoteAddr, r.UserAgent(), r.Header.Get("Origin"))
		conn, err := upgrader.Upgrade(w, r, nil)
		if err != nil {
			logger.Printf("upgrade browser ws: %v", err)
			return
		}
		defer conn.Close()
		hub.HandleBrowserConn(conn, r.RemoteAddr)
	})
	mux.Handle("/", spaHandler(staticSub))

	logger.Printf("listening on %s", *listenAddr)
	logger.Printf("device websocket: ws://127.0.0.1%s/ws/device", *listenAddr)
	logger.Printf("browser websocket: ws://127.0.0.1%s/ws/browser", *listenAddr)
	if err := http.ListenAndServe(*listenAddr, withLog(logger, mux)); err != nil {
		logger.Fatal(err)
	}
}

func withLog(logger *log.Logger, next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		start := time.Now()
		next.ServeHTTP(w, r)
		logger.Printf("%s %s remote=%s ua=%q dur=%s", r.Method, r.URL.Path, r.RemoteAddr, r.UserAgent(), time.Since(start).Round(time.Millisecond))
	})
}

func writeJSON(w http.ResponseWriter, status int, v any) {
	w.Header().Set("Content-Type", "application/json; charset=utf-8")
	w.WriteHeader(status)
	enc := json.NewEncoder(w)
	enc.SetIndent("", "  ")
	if err := enc.Encode(v); err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
	}
}

func spaHandler(staticFiles fs.FS) http.Handler {
	fileServer := http.FileServer(http.FS(staticFiles))
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		path := strings.TrimPrefix(r.URL.Path, "/")
		if path == "" {
			path = "index.html"
		}

		if _, err := fs.Stat(staticFiles, path); err == nil {
			fileServer.ServeHTTP(w, r)
			return
		}

		index, err := fs.ReadFile(staticFiles, "index.html")
		if err != nil {
			http.Error(w, fmt.Sprintf("index.html missing: %v", err), http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "text/html; charset=utf-8")
		_, _ = w.Write(index)
	})
}
