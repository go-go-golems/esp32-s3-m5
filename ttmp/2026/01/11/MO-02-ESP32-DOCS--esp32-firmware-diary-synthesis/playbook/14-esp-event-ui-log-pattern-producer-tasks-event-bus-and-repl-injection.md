---
Title: 'esp_event UI Log Pattern: Producer Tasks, Event Bus, and REPL Injection'
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - esp32
    - firmware
    - documentation
    - esp_event
    - ui
    - repl
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0028-cardputer-esp-event-demo/main/app_main.cpp
      Note: Basic esp_event demo implementation
    - Path: esp32-s3-m5/0030-cardputer-console-eventbus/main/app_main.cpp
      Note: esp_event with REPL injection
ExternalSources:
    - URL: https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32s3/api-reference/system/esp_event.html
      Note: Official esp_event documentation
Summary: 'Complete guide to building event-driven UIs with esp_event, multiple producer tasks, and REPL command injection.'
LastUpdated: 2026-01-12T12:00:00-05:00
WhatFor: 'Teach developers the esp_event UI log pattern for embedded displays.'
WhenToUse: 'Use when building event-driven firmware with real-time UI logging.'
---

# esp_event UI Log Pattern: Producer Tasks, Event Bus, and REPL Injection

## Introduction

Embedded firmware often needs to coordinate multiple asynchronous activities—keyboard scanning, sensor polling, network events—while maintaining a responsive display. The naïve approach of directly calling UI functions from each task leads to race conditions, flickering, and unmaintainable code.

This guide teaches a robust pattern: **multiple producer tasks post events to a central bus, and a single UI task dispatches and renders them**. The pattern provides:

- **Decoupling** — Producers don't know about the UI
- **Thread safety** — Only one task touches the display
- **Extensibility** — New event types require no UI task changes
- **Debuggability** — Events can be logged, replayed, or injected from REPL

We'll build this pattern step-by-step using ESP-IDF's `esp_event` library, culminating in a complete example with keyboard input, background producers, scrollable log display, and REPL command injection.

---

## Pattern Overview

```
┌──────────────────────────────────────────────────────────────────────────┐
│                           PRODUCER TASKS                                 │
├──────────────────┬──────────────────┬──────────────────┬────────────────┤
│   Keyboard Task  │  Heartbeat Task  │  Random Tasks    │  REPL Handler  │
│                  │                  │    (N)           │                │
│  ┌────────────┐  │  ┌────────────┐  │  ┌────────────┐  │ ┌────────────┐ │
│  │ Scan keys  │  │  │ Read heap  │  │  │ Generate   │  │ │ Parse cmd  │ │
│  │ Detect edge│  │  │ stats      │  │  │ random val │  │ │ argv       │ │
│  └─────┬──────┘  │  └─────┬──────┘  │  └─────┬──────┘  │ └─────┬──────┘ │
│        │         │        │         │        │         │       │        │
└────────┼─────────┴────────┼─────────┴────────┼─────────┴───────┼────────┘
         │                  │                  │                 │
         │      esp_event_post_to(loop, ...)   │                 │
         ▼                  ▼                  ▼                 ▼
┌────────────────────────────────────────────────────────────────────────┐
│                        EVENT QUEUE                                     │
│   ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐     │
│   │ KB  │ │ HB  │ │ RND │ │ KB  │ │ CON │ │ RND │ │ HB  │ │ ... │     │
│   └─────┘ └─────┘ └─────┘ └─────┘ └─────┘ └─────┘ └─────┘ └─────┘     │
└───────────────────────────────┬────────────────────────────────────────┘
                                │
                                │  esp_event_loop_run(loop, ticks)
                                ▼
┌────────────────────────────────────────────────────────────────────────┐
│                          UI TASK                                       │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                    Event Handler                                 │   │
│  │  switch (event_id) {                                            │   │
│  │    case KB_KEY:   update_kb_stats(); append_log(); break;       │   │
│  │    case HEARTBEAT: update_hb_stats(); append_log(); break;      │   │
│  │    case RAND:     update_rand_stats(); append_log(); break;     │   │
│  │    case CONSOLE:  append_log(); break;                          │   │
│  │  }                                                              │   │
│  │  ui.dirty = true;                                               │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                        │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                    Render (if dirty)                            │   │
│  │  if (ui.dirty) { render_header(); render_log(); push_sprite(); }│   │
│  └─────────────────────────────────────────────────────────────────┘   │
└────────────────────────────────────────────────────────────────────────┘
```

