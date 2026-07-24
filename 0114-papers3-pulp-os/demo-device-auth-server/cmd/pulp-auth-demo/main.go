package main

import (
	"context"
	"flag"
	"fmt"
	"os"
	"os/signal"
	"strings"
	"syscall"
	"time"

	"github.com/go-go-golems/pulp-device-auth-demo/internal/app"
	"github.com/rs/zerolog"
)

func main() {
	var (
		listen       = flag.String("listen", "0.0.0.0:8787", "HTTP listen address")
		publicBase   = flag.String("public-base-url", "", "externally reachable base URL (required)")
		stateDir     = flag.String("state-dir", "./var", "durable state directory")
		login        = flag.String("demo-login", "alice", "seeded demo login")
		passwordFile = flag.String("demo-password-file", "", "owner-only file containing demo password (required)")
		sensorEvery  = flag.Duration("sensor-interval", 500*time.Millisecond, "fake sensor sample interval")
		tlsCert      = flag.String("tls-cert", "", "TLS server certificate PEM (required for HTTPS)")
		tlsKey       = flag.String("tls-key", "", "TLS server private key PEM (required for HTTPS)")
		logLevel     = flag.String("log-level", "info", "trace|debug|info|warn|error")
	)
	flag.Parse()
	if *publicBase == "" || *passwordFile == "" {
		fmt.Fprintln(os.Stderr, "--public-base-url and --demo-password-file are required")
		os.Exit(2)
	}
	password, err := os.ReadFile(*passwordFile)
	if err != nil {
		fmt.Fprintf(os.Stderr, "read password file: %v\n", err)
		os.Exit(1)
	}
	password = []byte(strings.TrimRight(string(password), "\r\n"))
	defer clear(password)
	level, err := zerolog.ParseLevel(*logLevel)
	if err != nil {
		fmt.Fprintf(os.Stderr, "parse log level: %v\n", err)
		os.Exit(2)
	}
	logger := zerolog.New(zerolog.ConsoleWriter{Out: os.Stderr, TimeFormat: time.RFC3339}).Level(level).With().Timestamp().Logger()
	logger.Warn().Msg("DEVELOPMENT MODE: HTTP/WS bearer credentials are visible to the local network; never expose this listener to the Internet")

	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	defer stop()
	application, err := app.New(ctx, app.Config{
		Listen: *listen, PublicBaseURL: strings.TrimRight(*publicBase, "/"), StateDir: *stateDir,
		DemoLogin: *login, DemoPassword: password, SensorInterval: *sensorEvery,
		TLSCertFile: *tlsCert, TLSKeyFile: *tlsKey, Logger: logger,
	})
	if err != nil {
		logger.Fatal().Err(err).Msg("initialize application")
	}
	if err := application.Run(ctx); err != nil {
		logger.Error().Err(err).Msg("application stopped with error")
	}
	closeCtx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()
	if err := application.Close(closeCtx); err != nil {
		logger.Error().Err(err).Msg("close application")
	}
}
