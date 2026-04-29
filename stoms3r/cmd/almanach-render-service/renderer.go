package main

import (
	"context"
	"encoding/base64"
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
	var layoutJSON string

	if layoutOverride != nil {
		// Read the layout JSON from the request body
		data, err := io.ReadAll(layoutOverride)
		if err != nil {
			return nil, fmt.Errorf("read layout: %w", err)
		}
		layoutJSON = string(data)
	} else {
		// Build layout from data fetchers
		layout, err := buildDefaultLayout(s.cfg)
		if err != nil {
			return nil, fmt.Errorf("build layout: %w", err)
		}
		b, _ := json.Marshal(layout)
		layoutJSON = string(b)
	}

	return s.renderWithChrome(ctx, layoutJSON)
}

// renderWithChrome drives Chrome headless to render the layout.
func (s *Server) renderWithChrome(ctx context.Context, layoutJSON string) (*RenderResult, error) {
	start := time.Now()

	// Create a new Chrome tab for this render (shared Chrome process).
	tabCtx, tabCancel := chromedp.NewContext(s.allocatorCtx)
	defer tabCancel()

	// Timeout for this specific render.
	renderCtx, renderCancel := context.WithTimeout(tabCtx, 30*time.Second)
	defer renderCancel()

	var bitmapResult struct {
		Width  int    `json:"width"`
		Height int    `json:"height"`
		Data   string `json:"data"` // base64-encoded bitmap bytes
	}

	err := chromedp.Run(renderCtx,
		// 1. Navigate to the SPA served by this Go server
		chromedp.Navigate(fmt.Sprintf("http://localhost:%d/almanach", s.cfg.Port)),

		// 2. Wait for the SPA to signal readiness
		chromedp.WaitReady("window.almanachReady", chromedp.ByJSPath),

		// 3. Inject the layout data
		chromedp.Evaluate(fmt.Sprintf(`window.almanachLoadLayout(%s)`, layoutJSON), nil),

		// 4. Wait for React to finish rendering + fonts to load
		chromedp.Sleep(800*time.Millisecond),

		// 5. Export the bitmap from the SPA
		chromedp.Evaluate(`
			new Promise(resolve => {
				window.almanachExportBitmap().then(r => {
					const bytes = new Uint8Array(r.data);
					let binary = '';
					for (let i = 0; i < bytes.length; i++) binary += String.fromCharCode(bytes[i]);
					resolve({ width: r.width, height: r.height, data: btoa(binary) });
				}).catch(e => resolve({ error: e.message }));
			})
		`, &bitmapResult),
	)

	if err != nil {
		return nil, fmt.Errorf("chrome render: %w", err)
	}

	if bitmapResult.Data == "" {
		return nil, fmt.Errorf("chrome render: empty bitmap data")
	}

	// Decode the base64 bitmap
	bitmapBytes, err := base64.StdEncoding.DecodeString(bitmapResult.Data)
	if err != nil {
		return nil, fmt.Errorf("decode bitmap: %w", err)
	}

	bytesPerRow := bitmapResult.Width / 8
	if bytesPerRow == 0 {
		return nil, fmt.Errorf("invalid bitmap width: %d", bitmapResult.Width)
	}

	elapsed := time.Since(start)
	log.Printf("Rendered %dx%d bitmap (%d bytes) in %v", bitmapResult.Width, bitmapResult.Height, len(bitmapBytes), elapsed)

	return &RenderResult{
		Bitmap: &Bitmap{
			Width:       bitmapResult.Width,
			Height:      bitmapResult.Height,
			BytesPerRow: bytesPerRow,
			Data:        bitmapBytes,
		},
		Theme:      extractThemeFromLayout(layoutJSON),
		RenderedAt: time.Now().UTC().Format(time.RFC3339),
		LayoutJSON: layoutJSON,
	}, nil
}

// newChromeAllocator creates a shared Chrome process allocator.
func newChromeAllocator(chromePath string) (context.Context, context.CancelFunc) {
	opts := append(chromedp.DefaultExecAllocatorOptions[:],
		chromedp.Flag("headless", true),
		chromedp.Flag("disable-gpu", true),
		chromedp.Flag("no-sandbox", true),
		chromedp.Flag("disable-dev-shm-usage", true),
		chromedp.Flag("hide-scrollbars", true),
		chromedp.Flag("disable-extensions", true),
		chromedp.Flag("disable-background-networking", true),
		chromedp.Flag("disable-default-apps", true),
		chromedp.Flag("disable-sync", true),
		chromedp.Flag("mute-audio", true),
		chromedp.Flag("force-device-scale-factor", 1.0),
		chromedp.WindowSize(384, 2000),
	)

	if chromePath != "" {
		opts = append(opts, chromedp.ExecPath(chromePath))
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
