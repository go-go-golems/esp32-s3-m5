package app

import (
	"bytes"
	"context"
	"encoding/json"
	"io"
	"net/http"
	"net/http/httptest"
	"net/url"
	"os"
	"path/filepath"
	"testing"
	"time"

	"github.com/rs/zerolog"
)

func TestApplicationBootstrapsDeviceFlowAndProtectsAPI(t *testing.T) {
	state := t.TempDir()
	application, err := New(context.Background(), Config{
		Listen: "127.0.0.1:0", PublicBaseURL: "https://issuer.example.test:8787", StateDir: state,
		DemoLogin: "alice", DemoPassword: []byte("correct horse battery staple"),
		SensorInterval: time.Second, Logger: zerolog.Nop(),
	})
	if err != nil {
		t.Fatal(err)
	}
	defer application.Close(context.Background())
	server := httptest.NewServer(application.Handler())
	defer server.Close()

	assertStatus(t, server.Client(), server.URL+"/healthz", http.StatusOK)
	assertStatus(t, server.Client(), server.URL+"/idp/.well-known/openid-configuration", http.StatusOK)
	assertStatus(t, server.Client(), server.URL+"/api/v1/me", http.StatusUnauthorized)

	form := url.Values{
		"client_id": {deviceClientID},
		"scope":     {"openid profile demo.read sensors.read"},
		"resource":  {"https://issuer.example.test:8787/api"},
	}
	response, err := server.Client().Post(server.URL+"/idp/device_authorization", "application/x-www-form-urlencoded", bytes.NewBufferString(form.Encode()))
	if err != nil {
		t.Fatal(err)
	}
	defer response.Body.Close()
	if response.StatusCode != http.StatusOK {
		body, _ := io.ReadAll(response.Body)
		t.Fatalf("device authorization status=%d body=%s", response.StatusCode, body)
	}
	var device struct {
		DeviceCode      string `json:"device_code"`
		UserCode        string `json:"user_code"`
		VerificationURI string `json:"verification_uri"`
		ExpiresIn       int    `json:"expires_in"`
		Interval        int    `json:"interval"`
	}
	if err := json.NewDecoder(response.Body).Decode(&device); err != nil {
		t.Fatal(err)
	}
	if device.DeviceCode == "" || device.UserCode == "" || device.VerificationURI != "https://issuer.example.test:8787/idp/device" || device.ExpiresIn != 600 || device.Interval != 5 {
		t.Fatalf("unexpected device response: %#v", device)
	}

	for _, secret := range []string{"secrets/introspection.key", "secrets/token.key"} {
		info, err := os.Stat(filepath.Join(state, secret))
		if err != nil || info.Mode().Perm() != 0o600 {
			t.Fatalf("secret %s mode=%v err=%v", secret, info.Mode().Perm(), err)
		}
	}
}

func assertStatus(t *testing.T, client *http.Client, endpoint string, want int) {
	t.Helper()
	response, err := client.Get(endpoint)
	if err != nil {
		t.Fatal(err)
	}
	defer response.Body.Close()
	if response.StatusCode != want {
		body, _ := io.ReadAll(response.Body)
		t.Fatalf("GET %s status=%d want=%d body=%s", endpoint, response.StatusCode, want, body)
	}
}
