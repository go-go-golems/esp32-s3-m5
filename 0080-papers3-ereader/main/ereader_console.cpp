#include "ereader_console.h"
#include "ereader_app.h"

#include "esp_console.h"
#include "esp_log.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace ereader {

namespace {

void print_usage()
{
    std::printf("usage:\n");
    std::printf("  ereader list              List loaded books\n");
    std::printf("  ereader open <index>      Open a book\n");
    std::printf("  ereader page <number>     Jump to page\n");
    std::printf("  ereader info              Show reader state\n");
    std::printf("  ereader refresh           Force full EPD refresh\n");
}

int cmd_ereader(int argc, char** argv)
{
    if (argc < 2) {
        print_usage();
        return 1;
    }

    EReaderApp& app = GetApp();

    if (std::strcmp(argv[1], "list") == 0) {
        app.ListBooks();
        return 0;
    }
    if (std::strcmp(argv[1], "open") == 0) {
        if (argc < 3) { std::printf("usage: ereader open <index>\n"); return 1; }
        app.OpenBook(std::atoi(argv[2]));
        return 0;
    }
    if (std::strcmp(argv[1], "page") == 0) {
        if (argc < 3) { std::printf("usage: ereader page <number>\n"); return 1; }
        app.GotoPage(std::atoi(argv[2]));
        return 0;
    }
    if (std::strcmp(argv[1], "info") == 0) {
        app.ShowInfo();
        return 0;
    }
    if (std::strcmp(argv[1], "refresh") == 0) {
        app.ForceRefresh();
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
    repl_config.prompt = "ereader> ";
    repl_config.max_cmdline_length = 256;

    const esp_console_cmd_t cmd = {
        .command = "ereader",
        .help = "E-reader control (list, open, page, info, refresh)",
        .hint = nullptr,
        .func = cmd_ereader,
        .argtable = nullptr,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    // Short aliases
    const esp_console_cmd_t list_cmd = {
        .command = "list",
        .help = "List books",
        .hint = nullptr,
        .func = [](int, char**) -> int { GetApp().ListBooks(); return 0; },
        .argtable = nullptr,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&list_cmd));

    const esp_console_cmd_t refresh_cmd = {
        .command = "refresh",
        .help = "Force full EPD refresh",
        .hint = nullptr,
        .func = [](int, char**) -> int { GetApp().ForceRefresh(); return 0; },
        .argtable = nullptr,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&refresh_cmd));

    const esp_console_cmd_t info_cmd = {
        .command = "info",
        .help = "Show reader info",
        .hint = nullptr,
        .func = [](int, char**) -> int { GetApp().ShowInfo(); return 0; },
        .argtable = nullptr,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&info_cmd));

    esp_console_dev_usb_serial_jtag_config_t hw_config =
        ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

}  // namespace ereader
