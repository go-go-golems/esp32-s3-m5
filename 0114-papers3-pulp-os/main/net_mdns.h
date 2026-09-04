// mDNS wrapper (ESP-54 P2): advertises _http._tcp so "pulp.local" resolves
// while the web server is up. mDNS ships as the managed component
// espressif/mdns (moved out of the ESP-IDF 5.x tree into the registry).
//
// Lifecycle (Decision R-MDNSLIFE): lazy init on first MdnsAnnounce; the
// hostname is fixed "pulp"; announce happens inside serve.start() once
// WiFi is up; stop happens in serve.stop(), wifi.off(), and the power
// quiesce sequence so the name is never advertised on a dead link.
//
// Task contract: all functions are OWNER-ONLY. mdns_init/hostname_set/
// service_add are not documented thread-safe; the owner serializes them.
#pragma once

#include <stdint.h>
#include <stddef.h>

#include "app_events.h"

namespace pulp {

constexpr const char *kMdnsHost = "pulp";

// Initializes mdns + sets hostname "pulp" (idempotent). Owner-only.
StatusCode MdnsInit();
// Announces _http._tcp on the given port (idempotent: no-op if already
// announced on the same port; re-announces if the port changed).
StatusCode MdnsAnnounce(uint16_t port);
// Stops advertising and deinits mdns (idempotent). Owner-only.
StatusCode MdnsStop();

uint8_t MdnsStatus();                       // 0 off, 1 announced
void MdnsHost(char *out, size_t cap);       // "pulp"
void MdnsUrl(char *out, size_t cap);        // "http://pulp.local" or ""

// ---- ESP-58: app-server discovery (browse) ----
//
// MdnsBrowse queries _pulp-apps._tcp for kMdnsBrowseWindowMs on a
// throwaway worker task (the HttpWorker shape: single-flight, mailbox,
// ModuleDone(Mdns, kDoneMdnsBrowse, count, err) as the worker's last
// act). Owner-only. Returns Busy while a browse is in flight.
// WiFi down: completes immediately (count=0, err=1) without touching the
// radio — mirrors MdnsAnnounce's deferral logic.
//
// Results are a fixed snapshot valid until the next browse; JS reads it
// via the count/indexed accessors (the wifi.scan idiom). indexUrl is
// assembled here in C (IPv4 preferred over hostname, TXT path= with the
// /pulp/index.json default, :80 elided) so JS never string-builds URLs.
constexpr uint32_t kMdnsBrowseWindowMs = 3000;
constexpr uint32_t kMdnsMaxResults = 8;
constexpr const char *kMdnsAppSvc = "_pulp-apps";
constexpr const char *kMdnsDefaultIndexPath = "/pulp/index.json";

StatusCode MdnsBrowse();                    // owner-only, single-flight
uint32_t MdnsResultCount();                 // last completed browse
const char *MdnsResultName(uint32_t i);     // TXT name > instance; "" oob
const char *MdnsResultIndexUrl(uint32_t i); // "http://host:port/path"; "" oob
// Owner hook, called when ModuleDone(Mdns) is processed: applies a stop
// that arrived while the browse worker was in flight (MdnsStop defers
// instead of freeing the component under a live query).
void MdnsOnBrowseDone();

struct MdnsSnapshot;
void FillMdnsSnapshot(MdnsSnapshot *out);

}  // namespace pulp
