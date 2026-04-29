package main

import (
	"log"
	"net/http"
	"os"
	"path/filepath"
	"strings"
)

// registerStaticRoutes serves the Almanach Studio SPA files from the configured web directory.
// This is what Chrome headless will load when rendering a page.
func registerStaticRoutes(mux *http.ServeMux, webDir string) {
	// Verify the directory exists
	if _, err := os.Stat(webDir); os.IsNotExist(err) {
		log.Printf("WARNING: web dir %s does not exist — SPA will not be available", webDir)
	}

	mux.HandleFunc("/almanach", func(w http.ResponseWriter, r *http.Request) {
		serveFile(w, r, filepath.Join(webDir, "index.html"), "text/html; charset=utf-8")
	})

	mux.HandleFunc("/almanach/bundle.js", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Cache-Control", "public, max-age=3600")
		serveFile(w, r, filepath.Join(webDir, "almanach-bundle.js"), "application/javascript; charset=utf-8")
	})
}

func serveFile(w http.ResponseWriter, r *http.Request, path, contentType string) {
	data, err := os.ReadFile(path)
	if err != nil {
		http.Error(w, "not found", http.StatusNotFound)
		return
	}
	// Strip trailing NUL if present (embed artifact)
	data = bytesTrimTrailingNUL(data)
	w.Header().Set("Content-Type", contentType)
	w.Write(data)
}

func bytesTrimTrailingNUL(data []byte) []byte {
	for len(data) > 0 && data[len(data)-1] == 0 {
		data = data[:len(data)-1]
	}
	return data
}

// sanitizePath prevents directory traversal.
func sanitizePath(p string) string {
	p = filepath.Clean(p)
	for strings.HasPrefix(p, "../") {
		p = strings.TrimPrefix(p, "../")
	}
	return p
}
