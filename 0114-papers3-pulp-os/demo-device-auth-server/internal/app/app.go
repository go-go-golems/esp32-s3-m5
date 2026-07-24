package app

import (
	"context"
	"crypto/rand"
	"encoding/base64"
	"encoding/json"
	"errors"
	"fmt"
	"net/http"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"time"

	"github.com/coder/websocket"
	"github.com/go-go-golems/pulp-device-auth-demo/internal/authn"
	"github.com/go-go-golems/pulp-device-auth-demo/internal/sensors"
	"github.com/go-go-golems/tiny-idp/pkg/embeddedidp"
	"github.com/go-go-golems/tiny-idp/pkg/idp"
	"github.com/go-go-golems/tiny-idp/pkg/idpaccounts"
	"github.com/go-go-golems/tiny-idp/pkg/idpstore"
	"github.com/go-go-golems/tiny-idp/pkg/sqlitestore"
	"github.com/rs/zerolog"
	"golang.org/x/crypto/bcrypt"
	"golang.org/x/sync/errgroup"
)

const (
	deviceClientID   = "pulp-papers3"
	resourceClientID = "pulp-demo-api"
)

type Config struct {
	Listen         string
	PublicBaseURL  string
	StateDir       string
	DemoLogin      string
	DemoPassword   []byte
	SensorInterval time.Duration
	TLSCertFile    string
	TLSKeyFile     string
	Logger         zerolog.Logger
}

func (c Config) issuer() string   { return strings.TrimRight(c.PublicBaseURL, "/") + "/idp" }
func (c Config) audience() string { return strings.TrimRight(c.PublicBaseURL, "/") + "/api" }

type Application struct {
	cfg      Config
	store    *sqlitestore.Store
	provider *embeddedidp.Provider
	hub      *sensors.Hub
	server   *http.Server
	logger   zerolog.Logger
}

func New(ctx context.Context, cfg Config) (_ *Application, retErr error) {
	if ctx == nil {
		return nil, errors.New("context is required")
	}
	if cfg.Listen == "" || cfg.PublicBaseURL == "" || cfg.StateDir == "" || cfg.DemoLogin == "" || len(cfg.DemoPassword) == 0 {
		return nil, errors.New("listen, public base URL, state directory, demo login, and demo password are required")
	}
	if err := ensureStateDir(cfg.StateDir); err != nil {
		return nil, err
	}
	store, err := sqlitestore.Open(ctx, sqlitestore.DefaultConfig(filepath.Join(cfg.StateDir, "identity.sqlite")))
	if err != nil {
		return nil, fmt.Errorf("open identity store: %w", err)
	}
	defer func() {
		if retErr != nil {
			_ = store.Close()
		}
	}()

	accounts, err := idpaccounts.NewService(store, idpaccounts.Options{})
	if err != nil {
		return nil, fmt.Errorf("create account service: %w", err)
	}
	if err := ensureDemoAccount(ctx, store, accounts, cfg.DemoLogin, cfg.DemoPassword); err != nil {
		return nil, err
	}

	device := embeddedidp.DeviceClient(deviceClientID, []string{"openid", "profile", "demo.read", "sensors.read"})
	device.Client.AllowedAudiences = []string{cfg.audience()}
	if _, err := embeddedidp.Bootstrap(ctx, store, embeddedidp.BootstrapConfig{
		Mode: embeddedidp.DevMode, Clients: []embeddedidp.ClientSpec{device}, SigningKeyID: "pulp-demo-rs256-1",
	}); err != nil {
		return nil, fmt.Errorf("bootstrap identity provider: %w", err)
	}

	resourceKey, err := loadOrCreateKey(filepath.Join(cfg.StateDir, "secrets", "introspection.key"))
	if err != nil {
		return nil, err
	}
	resourceSecret := base64.RawURLEncoding.EncodeToString(resourceKey)
	clear(resourceKey)
	if err := ensureResourceClient(ctx, store, resourceSecret, cfg.audience()); err != nil {
		return nil, err
	}
	tokenKey, err := loadOrCreateKey(filepath.Join(cfg.StateDir, "secrets", "token.key"))
	if err != nil {
		return nil, err
	}
	defer clear(tokenKey)
	provider, err := embeddedidp.New(ctx, embeddedidp.Options{
		Issuer: cfg.issuer(), Mode: embeddedidp.DevMode, Store: store,
		Authenticator: accounts,
		Cookie:        embeddedidp.CookieConfig{SessionName: "pulp_idp_session", CSRFName: "pulp_idp_csrf", Path: "/idp"},
		Token:         embeddedidp.TokenConfig{SecretKey: tokenKey},
	})
	if err != nil {
		return nil, fmt.Errorf("construct embedded provider: %w", err)
	}
	defer func() {
		if retErr != nil {
			_ = provider.Close(context.Background())
		}
	}()

	transport, err := embeddedidp.NewInProcessIssuerTransport(cfg.issuer(), provider.Handler(), embeddedidp.InProcessTransportOptions{})
	if err != nil {
		return nil, fmt.Errorf("construct in-process issuer transport: %w", err)
	}
	authenticator, err := authn.New(authn.Config{
		Issuer: cfg.issuer(), ClientID: resourceClientID, ClientSecret: resourceSecret, Audience: cfg.audience(),
		HTTPClient: &http.Client{Transport: transport, Timeout: 10 * time.Second},
	})
	if err != nil {
		return nil, fmt.Errorf("construct resource authenticator: %w", err)
	}

	hub := sensors.NewHub(cfg.SensorInterval)
	application := &Application{cfg: cfg, store: store, provider: provider, hub: hub, logger: cfg.Logger}
	application.server = &http.Server{
		Addr: cfg.Listen, Handler: application.routes(authenticator),
		ReadHeaderTimeout: 5 * time.Second, ReadTimeout: 15 * time.Second,
		WriteTimeout: 15 * time.Second, IdleTimeout: 60 * time.Second,
	}
	return application, nil
}

