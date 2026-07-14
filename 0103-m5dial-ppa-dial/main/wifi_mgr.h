// WiFi manager: async STA connect with AP-fallback provisioning.
// Adapted from 0095-m5dial-wifi-bench wifi_app (simplified).
#pragma once

#include <cstdint>

enum class WifiState {
    kIdle = 0,
    kConnecting,
    kConnected,
    kApMode, // provisioning hotspot PPA-Dial active
};

inline constexpr const char *kApSsid = "PPA-Dial";
inline constexpr const char *kApPass = "ppadial123";

// Starts WiFi. Empty ssid goes straight to AP mode; otherwise tries STA and
// falls back to AP mode after retries are exhausted. Non-blocking.
bool wifi_mgr_start(const char *ssid, const char *pass);
WifiState wifi_mgr_state();
uint32_t wifi_mgr_sta_ip(); // network byte order; 0 if not connected
