// pulp-shelf: a single-binary PULP app shelf (ESP-59).
//
// Implements the ESP-58 shelf contract: advertise _pulp-apps._tcp over
// mDNS (TXT path=<index path>, name=<label>), serve a JSON index at that
// path, and serve each module at /apps/<id>.js. The Python reference
// implementation lives in the ESP-58 ticket
// (scripts/01-app-index-server.py) and stays authoritative for parity
// tests; the behavioral spec is the ESP-59 intern guide sections 3-4.
//
// Contract rules (each earned by an ESP-55/57 incident — law, not style):
//   - ids match [a-z0-9_-]{1,24} and module URLs end /<id>.js
//   - titles/subtitles are plain ASCII with none of &<>" (the device
//     never urldecodes and hand-builds JSON)
//   - index URLs are absolute; the device assembles nothing
//   - startup validation is fail-fast: bad metadata dies here, on the
//     host, before anything is advertised
package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"log"
	"net"
	"net/http"
	"os"
	"os/signal"
	"path/filepath"
	"regexp"
	"sort"
	"strings"
	"syscall"
	"time"

	"github.com/libp2p/zeroconf/v2"
)

var idRe = regexp.MustCompile(`^[a-z0-9_-]{1,24}$`)

// plainASCII reports whether s is printable ASCII with none of &<>".
// Mirrors the Python reference's plain_ascii; the device-side manifest
// path hand-builds JSON and never urldecodes, so these characters are
// rejected at the source instead of escaped.
func plainASCII(s string) bool {
	for _, r := range s {
		if r < 32 || r >= 127 || strings.ContainsRune(`&<>"`, r) {
			return false
		}
	}
	return true
}

// outboundIP returns the source address the kernel picks for the default
// route (the UDP-connect trick: no packet is sent; 192.0.2.1 is
// TEST-NET-1 and never routed). LAN peers must dial this address, so it
// is what every index URL carries.
func outboundIP() (net.IP, error) {
	conn, err := net.Dial("udp", "192.0.2.1:9")
	if err != nil {
		return nil, err
	}
	defer conn.Close()
	return conn.LocalAddr().(*net.UDPAddr).IP, nil
}

// ifaceFor finds the interface owning ip, so mDNS registers on exactly
// one interface. Joining the multicast group everywhere exhausted
// igmp_max_memberships on the dev host (ESP-58 diary: ENOBUFS).
func ifaceFor(ip net.IP) (net.Interface, error) {
	ifaces, err := net.Interfaces()
	if err != nil {
		return net.Interface{}, err
	}
	for _, ifc := range ifaces {
		addrs, err := ifc.Addrs()
		if err != nil {
			continue
		}
		for _, a := range addrs {
			if ipn, ok := a.(*net.IPNet); ok && ipn.IP.Equal(ip) {
				return ifc, nil
			}
		}
	}
	return net.Interface{}, fmt.Errorf("no interface owns %s", ip)
}

// App and Index field order matches the Python dict insertion order so
// encoding/json emits identical key sequences — the parity gate is a
// byte comparison after URL normalization.
type App struct {
	ID       string `json:"id"`
	Title    string `json:"title"`
	Subtitle string `json:"subtitle"`
	URL      string `json:"url"`
}

type Index struct {
	V    int    `json:"v"`
	Name string `json:"name"`
	Apps []App  `json:"apps"`
}

type sidecar struct {
	Title    string `json:"title"`
	Subtitle string `json:"subtitle"`
}

// invalidMeta returns the first offending field name and value, or "".
func invalidMeta(title, subtitle string) (string, string) {
	if !plainASCII(title) {
		return "title", title
	}
	if !plainASCII(subtitle) {
		return "subtitle", subtitle
	}
	return "", ""
}

// buildIndex scans dir once per call (drop a file in, it appears on the
// next fetch — a contract behavior, not an inefficiency). strict=true is
// the startup pass: any invalid id or non-ASCII metadata is a fatal
// error naming the file. At request time strict=false skips offenders
// with a log line instead, matching the reference's behavior of never
// 500ing on a directory that validated at startup.
func buildIndex(dir, name, baseURL string, strict bool) (Index, error) {
	entries, err := os.ReadDir(dir)
	if err != nil {
		return Index{}, err
	}
	names := make([]string, 0, len(entries))
	for _, e := range entries {
		if !e.IsDir() && strings.HasSuffix(e.Name(), ".js") {
			names = append(names, e.Name())
		}
	}
	sort.Strings(names) // match Python's sorted(os.listdir(...))
	idx := Index{V: 1, Name: name, Apps: []App{}}
	for _, fn := range names {
		id := strings.TrimSuffix(fn, ".js")
		if !idRe.MatchString(id) {
			// Parity with the Python reference: a non-conforming filename
			// is not an app module — skip with a warning in BOTH modes
			// (only bad metadata is fatal; it implies a broken sidecar
			// someone wrote on purpose).
			log.Printf("skip %s: id must match [a-z0-9_-]{1,24}", fn)
			continue
		}
		title, subtitle := id, ""
		scPath := filepath.Join(dir, id+".json")
		if raw, err := os.ReadFile(scPath); err == nil {
			var sc sidecar
			if err := json.Unmarshal(raw, &sc); err != nil {
				log.Printf("skip sidecar %s: %v", scPath, err)
			} else {
				if sc.Title != "" {
					title = sc.Title
				}
				subtitle = sc.Subtitle
			}
		}
		if bad, v := invalidMeta(title, subtitle); bad != "" {
			msg := fmt.Sprintf(
				"%s: %s %q is not plain ASCII (no &<>\") — fix the sidecar; the device never urldecodes",
				id, bad, v)
			if strict {
				return Index{}, fmt.Errorf("%s", msg)
			}
			log.Print(msg)
			continue // skip the app: never advertise what a device cannot digest
		}
		idx.Apps = append(idx.Apps, App{
			ID:       id,
			Title:    title,
			Subtitle: subtitle,
			URL:      baseURL + "/apps/" + fn,
		})
	}
	return idx, nil
}