func (a *Application) routes(authenticator *authn.Authenticator) http.Handler {
	mux := http.NewServeMux()
	mux.Handle("/idp/", a.provider.Handler())
	mux.HandleFunc("GET /healthz", func(w http.ResponseWriter, _ *http.Request) {
		writeJSON(w, http.StatusOK, map[string]any{"ok": true})
	})
	mux.HandleFunc("GET /api/v1/me", a.protected(authenticator, "demo.read", func(w http.ResponseWriter, _ *http.Request, p authn.Principal) {
		writeJSON(w, http.StatusOK, map[string]any{"subject": p.Subject, "client_id": p.ClientID, "scopes": p.Scopes, "expires_at": p.ExpiresAt})
	}))
	mux.HandleFunc("GET /api/v1/demo/fortune", a.protected(authenticator, "demo.read", func(w http.ResponseWriter, _ *http.Request, p authn.Principal) {
		writeJSON(w, http.StatusOK, map[string]any{"message": "Ink remembers what pixels forget.", "subject": p.Subject})
	}))
	mux.HandleFunc("GET /api/v1/sensors/snapshot", a.protected(authenticator, "sensors.read", func(w http.ResponseWriter, _ *http.Request, _ authn.Principal) {
		writeJSON(w, http.StatusOK, a.hub.Snapshot())
	}))
	mux.HandleFunc("GET /api/v1/sensors/ws", a.protected(authenticator, "sensors.read", a.serveWebSocket))
	return noStoreAPI(mux)
}

type protectedHandler func(http.ResponseWriter, *http.Request, authn.Principal)

