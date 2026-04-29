package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"time"
)

// sendBitmapToPrinter sends a 1-bit bitmap to the ESP32's /api/print/bitmap endpoint.
func sendBitmapToPrinter(printerURL string, bitmap *Bitmap, feedLines int) (map[string]any, error) {
	body := bytes.NewReader(bitmap.Data)

	req, err := http.NewRequest("POST", printerURL, body)
	if err != nil {
		return nil, fmt.Errorf("create request: %w", err)
	}

	req.Header.Set("Content-Type", "application/octet-stream")
	req.Header.Set("X-Width", fmt.Sprintf("%d", bitmap.Width))
	req.Header.Set("X-Height", fmt.Sprintf("%d", bitmap.Height))
	req.Header.Set("X-Feed", fmt.Sprintf("%d", feedLines))

	client := &http.Client{Timeout: 30 * time.Second}
	resp, err := client.Do(req)
	if err != nil {
		return nil, fmt.Errorf("printer request failed: %w", err)
	}
	defer resp.Body.Close()

	respBody, _ := io.ReadAll(resp.Body)

	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("printer returned %d: %s", resp.StatusCode, respBody)
	}

	var result map[string]any
	if err := json.Unmarshal(respBody, &result); err != nil {
		return map[string]any{"raw": string(respBody)}, nil
	}
	return result, nil
}
