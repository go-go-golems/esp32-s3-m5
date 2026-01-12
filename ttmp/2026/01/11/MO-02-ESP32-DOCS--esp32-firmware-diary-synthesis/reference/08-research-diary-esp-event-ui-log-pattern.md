---
Title: 'Research Diary: esp_event UI Log Pattern'
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - esp32
    - firmware
    - documentation
    - esp_event
    - ui
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0028-cardputer-esp-event-demo/main/app_main.cpp
      Note: Basic esp_event demo with keyboard + random producers
    - Path: esp32-s3-m5/0030-cardputer-console-eventbus/main/app_main.cpp
      Note: esp_event demo with esp_console REPL integration
ExternalSources: []
Summary: 'Research diary documenting investigation of the esp_event UI log pattern.'
LastUpdated: 2026-01-12T12:00:00-05:00
WhatFor: 'Capture research trail for the esp_event UI log guide.'
WhenToUse: 'Reference when writing or updating esp_event documentation.'
---

# Research Diary: esp_event UI Log Pattern

## Goal

Document the research process for understanding the esp_event-driven UI log pattern: how multiple producer tasks post events to a central bus, and how a UI task dispatches and renders them.

## Step 1: Review project diaries

Read the implementation diaries for 0028 and 0030 to understand the design evolution.

### Key insights from 0028 diary

**Threading model decision:**
The critical choice is WHERE handlers run. Options:
1. Dedicated event loop task (`task_name` set) — handlers run on that task
2. Manual dispatch (`task_name=NULL`) — call `esp_event_loop_run()` from any task

The demos chose option 2: **the UI task dispatches events**. This keeps display ownership in one task and avoids cross-task rendering issues.

**Event loop creation:**

```c
esp_event_loop_args_t args = {
    .queue_size = 32,
    .task_name = NULL,  // UI task dispatches
    ...
};
esp_event_loop_create(&args, &s_loop);
```

**Dispatch call:**

```c
// In UI loop
esp_event_loop_run(s_loop, ticks_budget);
render_if_dirty();
```

### Key insights from 0030 diary

**REPL integration:**
Adding `esp_console` introduces another event producer. The REPL command handler posts events using the same `esp_event_post_to()` pattern:

```c
static int cmd_evt(int argc, char **argv) {
    demo_console_post_event_t ev = {...};
    esp_event_post_to(s_loop, BUS_EVENT, EVT_CONSOLE_POST, &ev, sizeof(ev), 0);
}
```

**Console monitor:**
To observe events on the serial console (in addition to the LCD), 0030 adds a monitor queue:
- Event handler enqueues formatted lines (non-blocking)
- A low-priority monitor task drains the queue and prints

This avoids blocking the event handler.

## Step 2: Analyze event taxonomy

Read both `app_main.cpp` files to understand the event type design.

### Event base and IDs

```c
ESP_EVENT_DEFINE_BASE(CARDPUTER_DEMO_EVENT);

enum : int32_t {
    EVT_KB_KEY = 1,      // Raw keyboard key press
    EVT_KB_ACTION = 2,   // Semantic action (NavUp, etc.)
    EVT_RAND = 3,        // Random producer event
    EVT_HEARTBEAT = 4,   // Periodic system stats
    EVT_CONSOLE_POST = 5, // REPL-injected message
    EVT_CONSOLE_CLEAR = 6, // Clear the log
};
```

### Event payloads

Each event type has a fixed-size struct:

```c
typedef struct {
    int64_t ts_us;
    uint8_t keynum;
    uint8_t modifiers;
} demo_kb_key_event_t;

typedef struct {
    int64_t ts_us;
    uint8_t action;
    uint8_t modifiers;
} demo_kb_action_event_t;

typedef struct {
    int64_t ts_us;
    uint32_t heap_free;
    uint32_t dma_free;
} demo_heartbeat_event_t;
```

**Why fixed-size?** `esp_event_post_to()` copies the data, so dynamic allocation would be wasteful. Fixed structs are POD-friendly and avoid heap churn.

## Step 3: Analyze producer tasks

### Keyboard producer