func (a *Application) protected(authenticator *authn.Authenticator, scope string, next protectedHandler) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		result := authenticator.AuthenticateRequest(r.Context(), r, scope)
		switch result.Outcome {
		case authn.Authenticated:
			next(w, r, result.Principal)
		case authn.Forbidden:
			writeError(w, http.StatusForbidden, "forbidden")
		case authn.Unauthorized:
			w.Header().Set("WWW-Authenticate", `Bearer realm="pulp-demo"`)
			writeError(w, http.StatusUnauthorized, "unauthorized")
		default:
			writeError(w, http.StatusServiceUnavailable, "service_unavailable")
		}
	}
}

func (a *Application) serveWebSocket(w http.ResponseWriter, r *http.Request, p authn.Principal) {
	conn, err := websocket.Accept(w, r, &websocket.AcceptOptions{})
	if err != nil {
		a.logger.Warn().Err(err).Msg("websocket upgrade failed")
		return
	}
	defer conn.Close(websocket.StatusNormalClosure, "stream ended")
	conn.SetReadLimit(512)
	ctx, cancel := context.WithDeadline(r.Context(), p.ExpiresAt)
	defer cancel()
	readCtx := conn.CloseRead(ctx)

	sub := a.hub.Subscribe()
	defer sub.Close()
	for {
		select {
		case <-ctx.Done():
			_ = conn.Close(websocket.StatusPolicyViolation, "access token expired")
			return
		case <-readCtx.Done():
			return
		case sample, ok := <-sub.C:
			if !ok {
				return
			}
			payload, err := json.Marshal(sample)
			if err != nil {
				return
			}
			writeCtx, writeCancel := context.WithTimeout(ctx, 2*time.Second)
			err = conn.Write(writeCtx, websocket.MessageText, payload)
			writeCancel()
			if err != nil {
				return
			}
		}
	}
}

func noStoreAPI(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if strings.HasPrefix(r.URL.Path, "/api/") {
			w.Header().Set("Cache-Control", "no-store")
		}
		next.ServeHTTP(w, r)
	})
}

func writeJSON(w http.ResponseWriter, status int, value any) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	_ = json.NewEncoder(w).Encode(value)
}

func writeError(w http.ResponseWriter, status int, code string) {
	writeJSON(w, status, map[string]string{"error": code})
}

func (a *Application) Handler() http.Handler {
	if a == nil || a.server == nil {
		return http.NotFoundHandler()
	}
	return a.server.Handler
}

func (a *Application) Run(ctx context.Context) error {
	group, groupCtx := errgroup.WithContext(ctx)
	group.Go(func() error {
		err := a.hub.Run(groupCtx)
		if errors.Is(err, context.Canceled) {
			return nil
		}
		return err
	})
	group.Go(func() error {
		a.logger.Info().Str("listen", a.cfg.Listen).Str("issuer", a.cfg.issuer()).Str("audience", a.cfg.audience()).Msg("starting development device-auth service")
		var err error
		if strings.HasPrefix(a.cfg.PublicBaseURL, "https://") {
			if a.cfg.TLSCertFile == "" || a.cfg.TLSKeyFile == "" {
				return errors.New("HTTPS public base URL requires TLS certificate and key files")
			}
			err = a.server.ListenAndServeTLS(a.cfg.TLSCertFile, a.cfg.TLSKeyFile)
		} else {
			err = a.server.ListenAndServe()
		}
		if errors.Is(err, http.ErrServerClosed) {
			return nil
		}
		return err
	})
	group.Go(func() error {
		<-groupCtx.Done()
		shutdownCtx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
		defer cancel()
		return a.server.Shutdown(shutdownCtx)
	})
	return group.Wait()
}

func (a *Application) Close(ctx context.Context) error {
	var joined error
	if a.provider != nil {
		joined = errors.Join(joined, a.provider.Close(ctx))
	}
	if a.store != nil {
		joined = errors.Join(joined, a.store.Close())
	}
	return joined
}

func ensureStateDir(path string) error {
	if err := os.MkdirAll(path, 0o700); err != nil {
		return fmt.Errorf("create state directory: %w", err)
	}
	if err := os.Chmod(path, 0o700); err != nil {
		return fmt.Errorf("secure state directory: %w", err)
	}
	return nil
}