The key insight is that **event handlers run in the UI task's context** because we call `esp_event_loop_run()` there. This eliminates cross-task display access without requiring mutexes.

---

## Event Loop Setup

### Choosing the Threading Model

ESP-IDF's `esp_event` offers two threading models:

| Model | `task_name` | Dispatch | Use Case |
|-------|-------------|----------|----------|
| Dedicated task | Non-NULL | Automatic | Background processing |
| Manual dispatch | NULL | Call `esp_event_loop_run()` | UI-integrated handlers |

For UI logs, we want **manual dispatch** so that handlers run in the UI task:

```c
esp_event_loop_args_t args = {
    .queue_size = 32,           // Tune based on burst rate
    .task_name = NULL,          // Manual dispatch mode
    .task_priority = 0,         // Ignored
    .task_stack_size = 0,       // Ignored
    .task_core_id = 0,          // Ignored
};

esp_event_loop_handle_t s_loop = NULL;
ESP_ERROR_CHECK(esp_event_loop_create(&args, &s_loop));
```

### Queue Size Considerations

The queue size determines how many events can be buffered before `esp_event_post_to()` fails. Consider:

- **Burst rate** — How many events might arrive before dispatch?
- **Dispatch frequency** — How often does the UI task call `esp_event_loop_run()`?
- **Event payload size** — Larger payloads consume more queue memory

A queue size of 32 handles typical burst scenarios. Monitor drop counts to tune.

---

## Defining Events

### Event Base

Each event source defines a base identifier:

```c
ESP_EVENT_DEFINE_BASE(APP_BUS_EVENT);
```

### Event IDs

Enumerate the event types as `int32_t`:

```c
enum : int32_t {
    EVT_KB_KEY = 1,        // Keyboard key press
    EVT_KB_ACTION = 2,     // Semantic action (nav, enter)
    EVT_RAND = 3,          // Random producer
    EVT_HEARTBEAT = 4,     // Periodic stats
    EVT_CONSOLE_POST = 5,  // REPL-injected message
    EVT_CONSOLE_CLEAR = 6, // Clear log command
};
```

### Event Payloads

Design fixed-size POD structs for each event type:

```c
typedef struct {
    int64_t ts_us;        // Timestamp for ordering/debugging
    uint8_t keynum;       // Which key (1-56)
    uint8_t modifiers;    // Shift, Ctrl, Alt, Fn flags
} kb_key_event_t;

typedef struct {
    int64_t ts_us;
    uint8_t action;       // NavUp, NavDown, Enter, etc.
    uint8_t modifiers;
} kb_action_event_t;

typedef struct {
    int64_t ts_us;
    uint8_t producer_id;  // Which random producer
    uint32_t value;       // The random value
} rand_event_t;

typedef struct {
    int64_t ts_us;
    uint32_t heap_free;   // System stats
    uint32_t dma_free;
} heartbeat_event_t;

typedef struct {
    int64_t ts_us;
    char msg[96];         // REPL message
} console_post_event_t;
```

**Why fixed-size?** `esp_event_post_to()` copies the payload, so:
- No dynamic allocation needed
- Predictable memory usage
- Copy semantics are clear

---

## Producer Tasks

### Keyboard Producer

The keyboard producer scans the matrix and posts events on key edges:

