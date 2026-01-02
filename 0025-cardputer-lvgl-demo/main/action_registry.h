#pragma once

#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "control_plane.h"

enum class ActionId : uint8_t {
    OpenMenu = 1,
    OpenBasics = 2,
    OpenPomodoro = 3,
    OpenConsole = 4,
    PomodoroSet15 = 5,
    PomodoroSet25 = 6,
    PomodoroSet50 = 7,
    Screenshot = 8,
};

struct Action {
    ActionId id{};
    const char *command = nullptr;  // esp_console command name (e.g. "menu")
    const char *title = nullptr;    // Palette display title (e.g. "Open Menu")
    const char *keywords = nullptr; // Search terms (space-separated)
};

// Returns a stable pointer for the lifetime of the program.
const Action *action_registry_actions(size_t *out_count);

// Returns nullptr if not found.
const Action *action_registry_find_by_command(const char *command);

// Convert an action into a CtrlEvent (suitable for enqueuing to the UI thread).
// If the action requires arguments (e.g. PomodoroSetMinutes presets), they're embedded into CtrlEvent.arg.
bool action_registry_to_ctrl_event(ActionId id, CtrlEvent *out, TaskHandle_t reply_task = nullptr);

// Convenience: lookup by command name and enqueue.
bool action_registry_enqueue_by_command(QueueHandle_t ctrl_q, const char *command, TickType_t wait = 0);

// Convenience: enqueue by ActionId.
bool action_registry_enqueue(QueueHandle_t ctrl_q, ActionId id, TickType_t wait = 0, TaskHandle_t reply_task = nullptr);