func loadOrCreateKey(path string) ([]byte, error) {
	if value, err := os.ReadFile(path); err == nil {
		info, statErr := os.Stat(path)
		if statErr != nil || info.Mode().Perm() != 0o600 || len(value) != 32 {
			return nil, fmt.Errorf("secret %s must be 32 bytes with mode 0600", path)
		}
		return value, nil
	} else if !errors.Is(err, os.ErrNotExist) {
		return nil, err
	}
	if err := os.MkdirAll(filepath.Dir(path), 0o700); err != nil {
		return nil, err
	}
	value := make([]byte, 32)
	if _, err := rand.Read(value); err != nil {
		return nil, err
	}
	file, err := os.OpenFile(path, os.O_WRONLY|os.O_CREATE|os.O_EXCL, 0o600)
	if err != nil {
		clear(value)
		return nil, err
	}
	if _, err := file.Write(value); err != nil {
		_ = file.Close()
		clear(value)
		return nil, err
	}
	if err := file.Close(); err != nil {
		clear(value)
		return nil, err
	}
	return value, nil
}

func ensureDemoAccount(ctx context.Context, store idpstore.Store, accounts *idpaccounts.Service, login string, password []byte) error {
	_, err := accounts.Create(ctx, idpaccounts.CreateRequest{
		ID: "pulp-demo-alice", Subject: "pulp-demo-alice", Login: login, Password: password,
		Email: "alice@example.test", EmailVerified: true, Name: "Alice Paper",
	})
	if err == nil {
		return nil
	}
	if !errors.Is(err, idpstore.ErrDuplicate) {
		return fmt.Errorf("create demo account: %w", err)
	}
	user, getErr := store.GetUserByLogin(ctx, login)
	if getErr != nil || user.ID != "pulp-demo-alice" || user.Sub != "pulp-demo-alice" {
		return errors.New("persisted demo account conflicts with configured identity")
	}
	if _, authErr := accounts.AuthenticatePassword(ctx, login, string(password), idp.LoginMetadata{ClientID: deviceClientID}); authErr != nil {
		return errors.New("persisted demo account password conflicts with configuration")
	}
	return nil
}

func ensureResourceClient(ctx context.Context, store idpstore.Store, secret, audience string) error {
	desired := idpstore.Client{
		ID: resourceClientID, Public: false, AllowedGrantTypes: []string{idpstore.GrantAuthorizationCode},
		AllowedAudiences: []string{audience}, CanIntrospect: true,
		AccessTokenTTL: time.Hour, IDTokenTTL: time.Hour, RefreshTokenTTL: 24 * time.Hour,
	}
	existing, err := store.GetClient(ctx, resourceClientID)
	if errors.Is(err, idpstore.ErrNotFound) {
		desired.SecretHash, err = bcrypt.GenerateFromPassword([]byte(secret), bcrypt.DefaultCost)
		if err != nil {
			return err
		}
		now := time.Now().UTC()
		desired.CreatedAt, desired.UpdatedAt = now, now
		if err := desired.Validate(idpstore.DevMode); err != nil {
			return err
		}
		return store.PutClient(ctx, desired)
	}
	if err != nil {
		return err
	}
	if existing.Public || !existing.CanIntrospect || existing.Disabled || !equalSet(existing.AllowedAudiences, desired.AllowedAudiences) || !equalSet(existing.AllowedGrantTypes, desired.AllowedGrantTypes) || bcrypt.CompareHashAndPassword(existing.SecretHash, []byte(secret)) != nil {
		return errors.New("persisted resource client conflicts with configured security policy")
	}
	return nil
}

func equalSet(a, b []string) bool {
	left, right := append([]string(nil), a...), append([]string(nil), b...)
	sort.Strings(left)
	sort.Strings(right)
	return strings.Join(left, "\x00") == strings.Join(right, "\x00")
}
