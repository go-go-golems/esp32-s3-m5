package main

import (
	"bytes"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"net/http"
	"os"
	"strings"
	"time"
)

func runScriptEvalCLI(args []string) error {
	fs := flag.NewFlagSet("script-eval", flag.ContinueOnError)
	fs.SetOutput(os.Stderr)

	serverURL := fs.String("server", "http://127.0.0.1:18080", "server base URL")
	deviceID := fs.String("device", "", "target device id; auto-selects if exactly one device is connected")
	filename := fs.String("file", "", "path to JavaScript file to send")
	inlineCode := fs.String("code", "", "inline JavaScript source")
	timeoutMs := fs.Uint("timeout-ms", 1000, "script timeout in milliseconds")
	requestID := fs.Uint("request-id", 0, "request id; defaults to current unix time")

	if err := fs.Parse(args); err != nil {
		return err
	}

	code, sourceName, err := readScriptSource(*inlineCode, *filename)
	if err != nil {
		return err
	}

	baseURL := normalizeBaseURL(*serverURL)
	targetDevice, err := resolveTargetDevice(baseURL, *deviceID)
	if err != nil {
		return err
	}

	effectiveRequestID := uint32(*requestID)
	if effectiveRequestID == 0 {
		effectiveRequestID = uint32(time.Now().Unix())
	}

	msg := ScriptEvalMessage{
		Type:      "script_eval",
		DeviceID:  targetDevice,
		RequestID: effectiveRequestID,
		Code:      code,
		Filename:  sourceName,
		TimeoutMs: uint32(*timeoutMs),
	}

	payload, err := json.Marshal(msg)
	if err != nil {
		return fmt.Errorf("marshal script-eval payload: %w", err)
	}

	resp, err := http.Post(baseURL+"/api/script-eval", "application/json", bytes.NewReader(payload))
	if err != nil {
		return fmt.Errorf("post /api/script-eval: %w", err)
	}
	defer resp.Body.Close()

	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return fmt.Errorf("read /api/script-eval response: %w", err)
	}

	if len(body) == 0 {
		return fmt.Errorf("/api/script-eval returned empty response (status %s)", resp.Status)
	}

	if resp.StatusCode >= 300 {
		return fmt.Errorf("/api/script-eval returned %s: %s", resp.Status, strings.TrimSpace(string(body)))
	}

	fmt.Printf("device_id=%s request_id=%d source=%s\n", targetDevice, effectiveRequestID, sourceName)
	fmt.Println(string(body))
	return nil
}

func readScriptSource(inlineCode string, filename string) (string, string, error) {
	if inlineCode != "" && filename != "" {
		return "", "", fmt.Errorf("use either --code or --file, not both")
	}
	if inlineCode != "" {
		return inlineCode, "inline", nil
	}
	if filename != "" {
		data, err := os.ReadFile(filename)
		if err != nil {
			return "", "", fmt.Errorf("read %s: %w", filename, err)
		}
		return string(data), filename, nil
	}

	data, err := io.ReadAll(os.Stdin)
	if err != nil {
		return "", "", fmt.Errorf("read stdin: %w", err)
	}
	if len(bytes.TrimSpace(data)) == 0 {
		return "", "", fmt.Errorf("missing script source; provide --code, --file, or stdin")
	}
	return string(data), "stdin", nil
}

func resolveTargetDevice(baseURL string, preferred string) (string, error) {
	if preferred != "" {
		return preferred, nil
	}

	resp, err := http.Get(baseURL + "/api/status")
	if err != nil {
		return "", fmt.Errorf("get /api/status: %w", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode >= 300 {
		body, _ := io.ReadAll(resp.Body)
		return "", fmt.Errorf("/api/status returned %s: %s", resp.Status, strings.TrimSpace(string(body)))
	}

	var snapshot ServerSnapshot
	if err := json.NewDecoder(resp.Body).Decode(&snapshot); err != nil {
		return "", fmt.Errorf("decode /api/status: %w", err)
	}

	connected := make([]DeviceState, 0, len(snapshot.Devices))
	for _, device := range snapshot.Devices {
		if device.Connected {
			connected = append(connected, device)
		}
	}

	switch len(connected) {
	case 0:
		return "", fmt.Errorf("no connected devices found; pass --device once one is connected")
	case 1:
		return connected[0].DeviceID, nil
	default:
		ids := make([]string, 0, len(connected))
		for _, device := range connected {
			ids = append(ids, device.DeviceID)
		}
		return "", fmt.Errorf("multiple connected devices found: %s; pass --device", strings.Join(ids, ", "))
	}
}

func normalizeBaseURL(baseURL string) string {
	baseURL = strings.TrimSpace(baseURL)
	if baseURL == "" {
		return "http://127.0.0.1:18080"
	}
	if !strings.Contains(baseURL, "://") {
		baseURL = "http://" + baseURL
	}
	return strings.TrimRight(baseURL, "/")
}
