package main

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"log"
	"time"

	"github.com/chromedp/chromedp"
)

// RenderResult holds the output of a render pass.
type RenderResult struct {
	Bitmap     *Bitmap
	PNG        []byte // optional PNG for debug/download
	Theme      string
	RenderedAt string
	LayoutJSON string
}

// Bitmap represents a 1-bit monochrome image in MSB-first packed format.
type Bitmap struct {
	Width       int
	Height      int
	BytesPerRow int
	Data        []byte
}

// renderLayoutJSON builds a layout from fetchers and renders it via Chrome headless.
func (s *Server) render(ctx context.Context, layoutOverride io.Reader) (*RenderResult, error) {
	layoutJSON, err := s.layoutJSONFromReader(layoutOverride)
	if err != nil {
		return nil, err
	}

	return s.renderWithChrome(ctx, layoutJSON)
}

func (s *Server) layoutJSONFromReader(layoutOverride io.Reader) (string, error) {
	if layoutOverride != nil {
		data, err := io.ReadAll(layoutOverride)
		if err != nil {
			return "", fmt.Errorf("read layout: %w", err)
		}
		if len(bytes.TrimSpace(data)) > 0 {
			return string(data), nil
		}
	}

	layout, err := buildDefaultLayout(s.cfg)
	if err != nil {
		return "", fmt.Errorf("build layout: %w", err)
	}
	b, err := json.Marshal(layout)
	if err != nil {
		return "", fmt.Errorf("marshal layout: %w", err)
	}
	return string(b), nil
}

// renderWithChrome drives Chrome headless to render the layout.
func (s *Server) renderWithChrome(ctx context.Context, layoutJSON string) (*RenderResult, error) {
	start := time.Now()

	log.Printf("[render] Starting Chrome render for %d bytes of layout JSON", len(layoutJSON))

	tabCtx, tabCancel := chromedp.NewContext(s.allocatorCtx)
	defer tabCancel()

	renderCtx, renderCancel := context.WithTimeout(tabCtx, 30*time.Second)
	defer renderCancel()

	log.Printf("[render] Chrome tab created, running actions...")

	var screenshotBuf []byte

	// CSS to hide all editor UI and leave only the paper content visible.
	// Class names must match the JSX exactly: .topbar, .rail, .block-controls, etc.
	hideChromeJS := `(function() {
		var s = document.createElement('style');
		s.id = '__render-capture';
		s.textContent =
			'.topbar { display: none !important; }' +
			'.rail { display: none !important; }' +
			'.block-controls { display: none !important; }' +
			'.block-wrap { outline: none !important; }' +
			'.block-wrap::before { display: none !important; }' +
			'.paper-shell { filter: none !important; margin: 0 auto !important; box-shadow: none !important; }' +
			'.almanach-app { background: #ffffff !important; }';
		document.head.appendChild(s);
		document.querySelectorAll('.block-wrap').forEach(function(el) {
			el.classList.remove('selected');
		});
	})();`

	err := chromedp.Run(renderCtx,
		chromedp.Navigate(fmt.Sprintf("http://localhost:%d/almanach", s.cfg.Port)),
		chromedp.WaitVisible("body", chromedp.ByQuery),
		chromedp.Sleep(500*time.Millisecond),
		chromedp.Evaluate(fmt.Sprintf(`window.almanachLoadLayout(%s)`, layoutJSON), nil),
		chromedp.Sleep(1*time.Second),
		// Hide editor chrome
		chromedp.Evaluate(hideChromeJS, nil),
		chromedp.Sleep(200*time.Millisecond),
		// Screenshot just the paper element
		chromedp.Screenshot(".paper-shell", &screenshotBuf, chromedp.ByQuery, chromedp.NodeVisible),
		// Clean up
		chromedp.Evaluate(`document.getElementById('__render-capture')?.remove()`, nil),
	)

	if err != nil {
		log.Printf("[render] Chrome error: %v", err)
		return nil, fmt.Errorf("chrome render: %w", err)
	}

	log.Printf("[render] Screenshot captured: %d bytes PNG", len(screenshotBuf))

	bitmap, err := PngToBitmap(screenshotBuf, 128)
	if err != nil {
		return nil, fmt.Errorf("bitmap convert: %w", err)
	}

	elapsed := time.Since(start)
	log.Printf("Rendered %dx%d bitmap (%d bytes) in %v", bitmap.Width, bitmap.Height, len(bitmap.Data), elapsed)

	return &RenderResult{
		Bitmap:     bitmap,
		PNG:        screenshotBuf,
		Theme:      extractThemeFromLayout(layoutJSON),
		RenderedAt: time.Now().UTC().Format(time.RFC3339),
		LayoutJSON: layoutJSON,
	}, nil
}

// newChromeAllocator creates a Chrome allocator. Two modes:
//
//   - If CHROME_WS_URL is set (e.g. "ws://chrome:9222"), connects to a remote
//     headless-shell container. This is the Docker/production mode.
//   - Otherwise, launches a local Chrome process. This is the dev mode.
func newChromeAllocator(cfg Config) (context.Context, context.CancelFunc) {
	if cfg.ChromeWSURL != "" {
		log.Printf("Chrome mode: remote (%s)", cfg.ChromeWSURL)
		allocCtx, cancel := chromedp.NewRemoteAllocator(context.Background(), cfg.ChromeWSURL)
		return allocCtx, cancel
	}

	log.Printf("Chrome mode: local (launching Chrome)")
	opts := []chromedp.ExecAllocatorOption{
		chromedp.NoFirstRun,
		chromedp.NoDefaultBrowserCheck,
		chromedp.Headless,
		chromedp.Flag("disable-gpu", true),
		chromedp.Flag("no-sandbox", true),
		chromedp.Flag("disable-dev-shm-usage", true),
		chromedp.Flag("hide-scrollbars", true),
		chromedp.Flag("force-device-scale-factor", "1.0"),
		// Wide viewport so .paper-shell screenshot crops to just the element
		chromedp.WindowSize(1200, 2000),
	}

	if cfg.ChromePath != "" {
		opts = append(opts, chromedp.ExecPath(cfg.ChromePath))
	}

	allocCtx, cancel := chromedp.NewExecAllocator(context.Background(), opts...)
	return allocCtx, cancel
}

func extractThemeFromLayout(layoutJSON string) string {
	var layout struct {
		Theme string `json:"theme"`
	}
	if json.Unmarshal([]byte(layoutJSON), &layout) == nil && layout.Theme != "" {
		return layout.Theme
	}
	return "minimal"
}
