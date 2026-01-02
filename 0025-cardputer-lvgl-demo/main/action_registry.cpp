#include "action_registry.h"

#include <string.h>

namespace {

constexpr Action kActions[] = {
    {
        .id = ActionId::OpenMenu,
        .command = "menu",
        .title = "Open Menu",
        .keywords = "menu home demos",
    },
    {
        .id = ActionId::OpenBasics,
        .command = "basics",
        .title = "Open Basics",
        .keywords = "basics text textarea",
    },
    {
        .id = ActionId::OpenPomodoro,
        .command = "pomodoro",
        .title = "Open Pomodoro",
        .keywords = "pomodoro timer",
    },
    {
        .id = ActionId::OpenConsole,
        .command = "console",
        .title = "Open Console",
        .keywords = "console repl split",
    },
    {
        .id = ActionId::OpenSystemMonitor,
        .command = "sysmon",
        .title = "Open System Monitor",
        .keywords = "sysmon system monitor heap dma fps",
    },
    {
        .id = ActionId::PomodoroSet15,
        .command = nullptr,
        .title = "Pomodoro: Set minutes 15",
        .keywords = "pomodoro setmins 15",
    },
    {
        .id = ActionId::PomodoroSet25,
        .command = nullptr,
        .title = "Pomodoro: Set minutes 25",
        .keywords = "pomodoro setmins 25",
    },
    {
        .id = ActionId::PomodoroSet50,
        .command = nullptr,
        .title = "Pomodoro: Set minutes 50",
        .keywords = "pomodoro setmins 50",
    },
    {
        .id = ActionId::Screenshot,
        .command = "screenshot",
        .title = "Screenshot (USB-Serial/JTAG)",
        .keywords = "screenshot png capture",
    },
};

constexpr size_t kActionCount = sizeof(kActions) / sizeof(kActions[0]);

} // namespace

const Action *action_registry_actions(size_t *out_count) {
    if (out_count) *out_count = kActionCount;
    return kActions;
}

const Action *action_registry_find_by_command(const char *command) {
    if (!command || command[0] == '\0') return nullptr;
    for (size_t i = 0; i < kActionCount; i++) {
        const char *c = kActions[i].command;
        if (!c) continue;
        if (strcmp(c, command) == 0) return &kActions[i];
    }
    return nullptr;
}

bool action_registry_to_ctrl_event(ActionId id, CtrlEvent *out, TaskHandle_t reply_task) {
    if (!out) return false;
    CtrlEvent ev{};

    switch (id) {
    case ActionId::OpenMenu: ev.type = CtrlType::OpenMenu; break;
    case ActionId::OpenBasics: ev.type = CtrlType::OpenBasics; break;
    case ActionId::OpenPomodoro: ev.type = CtrlType::OpenPomodoro; break;
    case ActionId::OpenConsole: ev.type = CtrlType::OpenSplitConsole; break;
    case ActionId::OpenSystemMonitor: ev.type = CtrlType::OpenSystemMonitor; break;
    case ActionId::PomodoroSet15:
        ev.type = CtrlType::PomodoroSetMinutes;
        ev.arg = 15;
        break;
    case ActionId::PomodoroSet25:
        ev.type = CtrlType::PomodoroSetMinutes;
        ev.arg = 25;
        break;
    case ActionId::PomodoroSet50:
        ev.type = CtrlType::PomodoroSetMinutes;
        ev.arg = 50;
        break;
    case ActionId::Screenshot: ev.type = CtrlType::ScreenshotPngToUsbSerialJtag; break;
    }

    ev.reply_task = reply_task;
    *out = ev;
    return true;
}

bool action_registry_enqueue(QueueHandle_t ctrl_q, ActionId id, TickType_t wait, TaskHandle_t reply_task) {
    CtrlEvent ev{};
    if (!action_registry_to_ctrl_event(id, &ev, reply_task)) return false;
    return ctrl_send(ctrl_q, ev, wait);
}

bool action_registry_enqueue_by_command(QueueHandle_t ctrl_q, const char *command, TickType_t wait) {
    const Action *a = action_registry_find_by_command(command);
    if (!a) return false;
    return action_registry_enqueue(ctrl_q, a->id, wait);
}
