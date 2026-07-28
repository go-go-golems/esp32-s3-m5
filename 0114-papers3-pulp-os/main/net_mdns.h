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

struct MdnsSnapshot;
void FillMdnsSnapshot(MdnsSnapshot *out);

}  // namespace pulp
