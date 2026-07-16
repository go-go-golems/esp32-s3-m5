// WiFi station module (ESP-53 P3). Station mode only; the radio is OFF at
// boot and initializes lazily on the first scan/join (an e-reader budget
// decision — the radio costs ~80 mA).
//
// Task contract: all functions here are OWNER-ONLY unless noted. The
// esp_wifi/IP event handlers run on the system event task and touch only
// the module's POD state (scan mailbox, atomic status) before posting
// ModuleDone — the queue is the memory barrier. Credentials live in
// s3paper_storage (owner-only), so every credential decision (joinSaved
// sequencing, mark-ok) happens in the owner via WifiOwnerOnModuleDone.
#pragma once

#include <stdint.h>
#include <stddef.h>

#include "app_events.h"

namespace pulp {

// WifiStatus() values (JS-visible vocabulary).
enum : uint8_t {
    kWifiOff = 0,
    kWifiIdle = 1,
    kWifiScanning = 2,
    kWifiJoining = 3,
    kWifiUp = 4,
};

constexpr uint32_t kWifiScanMax = 16;

// Async verbs: validate + start, completion arrives as ModuleDone{Wifi}.
// Busy when another wifi operation is in flight.
StatusCode WifiScan();
StatusCode WifiJoin(const char *ssid, const char *pass);
// Tries stored credentials ordered by last_ok descending; delivers one
// join-done to JS (success on the first network that yields an IP,
// failure after the list is exhausted).
StatusCode WifiJoinSaved();
// Radio down (esp_wifi_stop); state -> Off. Cheap to re-arm lazily.
StatusCode WifiOff();

// Owner-loop hook: enforces the join timeout.
void WifiTick(int64_t now_us);

// Owner-side interceptor for ModuleDone{Wifi}: advances the joinSaved
// sequence and marks credentials on success. Returns false when the event
// is consumed internally (JS sees nothing); may adjust value/err.
bool WifiOwnerOnModuleDone(int32_t kind, int32_t *value, int32_t *err);

// Status surface (owner-only reads).
uint8_t WifiStatus();
void WifiIp(char *out, size_t cap);      // "" when not up
const char *WifiSsidCurrent();           // "" when not up
int32_t WifiRssiCurrent();               // 0 when not up

// Scan mailbox accessors (valid after a scan completion).
uint32_t WifiScanCount();
const char *WifiScanSsid(uint32_t i);    // nullptr out of range
int32_t WifiScanRssi(uint32_t i);
int32_t WifiScanSecure(uint32_t i);      // 0 open, 1 secured

struct NetSnapshot;
void FillNetSnapshot(NetSnapshot *out);

}  // namespace pulp