```c
static void keyboard_task(void *arg) {
    cardputer_kb::MatrixScanner kb;
    kb.init();
    
    std::vector<uint8_t> prev_pressed;
    
    while (true) {
        // Scan the matrix
        auto snap = kb.scan();
        const auto &down = snap.pressed_keynums;
        const uint8_t mods = modifiers_from_pressed(down);
        
        // Check for semantic actions (nav bindings)
        const auto *active = cardputer_kb::decode_best(down, bindings, N);
        if (active && is_new_action(active, prev_action)) {
            kb_action_event_t ev = {
                .ts_us = esp_timer_get_time(),
                .action = (uint8_t)active->action,
                .modifiers = mods,
            };
            if (esp_event_post_to(s_loop, APP_BUS_EVENT, EVT_KB_ACTION, 
                                   &ev, sizeof(ev), 0) != ESP_OK) {
                s_post_drops.fetch_add(1);
            }
        }
        
        // Post edge events for non-modifier keys
        for (auto keynum : down) {
            if (contains(prev_pressed, keynum)) continue;  // Not new
            if (is_modifier_keynum(keynum)) continue;       // Skip modifiers
            
            kb_key_event_t ev = {
                .ts_us = esp_timer_get_time(),
                .keynum = keynum,
                .modifiers = mods,
            };
            if (esp_event_post_to(s_loop, APP_BUS_EVENT, EVT_KB_KEY, 
                                   &ev, sizeof(ev), 0) != ESP_OK) {
                s_post_drops.fetch_add(1);
            }
        }
        
        prev_pressed = down;
        vTaskDelay(pdMS_TO_TICKS(10));  // Scan period
    }
}
```

**Key design decisions:**

- **Edge detection** — Only post when a key transitions from up to down
- **Modifier exclusion** — Modifiers are tracked in the `modifiers` field, not as separate events
- **Action vs. key events** — Semantic actions (NavUp) get their own event type
- **Non-blocking post** — Use timeout=0 to never block producers

### Random Producer

Random producers demonstrate multi-producer scenarios:

```c
static void rand_task(void *arg) {
    const uint8_t producer_id = (uint8_t)(uintptr_t)arg;
    
    while (true) {
        rand_event_t ev = {
            .ts_us = esp_timer_get_time(),
            .producer_id = producer_id,
            .value = esp_random(),
        };
        
        if (esp_event_post_to(s_loop, APP_BUS_EVENT, EVT_RAND, 
                               &ev, sizeof(ev), 0) != ESP_OK) {
            s_post_drops.fetch_add(1);
        }
        
        // Random delay to vary event timing
        const uint32_t delay = min_ms + (esp_random() % (max_ms - min_ms + 1));
        vTaskDelay(pdMS_TO_TICKS(delay));
    }
}

// Start multiple producers
for (int i = 0; i < N_RAND_PRODUCERS; i++) {
    xTaskCreate(rand_task, "rand", 4096, (void*)(uintptr_t)i, 5, NULL);
}
```

### Heartbeat Producer

Periodic system stats:

```c
static void heartbeat_task(void *arg) {
    while (true) {
        heartbeat_event_t ev = {
            .ts_us = esp_timer_get_time(),
            .heap_free = esp_get_free_heap_size(),
            .dma_free = heap_caps_get_free_size(MALLOC_CAP_DMA),
        };
        
        esp_event_post_to(s_loop, APP_BUS_EVENT, EVT_HEARTBEAT, 
                          &ev, sizeof(ev), 0);
        
        vTaskDelay(pdMS_TO_TICKS(5000));  // Every 5 seconds
    }
}
```

---

## Event Handler

The handler processes events and updates UI state:

