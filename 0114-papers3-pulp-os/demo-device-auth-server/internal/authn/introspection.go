package authn

import (
	"context"
	"crypto/hmac"
	"crypto/rand"
	"crypto/sha256"
	"encoding/hex"
	"encoding/json"
	"io"
	"net/http"
	"net/url"
	"strings"
	"sync"
	"time"
)

type Outcome uint8

const (
	Unavailable Outcome = iota
	Unauthorized
	Forbidden
	Authenticated
)

type Principal struct {
	Subject   string
	ClientID  string
	Scopes    []string
	ExpiresAt time.Time
}

type Result struct {
	Outcome   Outcome
	Principal Principal
}

type Config struct {
	Issuer       string
	ClientID     string
	ClientSecret string
	Audience     string
	HTTPClient   *http.Client
	Now          func() time.Time
}

type cacheEntry struct {
	active    bool
	principal Principal
	expiresAt time.Time
}

type Authenticator struct {
	issuer, endpoint, clientID, clientSecret, audience string
	client                                             *http.Client
	now                                                func() time.Time
	cacheKey                                           [32]byte
	mu                                                 sync.Mutex
	cache                                              map[string]cacheEntry
}

type response struct {
	Active    bool     `json:"active"`
	Issuer    string   `json:"iss"`
	Subject   string   `json:"sub"`
	ClientID  string   `json:"client_id"`
	Scope     string   `json:"scope"`
	Audience  []string `json:"aud"`
	Expires   int64    `json:"exp"`
	TokenType string   `json:"token_type"`
}

func New(cfg Config) (*Authenticator, error) {
	issuer, err := canonicalURL(cfg.Issuer)
	if err != nil {
		return nil, err
	}
	if strings.TrimSpace(cfg.ClientID) == "" || cfg.ClientSecret == "" || strings.TrimSpace(cfg.Audience) == "" {
		return nil, &url.Error{Op: "configure", URL: issuer, Err: errRequiredConfig}
	}
	client := cfg.HTTPClient
	if client == nil {
		client = &http.Client{Timeout: 10 * time.Second}
	}
	now := cfg.Now
	if now == nil {
		now = time.Now
	}
	a := &Authenticator{
		issuer: issuer, endpoint: issuer + "/introspect", clientID: cfg.ClientID,
		clientSecret: cfg.ClientSecret, audience: cfg.Audience, client: client,
		now: now, cache: make(map[string]cacheEntry),
	}
	if _, err := rand.Read(a.cacheKey[:]); err != nil {
		return nil, err
	}
	return a, nil
}

var errRequiredConfig = &configError{"issuer, client ID, client secret, and audience are required"}

type configError struct{ text string }

func (e *configError) Error() string { return e.text }

func canonicalURL(raw string) (string, error) {
	u, err := url.Parse(strings.TrimSpace(raw))
	if err != nil || !u.IsAbs() || u.Host == "" || u.User != nil || u.RawQuery != "" || u.Fragment != "" {
		return "", &url.Error{Op: "validate", URL: raw, Err: &configError{"invalid absolute issuer URL"}}
	}
	u.Path = strings.TrimRight(u.Path, "/")
	return u.String(), nil
}

func (a *Authenticator) AuthenticateRequest(ctx context.Context, r *http.Request, requiredScopes ...string) Result {
	if a == nil || r == nil {
		return Result{Outcome: Unavailable}
	}
	token, ok := parseBearer(r.Header.Values("Authorization"))
	if !ok {
		return Result{Outcome: Unauthorized}
	}
	key := a.tokenKey(token)
	if entry, ok := a.cached(key); ok {
		return authorize(entry, requiredScopes)
	}
	introspection, available := a.introspect(ctx, token)
	if !available {
		return Result{Outcome: Unavailable}
	}
	principal, valid := a.validate(introspection)
	if !valid {
		a.store(key, cacheEntry{expiresAt: a.now().Add(3 * time.Second)})
		return Result{Outcome: Unauthorized}
	}
	expires := a.now().Add(30 * time.Second)
	if principal.ExpiresAt.Before(expires) {
		expires = principal.ExpiresAt
	}
	entry := cacheEntry{active: true, principal: principal, expiresAt: expires}
	a.store(key, entry)
	return authorize(entry, requiredScopes)
}

func parseBearer(values []string) (string, bool) {
	if len(values) != 1 {
		return "", false
	}
	parts := strings.Fields(values[0])
	returnValue := len(parts) == 2 && strings.EqualFold(parts[0], "Bearer") && parts[1] != "" && !strings.ContainsAny(parts[1], "\r\n")
	if !returnValue {
		return "", false
	}
	return parts[1], true
}

func (a *Authenticator) introspect(ctx context.Context, token string) (response, bool) {
	form := url.Values{"token": {token}}
	req, err := http.NewRequestWithContext(ctx, http.MethodPost, a.endpoint, strings.NewReader(form.Encode()))
	if err != nil {
		return response{}, false
	}
	req.Header.Set("Content-Type", "application/x-www-form-urlencoded")
	req.SetBasicAuth(a.clientID, a.clientSecret)
	resp, err := a.client.Do(req)
	if err != nil {
		return response{}, false
	}
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusOK {
		return response{}, false
	}
	var out response
	decoder := json.NewDecoder(io.LimitReader(resp.Body, 16<<10))
	if err := decoder.Decode(&out); err != nil {
		return response{}, false
	}
	return out, true
}

func (a *Authenticator) validate(in response) (Principal, bool) {
	now := a.now().UTC()
	expires := time.Unix(in.Expires, 0).UTC()
	if !in.Active || in.Issuer != a.issuer || !strings.EqualFold(in.TokenType, "Bearer") || strings.TrimSpace(in.Subject) == "" || !now.Before(expires) || !contains(in.Audience, a.audience) {
		return Principal{}, false
	}
	return Principal{Subject: in.Subject, ClientID: in.ClientID, Scopes: strings.Fields(in.Scope), ExpiresAt: expires}, true
}

func authorize(entry cacheEntry, required []string) Result {
	if !entry.active {
		return Result{Outcome: Unauthorized}
	}
	for _, scope := range required {
		if !contains(entry.principal.Scopes, scope) {
			return Result{Outcome: Forbidden}
		}
	}
	p := entry.principal
	p.Scopes = append([]string(nil), p.Scopes...)
	return Result{Outcome: Authenticated, Principal: p}
}

func contains(values []string, want string) bool {
	for _, value := range values {
		if value == want {
			return true
		}
	}
	return false
}

func (a *Authenticator) tokenKey(token string) string {
	mac := hmac.New(sha256.New, a.cacheKey[:])
	_, _ = mac.Write([]byte(token))
	return hex.EncodeToString(mac.Sum(nil))
}

func (a *Authenticator) cached(key string) (cacheEntry, bool) {
	a.mu.Lock()
	defer a.mu.Unlock()
	entry, ok := a.cache[key]
	if ok && !a.now().Before(entry.expiresAt) {
		delete(a.cache, key)
		return cacheEntry{}, false
	}
	return entry, ok
}

func (a *Authenticator) store(key string, entry cacheEntry) {
	a.mu.Lock()
	a.cache[key] = entry
	a.mu.Unlock()
}
