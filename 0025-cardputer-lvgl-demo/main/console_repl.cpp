/*
 * Cardputer LVGL demo: host-scriptable control plane via ESP-IDF `esp_console`.
 *
 * Important: the esp_console REPL runs in its own task. It must not mutate LVGL directly.
 * Commands should enqueue CtrlEvents to be executed on the main UI thread (app_main loop).
 */

#include "console_repl.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "sdkconfig.h"

#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include "control_plane.h"

static const char *TAG = "lvgl_console";

static QueueHandle_t s_ctrl_q = nullptr;

static bool ensure_ctrl_queue(void) {
    if (s_ctrl_q) return true;
    printf("ERR: console not initialized (no ctrl queue)\n");
    return false;
}

static int cmd_heap(int, char **) {
    const uint32_t heap_free = esp_get_free_heap_size();
    printf("heap_free=%" PRIu32 "\n", heap_free);
    return 0;
}

static int cmd_menu(int, char **) {
    if (!ensure_ctrl_queue()) return 1;

    CtrlEvent ev{};
    ev.type = CtrlType::OpenMenu;
    if (!ctrl_send(s_ctrl_q, ev, pdMS_TO_TICKS(250))) {
        printf("ERR: menu busy (queue full)\n");
        return 1;
    }
    printf("OK\n");
    return 0;
}

static int cmd_basics(int, char **) {
    if (!ensure_ctrl_queue()) return 1;

    CtrlEvent ev{};
    ev.type = CtrlType::OpenBasics;
    if (!ctrl_send(s_ctrl_q, ev, pdMS_TO_TICKS(250))) {
        printf("ERR: basics busy (queue full)\n");
        return 1;
    }
    printf("OK\n");
    return 0;
}

static int cmd_pomodoro(int, char **) {
    if (!ensure_ctrl_queue()) return 1;

    CtrlEvent ev{};
    ev.type = CtrlType::OpenPomodoro;
    if (!ctrl_send(s_ctrl_q, ev, pdMS_TO_TICKS(250))) {
        printf("ERR: pomodoro busy (queue full)\n");
        return 1;
    }
    printf("OK\n");
    return 0;
}

static int cmd_setmins(int argc, char **argv) {
    if (!ensure_ctrl_queue()) return 1;
    if (argc < 2 || !argv[1] || argv[1][0] == '\0') {
        printf("ERR: usage: setmins <minutes>\n");
        return 1;
    }

    char *end = nullptr;
    long minutes = strtol(argv[1], &end, 10);
    if (!end || *end != '\0') {
        printf("ERR: invalid minutes: %s\n", argv[1]);
        return 1;
    }
    if (minutes < 1) minutes = 1;
    if (minutes > 99) minutes = 99;

    CtrlEvent ev{};
    ev.type = CtrlType::PomodoroSetMinutes;
    ev.arg = (int32_t)minutes;
    if (!ctrl_send(s_ctrl_q, ev, pdMS_TO_TICKS(250))) {
        printf("ERR: setmins busy (queue full)\n");
        return 1;
    }

    printf("OK minutes=%ld\n", minutes);
    return 0;
}

static int cmd_screenshot(int, char **) {
    if (!ensure_ctrl_queue()) return 1;

    CtrlEvent ev{};
    ev.type = CtrlType::ScreenshotPngToUsbSerialJtag;
    ev.reply_task = xTaskGetCurrentTaskHandle();

    if (!ctrl_send(s_ctrl_q, ev, pdMS_TO_TICKS(250))) {
        printf("ERR: screenshot busy (queue full)\n");
        return 1;
    }

    // Block until the UI thread completes the capture and notifies us.
    // The UI thread sends the PNG bytes over USB-Serial/JTAG during this wait.
    uint32_t value = 0;
    if (xTaskNotifyWait(0, UINT32_MAX, &value, pdMS_TO_TICKS(15000)) != pdTRUE) {
        printf("ERR: screenshot timeout\n");
        return 1;
    }
    if (value == 0) {
        printf("ERR: screenshot failed\n");
        return 1;
    }

    printf("OK len=%" PRIu32 "\n", value);
    return 0;
}

static void console_register_commands(void) {
    esp_console_cmd_t cmd = {};

    cmd = {};
    cmd.command = "heap";
    cmd.help = "Print free heap bytes";
    cmd.func = &cmd_heap;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "menu";
    cmd.help = "Open the LVGL demo menu screen";
    cmd.func = &cmd_menu;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "basics";
    cmd.help = "Open the Basics demo screen";
    cmd.func = &cmd_basics;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "pomodoro";
    cmd.help = "Open the Pomodoro demo screen";
    cmd.func = &cmd_pomodoro;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "setmins";
    cmd.help = "Set Pomodoro duration in minutes (1-99)";
    cmd.func = &cmd_setmins;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "screenshot";
    cmd.help = "Capture current display as framed PNG over USB-Serial/JTAG";
    cmd.func = &cmd_screenshot;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void console_start(QueueHandle_t ctrl_queue) {
    s_ctrl_q = ctrl_queue;

#if !CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    ESP_LOGW(TAG, "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG is disabled; no REPL started");
    return;
#else
    esp_console_repl_t *repl = nullptr;

    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "lvgl> ";
    repl_config.task_stack_size = 4096;

    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();

    esp_err_t err = esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_new_repl_usb_serial_jtag failed: %s", esp_err_to_name(err));
        return;
    }

    esp_console_register_help_command();
    console_register_commands();

    err = esp_console_start_repl(repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_start_repl failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "esp_console started over USB-Serial/JTAG (commands: help, heap, menu, basics, pomodoro, setmins, screenshot)");
#endif
}