```c
static void event_handler(void *arg, esp_event_base_t base, 
                           int32_t event_id, void *event_data) {
    UiState *ui = (UiState *)arg;
    if (base != APP_BUS_EVENT) return;
    
    switch (event_id) {
    case EVT_KB_KEY: {
        ui->kb_key_events++;
        const kb_key_event_t *ev = (const kb_key_event_t *)event_data;
        
        // Format log line
        char line[128];
        snprintf(line, sizeof(line), "[kb] key=%u mods=0x%02x", 
                 ev->keynum, ev->modifiers);
        lines_append(*ui, line);
        
        // Handle UI controls
        if (ev->keynum == KEY_DEL && ev->modifiers == 0) {
            lines_clear(*ui);  // Del clears log
        }
        if (ev->keynum == KEY_SPACE && (ev->modifiers & MOD_FN)) {
            ui->paused = !ui->paused;  // Fn+Space toggles pause
        }
        break;
    }
    
    case EVT_KB_ACTION: {
        ui->kb_action_events++;
        const kb_action_event_t *ev = (const kb_action_event_t *)event_data;
        
        // Handle scroll controls
        switch ((Action)ev->action) {
        case Action::NavUp:   ui->scrollback++; break;
        case Action::NavDown: ui->scrollback = max(0, ui->scrollback - 1); break;
        case Action::Back:    ui->scrollback = 0; break;  // Jump to tail
        default: break;
        }
        
        // Log the action
        char line[64];
        snprintf(line, sizeof(line), "[kb] action=%s", action_name(ev->action));
        lines_append(*ui, line);
        break;
    }
    
    case EVT_RAND: {
        ui->rand_events++;
        const rand_event_t *ev = (const rand_event_t *)event_data;
        
        char line[80];
        snprintf(line, sizeof(line), "[rnd] p%u val=0x%08x", 
                 ev->producer_id, ev->value);
        lines_append(*ui, line);
        break;
    }
    
    case EVT_HEARTBEAT: {
        ui->heartbeat_events++;
        const heartbeat_event_t *ev = (const heartbeat_event_t *)event_data;
        
        char line[96];
        snprintf(line, sizeof(line), "[hb] heap=%u dma=%u", 
                 ev->heap_free, ev->dma_free);
        lines_append(*ui, line);
        break;
    }
    
    case EVT_CONSOLE_POST: {
        ui->console_posts++;
        const console_post_event_t *ev = (const console_post_event_t *)event_data;
        
        char line[128];
        snprintf(line, sizeof(line), "[con] %s", ev->msg);
        lines_append(*ui, line);
        break;
    }
    
    case EVT_CONSOLE_CLEAR:
        lines_clear(*ui);
        break;
    }
}
```

**Critical rule**: **Handlers must not block**. The handler runs in the UI task's context; blocking would freeze both event processing and display updates.

---

## UI State Management

### State Structure

```c
struct UiState {
    // Display resources
    m5gfx::M5GFX display;
    M5Canvas screen;
    
    // Log buffer
    std::deque<std::string> lines;
    size_t dropped_lines = 0;
    
    // Scroll state
    int scrollback = 0;  // 0 = follow tail, >0 = scroll up
    
    // Control flags
    bool paused = false;
    bool dirty = true;
    
    // Counters for header
    uint32_t kb_key_events = 0;
    uint32_t kb_action_events = 0;
    uint32_t rand_events = 0;
    uint32_t heartbeat_events = 0;
    uint32_t console_posts = 0;
};
```

### Line Buffer Management

```c
static void lines_append(UiState &ui, std::string s) {
    if (ui.paused) return;  // Don't append while paused
    
    ui.lines.push_back(std::move(s));
    
    // Trim old lines to prevent memory growth
    while (ui.lines.size() > MAX_LINES) {
        ui.lines.pop_front();
        ui.dropped_lines++;
    }
    
    ui.dirty = true;  // Mark for redraw
}

static void lines_clear(UiState &ui) {
    ui.lines.clear();
    ui.dropped_lines = 0;
    ui.scrollback = 0;
    ui.dirty = true;
}
```

### Dirty Flag Pattern

The dirty flag prevents unnecessary redraws:

```c
static void render_ui(UiState &ui) {
    if (!ui.dirty) return;  // Nothing changed
    ui.dirty = false;
    
    // ... render header, log lines, etc.
    
    ui.screen.pushSprite(0, 0);
}
```

This optimization is critical for embedded displays where redraw is expensive.

---

## UI Rendering

### Layout

```
┌────────────────────────────────────────────────────────────────┐
│  0030: esp_event bus + esp_console (Cardputer)                │ Row 0
├────────────────────────────────────────────────────────────────┤
│  kb=42 act=12 con=5 rand=100 hb=8 drops=0 heap=200k dma=80k  │ Row 1
├────────────────────────────────────────────────────────────────┤
│  Fn+W/S scroll  Fn+Space pause  Del clear  (console: evt ...) │ Row 2
├────────────────────────────────────────────────────────────────┤
│  [kb] key=15 mods=0x00                                        │ Row 3
│  [kb] action=Enter                                            │ Row 4
│  [rnd] p0 val=0x12345678                                      │ Row 5
│  [hb] heap=204800 dma=81920                                   │ Row 6
│  [con] hello from REPL                                        │ Row 7
│  ...                                                          │ ...
└────────────────────────────────────────────────────────────────┘
```

