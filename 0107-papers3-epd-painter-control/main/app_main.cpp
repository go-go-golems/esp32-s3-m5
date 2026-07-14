#include <array>
#include <cstdio>
#include <cstring>

#include "EPD_Painter.h"
#include "esp_console.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

constexpr char kTag[] = "epd_control";
constexpr char kUpstreamCommit[] = "753c521da8aef59756df07c1a4eb88f1c64c8227";
constexpr char kExpectedIdf[] = "v5.4.2";

EPD_Painter g_display(EPD_PAINTER_PRESET);

enum class ControlState {
    kInitFailed,
    kBootLocked,
};

ControlState g_state = ControlState::kInitFailed;
bool g_preset_valid = false;

const char* StateName()
{
    switch (g_state) {
        case ControlState::kBootLocked:
            return "BOOT_LOCKED";
        case ControlState::kInitFailed:
        default:
            return "INIT_FAILED";
    }
}

bool ValidatePreset()
{
    const auto& cfg = g_display.getConfig();
    constexpr std::array<int8_t, 8> expected_data = {6, 14, 7, 12, 9, 11, 8, 10};
    const bool controls_ok = cfg.width == 960 && cfg.height == 540 && cfg.pin_pwr == 46 &&
                             cfg.pin_spv == 17 && cfg.pin_ckv == 18 && cfg.pin_sph == 13 &&
                             cfg.pin_oe == 45 && cfg.pin_le == 15 && cfg.pin_cl == 16;
    bool data_ok = true;
    for (size_t i = 0; i < expected_data.size(); ++i) {
        data_ok = data_ok && cfg.data_pins[i] == expected_data[i];
    }
    return controls_ok && data_ok;
}

void PrintHelp()
{
    std::printf("commands:\n");
    std::printf("  epd help    show this no-drive command set\n");
    std::printf("  epd status  report initialization, power, stage, and heap state\n");
    std::printf("panel operations are intentionally absent until P0.16\n");
}

void PrintStatus()
{
    std::printf(
        "EPD_CONTROL_STATUS state=%s initialized=%s preset=%s pending=%d rails=%s "
        "idf=%s expected_idf=%s upstream=%s psram=%s heap_free=%u heap_min=%u\n",
        StateName(), g_display.initialized() ? "yes" : "no", g_preset_valid ? "match" : "mismatch",
        g_display.pendingStages(), g_display.panelPowerActive() ? "active" : "idle", esp_get_idf_version(),
        kExpectedIdf, kUpstreamCommit, esp_psram_is_initialized() ? "ready" : "missing",
        static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_8BIT)),
        static_cast<unsigned>(heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT)));
}

int CommandEpd(int argc, char** argv)
{
    if (argc < 2 || std::strcmp(argv[1], "help") == 0) {
        PrintHelp();
        return 0;
    }
    if (std::strcmp(argv[1], "status") == 0) {
        PrintStatus();
        return 0;
    }
    std::printf("error: unsupported no-drive command '%s'\n", argv[1]);
    PrintHelp();
    return 1;
}

void StartConsole()
{
    const esp_console_cmd_t command = {
        .command = "epd",
        .help = "PaperS3 independent EPD control (no-drive gate)",
        .hint = nullptr,
        .func = CommandEpd,
        .argtable = nullptr,
        .func_w_context = nullptr,
        .context = nullptr,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&command));

    esp_console_repl_t* repl = nullptr;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "epd-control> ";
    repl_config.max_cmdline_length = 256;
    const esp_console_dev_usb_serial_jtag_config_t hardware_config =
        ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hardware_config, &repl_config, &repl));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

}  // namespace

extern "C" void app_main(void)
{
    std::printf("\nPaperS3 independent EPD control boot\n");
    std::printf("control.version=p0.15 upstream=%s idf=%s expected_idf=%s\n", kUpstreamCommit,
                esp_get_idf_version(), kExpectedIdf);

    g_preset_valid = ValidatePreset();
    if (!g_preset_valid) {
        ESP_LOGE(kTag, "PaperS3 preset mismatch; driver initialization refused");
    } else if (!esp_psram_is_initialized()) {
        ESP_LOGE(kTag, "octal PSRAM unavailable; driver initialization refused");
    } else {
        g_display.setAutoShutdown(false);
        g_display.setInterlaceMode(false);
        g_display.setQuality(EPD_Painter::Quality::QUALITY_HIGH);
        if (g_display.begin()) {
            g_state = ControlState::kBootLocked;
        } else {
            ESP_LOGE(kTag, "EPD_Painter begin failed; panel commands remain unavailable");
        }
    }

    PrintStatus();
    StartConsole();
    ESP_LOGI(kTag, "no-drive console ready; no panel operation executed at boot");
}
