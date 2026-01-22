package restclient

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"strings"
	"time"

	"mled-server/internal/mledhost"
	"mled-server/internal/storage"
)

type Client struct {
	baseURL string
	http    *http.Client
}

func New(baseURL string, timeout time.Duration) *Client {
	baseURL = strings.TrimRight(strings.TrimSpace(baseURL), "/")
	if timeout <= 0 {
		timeout = 5 * time.Second
	}
	return &Client{
		baseURL: baseURL,
		http:    &http.Client{Timeout: timeout},
	}
}

type apiError struct {
	Error string `json:"error"`
}

func (c *Client) doJSON(ctx context.Context, method string, path string, in any, out any) error {
	if !strings.HasPrefix(path, "/") {
		path = "/" + path
	}

	var body io.Reader
	if in != nil {
		b, err := json.Marshal(in)
		if err != nil {
			return err
		}
		body = bytes.NewReader(b)
	}

	req, err := http.NewRequestWithContext(ctx, method, c.baseURL+path, body)
	if err != nil {
		return err
	}
	if in != nil {
		req.Header.Set("content-type", "application/json; charset=utf-8")
	}

	resp, err := c.http.Do(req)
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	b, err := io.ReadAll(io.LimitReader(resp.Body, 1<<20))
	if err != nil {
		return err
	}

	if resp.StatusCode < 200 || resp.StatusCode >= 300 {
		var apiErr apiError
		if err := json.Unmarshal(b, &apiErr); err == nil && apiErr.Error != "" {
			return fmt.Errorf("%s %s: %s", method, path, apiErr.Error)
		}
		return fmt.Errorf("%s %s: http %d", method, path, resp.StatusCode)
	}

	if out == nil {
		return nil
	}
	return json.Unmarshal(b, out)
}

type Status struct {
	EpochID uint32 `json:"epoch_id"`
	ShowMS  uint32 `json:"show_ms"`
	Running bool   `json:"running"`
}

func (c *Client) Status(ctx context.Context) (Status, error) {
	var out Status
	if err := c.doJSON(ctx, http.MethodGet, "/api/status", nil, &out); err != nil {
		return Status{}, err
	}
	return out, nil
}

func (c *Client) Nodes(ctx context.Context) ([]mledhost.NodeDTO, error) {
	var out []mledhost.NodeDTO
	if err := c.doJSON(ctx, http.MethodGet, "/api/nodes", nil, &out); err != nil {
		return nil, err
	}
	return out, nil
}

func (c *Client) Presets(ctx context.Context) ([]storage.Preset, error) {
	var out []storage.Preset
	if err := c.doJSON(ctx, http.MethodGet, "/api/presets", nil, &out); err != nil {
		return nil, err
	}
	return out, nil
}

func (c *Client) CreatePreset(ctx context.Context, p storage.Preset) (storage.Preset, error) {
	var out storage.Preset
	if err := c.doJSON(ctx, http.MethodPost, "/api/presets", p, &out); err != nil {
		return storage.Preset{}, err
	}
	return out, nil
}

func (c *Client) UpdatePreset(ctx context.Context, id string, p storage.Preset) (storage.Preset, error) {
	var out storage.Preset
	if err := c.doJSON(ctx, http.MethodPut, "/api/presets/"+id, p, &out); err != nil {
		return storage.Preset{}, err
	}
	return out, nil
}

func (c *Client) DeletePreset(ctx context.Context, id string) error {
	return c.doJSON(ctx, http.MethodDelete, "/api/presets/"+id, nil, nil)
}

func (c *Client) Settings(ctx context.Context) (storage.Settings, error) {
	var out storage.Settings
	if err := c.doJSON(ctx, http.MethodGet, "/api/settings", nil, &out); err != nil {
		return storage.Settings{}, err
	}
	return out, nil
}

func (c *Client) UpdateSettings(ctx context.Context, next storage.Settings) (storage.Settings, error) {
	var out storage.Settings
	if err := c.doJSON(ctx, http.MethodPut, "/api/settings", next, &out); err != nil {
		return storage.Settings{}, err
	}
	return out, nil
}

type ApplyRequest struct {
	NodeIDs any                   `json:"node_ids"`
	Config  storage.PatternConfig `json:"config"`
}

func (c *Client) Apply(ctx context.Context, req ApplyRequest) (mledhost.ApplyResult, error) {
	var out mledhost.ApplyResult
	if err := c.doJSON(ctx, http.MethodPost, "/api/apply", req, &out); err != nil {
		return mledhost.ApplyResult{}, err
	}
	return out, nil
}