### Render Implementation

```c
static void render_ui(UiState &ui) {
    if (!ui.dirty) return;
    ui.dirty = false;
    
    ui.screen.fillScreen(TFT_BLACK);
    ui.screen.setTextSize(1, 1);
    
    const int line_h = ui.screen.fontHeight() + 1;
    const int header_rows = 3;
    
    // Row 0: Title
    ui.screen.setTextColor(TFT_WHITE, TFT_BLACK);
    ui.screen.drawString("0030: esp_event + esp_console", 2, 0);
    
    // Row 1: Stats
    char hdr[180];
    snprintf(hdr, sizeof(hdr),
             "kb=%u act=%u con=%u rand=%u hb=%u drops=%u heap=%u",
             ui.kb_key_events, ui.kb_action_events, ui.console_posts,
             ui.rand_events, ui.heartbeat_events, ui.post_drops,
             esp_get_free_heap_size());
    ui.screen.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    ui.screen.drawString(hdr, 2, line_h);
    
    // Row 2: Controls hint
    ui.screen.setTextColor(TFT_DARKGREY, TFT_BLACK);
    ui.screen.drawString("Fn+W/S scroll  Fn+Space pause  Del clear", 2, 2*line_h);
    
    // Log area
    const int y0 = header_rows * line_h;
    const int visible_rows = (ui.screen.height() - y0) / line_h;
    
    // Calculate visible window with scrollback
    int start = (int)ui.lines.size() - visible_rows - ui.scrollback;
    if (start < 0) start = 0;
    
    ui.screen.setTextColor(TFT_GREEN, TFT_BLACK);
    for (int i = 0; i < visible_rows; i++) {
        int idx = start + i;
        if (idx >= (int)ui.lines.size()) break;
        ui.screen.drawString(ui.lines[idx].c_str(), 2, y0 + i * line_h);
    }
    
    // Paused overlay
    if (ui.paused) {
        ui.screen.fillRect(0, ui.screen.height() - line_h - 2, 
                           ui.screen.width(), line_h + 2, TFT_DARKGREY);
        ui.screen.setTextColor(TFT_WHITE, TFT_DARKGREY);
        ui.screen.drawString("PAUSED (Fn+Space to resume)", 2, 
                             ui.screen.height() - line_h - 1);
    }
    
    ui.screen.pushSprite(0, 0);
}
```

### Keyboard Controls

| Control | Action |
|---------|--------|
| Fn+W | Scroll up one line |
| Fn+S | Scroll down one line |
| Fn+A | Scroll up 5 lines |
| Fn+D | Scroll down 5 lines |
| Fn+Space | Toggle pause/resume |
| Del | Clear log |
| Fn+1 | Jump to tail (live mode) |

---

## REPL Injection

### Console Setup

The REPL runs on a separate task but posts to the same event bus:

```c
static void console_start(void) {
    esp_console_repl_t *repl = NULL;
    
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "bus> ";
    
    // Create REPL based on configured console backend
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    esp_console_dev_usb_serial_jtag_config_t hw = 
        ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    esp_console_new_repl_usb_serial_jtag(&hw, &repl_config, &repl);
#elif CONFIG_ESP_CONSOLE_UART_DEFAULT
    esp_console_dev_uart_config_t hw = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    esp_console_new_repl_uart(&hw, &repl_config, &repl);
#endif
    
    // Register commands
    esp_console_register_help_command();
    
    esp_console_cmd_t cmd = {
        .command = "evt",
        .help = "Event bus control: evt post|spam|clear|status|monitor",
        .func = &cmd_evt,
    };
    esp_console_cmd_register(&cmd);
    
    esp_console_start_repl(repl);
}
```

### Command Handler

