package authn

import (
	"context"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"testing"
	"time"
)

func TestAuthenticateRequestOutcomesAndCache(t *testing.T) {
	now := time.Unix(1_700_000_000, 0).UTC()
	calls := 0
	var issuer string
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		calls++
		user, pass, ok := r.BasicAuth()
		if !ok || user != "api" || pass != "secret" {
			t.Errorf("bad basic auth %q/%q", user, pass)
		}
		_ = r.ParseForm()
		active := r.Form.Get("token") == "good"
		_ = json.NewEncoder(w).Encode(response{
			Active: active, Issuer: issuer, Subject: "alice", ClientID: "device",
			Scope: "demo.read sensors.read", Audience: []string{issuer + "/api"},
			Expires: now.Add(time.Hour).Unix(), TokenType: "Bearer",
		})
	}))
	defer server.Close()
	issuer = server.URL
	authenticator, err := New(Config{Issuer: issuer, ClientID: "api", ClientSecret: "secret", Audience: issuer + "/api", HTTPClient: server.Client(), Now: func() time.Time { return now }})
	if err != nil {
		t.Fatal(err)
	}

	request := httptest.NewRequest(http.MethodGet, issuer+"/api/v1/me", nil)
	if got := authenticator.AuthenticateRequest(context.Background(), request, "demo.read"); got.Outcome != Unauthorized {
		t.Fatalf("missing token outcome = %v", got.Outcome)
	}
	request.Header.Set("Authorization", "Bearer bad")
	if got := authenticator.AuthenticateRequest(context.Background(), request, "demo.read"); got.Outcome != Unauthorized {
		t.Fatalf("bad token outcome = %v", got.Outcome)
	}
	request.Header.Set("Authorization", "Bearer good")
	if got := authenticator.AuthenticateRequest(context.Background(), request, "admin"); got.Outcome != Forbidden {
		t.Fatalf("missing scope outcome = %v", got.Outcome)
	}
	if got := authenticator.AuthenticateRequest(context.Background(), request, "demo.read"); got.Outcome != Authenticated || got.Principal.Subject != "alice" {
		t.Fatalf("good token result = %#v", got)
	}
	if calls != 2 {
		t.Fatalf("introspection calls = %d, want 2 (inactive + active cache)", calls)
	}
}

func TestRejectsMultipleAuthorizationHeaders(t *testing.T) {
	authenticator := &Authenticator{}
	request := httptest.NewRequest(http.MethodGet, "http://example.test", nil)
	request.Header.Add("Authorization", "Bearer one")
	request.Header.Add("Authorization", "Bearer two")
	if got := authenticator.AuthenticateRequest(context.Background(), request); got.Outcome != Unauthorized {
		t.Fatalf("outcome = %v", got.Outcome)
	}
}
