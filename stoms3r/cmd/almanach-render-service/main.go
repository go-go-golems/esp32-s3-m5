package main

import (
	"context"
	"fmt"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"
)

// Version is set at build time via -ldflags.
var Version = "dev"

func main() {
	cfg := loadConfig()
	log.Printf("Almanach Render Service %s starting on :%d", Version, cfg.Port)
	log.Printf("  Web dir:     %s", cfg.WebDir)
	log.Printf("  Printer IP:  %s", cfg.PrinterIP)
	log.Printf("  Chrome:      %s", cfg.ChromePath)

	// Create a global Chrome allocator (one Chrome process shared across requests).
	allocatorCtx, allocatorCancel := newChromeAllocator(cfg.ChromePath)
	defer allocatorCancel()

	// Create the server with all dependencies.
	srv := &Server{
		cfg:           cfg,
		allocatorCtx:  allocatorCtx,
		allocatorDone: allocatorCancel,
	}

	mux := http.NewServeMux()
	srv.RegisterRoutes(mux)

	httpServer := &http.Server{
		Addr:         fmt.Sprintf(":%d", cfg.Port),
		Handler:      mux,
		ReadTimeout:  15 * time.Second,
		WriteTimeout: 60 * time.Second, // long for render requests
		IdleTimeout:  120 * time.Second,
	}

	// Graceful shutdown on SIGINT/SIGTERM.
	done := make(chan os.Signal, 1)
	signal.Notify(done, os.Interrupt, syscall.SIGTERM)

	go func() {
		if err := httpServer.ListenAndServe(); err != nil && err != http.ErrServerClosed {
			log.Fatalf("HTTP server error: %v", err)
		}
	}()

	<-done
	log.Println("Shutting down...")

	shutdownCtx, shutdownCancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer shutdownCancel()

	if err := httpServer.Shutdown(shutdownCtx); err != nil {
		log.Printf("HTTP shutdown error: %v", err)
	}

	log.Println("Stopped.")
}
