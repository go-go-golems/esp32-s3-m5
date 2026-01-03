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
#include <string.h>
#include <ctype.h>

#include <vector>

#include "sdkconfig.h"

#include "lvgl.h"

#include "action_registry.h"
#include "sdcard_fatfs.h"

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

static int cmd_waitms(int argc, char **argv) {
    if (argc < 2 || !argv[1] || argv[1][0] == '\0') {
        printf("ERR: usage: waitms <ms>\n");
        return 1;
    }
    char *end = nullptr;
    long ms = strtol(argv[1], &end, 10);
    if (!end || *end != '\0' || ms < 0) {
        printf("ERR: invalid ms: %s\n", argv[1]);
        return 1;
    }
    if (ms > 60000) ms = 60000;
    vTaskDelay(pdMS_TO_TICKS(ms));
    printf("OK\n");
    return 0;
}

static int cmd_action_dispatch(int argc, char **argv) {
    (void)argc;
    if (!ensure_ctrl_queue()) return 1;
    if (!argv || !argv[0] || argv[0][0] == '\0') {
        printf("ERR: missing command name\n");
        return 1;
    }

    const bool ok = action_registry_enqueue_by_command(s_ctrl_q, argv[0], pdMS_TO_TICKS(250));
    if (!ok) {
        printf("ERR: %s busy (queue full)\n", argv[0]);
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

static int cmd_saveshot(int, char **) {
    if (!ensure_ctrl_queue()) return 1;

    static constexpr size_t kPathCap = 128;
    char *path = (char *)calloc(1, kPathCap);
    if (!path) {
        printf("ERR: oom\n");
        return 1;
    }

    CtrlEvent ev{};
    ev.type = CtrlType::ScreenshotPngSaveToSd;
    ev.reply_task = xTaskGetCurrentTaskHandle();
    ev.ptr = path;
    ev.arg = (int32_t)kPathCap;

    if (!ctrl_send(s_ctrl_q, ev, pdMS_TO_TICKS(250))) {
        free(path);
        printf("ERR: saveshot busy (queue full)\n");
        return 1;
    }

    uint32_t value = 0;
    if (xTaskNotifyWait(0, UINT32_MAX, &value, pdMS_TO_TICKS(15000)) != pdTRUE) {
        free(path);
        printf("ERR: saveshot timeout\n");
        return 1;
    }
    if (value == 0) {
        free(path);
        printf("ERR: saveshot failed\n");
        return 1;
    }

    printf("OK len=%" PRIu32 " path=%s\n", value, path[0] ? path : "(unknown)");
    free(path);
    return 0;
}

static int cmd_palette(int, char **) {
    if (!ensure_ctrl_queue()) return 1;

    CtrlEvent ev{};
    ev.type = CtrlType::TogglePalette;
    if (!ctrl_send(s_ctrl_q, ev, pdMS_TO_TICKS(250))) {
        printf("ERR: palette busy (queue full)\n");
        return 1;
    }

    printf("OK\n");
    return 0;
}

static bool parse_key_token(const char *tok, std::vector<uint32_t> *out) {
    if (!tok || !out) return false;
    if (tok[0] == '\0') return false;

    if (strcmp(tok, "up") == 0) {
        out->push_back(LV_KEY_UP);
        return true;
    }
    if (strcmp(tok, "down") == 0) {
        out->push_back(LV_KEY_DOWN);
        return true;
    }
    if (strcmp(tok, "left") == 0) {
        out->push_back(LV_KEY_LEFT);
        return true;
    }
    if (strcmp(tok, "right") == 0) {
        out->push_back(LV_KEY_RIGHT);
        return true;
    }
    if (strcmp(tok, "enter") == 0) {
        out->push_back(LV_KEY_ENTER);
        return true;
    }
    if (strcmp(tok, "tab") == 0) {
        out->push_back(LV_KEY_NEXT);
        return true;
    }
    if (strcmp(tok, "esc") == 0) {
        out->push_back(LV_KEY_ESC);
        return true;
    }
    if (strcmp(tok, "bksp") == 0 || strcmp(tok, "backspace") == 0) {
        out->push_back(LV_KEY_BACKSPACE);
        return true;
    }
    if (strcmp(tok, "space") == 0) {
        out->push_back((uint32_t)' ');
        return true;
    }

    if (strncmp(tok, "str:", 4) == 0) {
        const char *s = tok + 4;
        while (*s) {
            out->push_back((uint32_t)(unsigned char)*s);
            s++;
        }
        return true;
    }

    if (strlen(tok) == 1) {
        out->push_back((uint32_t)(unsigned char)tok[0]);
        return true;
    }

    char *end = nullptr;
    long v = 0;
    if (strncmp(tok, "0x", 2) == 0 || strncmp(tok, "0X", 2) == 0) {
        v = strtol(tok, &end, 16);
    } else {
        bool all_digits = true;
        for (const char *p = tok; *p; p++) {
            if (!isdigit((unsigned char)*p)) {
                all_digits = false;
                break;
            }
        }
        if (all_digits) {
            v = strtol(tok, &end, 10);
        }
    }

    if (end && *end == '\0' && v > 0 && v <= 0x10FFFF) {
        out->push_back((uint32_t)v);
        return true;
    }

    return false;
}

static bool parse_keycode_number(const char *tok, uint32_t *out) {
    if (!tok || !out) return false;
    if (tok[0] == '\0') return false;

    char *end = nullptr;
    unsigned long v = 0;
    if (strncmp(tok, "0x", 2) == 0 || strncmp(tok, "0X", 2) == 0) {
        v = strtoul(tok, &end, 16);
    } else {
        v = strtoul(tok, &end, 10);
    }
    if (!end || *end != '\0') return false;
    if (v == 0 || v > 0x10FFFFUL) return false;
    *out = (uint32_t)v;
    return true;
}

static int cmd_keys(int argc, char **argv) {
    if (!ensure_ctrl_queue()) return 1;
    if (argc < 2) {
        printf("ERR: usage: keys <token...>\n");
        printf("  tokens: up down left right enter tab esc bksp space\n");
        printf("  tokens: <single-char> | 0xNN | <decimal> | str:hello\n");
        return 1;
    }

    std::vector<uint32_t> keys;
    keys.reserve((size_t)(argc - 1));
    for (int i = 1; i < argc; i++) {
        if (!parse_key_token(argv[i], &keys)) {
            printf("ERR: invalid token: %s\n", argv[i] ? argv[i] : "(null)");
            return 1;
        }
    }
    if (keys.empty()) {
        printf("ERR: no keys\n");
        return 1;
    }
    if (keys.size() > 64) {
        printf("ERR: too many keys (max 64)\n");
        return 1;
    }

    uint32_t *buf = (uint32_t *)malloc(keys.size() * sizeof(uint32_t));
    if (!buf) {
        printf("ERR: oom\n");
        return 1;
    }
    for (size_t i = 0; i < keys.size(); i++) {
        buf[i] = keys[i];
    }

    CtrlEvent ev{};
    ev.type = CtrlType::InjectKeys;
    ev.arg = (int32_t)keys.size();
    ev.ptr = buf;
    if (!ctrl_send(s_ctrl_q, ev, pdMS_TO_TICKS(250))) {
        free(buf);
        printf("ERR: keys busy (queue full)\n");
        return 1;
    }

    printf("OK count=%u\n", (unsigned)keys.size());
    return 0;
}

static int cmd_keycodes(int argc, char **argv) {
    if (!ensure_ctrl_queue()) return 1;
    if (argc < 2) {
        printf("ERR: usage: keycodes <code...>\n");
        printf("  codes: 0xNN | <decimal> (1..0x10FFFF)\n");
        return 1;
    }

    std::vector<uint32_t> keys;
    keys.reserve((size_t)(argc - 1));
    for (int i = 1; i < argc; i++) {
        uint32_t code = 0;
        if (!parse_keycode_number(argv[i], &code)) {
            printf("ERR: invalid code: %s\n", argv[i] ? argv[i] : "(null)");
            return 1;
        }
        keys.push_back(code);
    }

    uint32_t *buf = (uint32_t *)malloc(keys.size() * sizeof(uint32_t));
    if (!buf) {
        printf("ERR: oom\n");
        return 1;
    }
    for (size_t i = 0; i < keys.size(); i++) {
        buf[i] = keys[i];
    }

    CtrlEvent ev{};
    ev.type = CtrlType::InjectKeys;
    ev.arg = (int32_t)keys.size();
    ev.ptr = buf;
    if (!ctrl_send(s_ctrl_q, ev, pdMS_TO_TICKS(250))) {
        free(buf);
        printf("ERR: keycodes busy (queue full)\n");
        return 1;
    }

    printf("OK count=%u\n", (unsigned)keys.size());
    return 0;
}

static int cmd_sdstat(int, char **) {
    printf("sd.mounted=%d\n", sdcard_is_mounted() ? 1 : 0);
    printf("sd.mount_path=%s\n", sdcard_mount_path());
    printf("sd.spi_host_slot=%d\n", sdcard_spi_host_slot());
    printf("sd.pins.miso=%d mosi=%d sck=%d cs=%d\n", sdcard_pin_miso(), sdcard_pin_mosi(), sdcard_pin_sck(),
           sdcard_pin_cs());
    printf("sd.last_err=%s (%d)\n", esp_err_to_name(sdcard_last_error()), (int)sdcard_last_error());
    printf("sd.last_errno=%d\n", sdcard_last_errno());
    return 0;
}

static int cmd_sdmount(int, char **) {
    esp_err_t err = sdcard_mount();
    printf("sdmount=%s (%d)\n", esp_err_to_name(err), (int)err);
    return err == ESP_OK ? 0 : 1;
}

static int cmd_sdumount(int, char **) {
    esp_err_t err = sdcard_unmount();
    printf("sdumount=%s (%d)\n", esp_err_to_name(err), (int)err);
    return err == ESP_OK ? 0 : 1;
}

static int cmd_sdls(int argc, char **argv) {
    const char *path = (argc >= 2 && argv[1] && argv[1][0] != '\0') ? argv[1] : sdcard_mount_path();
    esp_err_t err = sdcard_mount();
    if (err != ESP_OK) {
        printf("ERR: mount failed: %s\n", esp_err_to_name(err));
        return 1;
    }

    std::vector<SdDirEntry> entries;
    err = sdcard_list_dir(path, &entries);
    if (err != ESP_OK) {
        printf("ERR: list failed: %s\n", esp_err_to_name(err));
        return 1;
    }

    printf("dir=%s count=%u\n", path, (unsigned)entries.size());
    for (size_t i = 0; i < entries.size() && i < 64; i++) {
        printf("%c %s\n", entries[i].is_dir ? 'd' : 'f', entries[i].name.c_str());
    }
    if (entries.size() > 64) {
        printf("... (%u more)\n", (unsigned)(entries.size() - 64));
    }
    return 0;
}

static int cmd_sdcat(int argc, char **argv) {
    if (argc < 2 || !argv[1] || argv[1][0] == '\0') {
        printf("ERR: usage: sdcat <abs_path> [max_bytes]\n");
        return 1;
    }
    const char *path = argv[1];
    size_t max_bytes = 4096;
    if (argc >= 3 && argv[2] && argv[2][0] != '\0') {
        char *end = nullptr;
        long v = strtol(argv[2], &end, 10);
        if (!end || *end != '\0' || v <= 0) {
            printf("ERR: invalid max_bytes: %s\n", argv[2]);
            return 1;
        }
        max_bytes = (size_t)v;
    }

    esp_err_t err = sdcard_mount();
    if (err != ESP_OK) {
        printf("ERR: mount failed: %s\n", esp_err_to_name(err));
        return 1;
    }

    std::string out;
    bool truncated = false;
    err = sdcard_read_file_preview(path, max_bytes, &out, &truncated);
    if (err != ESP_OK) {
        printf("ERR: read failed: %s\n", esp_err_to_name(err));
        return 1;
    }

    printf("file=%s bytes=%u%s\n", path, (unsigned)out.size(), truncated ? " truncated" : "");
    fwrite(out.data(), 1, out.size(), stdout);
    printf("\n");
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
    cmd.command = "waitms";
    cmd.help = "Sleep (console task) for N milliseconds";
    cmd.func = &cmd_waitms;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    // Screen navigation commands come from the shared ActionRegistry (also used by the on-device palette).
    size_t action_count = 0;
    const Action *actions = action_registry_actions(&action_count);
    for (size_t i = 0; i < action_count; i++) {
        const Action &a = actions[i];
        if (!a.command || a.command[0] == '\0') continue;
        if (strcmp(a.command, "screenshot") == 0) continue; // handled by cmd_screenshot (blocking + reply_task)
        if (strcmp(a.command, "saveshot") == 0) continue;   // handled by cmd_saveshot (blocking + reply_task)
        if (strcmp(a.command, "heap") == 0) continue;       // handled by cmd_heap (direct response)

        cmd = {};
        cmd.command = a.command;
        cmd.help = a.title;
        cmd.func = &cmd_action_dispatch;
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    }

    cmd = {};
    cmd.command = "palette";
    cmd.help = "Toggle the Ctrl+P command palette overlay";
    cmd.func = &cmd_palette;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "keys";
    cmd.help = "Inject LVGL keycodes (tokens like up/down/enter, char, 0xNN, str:hello)";
    cmd.func = &cmd_keys;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "keycodes";
    cmd.help = "Inject raw LVGL keycodes (decimal/hex)";
    cmd.func = &cmd_keycodes;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "sdstat";
    cmd.help = "Print MicroSD mount debug state (pins/host/last error)";
    cmd.func = &cmd_sdstat;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "sdmount";
    cmd.help = "Attempt to mount MicroSD at /sd";
    cmd.func = &cmd_sdmount;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "sdumount";
    cmd.help = "Unmount MicroSD and free the SPI bus";
    cmd.func = &cmd_sdumount;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "sdls";
    cmd.help = "List directory entries: sdls [/sd/path]";
    cmd.func = &cmd_sdls;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd = {};
    cmd.command = "sdcat";
    cmd.help = "Preview file bytes: sdcat <abs_path> [max_bytes]";
    cmd.func = &cmd_sdcat;
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

    cmd = {};
    cmd.command = "saveshot";
    cmd.help = "Capture current display and save PNG to /sd/shots/ (8.3 filenames)";
    cmd.func = &cmd_saveshot;
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

    ESP_LOGI(TAG,
             "esp_console started over USB-Serial/JTAG (commands: help, heap, menu, basics, pomodoro, console, sysmon, files, "
             "palette, setmins, screenshot, saveshot, keys, keycodes, sdstat, sdmount, sdumount, sdls, sdcat)");
#endif
}