func main() {
	dirFlag := flag.String("dir", "", "directory of .js modules (required)")
	port := flag.Int("port", 8123, "listen port")
	name := flag.String("name", "App Shelf", "server label (TXT name)")
	indexPath := flag.String("index-path", "/pulp/index.json", "index endpoint path")
	noAdvertise := flag.Bool("no-advertise", false, "HTTP only (contract tests without mDNS)")
	flag.Parse()

	if *dirFlag == "" {
		fmt.Fprintln(os.Stderr, "usage: pulp-shelf -dir <modules> [-port 8123] [-name \"App Shelf\"]")
		os.Exit(2)
	}
	dir, err := filepath.Abs(*dirFlag)
	if err != nil {
		log.Fatal(err)
	}
	if st, err := os.Stat(dir); err != nil || !st.IsDir() {
		log.Fatalf("not a directory: %s", dir)
	}
	if !plainASCII(*name) {
		log.Fatal(`-name must be plain ASCII with no &<>"`)
	}

	ip, err := outboundIP()
	if err != nil {
		log.Fatalf("cannot determine outbound IP: %v", err)
	}
	baseURL := fmt.Sprintf("http://%s:%d", ip, *port)

	// Fail fast on bad metadata before advertising anything.
	idx, err := buildIndex(dir, *name, baseURL, true)
	if err != nil {
		log.Fatal(err)
	}
	log.Printf("serving %d app(s) from %s", len(idx.Apps), dir)
	log.Printf("index: %s%s", baseURL, *indexPath)

	moduleRe := regexp.MustCompile(`^/apps/([a-z0-9_-]{1,24})\.js$`)
	mux := http.NewServeMux()
	mux.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
			return
		}
		switch {
		case r.URL.Path == *indexPath:
			fresh, err := buildIndex(dir, *name, baseURL, false)
			if err != nil {
				http.Error(w, "index scan failed", http.StatusInternalServerError)
				return
			}
			body, _ := json.Marshal(fresh)
			w.Header().Set("Content-Type", "application/json")
			w.Write(body)
		default:
			m := moduleRe.FindStringSubmatch(r.URL.Path)
			if m == nil {
				http.NotFound(w, r)
				return
			}
			// The regexp's charset makes traversal inexpressible; the
			// join below can only name a file inside dir.
			body, err := os.ReadFile(filepath.Join(dir, m[1]+".js"))
			if err != nil {
				http.NotFound(w, r)
				return
			}
			w.Header().Set("Content-Type", "application/javascript")
			w.Write(body)
		}
		log.Printf("http: %s GET %s", r.RemoteAddr, r.URL.Path)
	})

	var mdns *zeroconf.Server
	if !*noAdvertise {
		ifc, err := ifaceFor(ip)
		if err != nil {
			log.Fatalf("mdns interface: %v", err)
		}
		mdns, err = zeroconf.Register(*name, "_pulp-apps._tcp", "local.",
			*port,
			[]string{"path=" + *indexPath, "name=" + *name},
			[]net.Interface{ifc})
		if err != nil {
			log.Fatalf("mdns register: %v", err)
		}
		log.Printf("advertising _pulp-apps._tcp %q at %s:%d (iface %s)",
			*name, ip, *port, ifc.Name)
	}

	srv := &http.Server{Addr: fmt.Sprintf(":%d", *port), Handler: mux}
	go func() {
		if err := srv.ListenAndServe(); err != nil && err != http.ErrServerClosed {
			log.Fatal(err)
		}
	}()

	sig := make(chan os.Signal, 1)
	signal.Notify(sig, os.Interrupt, syscall.SIGTERM)
	<-sig
	if mdns != nil {
		mdns.Shutdown() // sends the mDNS goodbye so no stale record lingers
		log.Print("service withdrawn")
	}
	// Bounded drain; the process is exiting either way.
	shutdownTimer := time.AfterFunc(3*time.Second, func() { os.Exit(0) })
	defer shutdownTimer.Stop()
	_ = srv.Close()
}