```c
static int cmd_evt(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: evt post <text> | evt spam <n> | evt clear | "
               "evt status | evt monitor on|off\n");
        return 1;
    }
    
    // Post a message
    if (strcmp(argv[1], "post") == 0) {
        if (argc < 3) {
            printf("usage: evt post <text>\n");
            return 1;
        }
        
        console_post_event_t ev = {.ts_us = esp_timer_get_time()};
        strlcpy(ev.msg, argv[2], sizeof(ev.msg));
        
        esp_err_t err = esp_event_post_to(s_loop, APP_BUS_EVENT, 
                                           EVT_CONSOLE_POST, &ev, sizeof(ev), 0);
        if (err != ESP_OK) {
            printf("post failed: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("ok\n");
        return 0;
    }
    
    // Spam test
    if (strcmp(argv[1], "spam") == 0) {
        int n = (argc >= 3) ? atoi(argv[2]) : 10;
        for (int i = 0; i < n; i++) {
            console_post_event_t ev = {.ts_us = esp_timer_get_time()};
            snprintf(ev.msg, sizeof(ev.msg), "spam %d/%d", i+1, n);
            esp_event_post_to(s_loop, APP_BUS_EVENT, 
                              EVT_CONSOLE_POST, &ev, sizeof(ev), 0);
        }
        printf("ok (%d)\n", n);
        return 0;
    }
    
    // Clear log
    if (strcmp(argv[1], "clear") == 0) {
        esp_event_post_to(s_loop, APP_BUS_EVENT, EVT_CONSOLE_CLEAR, NULL, 0, 0);
        printf("ok\n");
        return 0;
    }
    
    // Status
    if (strcmp(argv[1], "status") == 0) {
        printf("drops=%u\n", s_post_drops.load());
        return 0;
    }
    
    return 1;
}
```

### REPL Commands Reference

| Command | Description | Example |
|---------|-------------|---------|
| `evt post <msg>` | Post message to bus | `evt post hello` |
| `evt spam <n>` | Post N rapid messages | `evt spam 50` |
| `evt clear` | Clear the on-screen log | `evt clear` |
| `evt status` | Show post drop count | `evt status` |
| `evt monitor on/off` | Mirror events to console | `evt monitor on` |

### Console Monitor

To observe events on the serial console as well as the LCD, add a monitor queue:

```c
static std::atomic<bool> s_monitor_enabled{false};
static QueueHandle_t s_monitor_q = NULL;

static void monitor_enqueue(const char *line) {
    if (!s_monitor_enabled.load()) return;
    
    monitor_line_t m = {};
    strlcpy(m.line, line, sizeof(m.line));
    xQueueSend(s_monitor_q, &m, 0);  // Non-blocking
}

static void monitor_task(void *arg) {
    monitor_line_t m;
    while (true) {
        if (xQueueReceive(s_monitor_q, &m, portMAX_DELAY) == pdTRUE) {
            printf("%s\n", m.line);
        }
    }
}
```

Call `monitor_enqueue()` from the event handler to mirror each event line.

---

## UI Task Main Loop

The UI task ties everything together:

```c
static void ui_task(void *arg) {
    // Create event loop (manual dispatch)
    esp_event_loop_args_t args = {
        .queue_size = 32,
        .task_name = NULL,
    };
    esp_event_loop_create(&args, &s_loop);
    
    // Initialize display
    UiState ui;
    ui.display.init();
    ui.screen.createSprite(ui.display.width(), ui.display.height());
    
    // Register handler
    esp_event_handler_register_with(s_loop, APP_BUS_EVENT, ESP_EVENT_ANY_ID, 
                                     &event_handler, &ui);
    
    // Start producer tasks
    xTaskCreate(keyboard_task, "kb", 4096, NULL, 8, NULL);
    xTaskCreate(heartbeat_task, "hb", 4096, NULL, 5, NULL);
    for (int i = 0; i < N_RAND; i++) {
        xTaskCreate(rand_task, "rnd", 4096, (void*)(uintptr_t)i, 5, NULL);
    }
    
    // Main loop
    const int64_t frame_us = 1000000 / TARGET_FPS;
    int64_t last_frame = 0;
    
    while (true) {
        // Dispatch pending events (handlers run here)
        esp_event_loop_run(s_loop, pdMS_TO_TICKS(5));
        
        // Render at fixed cadence
        int64_t now = esp_timer_get_time();
        if (now - last_frame >= frame_us) {
            last_frame = now;
            render_ui(ui);
        }
        
        vTaskDelay(1);
    }
}

extern "C" void app_main(void) {
    xTaskCreate(ui_task, "ui", 8192, NULL, 10, NULL);
}
```

