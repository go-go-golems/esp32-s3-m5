#include "gnosis_console.h"
#include "gnosis_app.h"

#include "esp_console.h"
#include "esp_log.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace gnosis {

namespace {

static const char* TAG = "gnosis_console";

void print_usage()
{
    std::printf("usage:\n");
    std::printf("  gnosis list                  List available presets\n");
    std::printf("  gnosis switch <name|index>   Switch to a preset\n");
    std::printf("  gnosis refresh               Force full EPD refresh\n");
    std::printf("  gnosis gauge <label> <val>   Set a gauge value\n");
    std::printf("  gnosis status                Show engine status\n");
}

int cmd_gnosis(int argc, char** argv)
{
    if (argc < 2) {
        print_usage();
        return 1;
    }

    GnosisApp& app = GetApp();

    if (std::strcmp(argv[1], "list") == 0) {
        app.ListPresets();
        return 0;
    }

    if (std::strcmp(argv[1], "switch") == 0) {
        if (argc < 3) {
            std::printf("usage: gnosis switch <name|index>\n");
            std::printf("  names: ");
            for (int i = 0; i < kPresetCount; ++i) {
                std::printf("%s%s", kPresets[i].name, i < kPresetCount - 1 ? ", " : "\n");
            }
            return 1;
        }

        // Try as integer index first
        char* endp = nullptr;
        long idx = std::strtol(argv[2], &endp, 10);
        if (endp != argv[2] && *endp == '\0') {
            if (idx >= 0 && idx < kPresetCount) {
                app.SwitchPreset(static_cast<int>(idx));
                return 0;
            } else {
                std::printf("index out of range (0-%d)\n", kPresetCount - 1);
                return 1;
            }
        }

        // Try as name
        app.SwitchPresetByName(argv[2]);
        return 0;
    }

    if (std::strcmp(argv[1], "refresh") == 0) {
        app.ForceFullRefresh();
        return 0;
    }

    if (std::strcmp(argv[1], "gauge") == 0) {
        if (argc < 4) {
            std::printf("usage: gnosis gauge <label> <value>\n");
            return 1;
        }
        int val = std::atoi(argv[3]);
        app.SetGaugeValue(argv[2], val);
        return 0;
    }

    if (std::strcmp(argv[1], "status") == 0) {
        std::printf("preset: [%d] %s\n", app.CurrentPreset(),
                    kPresets[app.CurrentPreset()].name);
        std::printf("nodes: %zu / %zu\n", app.Pool().Used(), kMaxNodes);
        std::printf("screen: %dx%d\n", 960, 540);
        return 0;
    }

    print_usage();
    return 1;
}

}  // namespace

void ConsoleInit()
{
    esp_console_repl_t* repl = nullptr;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "gnosis> ";
    repl_config.max_cmdline_length = 256;

    // Register command
    const esp_console_cmd_t cmd = {
        .command = "gnosis",
        .help = "Gnosis layout engine control (list, switch, refresh, gauge, status)",
        .hint = nullptr,
        .func = cmd_gnosis,
        .argtable = nullptr,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    // Also register short aliases
    const esp_console_cmd_t switch_cmd = {
        .command = "switch",
        .help = "Switch preset (alias for 'gnosis switch')",
        .hint = nullptr,
        .func = [](int argc, char** argv) -> int {
            // Rebuild argv with "gnosis" "switch" prepended
            if (argc < 2) {
                std::printf("usage: switch <name|index>\n");
                return 1;
            }
            GetApp().SwitchPresetByName(argv[1]);
            return 0;
        },
        .argtable = nullptr,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&switch_cmd));

    const esp_console_cmd_t list_cmd = {
        .command = "list",
        .help = "List presets (alias for 'gnosis list')",
        .hint = nullptr,
        .func = [](int, char**) -> int {
            GetApp().ListPresets();
            return 0;
        },
        .argtable = nullptr,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&list_cmd));

    const esp_console_cmd_t refresh_cmd = {
        .command = "refresh",
        .help = "Force full EPD refresh",
        .hint = nullptr,
        .func = [](int, char**) -> int {
            GetApp().ForceFullRefresh();
            return 0;
        },
        .argtable = nullptr,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&refresh_cmd));

    // Start USB Serial/JTAG REPL
    esp_console_dev_usb_serial_jtag_config_t hw_config =
        ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
    ESP_LOGI(TAG, "console REPL started");
}

}  // namespace gnosis