```c
static void keyboard_task(void *arg) {
    cardputer_kb::MatrixScanner kb;
    kb.init();
    
    std::vector<uint8_t> prev_pressed;
    
    while (true) {
        auto snap = kb.scan();
        
        // Post action events (nav bindings)
        const auto *active = decode_best(...);
        if (active && is_new_action) {
            demo_kb_action_event_t ev = {...};
            esp_event_post_to(s_loop, EVENT_BASE, EVT_KB_ACTION, &ev, sizeof(ev), 0);
        }
        
        // Post key events (edges only, excluding modifiers)
        for (auto keynum : snap.pressed_keynums) {
            if (is_new_press && !is_modifier) {
                demo_kb_key_event_t ev = {...};
                esp_event_post_to(s_loop, EVENT_BASE, EVT_KB_KEY, &ev, sizeof(ev), 0);
            }
        }
        
        prev_pressed = snap.pressed_keynums;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

### Random producer

```c
static void rand_task(void *arg) {
    const uint8_t producer_id = (uint8_t)(uintptr_t)arg;
    
    while (true) {
        demo_rand_event_t ev = {
            .ts_us = esp_timer_get_time(),
            .producer_id = producer_id,
            .value = esp_random(),
        };
        esp_event_post_to(s_loop, EVENT_BASE, EVT_RAND, &ev, sizeof(ev), 0);
        
        // Random delay between posts
        vTaskDelay(pdMS_TO_TICKS(rand_between(min, max)));
    }
}
```

### Heartbeat producer

```c
static void heartbeat_task(void *arg) {
    while (true) {
        demo_heartbeat_event_t ev = {
            .ts_us = esp_timer_get_time(),
            .heap_free = esp_get_free_heap_size(),
            .dma_free = heap_caps_get_free_size(MALLOC_CAP_DMA),
        };
        esp_event_post_to(s_loop, EVENT_BASE, EVT_HEARTBEAT, &ev, sizeof(ev), 0);
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

## Step 4: Analyze the event handler

The handler runs in the UI task context (because that's where `esp_event_loop_run()` is called):

```c
static void demo_event_handler(void *arg, esp_event_base_t base, 
                                int32_t event_id, void *event_data) {
    UiState *ui = (UiState *)arg;
    
    switch (event_id) {
    case EVT_KB_KEY:
        ui->kb_key_events++;
        format_and_append_line(ui, event_data);
        handle_ui_controls(ui, event_data);  // Del=clear, Fn+Space=pause
        break;
        
    case EVT_KB_ACTION:
        ui->kb_action_events++;
        format_and_append_line(ui, event_data);
        handle_scroll_controls(ui, event_data);  // NavUp/Down
        break;
        
    case EVT_RAND:
        ui->rand_events++;
        format_and_append_line(ui, event_data);
        break;
        
    case EVT_HEARTBEAT:
        ui->heartbeat_events++;
        format_and_append_line(ui, event_data);
        break;
    }
}
```

**Key principle**: The handler updates state and sets `dirty=true`, but doesn't render. Rendering happens separately at a fixed cadence.

## Step 5: Analyze the UI loop

```c
static void ui_task(void *arg) {
    // Create event loop (no dedicated task)
    esp_event_loop_create(&args, &s_loop);
    
    // Initialize display
    ui.display.init();
    ui.screen.createSprite(w, h);
    
    // Register handler
    esp_event_handler_register_with(s_loop, EVENT_BASE, ESP_EVENT_ANY_ID, 
                                     &demo_event_handler, &ui);
    
    // Start producer tasks
    xTaskCreate(keyboard_task, ...);
    xTaskCreate(heartbeat_task, ...);
    for (i = 0; i < N; i++) xTaskCreate(rand_task, ...);
    
    // Main loop
    while (true) {
        // Dispatch events (handlers run here!)
        esp_event_loop_run(s_loop, DISPATCH_TICKS);
        
        // Render at fixed FPS
        if (time_for_frame()) {
            render_ui(ui);
        }
        
        vTaskDelay(1);
    }
}
```

## Step 6: Analyze REPL injection (0030)

### Console setup

```c
static void console_start(void) {
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "bus> ";
    
    // Create REPL based on console backend
    esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl);
    
    // Register custom command
    esp_console_cmd_t cmd = {.command = "evt", .func = &cmd_evt};
    esp_console_cmd_register(&cmd);
    
    esp_console_start_repl(repl);
}
```

### Command handler

```c
static int cmd_evt(int argc, char **argv) {
    if (strcmp(argv[1], "post") == 0) {
        demo_console_post_event_t ev = {...};
        strlcpy(ev.msg, argv[2], sizeof(ev.msg));
        esp_event_post_to(s_loop, EVENT_BASE, EVT_CONSOLE_POST, &ev, sizeof(ev), 0);
        return 0;
    }
    
    if (strcmp(argv[1], "clear") == 0) {
        esp_event_post_to(s_loop, EVENT_BASE, EVT_CONSOLE_CLEAR, NULL, 0, 0);
        return 0;
    }
    
    if (strcmp(argv[1], "spam") == 0) {
        int n = atoi(argv[2]);
        for (int i = 0; i < n; i++) {
            demo_console_post_event_t ev = {...};
            snprintf(ev.msg, sizeof(ev.msg), "spam %d/%d", i+1, n);
            esp_event_post_to(...);
        }
        return 0;
    }
    
    return 1;
}
```

### Console commands

| Command | Effect |
|---------|--------|
| `evt post hello` | Posts message to event bus |
| `evt spam 10` | Posts 10 rapid messages |
| `evt clear` | Clears the on-screen log |
| `evt status` | Shows post drop count |
| `evt monitor on/off` | Toggle console event mirror |

## Step 7: Analyze UI state and rendering

### UI state structure

```c
struct UiState {
    m5gfx::M5GFX display;
    M5Canvas screen;
    
    std::deque<std::string> lines;  // Log lines
    size_t dropped_lines;            // Trimmed from front
    int scrollback;                  // 0=follow tail
    
    bool paused;                     // Stop appending
    bool dirty;                      // Need redraw
    
    // Event counters
    uint32_t kb_key_events;
    uint32_t kb_action_events;
    uint32_t rand_events;
    uint32_t heartbeat_events;
};
```

### Line management

```c
static void lines_append(UiState &ui, std::string s) {
    if (ui.paused) return;  // Don't append while paused
    ui.lines.push_back(std::move(s));
    
    // Trim old lines
    while (ui.lines.size() > MAX_LINES) {
        ui.lines.pop_front();
        ui.dropped_lines++;
    }
    
    ui.dirty = true;
}
```

### Render function

```c
static void render_ui(UiState &ui) {
    if (!ui.dirty) return;
    ui.dirty = false;
    
    ui.screen.fillScreen(TFT_BLACK);
    
    // Header with stats
    draw_header(ui);
    
    // Log lines with scrollback
    int start = lines.size() - visible_rows - scrollback;
    for (int i = start; i < start + visible_rows; i++) {
        draw_line(ui, i);
    }
    
    // Paused indicator
    if (ui.paused) draw_pause_overlay(ui);
    
    ui.screen.pushSprite(0, 0);
}
```

## Quick Reference

### esp_event APIs used

| API | Purpose |
|-----|---------|
| `esp_event_loop_create()` | Create custom event loop |
| `esp_event_handler_register_with()` | Register handler for event types |
| `esp_event_post_to()` | Post event from producer |
| `esp_event_loop_run()` | Dispatch pending events |
| `ESP_EVENT_DEFINE_BASE()` | Define event base identifier |

### Threading model

```
┌─────────────┐   ┌─────────────┐   ┌─────────────┐
│ Keyboard    │   │ Heartbeat   │   │ Random      │
│ Task        │   │ Task        │   │ Tasks (N)   │
└──────┬──────┘   └──────┬──────┘   └──────┬──────┘
       │                 │                 │
       │   esp_event_post_to()             │
       ▼                 ▼                 ▼
┌────────────────────────────────────────────────┐
│              Event Queue                       │
└────────────────────────┬───────────────────────┘
                         │
                         │ esp_event_loop_run()
                         ▼
┌────────────────────────────────────────────────┐
│                UI Task                         │
│                                                │
│  ┌────────────────────┐                        │
│  │  Event Handler     │ ◄── Runs HERE         │
│  │  (update state)    │                        │
│  └────────────────────┘                        │
│                                                │
│  ┌────────────────────┐                        │
│  │  Render UI         │                        │
│  │  (if dirty)        │                        │
│  └────────────────────┘                        │
└────────────────────────────────────────────────┘
```

### Keyboard controls

| Control | Action |
|---------|--------|
| Fn+W/S | Scroll up/down |
| Fn+Space | Pause/resume log |
| Del | Clear log |
| Fn+1 | Jump to tail |
