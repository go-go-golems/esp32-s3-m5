package zigbee

import (
	"context"
	"crypto/tls"
	"crypto/x509"
	"errors"
	"fmt"
	"math/rand"
	"os"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

func Connect(ctx context.Context, settings Settings) (mqtt.Client, time.Duration, byte, error) {
	timeout, err := settings.TimeoutDuration()
	if err != nil {
		return nil, 0, 0, err
	}
	qos, err := settings.QOSByte()
	if err != nil {
		return nil, 0, 0, err
	}

	opts := mqtt.NewClientOptions()
	opts.AddBroker(settings.Broker)
	opts.SetClientID(fmt.Sprintf("zigctl-%d", rand.Int()))
	opts.SetCleanSession(true)
	opts.SetConnectTimeout(timeout)
	if settings.TLSEnabled() {
		tlsCfg, err := tlsConfigFromSettings(settings)
		if err != nil {
			return nil, 0, 0, err
		}
		opts.SetTLSConfig(tlsCfg)
	}

	client := mqtt.NewClient(opts)
	token := client.Connect()
	if !token.WaitTimeout(timeout) {
		return nil, 0, 0, fmt.Errorf("mqtt connect timeout after %s", timeout)
	}
	if err := token.Error(); err != nil {
		return nil, 0, 0, err
	}

	if ctx.Err() != nil {
		client.Disconnect(250)
		return nil, 0, 0, ctx.Err()
	}

	return client, timeout, qos, nil
}

func tlsConfigFromSettings(settings Settings) (*tls.Config, error) {
	rootCAs, err := x509.SystemCertPool()
	if err != nil || rootCAs == nil {
		rootCAs = x509.NewCertPool()
	}

	if settings.CAFile != "" {
		data, err := os.ReadFile(settings.CAFile)
		if err != nil {
			return nil, fmt.Errorf("read cafile: %w", err)
		}
		if ok := rootCAs.AppendCertsFromPEM(data); !ok {
			return nil, errors.New("failed to parse CA file")
		}
	}

	var certs []tls.Certificate
	if settings.CertFile != "" || settings.KeyFile != "" {
		if settings.CertFile == "" || settings.KeyFile == "" {
			return nil, errors.New("both --cert and --key are required for client certificates")
		}
		cert, err := tls.LoadX509KeyPair(settings.CertFile, settings.KeyFile)
		if err != nil {
			return nil, fmt.Errorf("load client cert/key: %w", err)
		}
		certs = []tls.Certificate{cert}
	}

	return &tls.Config{
		MinVersion:   tls.VersionTLS12,
		RootCAs:      rootCAs,
		Certificates: certs,
	}, nil
}