---

## Configuration Options

Both demos expose `menuconfig` options for tuning:

```kconfig
menu "Tutorial Config"
    config EVENT_QUEUE_SIZE
        int "Event queue size"
        default 32
        
    config KB_SCAN_PERIOD_MS
        int "Keyboard scan period (ms)"
        default 10
        
    config HEARTBEAT_PERIOD_MS
        int "Heartbeat period (ms)"
        default 5000
        
    config RAND_PRODUCERS
        int "Number of random producers"
        default 2
        
    config RAND_MIN_PERIOD_MS
        int "Random producer min delay (ms)"
        default 500
        
    config RAND_MAX_PERIOD_MS
        int "Random producer max delay (ms)"
        default 2000
        
    config UI_TARGET_FPS
        int "Target UI frames per second"
        default 30
        
    config UI_MAX_LINES
        int "Maximum log lines to keep"
        default 100
        
    config DISPATCH_TICKS
        int "Max ticks per esp_event_loop_run() call"
        default 5
endmenu
```

---

## Troubleshooting

### Events Not Appearing

| Symptom | Cause | Solution |
|---------|-------|----------|
| No events on screen | Handler not registered | Check `esp_event_handler_register_with()` return |
| No events on screen | Dispatch not called | Ensure `esp_event_loop_run()` in UI loop |
| Events drop during bursts | Queue too small | Increase `EVENT_QUEUE_SIZE` |
| Display frozen | Handler blocking | Remove blocking calls from handler |

### Post Drops

Monitor the `s_post_drops` counter. If it grows:
1. Increase queue size
2. Increase dispatch frequency
3. Reduce producer rate
4. Check for handler blocking

### REPL Not Working

| Symptom | Cause | Solution |
|---------|-------|----------|
| No `bus>` prompt | Console not enabled | Check `CONFIG_ESP_CONSOLE_*` |
| Commands ignored | `evt` not registered | Verify `esp_console_cmd_register()` |
| Output garbled | Wrong baud rate | Match host terminal baud |

### Memory Growth

If heap decreases over time:
- Check `UI_MAX_LINES` is set appropriately
- Verify lines are being trimmed
- Look for string allocation leaks

---

## Build and Flash

```bash
# Build the basic demo
cd esp32-s3-m5/0028-cardputer-esp-event-demo
idf.py set-target esp32s3
idf.py build flash monitor

# Build the REPL + event bus demo
cd esp32-s3-m5/0030-cardputer-console-eventbus
idf.py set-target esp32s3
idf.py build flash monitor
```

After flashing 0030, open the serial monitor and try:
```
bus> evt post "Hello from REPL"
bus> evt spam 10
bus> evt clear
bus> evt status
```

---

## Pattern Extensions

### Adding New Event Types

1. Add event ID to the enum
2. Define payload struct
3. Add case to handler switch
4. Create producer task or REPL command

### Protobuf Encoding

For binary-efficient storage or transmission, encode events as protobuf. See the `evt pb on/off/last` commands in 0030 for an example using nanopb.

### WebSocket Streaming

Bridge events to WebSocket for browser debugging:
1. Register a second handler that enqueues to a WebSocket queue
2. Send events as JSON or binary frames
3. Browser receives real-time event stream

---

## Summary

The esp_event UI log pattern provides a clean separation between event producers and the UI consumer. Key takeaways:

1. **Use manual dispatch** (`task_name=NULL`) to keep handlers in the UI task
2. **Design fixed-size payloads** for efficient copying
3. **Never block in handlers** — update state and set dirty flag
4. **Use the dirty flag pattern** to minimize redraws
5. **REPL injection** enables powerful debugging without code changes
6. **Monitor post drops** to tune queue size

This pattern scales to complex firmware with many event sources while maintaining a responsive, single-owner display.

---

## References

- [ESP-IDF Event Loop Library](https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32s3/api-reference/system/esp_event.html)
- [ESP-IDF Console Component](https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32s3/api-reference/system/console.html)
- `0028-cardputer-esp-event-demo/` — Basic event bus demo
- `0030-cardputer-console-eventbus/` — REPL + event bus integration
