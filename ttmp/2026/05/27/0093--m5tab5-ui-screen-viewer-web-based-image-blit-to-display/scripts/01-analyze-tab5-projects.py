#!/usr/bin/env python3
"""
0093-tab5-ui-screen-viewer: Analyze Tab5 project structure and display specs.

Summarizes the key Tab5 firmware projects, display parameters, and
identifies the best fork base for the screen viewer firmware.

Usage:
    python3 01-analyze-tab5-projects.py
"""

import os
import re
import sys

BASE = "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5"
TAB5_PROJECTS = [
    "0050-tab5-web-text-echo",
    "0051-tab5-boot-logo",
]

M5TAB5_USERDEMO = "/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Tab5-UserDemo"

def analyze_project(path, name):
    """Check which features a Tab5 project has."""
    main_dir = os.path.join(path, "main")
    features = {"display": False, "wifi": False, "http": False, "console": False}

    if not os.path.isdir(main_dir):
        return features

    for f in os.listdir(main_dir):
        if f.endswith(".c") or f.endswith(".cpp"):
            fpath = os.path.join(main_dir, f)
            content = open(fpath).read()
            if "bsp_display" in content or "lv_img" in content:
                features["display"] = True
            if "esp_wifi" in content or "wifi_app" in content:
                features["wifi"] = True
            if "httpd_" in content or "http_server" in content:
                features["http"] = True
            if "esp_console" in content or "wifi_console" in content:
                features["console"] = True

    return features

def extract_display_specs(bsp_path):
    """Extract display resolution and color format from BSP header."""
    specs = {}
    display_h = os.path.join(bsp_path, "include", "bsp", "display.h")
    if os.path.isfile(display_h):
        content = open(display_h).read()
        m = re.search(r"#define BSP_LCD_H_RES\s*\((\d+)\)", content)
        if m:
            specs["h_res"] = int(m.group(1))
        m = re.search(r"#define BSP_LCD_V_RES\s*\((\d+)\)", content)
        if m:
            specs["v_res"] = int(m.group(1))
        m = re.search(r"#define BSP_LCD_BITS_PER_PIXEL\s*\((\d+)\)", content)
        if m:
            specs["bpp"] = int(m.group(1))
        m = re.search(r"#define BSP_LCD_BIGENDIAN\s*\((\d+)\)", content)
        if m:
            specs["big_endian"] = int(m.group(1))
    return specs

def main():
    print("=" * 60)
    print("Tab5 Project Analysis for UI Screen Viewer (0093)")
    print("=" * 60)

    # Analyze existing Tab5 projects
    print("\n--- Tab5 Firmware Projects in esp32-s3-m5 ---\n")
    for proj in TAB5_PROJECTS:
        path = os.path.join(BASE, proj)
        features = analyze_project(path, proj)
        print(f"  {proj}:")
        print(f"    Display: {'✅' if features['display'] else '❌'}")
        print(f"    WiFi:    {'✅' if features['wifi'] else '❌'}")
        print(f"    HTTP:    {'✅' if features['http'] else '❌'}")
        print(f"    Console: {'✅' if features['console'] else '❌'}")

    # Analyze M5Tab5-UserDemo
    print(f"\n--- M5Tab5-UserDemo ---\n")
    userdemo_path = os.path.join(M5TAB5_USERDEMO, "platforms", "tab5")
    features = analyze_project(userdemo_path, "M5Tab5-UserDemo")
    print(f"  M5Tab5-UserDemo:")
    print(f"    Display: {'✅' if features['display'] else '❌'}")
    print(f"    WiFi:    {'✅' if features['wifi'] else '❌'}")
    print(f"    HTTP:    {'✅' if features['http'] else '❌'}")
    print(f"    Console: {'✅' if features['console'] else '❌'}")
    print(f"    ⚠️  Complex HAL init → crashes on boards with missing peripherals")

    # Display specs
    print(f"\n--- Display Specifications ---\n")
    bsp_path = os.path.join(BASE, "0051-tab5-boot-logo", "components", "m5stack_tab5")
    specs = extract_display_specs(bsp_path)
    print(f"  Resolution: {specs.get('h_res', '?')} × {specs.get('v_res', '?')} portrait")
    landscape_w = specs.get('v_res', 1280)
    landscape_h = specs.get('h_res', 720)
    print(f"  Landscape:  {landscape_w} × {landscape_h} (after ROTATION_90)")
    print(f"  Color:      RGB565 ({specs.get('bpp', '?')}-bit)")
    print(f"  Endianness: {'big' if specs.get('big_endian', 0) else 'little'}-endian")
    frame_bytes = landscape_w * landscape_h * 2
    print(f"  Frame size: {frame_bytes:,} bytes ({frame_bytes / 1024 / 1024:.2f} MB)")

    # Recommendation
    print(f"\n--- Recommendation ---\n")
    print(f"  Fork base: 0051-tab5-boot-logo")
    print(f"  - Has display + WiFi + HTTP + console (all needed components)")
    print(f"  - Simple BSP init (no complex peripheral HAL)")
    print(f"  - Proven to boot and display correctly")
    print(f"  - Target directory: 0093-tab5-ui-screen-viewer")

if __name__ == "__main__":
    main()
