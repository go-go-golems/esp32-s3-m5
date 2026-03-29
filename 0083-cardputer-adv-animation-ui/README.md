# Tutorial 0083 - Cardputer-ADV Animation UI

Keyboard-driven, sprite-rendered animation UI for Cardputer-ADV.

## Features

- Cardputer-ADV display bring-up via `M5Unified`
- Semantic keyboard events via `cardputer_kb::UnifiedScanner`
- Eased scroll target motion inspired by the imported minimap donor
- Full-screen `M5Canvas` rendering with minimap, scrollbar, viewport, and help overlay

## Build

```bash
./build.sh build
```

## Flash + Monitor

```bash
./build.sh -p /dev/ttyACM0 flash monitor
```

## tmux helper

```bash
./build.sh tmux-flash-monitor
```
